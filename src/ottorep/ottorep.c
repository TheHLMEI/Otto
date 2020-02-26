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
#include "ottolog.h"
#include "ottosignal.h"
#include "ottoutil.h"

int server_socket = -1;

// ottorep defines
#define OTTO_DETAIL   1
#define OTTO_QUERY    2
#define OTTO_SUMMARY  3
#define OTTO_PID      4

// ottorep variables
int     ottorep_type;
int     ottorep_header_printed=OTTO_FALSE;
int     ottorep_level=MAXLVL;
int     ottorep_repeat=0;
char    ottorep_name[NAMLEN+1];

char *indentation[9] = {"", " ", "  ", "   ", "    ", "     ", "      ", "       ", "        "};

// ottorep function prototypes
void  ottorep_exit(int signum);
int   ottorep_getargs(int argc, char **argv);
void  ottorep_usage(void);
int   ottorep (void);
int   ottorep_chain (int id, int levelmod, int check);
void  output_jobwork_detail (int id);
void  output_jobwork_pid (int id);
void  output_jobwork_query (int id, int levelmod);
int   output_jobwork_summary (int id, int levelmod);
void  output_expression (char *s);
void  output_expression_status (char *s);
int   evaluate_stat(ecnd_st *c);


int
main(int argc, char *argv[])
{
	int retval = init_cfg(argc, argv);

	if(retval == OTTO_SUCCESS)
		retval = init_signals(ottorep_exit);

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



/*------------------------------- ottorep functions -------------------------------*/
int
ottorep_getargs(int argc, char **argv)
{
	int retval = OTTO_SUCCESS;
	int i_opt;

	// initialize values
	ottorep_type         = OTTO_SUMMARY;
	ottorep_name[0]      = '\0';

	// Suppress getopt from printing an error.
	opterr = 0;

	// Get all parameters.
	while((i_opt = getopt(argc, argv, ":dDhHj:J:l:L:pPqQr:R:sS")) != -1 && retval != OTTO_FAIL)
	{
		switch(tolower(i_opt))
		{
			case 'd':
				ottorep_type = OTTO_DETAIL;
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
					strcpy(ottorep_name, optarg);
				break;
			case 'l':
				ottorep_level = atoi(optarg);
				if(ottorep_level < 0 || ottorep_level > MAXLVL)
					ottorep_level = MAXLVL;
				break;
			case 'p':
				ottorep_type = OTTO_PID;
				break;
			case 'q':
				ottorep_type = OTTO_QUERY;
				break;
			case 'r':
				ottorep_repeat = atoi(optarg);
				if(ottorep_repeat != 0 && (ottorep_repeat < 5 || ottorep_repeat > 300))
					ottorep_repeat = 30;
				break;
			case 's':
				ottorep_type = OTTO_SUMMARY;
				break;
			case ':' :
				retval = OTTO_FAIL;
				break;
		}
	}

	if(ottorep_name[0] == '\0')
	{
		fprintf(stderr, "Job name (-J) is required\n");
		retval = OTTO_FAIL;
	}

	if(ottorep_type != OTTO_SUMMARY)
		ottorep_repeat = 0;

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
	int        ret = OTTO_SUCCESS;
	int        repeat;
	time_t     now;
	struct tm *tm_time;
	char       c_time[25];

	repeat = OTTO_TRUE;
	while(repeat == OTTO_TRUE)
	{
		repeat                 = OTTO_FALSE;
		ottorep_header_printed = OTTO_FALSE;

		copy_jobwork();

		if(ottorep_type == OTTO_SUMMARY)
		{
			if(ottorep_repeat != 0)
			{
				system("clear");
				now = time(0);
				tm_time = localtime(&now);
				strftime(c_time, 256, "%m/%d/%Y %T", tm_time);
				printf("%-20s ", c_time);
				printf("(refresh: %d sec)\n", ottorep_repeat);
			}
		}

		if(ottorep_chain(root_job->head, 0, OTTO_TRUE) > 0 && ottorep_repeat != 0)
		{
			repeat = OTTO_TRUE;
			sleep(ottorep_repeat);
		}
	}

	return(ret);
}



int
ottorep_chain(int id, int levelmod, int do_check)
{
	int check, retval = 0;

	if(do_check == OTTO_TRUE || (id > -1 && jobwork[id].level - levelmod <= ottorep_level))
	{
		while(id != -1)
		{
			check = do_check;

			if((check == OTTO_FALSE) ||
				(check == OTTO_TRUE && strwcmp(jobwork[id].name, ottorep_name) == OTTO_TRUE))
			{
				if(jobwork[jobwork[id].parent].print != OTTO_TRUE)
					levelmod = jobwork[id].level;
				jobwork[id].print = OTTO_TRUE;
				switch(ottorep_type)
				{
					case OTTO_DETAIL:
						output_jobwork_detail(id);
						break;
					case OTTO_PID:
						output_jobwork_pid(id);
						break;
					case OTTO_QUERY:
						output_jobwork_query(id, levelmod);
						break;
					case OTTO_SUMMARY:
						retval += output_jobwork_summary(id, levelmod);
						break;
				}
			}
			if(jobwork[id].type == 'b')
			{
				if(jobwork[id].print == OTTO_TRUE)
				{
					check = OTTO_FALSE;
				}
				retval += ottorep_chain(jobwork[id].head, levelmod, check);
			}

			id = jobwork[id].next;
		}
	}

	return(retval);
}



void
output_jobwork_detail(int id)
{
	time_t     tmp_time_t;
	struct tm *tm_time;
	char       c_time[25];
	int        i, j, one_printed;

	printf("---------------\n");
	printf("id             : %d\n",  jobwork[id].id);
	printf("linkage        : parent %d, head %d, tail %d, prev %d, next %d\n",
			 jobwork[id].parent, jobwork[id].head, jobwork[id].tail, jobwork[id].prev, jobwork[id].next);
	printf("level          : %d\n",  jobwork[id].level);
	printf("name           : %s\n",  jobwork[id].name);
	printf("type           : %c\n",  jobwork[id].type);
	printf("box_name       : %s\n",  jobwork[id].box_name);
	printf("description    : %s\n",  jobwork[id].description);
	if(jobwork[id].type != 'b')
		printf("command        : %s\n",  jobwork[id].command);
	printf("condition      : %s\n", jobwork[id].condition);
	if(jobwork[id].condition[0] != '\0')
	{
		printf("expression1    : ");
		output_expression(jobwork[id].expression);
		printf("\n");
		printf("expression2    : ");
		output_expression_status(jobwork[id].expression);
		printf("\n");
		printf("expr_fail      : %d\n",  jobwork[id].expr_fail);
	}
	printf("auto_hold      : %d\n",  jobwork[id].auto_hold);
	printf("base_auto_hold : %d\n",  jobwork[id].base_auto_hold);
	printf("on_noexec      : %d\n",  jobwork[id].on_noexec);
	if(jobwork[id].date_conditions == 0)
		printf("date_conditions: 0\n");
	else
		printf("date_conditions: 1\n");

	printf("days_of_week   : ");
	if(jobwork[id].days_of_week == OTTO_ALL)
	{
		printf("all");
	}
	else
	{
		one_printed = OTTO_FALSE;
		for(i=0; i<7; i++)
		{
			if(jobwork[id].days_of_week & (1L << i))
			{
				if(one_printed == OTTO_TRUE)
					printf(",");
				switch((1L << i))
				{
					case OTTO_SUN: printf("su"); break;
					case OTTO_MON: printf("mo"); break;
					case OTTO_TUE: printf("tu"); break;
					case OTTO_WED: printf("we"); break;
					case OTTO_THU: printf("th"); break;
					case OTTO_FRI: printf("fr"); break;
					case OTTO_SAT: printf("sa"); break;
				}
				one_printed = OTTO_TRUE;
			}
		}
	}
	printf("\n");

	switch(jobwork[id].date_conditions)
	{
		default:
			printf("start_mins     :\n");
			printf("start_times    :\n");
			break;
		case OTTO_USE_STARTMINS:
			printf("start_mins     : ");
			one_printed = OTTO_FALSE;
			for(i=0; i<60; i++)
			{
				if(jobwork[id].start_mins & (1L << i))
				{
					if(one_printed == OTTO_TRUE)
						printf(",");
					printf("%d", i);
					one_printed = OTTO_TRUE;
				}
			}
			printf("\n");
			printf("start_times    :\n");
			break;
		case OTTO_USE_STARTTIMES:
			printf("start_mins     :\n");
			printf("start_times    : \"");
			one_printed = OTTO_FALSE;
			for(i=0; i<24; i++)
			{
				for(j=0; j<60; j++)
				{
					if(jobwork[id].start_times[i] & (1L << j))
					{
						if(one_printed == OTTO_TRUE)
							printf(",");
						printf("%d:%02d", i, j);
						one_printed = OTTO_TRUE;
					}
				}
			}
			printf("\"\n");
			break;
	}

	printf("status         : ");
	switch(jobwork[id].status)
	{
		case STAT_IN: printf("IN\n"); break;
		case STAT_AC: printf("AC\n"); break;
		case STAT_RU: printf("RU\n"); break;
		case STAT_SU: printf("SU\n"); break;
		case STAT_FA: printf("FA\n"); break;
		case STAT_TE: printf("TE\n"); break;
		case STAT_OH: printf("OH\n"); break;
		default:      printf("--\n"); break;
	}
	if(jobwork[id].type != 'b')
		printf("pid            : %d\n",  jobwork[id].pid);
	printf("exit_status    : %d\n",  jobwork[id].exit_status);
	printf("start          : ");
	if(jobwork[id].start == 0)
	{
		printf("-\n");
	}
	else
	{
		tmp_time_t = jobwork[id].start;
	   tm_time = localtime(&tmp_time_t);
		strftime(c_time, 256, "%m/%d/%Y %T", tm_time);
		printf("%s\n", c_time);
	}

	printf("finish         : ");
	if(jobwork[id].finish == 0)
	{
		printf("-\n");
	}
	else
	{
	   tmp_time_t = jobwork[id].finish;
	   tm_time = localtime(&tmp_time_t);
		strftime(c_time, 256, "%m/%d/%Y %T", tm_time);
		printf("%s\n", c_time);
	}

	printf("duration       : ");
	if(jobwork[id].duration == 0 && (jobwork[id].start == 0 || jobwork[id].finish == 0))
	{
		printf("-\n");
	}
	else
	{
		printf("%02ld:%02ld:%02ld\n", jobwork[id].duration / 3600, jobwork[id].duration / 60 % 60, jobwork[id].duration % 60);
	}

	printf("\n");
}



void
output_jobwork_pid(int id)
{
	if(jobwork[id].type == 'b' || jobwork[id].pid <= 0)
		printf("-1\n");
	else
		printf("%d\n",  jobwork[id].pid);
}



void
output_jobwork_query(int id, int levelmod)
{
	char *indent;
	int64_t one_printed = OTTO_FALSE, i = 0, j = 0;

	indent = indentation[jobwork[id].level-levelmod];

	printf("%s/* ----------------- %s ----------------- */\n", indent, jobwork[id].name);
	printf("\n");

	printf("%sinsert_job:      %s\n", indent, jobwork[id].name);
	printf("%sjob_type:        %c\n", indent, jobwork[id].type);

	if(jobwork[id].box_name[0] != '\0')
		printf("%sbox_name:        %s\n", indent, jobwork[id].box_name);

	if(jobwork[id].command[0] != '\0')
		printf("%scommand:         %s\n", indent, jobwork[id].command);

	if(jobwork[id].condition[0] != '\0')
	{
		printf("%scondition:       %s\n", indent, jobwork[id].condition);
	}

	if(jobwork[id].description[0] != '\0')
		printf("%sdescription:     %s\n", indent, jobwork[id].description);

	if(jobwork[id].base_auto_hold == OTTO_TRUE)
		printf("%sauto_hold:       1\n", indent);

	if(jobwork[id].date_conditions != OTTO_FALSE)
	{
		printf("%sdate_conditions: 1\n", indent);

		printf("%sdays_of_week:    ", indent);
		if(jobwork[id].days_of_week == OTTO_ALL)
		{
			printf("all");
		}
		else
		{
			one_printed = OTTO_FALSE;
			for(i=0; i<7; i++)
			{
				if(jobwork[id].days_of_week & (1L << i))
				{
					if(one_printed == OTTO_TRUE)
						printf(",");
					switch((1L << i))
					{
						case OTTO_SUN: printf("su"); break;
						case OTTO_MON: printf("mo"); break;
						case OTTO_TUE: printf("tu"); break;
						case OTTO_WED: printf("we"); break;
						case OTTO_THU: printf("th"); break;
						case OTTO_FRI: printf("fr"); break;
						case OTTO_SAT: printf("sa"); break;
					}
					one_printed = OTTO_TRUE;
				}
			}
		}
		printf("\n");

		switch(jobwork[id].date_conditions)
		{
			case OTTO_USE_STARTMINS:
				printf("%sstart_mins:      ", indent);
				one_printed = OTTO_FALSE;
				for(i=0; i<60; i++)
				{
					if(jobwork[id].start_mins & (1L << i))
					{
						if(one_printed == OTTO_TRUE)
							printf(",");
						printf("%d", (int)i);
						one_printed = OTTO_TRUE;
					}
				}
				printf("\n");
				break;
			case OTTO_USE_STARTTIMES:
				printf("%sstart_times:     \"", indent);
				one_printed = OTTO_FALSE;
				for(i=0; i<24; i++)
				{
					for(j=0; j<60; j++)
					{
						if(jobwork[id].start_times[i] & (1L << j))
						{
							if(one_printed == OTTO_TRUE)
								printf(",");
							printf("%d:%02d", (int)i, (int)j);
							one_printed = OTTO_TRUE;
						}
					}
				}
				printf("\"\n");
				break;
		}
	}

	printf("\n");
}



int
output_jobwork_summary(int id, int levelmod)
{
	int        retval = 0;
	char      *indent;
	time_t     tmp_time_t, now, duration;
	struct tm *tm_time;
	char       c_time[25], *ne_str="", *su_str;

	now = time(0);

	if(ottorep_header_printed == OTTO_FALSE)
	{
		printf("\n");
		printf("Job Name                                                         Last Start           Last End             ST Runtime\n");
		printf("________________________________________________________________ ____________________ ____________________ __ ________\n");

		ottorep_header_printed = OTTO_TRUE;
	}

	indent = indentation[jobwork[id].level-levelmod];

	printf("%s%-*s ", indent, 64-jobwork[id].level+levelmod, jobwork[id].name);

	if(jobwork[id].start == 0)
	{
		printf("%-20s ", "-----");
	}
	else
	{
		tmp_time_t = jobwork[id].start;
	   tm_time = localtime(&tmp_time_t);
	   strftime(c_time, 256, "%m/%d/%Y %T", tm_time);
		printf("%-20s ", c_time);
	}

	if(jobwork[id].finish == 0)
	{
		printf("%-20s ", "-----");
	}
	else
	{
	   tmp_time_t = jobwork[id].finish;
	   tm_time = localtime(&tmp_time_t);
	   strftime(c_time, 256, "%m/%d/%Y %T", tm_time);
		printf("%-20s ", c_time);
	}

	if(jobwork[id].on_noexec == OTTO_FALSE)
	{
		su_str = "SU ";
	}
	else
	{
		su_str = "NE ";
		ne_str = "onnoexec";
	}

	if(jobwork[id].auto_hold == OTTO_TRUE)
	{
		ne_str = "autohold";
	}

	if(jobwork[id].expr_fail == OTTO_TRUE)
	{
		ne_str = "exprfail";
	}

	switch(jobwork[id].status)
	{
		case STAT_IN: printf("IN "); retval++; break;
		case STAT_AC: printf("AC "); retval++; break;
		case STAT_RU: printf("RU "); retval++; break;
		case STAT_SU: printf("%s", su_str); break;
		case STAT_FA: printf("FA "); break;
		case STAT_TE: printf("TE "); break;
		case STAT_OH: printf("OH "); break;
		default:      printf("-- "); break;
	}

	if(jobwork[id].duration == 0 && (jobwork[id].start == 0 || jobwork[id].finish == 0))
	{
		if(jobwork[id].status == STAT_RU || ne_str[0] == '\0')
		{
			if(jobwork[id].start != 0 && cfg.show_sofar == OTTO_TRUE)
			{
				duration = now - jobwork[id].start;
				printf("%02ld:%02ld:%02ld\n", duration / 3600, duration / 60 % 60, duration % 60);
			}
			else
			{
				printf("--------\n");
			}
		}
		else
			printf("%s\n", ne_str);

	}
	else
	{
		printf("%02ld:%02ld:%02ld\n", jobwork[id].duration / 3600, jobwork[id].duration / 60 % 60, jobwork[id].duration % 60);
	}

	return(retval);
}



void
output_expression(char *s)
{
	int exit_loop  = OTTO_FALSE;
	int16_t index;
	char *i;

	while(*s != '\0' && exit_loop == OTTO_FALSE)
	{
		switch(*s)
		{
			case '&':
			case '|':
			case '(':
			case ')':
			default:
				printf("%c", *s);
				break;

			case 'd': 
			case 'f': 
			case 'n': 
			case 'r': 
			case 's': 
			case 't':
				printf("%c", *s++);
				i = (char *)&index;
				*i++ = *s++;
				*i   = *s;
				if(index < 0)
					printf("----");
				else
					printf("%04d", index);
				break;
		}

		s++;
	}
}



void
output_expression_status(char *s)
{
	int exit_loop  = OTTO_FALSE;
	ecnd_st c;
	int value;

	while(*s != '\0' && exit_loop == OTTO_FALSE)
	{
		switch(*s)
		{
			case '&':
			case '|':
			case '(':
			case ')':
			default:
				printf("%c", *s);
				break;

			case 'd': 
			case 'f': 
			case 'n': 
			case 'r': 
			case 's': 
			case 't':
				c.s = s;
				value = evaluate_stat(&c);
				if(value != OTTO_FAIL)
					printf(" (%d) ", value);
				else
					printf(" (-) ");
				s+=2;
				break;
		}

		s++;
	}
}



int
evaluate_stat(ecnd_st *c)
{
	int retval = OTTO_FALSE;
	int16_t index;
	char *i;
	char test;

	i = (char *)&index;

	test = *(c->s++);
	*i++ = *(c->s++);
	*i   = *(c->s);

	if(index >= 0)
	{
		switch(test)
		{
			case 'd':
				if(job[index].status == STAT_SU ||
					job[index].status == STAT_FA ||
					job[index].status == STAT_TE)
					retval = OTTO_TRUE;
				break;

			case 'f':
				if(job[index].status == STAT_FA)
					retval = OTTO_TRUE;
				break;

			case 'n':
				if(job[index].status != STAT_RU)
					retval = OTTO_TRUE;
				break;

			case 'r':
				if(job[index].status == STAT_RU)
					retval = OTTO_TRUE;
				break;

			case 's':
				if(job[index].status == STAT_SU)
					retval = OTTO_TRUE;
				break;

			case 't':
				if(job[index].status == STAT_TE)
					retval = OTTO_TRUE;
				break;

			default:
				retval = OTTO_FALSE;
				break;
		}
	}
	else
	{
		retval = OTTO_FAIL;
	}

	return(retval);
}



/*
vim: ts=3 sw=3 ai
*/
