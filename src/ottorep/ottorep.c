#include <ctype.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "ottobits.h"
#include "ottocfg.h"
#include "ottocond.h"
#include "ottodb.h"
#include "ottoipc.h"
#include "ottoutil.h"
#include "signals.h"
#include "simplelog.h"

#include "otto_dtl_writer.h"
#include "otto_jil_writer.h"
#include "otto_pid_writer.h"
#include "otto_sum_writer.h"

// ottorep defines
enum REPORT
{
	UNKNOWN_REPORT,
	DETAIL_REPORT,
	QUERY_REPORT,
	SUMMARY_REPORT,
	PID_REPORT,
	REPORT_TOTAL
};

SIMPLELOG *logp = NULL;

// ottorep variables
int     report_type;
int     report_level=MAXLVL;
int     report_repeat=0;
char    jobname[NAMLEN+1];

// ottorep function prototypes
void  ottorep_exit(int signum);
int   ottorep_getargs(int argc, char **argv);
void  ottorep_usage(void);
int   ottorep(void);


int
main(int argc, char *argv[])
{
	int retval = init_cfg(argc, argv);

	if(retval == OTTO_SUCCESS)
		init_signals(ottorep_exit);

	if(retval == OTTO_SUCCESS)
		if((logp = simplelog_init(cfg.env_ottolog, cfg.progname, INFO, 0)) == NULL)
			retval = OTTO_FAIL;

	if(retval == OTTO_SUCCESS)
	{
		log_initialization();
		log_args(argc, argv);
	}

	if(retval == OTTO_SUCCESS)
		retval = read_cfgfile();

	if(retval == OTTO_SUCCESS)
		retval = ottorep_getargs(argc, argv);

	if(retval == OTTO_SUCCESS)
		retval = open_ottodb(OTTO_CLIENT);

	if(retval == OTTO_SUCCESS)
		retval = ottorep();

	return(retval);
}



void
ottorep_exit(int signum)
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



/*------------------------------- ottorep functions -------------------------------*/
int
ottorep_getargs(int argc, char **argv)
{
	int retval = OTTO_SUCCESS;
	int i_opt;

	// initialize values
	report_type     = SUMMARY_REPORT;
	jobname[0]      = '\0';

	// Suppress getopt from printing an error.
	opterr = 0;

	// Get all parameters.
	while((i_opt = getopt(argc, argv, ":dDhHj:J:l:L:pPqQr:R:sS")) != -1 && retval != OTTO_FAIL)
	{
		switch(tolower(i_opt))
		{
			case 'd':
				report_type = DETAIL_REPORT;
				break;
			case 'h':
				ottorep_usage();
				break;
			case 'j':
				if(strlen(optarg) > NAMLEN)
				{
					fprintf(stderr, "Job name too long.  Job name must be <= %d characters\n", NAMLEN);
					retval = OTTO_FAIL;
				}
				else
					strcpy(jobname, optarg);
				break;
			case 'l':
				report_level = atoi(optarg);
				if(report_level < 0 || report_level > MAXLVL)
					report_level = MAXLVL;
				break;
			case 'p':
				report_type = PID_REPORT;
				break;
			case 'q':
				report_type = QUERY_REPORT;
				break;
			case 'r':
				report_repeat = atoi(optarg);
				if(report_repeat != 0 && (report_repeat < 5 || report_repeat > 300))
					report_repeat = 30;
				break;
			case 's':
				report_type = SUMMARY_REPORT;
				break;
			case ':' :
				retval = OTTO_FAIL;
				break;
		}
	}

	if(jobname[0] == '\0')
	{
		fprintf(stderr, "Job name (-J) is required\n");
		retval = OTTO_FAIL;
	}

	if(report_type != SUMMARY_REPORT)
		report_repeat = 0;

	return(retval);
}



void
ottorep_usage(void)
{

	printf("   NAME\n");
	printf("      ottorep - Command line interface to report information about\n");
	printf("                Otto jobs and objects\n");
	printf("\n");
	printf("   SYNOPSIS\n");
	printf("      ottorep -J job_name [-d|-h|-p|-q|-s]\n");
	printf("              [-L print_level][-r repeat_interval]\n");
	printf("\n");
	printf("   DESCRIPTION\n");
	printf("      Lists information about jobs in the Otto job database.\n");
	printf("      It can also export job definitions in JIL script format.\n");
	printf("\n");
	printf("      When ottorep reports on nested jobs subordinate jobs are\n");
	printf("      indented to illustrate the hierarchy.\n");
	printf("\n");
	printf("      The -J option is required.\n");
	printf("\n");
	printf("   OPTIONS\n");
	printf("      -d Generates a detail report for the specified job.\n");
	printf("\n");
	printf("      -h Print this help and exit.\n");
	printf("\n");
	printf("      -J job_name\n");
	printf("         Identifies the job on which to report.  You can use\n");
	printf("         the percent (%%) and underscore (_) characters as\n");
	printf("         wildcards in this value.  To report on all jobs,\n");
	printf("         specify a single percent character.\n");
	printf("\n");
	printf("      -L print_level\n");
	printf("         Specifies the number of levels into the box job\n");
	printf("         hierarchy to report.\n");
	printf("\n");
	printf("      -p Prints the process ID for the specified job.\n");
	printf("\n");
	printf("      -q Generates a JIL script for the specified job.\n");
	printf("\n");
	printf("      -r repeat_interval\n");
	printf("         Generates a job report every repeat_interval seconds\n");
	printf("         until Ctrl_c is pressed.\n");
	printf("\n");
	printf("      -s Generates a summary report for the specified job.\n");
	printf("         (default)\n");
	exit(0);

}



int
ottorep(void)
{
	int        retval = OTTO_SUCCESS;
	int        repeat = OTTO_TRUE;
	int        i;
	time_t     now;
	struct tm *tm_time;
	char       c_time[25];
	JOBLIST    joblist;


	if((joblist.item = (JOB *)calloc(cfg.ottodb_maxjobs, sizeof(JOB))) == NULL)
		retval = OTTO_FAIL;

	if(retval == OTTO_SUCCESS)
	{
		while(repeat == OTTO_TRUE)
		{
			copy_jobwork();

			joblist.nitems = 0;

			build_joblist(&joblist, jobname, root_job->head, 0, report_level, OTTO_TRUE);

			repeat = OTTO_FALSE;
			if(report_type == SUMMARY_REPORT)
			{
				if(report_repeat != 0)
				{
					system("clear");
					now = time(0);
					tm_time = localtime(&now);
					strftime(c_time, 256, "%m/%d/%Y %T", tm_time);
					printf("%-20s ", c_time);
					printf("(refresh: %d sec)\n", report_repeat);

					// check if the repeat is still necessary
					for(i=0; i<joblist.nitems && repeat == OTTO_FALSE; i++)
					{
						switch(joblist.item[i].status)
						{
							case STAT_IN:
							case STAT_AC:
							case STAT_RU:
								repeat = OTTO_TRUE;
								break;
							default:
								break;
						}
					}
				}
			}

			switch(report_type)
			{
				case DETAIL_REPORT:  write_dtl(&joblist); break;
				case PID_REPORT:     write_pid(&joblist); break;
				case QUERY_REPORT:   write_jil(&joblist); break;
				case SUMMARY_REPORT: write_sum(&joblist); break;
			}

			if(repeat == OTTO_TRUE && report_repeat != 0)
			{
				sleep(report_repeat);
			}
		}

		free(joblist.item);
	}

	return(retval);
}



/*
vim: ts=3 sw=3 ai
*/
