#include <ctype.h>
#include <execinfo.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "otto.h"
#include "otto_json.h"

OTTOLOG *logp = NULL;
DBCTX    ctx  = {0};

// ottoexp variables
int     export_type;
int     export_level=MAXLVL;
int     autoschedule;
char    jobname[NAMLEN+1];

int ottosysd_socket = -1;
ottoipc_simple_pdu_st send_pdu;


// ottoexp function prototypes
void  ottoexp_exit(int signum);
int   ottoexp_getargs(int argc, char **argv);
void  ottoexp_usage(void);
int   ottoexp (void);
int   ottoexp_export(void);
int   ottoexp_list(JOBLIST *joblist);
int   insert_task(JOB *item);



int
main(int argc, char *argv[])
{
   int retval = init_cfg(argc, argv);

   if(retval == OTTO_SUCCESS)
      init_signals(ottoexp_exit);

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
      retval = ottoexp_getargs(argc, argv);

   if(retval == OTTO_SUCCESS)
   {
      if((ottosysd_socket = init_client_ipc(cfg.server_addr, cfg.ottosysd_port)) == -1)
         retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS)
   {
      retval = open_ottodb(OTTO_CLIENT);
   }

   if(retval == OTTO_SUCCESS)
      retval = ottoexp();

   if(ottosysd_socket != -1)
   {
      close(ottosysd_socket);
   }

   return(retval);
}



void
ottoexp_exit(int signum)
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
ottoexp_getargs(int argc, char **argv)
{
   int retval = OTTO_SUCCESS;
   int i_opt;
   char *export_optarg = NULL;

   // initialize values
   export_type = OTTO_UNKNOWN;
   autoschedule  = OTTO_FALSE;
   jobname[0]    = '\0';


   // Suppress getopt from printing an error.
   opterr = 0;

   // Get all parameters.
   while((i_opt = getopt(argc, argv, ":aAf:F:hHj:J:l:L:")) != -1 && retval != OTTO_FAIL)
   {
      switch(tolower(i_opt))
      {
         case 'a':
            autoschedule = OTTO_TRUE;
            break;
         case 'f' :
            if(optarg[0] != '-')
            {
               export_type = get_file_format(optarg);
               export_optarg = optarg;
            }
            break;
         case 'h':
            ottoexp_usage();
            break;
         case 'j' :
            if(optarg[0] != '-')
            {
               if(strlen(optarg) > sizeof(jobname)-1)
               {
                  fprintf(stderr, "Job name too long.  Job name must be <= %d characters\n", (int)sizeof(jobname)-1);
                  retval = OTTO_FAIL;
               }
               else
                  strcpy(jobname, optarg);
            }
            break;
         case 'l':
            export_level = atoi(optarg);
            if(export_level < 0 || export_level > MAXLVL)
               export_level = MAXLVL;
            break;
         case ':' :
            retval = OTTO_FAIL;
            break;
      }
   }

   if(export_type == OTTO_UNKNOWN)
   {
      if(export_optarg != NULL)
         fprintf(stderr, "Unknown export file type.  %s\n", export_optarg);
      else
         fprintf(stderr, "Missing export file type.\n");

      ottoexp_usage();
      retval = OTTO_FAIL;
   }

   return(retval);
}



void
ottoexp_usage(void)
{

   printf("   NAME\n");
   printf("      ottoexp - Command line interface to export the Otto job database\n");
   printf("                in a specified format\n");
   printf("\n");
   printf("   SYNOPSIS\n");
   printf("      ottoexp -f <format> [-J job_name] [-a]\n");
   printf("      ottoexp -h\n");
   printf("\n");
   printf("   DESCRIPTION\n");
   printf("      Runs Otto's export routines to export all or part of\n");
   printf("      the Otto job database in a supported format.\n");
   printf("\n");
   printf("   OPTIONS\n");
   printf("      -a Used with the MSP format to export jobflow structure but not\n");
   printf("         runtime statistics.\n");
   printf("\n");
   printf("      -f <format> - Export jobflow information in a particular format.\n");
   printf("         CSV - export as a csv file\n");
   printf("         JIL - export as (mostly) autosys compliant JIL\n");
   printf("         MSP - export as MS Project Data Interchange compliant XML\n");
   printf("\n");
   printf("      -h Print this help and exit.\n");
   printf("\n");
   printf("      -J job_name\n");
   printf("         Used to limit processing to the specified job or box.\n");
   printf("\n");
   printf("      -L level\n");
   printf("         Specifies the number of levels into the box job\n");
   printf("         hierarchy to export.\n");
   exit(0);

}



int
ottoexp(void)
{
   int retval = OTTO_SUCCESS;
   JOBLIST joblist;

   memset(&joblist, 0, sizeof(joblist));

   if((joblist.item = (JOB *)calloc(cfg.ottodb_maxjobs, sizeof(JOB))) == NULL)
      retval = OTTO_FAIL;

   copy_jobwork(&ctx);

   build_joblist(&joblist, &ctx, jobname, root_job->head, 0, export_level, OTTO_TRUE);

   if(retval == OTTO_SUCCESS)
   {
      switch(export_type)
      {
         case OTTO_CSV:
            write_csv(&joblist);
            break;
         case OTTO_JIL:
            write_jil(&joblist);
            break;
         case OTTO_MSP:
            write_mspdi(&joblist, autoschedule);
            break;
         case OTTO_JSON:
            write_json(&joblist);
         default:
            break;
      }
   }

   return(retval);
}



