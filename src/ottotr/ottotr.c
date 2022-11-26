#include <ctype.h>
#include <execinfo.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "otto.h"

OTTOLOG *logp = NULL;



char *jobname     = NULL;
int   input_type  = OTTO_UNKNOWN;
int   output_type = OTTO_UNKNOWN;


// ottotr function prototypes
void  ottotr_exit(int signum);
int   ottotr_getargs(int argc, char **argv);
void  ottotr_usage(void);
int   ottotr (void);



int
main(int argc, char *argv[])
{
   int retval = init_cfg(argc, argv);

   if(retval == OTTO_SUCCESS)
      init_signals(ottotr_exit);

   if(retval == OTTO_SUCCESS)
      if((logp = ottolog_init(cfg.env_ottolog, cfg.progname, INFO, 0)) == NULL)
         retval = OTTO_FAIL;

   if(retval == OTTO_SUCCESS)
   {
      log_initialization();
      log_args(argc, argv);
   }

   if(retval == OTTO_SUCCESS)
      retval = read_cfgfile();

   if(retval == OTTO_SUCCESS)
      retval = ottotr_getargs(argc, argv);

   if(retval == OTTO_SUCCESS)
      retval = ottotr();

   return(retval);
}



void
ottotr_exit(int signum)
{
   int j, nptrs;
   void *buffer[100];
   char **strings;

   if(signum > 0)
   {
      lprintf(logp, MAJR, "Shutting down. Caught signal %d (%s).\n", signum, strsignal(signum));

      nptrs   = backtrace(buffer, 100);
      strings = backtrace_symbols(buffer, nptrs);
      if(strings == NULL)
      {
         lprintf(logp, MAJR, "Error getting backtrace symbols.\n");
      }
      else
      {
         lsprintf(logp, INFO, "Backtrace:\n");
         for (j = 0; j < nptrs; j++)
            lsprintf(logp, CATI, "  %s\n", strings[j]);
         lsprintf(logp, END, "");
      }
   }
   else
   {
      lprintf(logp, MAJR, "Shutting down. Reason code %d.\n", signum);
   }

   exit(-1);
}



int
ottotr_getargs(int argc, char **argv)
{
   int retval = OTTO_SUCCESS;
   int i_opt;
   char *input_optarg = NULL, *output_optarg = NULL;

   // initialize values
   jobname       = NULL;
   input_type    = OTTO_UNKNOWN;
   output_type   = OTTO_UNKNOWN;


   // Suppress getopt from printing an error.
   opterr = 0;

   // Get all parameters.
   while((i_opt = getopt(argc, argv, ":hHi:I:j:J:o:O:")) != -1 && retval != OTTO_FAIL)
   {
      switch(tolower(i_opt))
      {
         case 'i':
            if(optarg[0] != '-')
            {
               input_type    = get_file_format(optarg);
               input_optarg  = optarg;
            }
            break;
         case 'h':
            ottotr_usage();
            break;
         case 'j' :
            if(optarg[0] != '-')
            {
               jobname = optarg;
            }
            break;
         case 'o':
            if(optarg[0] != '-')
            {
               output_type   = get_file_format(optarg);
               output_optarg = optarg;
            }
            break;
         case ':' :
            retval = OTTO_FAIL;
            break;
      }
   }

   if(input_type == OTTO_UNKNOWN)
   {
      if(input_optarg != NULL)
         fprintf(stderr, "Unknown input file type.  %s\n", input_optarg);
      else
         fprintf(stderr, "Missing input file type.\n");

      retval = OTTO_FAIL;
   }
   if(output_type == OTTO_UNKNOWN)
   {
      if(output_optarg != NULL)
         fprintf(stderr, "Unknown output file type.  %s\n", output_optarg);
      else
         fprintf(stderr, "Missing input file type.\n");
      retval = OTTO_FAIL;
   }

   return(retval);
}



void
ottotr_usage(void)
{

   printf("   NAME\n");
   printf("      ottotr - Command line interface to translate Otto job\n");
   printf("                 definition files between supported formats\n");
   printf("\n");
   printf("   SYNOPSIS\n");
   printf("      ottotr -i [CSV|JIL|MSP] -o [CSV|JIL|MSP] [-J job_name] <input_file >output_file\n");
   printf("\n");
   printf("   DESCRIPTION\n");
   printf("      Invokes Otto's file format processors to translate job\n");
   printf("      definition files.\n");
   printf("\n");
   printf("   OPTIONS\n");
   printf("\n");
   printf("      -i Specify the input file format (required).\n");
   printf("\n");
   printf("      -J job_name\n");
   printf("         Limit processing to the specified job or box.\n");
   printf("\n");
   printf("      -o Specify the output file format (required).\n");
   printf("\n");
   printf("   SUPPORTED FORMATS\n");
   printf("\n");
   printf("      CSV  - Mime quoted comma separated values\n");
   printf("      JIL  - Autosys compliant JIL.\n");
   printf("      MSP  - MS Project Data Interchange compliant XML.\n");
   printf("\n");
   exit(0);

}



int
ottotr(void)
{
   int     retval = OTTO_SUCCESS;
   DYNBUF  b;
   JOBLIST joblist;

   retval = read_stdin(&b, "ottotr");

   if(retval == OTTO_SUCCESS)
   {
      switch(input_type)
      {
         case OTTO_CSV:
            retval = parse_csv(&b, &joblist);
            break;
         case OTTO_JIL:
            retval = parse_jil(&b, &joblist);
            break;
         case OTTO_MSP:
            retval = parse_mspdi(&b, &joblist);
            break;
         default:
            retval = OTTO_FAIL;
            break;
      }
   }

   if(retval == OTTO_SUCCESS && jobname != NULL)
   {
      retval = ottojob_reduce_list(&joblist, jobname);
   }

   if(retval == OTTO_SUCCESS)
   {
      switch(output_type)
      {
         case OTTO_CSV:
            retval = write_csv(&joblist);
            break;
         case OTTO_JIL:
            retval = write_jil(&joblist);
            break;
         case OTTO_MSP:
            retval = write_mspdi(&joblist, OTTO_TRUE);
            break;
         default:
            retval = OTTO_FAIL;
            break;
      }
   }

   return(retval);
}



