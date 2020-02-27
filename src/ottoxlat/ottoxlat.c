#include <ctype.h>
#include <execinfo.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "ottobits.h"
#include "ottocfg.h"
#include "ottolog.h"
#include "ottojob.h"
#include "ottosignal.h"
#include "ottoutil.h"

#include "otto_dtl_writer.h"
#include "otto_jil_reader.h"
#include "otto_jil_writer.h"
#include "otto_mpp_reader.h"
#include "otto_mpp_writer.h"


enum FILETYPES
{
	TYPE_UNKNOWN,
	TYPE_JIL,
	TYPE_MPP,
	TYPE_DTL,
	TYPE_TOTAL
};



char *jobname     = NULL;
int   input_type  = TYPE_UNKNOWN;
int   output_type = TYPE_UNKNOWN;


// ottoxlat function prototypes
void  ottoxlat_exit(int signum);
int   ottoxlat_getargs(int argc, char **argv);
void  ottoxlat_usage(void);
int   ottoxlat (void);
int   get_type(char *optarg);



int
main(int argc, char *argv[])
{
	int retval = init_cfg(argc, argv);

	if(retval == OTTO_SUCCESS)
		retval = init_signals(ottoxlat_exit);

	if(retval == OTTO_SUCCESS)
		retval = init_log(cfg.env_ottolog, cfg.progname, INFO, 0);

	if(retval == OTTO_SUCCESS)
	{
		log_initialization();
		log_args(argc, argv);
	}

	if(retval == OTTO_SUCCESS)
		retval = read_cfgfile();

	if(retval == OTTO_SUCCESS)
		retval = ottoxlat_getargs(argc, argv);

	if(retval == OTTO_SUCCESS)
		retval = ottoxlat();

	return(retval);
}



void
ottoxlat_exit(int signum)
{
	int j, nptrs;
	void *buffer[100];
	char **strings;

	if(signum > 0)
	{
		lprintf(MAJR, "Shutting down. Caught signal %d (%s).\n", signum, strsignal(signum));

		nptrs   = backtrace(buffer, 100);
		strings = backtrace_symbols(buffer, nptrs);
		if(strings == NULL)
		{
			lprintf(MAJR, "Error getting backtrace symbols.\n");
		}
		else
		{
			lsprintf(INFO, "Backtrace:\n");
			for (j = 0; j < nptrs; j++)
				lsprintf(CATI, "  %s\n", strings[j]);
			lsprintf(END, "");
		}
	}
	else
	{
		lprintf(MAJR, "Shutting down. Reason code %d.\n", signum);
	}

	exit(-1);
}



int
ottoxlat_getargs(int argc, char **argv)
{
	int retval = OTTO_SUCCESS;
	int i_opt;
	char *input_optarg = NULL, *output_optarg = NULL;

	// initialize values
	jobname       = NULL;
	input_type    = TYPE_UNKNOWN;
	output_type   = TYPE_UNKNOWN;


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
					input_type    = get_type(optarg);
					input_optarg  = optarg;
					if(input_type == TYPE_DTL)
						input_type = TYPE_UNKNOWN;
				}
				break;
			case 'h':
				ottoxlat_usage();
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
					output_type   = get_type(optarg);
					output_optarg = optarg;
				}
				break;
			case ':' :
				retval = OTTO_FAIL;
				break;
		}
	}

	if(input_type == TYPE_UNKNOWN)
	{
		if(input_optarg != NULL)
			fprintf(stderr, "Unknown input file type.  %s\n", input_optarg);
		else
			fprintf(stderr, "Missing input file type.\n");

		retval = OTTO_FAIL;
	}
	if(output_type == TYPE_UNKNOWN)
	{
		if(output_optarg != NULL)
			fprintf(stderr, "Unknown output file type.  %s\n", output_optarg);
		else
			fprintf(stderr, "Missing input file type.\n");
		retval = OTTO_FAIL;
	}
	/*
	else
	{
		if(input_type == output_type)
		{
			fprintf(stderr, "Input and output file types are the same.  %s\n", input_optarg);
			retval = OTTO_FAIL;
		}
	}
	*/

	return(retval);
}



void
ottoxlat_usage(void)
{

	printf("   NAME\n");
	printf("      ottoxlat - Command line interface to translate Otto job\n");
	printf("                 definition files between supported formats\n");
	printf("\n");
	printf("   SYNOPSIS\n");
	printf("      ottoxlat -i [JIL|MPP] -o [MPP|JIL] [-J job_name] <input_file >output_file\n");
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
	printf("      JIL  - Autosys compliant JIL.\n");
	printf("      MPP  - MS Project compliant XML.\n");
	printf("\n");
	exit(0);

}



int
ottoxlat(void)
{
	int     retval = OTTO_SUCCESS;
   BUF_st  b;
	JOBLIST joblist;

	retval = read_stdin(&b, "ottoxlat");

	if(retval == OTTO_SUCCESS)
	{
		switch(input_type)
		{
			case TYPE_JIL:
				retval = parse_jil(&b, &joblist);
				break;
			case TYPE_MPP:
				retval = parse_mpp_xml(&b, &joblist);
				break;
			default:
				retval = OTTO_FAIL;
			break;
		}
	}

	if(retval == OTTO_SUCCESS)
	{
		switch(output_type)
		{
			case TYPE_DTL:
				retval = write_dtl(&joblist);
				break;
			case TYPE_JIL:
				retval = write_jil(&joblist);
				break;
			case TYPE_MPP:
				retval = write_mpp_xml(&joblist, OTTO_TRUE);
				break;
			default:
			retval = OTTO_FAIL;
			break;
		}
	}

	return(retval);
}



int
get_type(char *optarg)
{
	int retval = TYPE_UNKNOWN;

	if(strcasecmp(optarg, "DTL") == 0)
		retval = TYPE_DTL;

	if(strcasecmp(optarg, "JIL") == 0)
		retval = TYPE_JIL;

	if(strcasecmp(optarg, "MPP") == 0)
		retval = TYPE_MPP;

	return(retval);
}



//
// vim: ts=3 sw=3 ai
//
