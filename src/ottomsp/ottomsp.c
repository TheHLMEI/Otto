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
#include "ottodb.h"
#include "ottoipc.h"
#include "ottolog.h"
#include "ottojob.h"
#include "otto_mpp_reader.h"
#include "otto_mpp_writer.h"
#include "ottosignal.h"
#include "ottoutil.h"


// action types
enum ACTIONS
{
	UNKNOWN,
	EXPORT    = 'e',
	IMPORT    = 'i',
	LIST      = 'l',
	VALIDATE  = 'v',
	ACTION_TOTAL
};


// ottomsp variables
int     action;
int16_t jobid = 1;
int16_t start = -1;
int     autoschedule;

char    jobname[NAMLEN+1];

int server_socket = -1;
ottoipc_simple_pdu_st send_pdu;


// ottomsp function prototypes
void  ottomsp_exit(int signum);
int   ottomsp_getargs(int argc, char **argv);
void  ottomsp_usage(void);
int   ottomsp (void);
int   ottomsp_import(JOBLIST *joblist);
int   ottomsp_export(void);
int   ottomsp_list(JOBLIST *joblist);
void  prep_tasks(int16_t head);
void  output_xml_preamble();
void  output_xml_tasks(int16_t id, char *level);
void  output_xml_task(int i, char *outline);
void  output_xml_predecessors(int i);
void  output_xml_postamble();
void  buffer_jil_preamble(DYNBUF *o);
void  buffer_jil_task(DYNBUF *o, int i);
void  buffer_name(DYNBUF *o, char *buf);
void  buffer_xml(DYNBUF *o, char *s);
void  report_xml(char *s);
int   bprintf(DYNBUF *b, char *format, ...);



int
main(int argc, char *argv[])
{
	int retval = init_cfg(argc, argv);

	if(retval == OTTO_SUCCESS)
		retval = init_signals(ottomsp_exit);

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
		retval = ottomsp_getargs(argc, argv);

	if(retval == OTTO_SUCCESS)
	{
		if((server_socket = init_client_ipc(cfg.server_addr, cfg.server_port)) == -1)
			retval = OTTO_FAIL;
	}

	if(retval == OTTO_SUCCESS)
		retval = ottomsp();

	if(server_socket != -1)
	{
		close(server_socket);
	}

	return(retval);
}



void
ottomsp_exit(int signum)
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
ottomsp_getargs(int argc, char **argv)
{
	int retval = OTTO_SUCCESS;
	int i_opt;

	// initialize values
	action       = UNKNOWN;
	autoschedule = OTTO_FALSE;
	jobname[0]   = '\0';


	// Suppress getopt from printing an error.
	opterr = 0;

	// Get all parameters.
	while((i_opt = getopt(argc, argv, ":aAeEhHiIj:J:lLvV")) != -1 && retval != OTTO_FAIL)
	{
		switch(tolower(i_opt))
		{
			case 'a':
				autoschedule = OTTO_TRUE;
				break;
			case 'e':
			case 'i':
			case 'l':
			case 'v' :
				if(action != UNKNOWN && action != tolower(i_opt))
				{
					fprintf(stderr, "Don't use -%c and -%c options at the same time.\n", action, tolower(i_opt));
					retval = OTTO_FAIL;
				}
				else
				{
					action = tolower(i_opt);
				}
				break;
			case 'h':
				ottomsp_usage();
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
			case ':' :
				retval = OTTO_FAIL;
				break;
		}
	}

	if(action == UNKNOWN)
	{
		ottomsp_usage();
		retval = OTTO_FAIL;
	}

	return(retval);
}



void
ottomsp_usage(void)
{

	printf("   NAME\n");
	printf("      ottomsp - Command line interface to define or report Otto jobs\n");
	printf("                and objects using MS Project files in XML format\n");
	printf("\n");
	printf("   SYNOPSIS\n");
	printf("      ottomsp -e [-J job_name] [-a]\n");
	printf("      ottomsp -h\n");
	printf("      ottomsp -i [-J job_name]\n");
	printf("      ottomsp -l\n");
	printf("      ottomsp -v\n");
	printf("\n");
	printf("   DESCRIPTION\n");
	printf("      Runs Otto's MS Project processor to manipulate or report\n");
	printf("      jobs in the Otto job database.\n");
	printf("\n");
	printf("      You can use the ottomsp command in two ways:\n");
	printf("\n");
	printf("      *  To write job definitions and statistics to a MS Project\n");
	printf("         compatible XML file.  MS Project files are written to\n");
	printf("         ottomsp's stdout.\n");
	printf("\n");
	printf("      *  To read, validate, translate, and apply job definitions\n");
	printf("         specified in a MS Project XML file.  MS Project files\n");
	printf("         are read from ottomsp's stdin.\n");
	printf("\n");
	printf("   OPTIONS\n");
	printf("      -a Used with -e to export jobflow structure but not\n");
	printf("         runtime statistics.\n");
	printf("\n");
	printf("      -e Export a MS Project file.\n");
	printf("\n");
	printf("      -h Print this help and exit.\n");
	printf("\n");
	printf("      -i Import a MS Project file.\n");
	printf("\n");
	printf("      -J job_name\n");
	printf("         Used with -e, or -i to limit processing to the\n");
	printf("         specified job or box.\n");
	printf("\n");
	printf("      -l List the jobs defined in a MS Project file.\n");
	printf("\n");
	printf("      -v Verify a MS Project file contains the information\n");
	printf("         necessary to define Otto jobs.\n");
	exit(0);

}



int
ottomsp(void)
{
	int      retval = OTTO_SUCCESS;
   DYNBUF  b;
	JOBLIST joblist;

	switch(action)
	{
		case EXPORT:
			retval = ottomsp_export();
			break;
		default:
			memset(&b, 0, sizeof(b));

			if((retval = read_stdin(&b, "ottomsp")) == OTTO_SUCCESS)
			{
				if((retval = parse_mpp_xml(&b, &joblist)) == OTTO_SUCCESS)
				{
					switch(action)
					{
						case IMPORT:
							retval = ottomsp_import(&joblist);
							break;
						case LIST:
							retval = ottomsp_list(&joblist);
							break;
					}
				}
			}
			break;
	}

	return(retval);
}


#include "ottomsp_e.c"
#include "ottomsp_i.c"
#include "ottomsp_l.c"

//
// vim: ts=3 sw=3 ai
//
