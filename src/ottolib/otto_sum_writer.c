#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "ottobits.h"
#include "ottocfg.h"
#include "ottocond.h"
#include "otto_sum_writer.h"


char *indentation[9] = {"", " ", "  ", "   ", "    ", "     ", "      ", "       ", "        "};


void write_summary_header(void);
void write_job_summary(JOB *item);



int
write_sum(JOBLIST *joblist)
{
	int retval = OTTO_SUCCESS;
	int i;

	if(joblist == NULL)
		retval = OTTO_FAIL;

	if(retval == OTTO_SUCCESS)
	{
		for(i=0; i<joblist->nitems; i++)
		{
			if(i == 0)
				write_summary_header();

			write_job_summary(&joblist->item[i]);
		}
	}

	return(retval);
}



void
write_summary_header()
{
	char *underline = "____________________________________________________________________________________";

	printf("\n");
	printf("%64.64s %20.20s %20.20s %2.2s %8.8s\n", "Job Name", "Last Start", "Last End", "ST",      "Runtime");
	printf("%64.64s %20.20s %20.20s %2.2s %8.8s\n", underline,  underline,    underline,  underline, underline);
}




void
write_job_summary(JOB *item)
{
	time_t     tmp_time_t, now, duration;
	struct tm *tm_time;
	char       c_time[25], *ne_str="", *su_str;

	now = time(0);

	printf("%s%-*s ", indentation[item->level], 64-item->level, item->name);

	if(item->start == 0)
	{
		printf("%-20s ", "-----");
	}
	else
	{
		tmp_time_t = item->start;
		tm_time = localtime(&tmp_time_t);
		strftime(c_time, 256, "%m/%d/%Y %T", tm_time);
		printf("%-20s ", c_time);
	}

	if(item->finish == 0)
	{
		printf("%-20s ", "-----");
	}
	else
	{
		tmp_time_t = item->finish;
		tm_time = localtime(&tmp_time_t);
		strftime(c_time, 256, "%m/%d/%Y %T", tm_time);
		printf("%-20s ", c_time);
	}

	if(item->on_noexec == OTTO_FALSE)
	{
		su_str = "SU";
	}
	else
	{
		su_str = "NE";
		ne_str = "onnoexec";
	}

	if(item->auto_hold == OTTO_TRUE)
	{
		ne_str = "autohold";
	}

	if(item->expr_fail == OTTO_TRUE)
	{
		ne_str = "exprfail";
	}

	switch(item->status)
	{
		case STAT_IN: printf("IN "); break;
		case STAT_AC: printf("AC "); break;
		case STAT_RU: printf("RU "); break;
		case STAT_SU: printf("%s ", su_str); break;
		case STAT_FA: printf("FA "); break;
		case STAT_TE: printf("TE "); break;
		case STAT_OH: printf("OH "); break;
		default:      printf("-- "); break;
	}

	if(item->duration == 0 && (item->start == 0 || item->finish == 0))
	{
		if(item->status == STAT_RU || ne_str[0] == '\0')
		{
			if(item->start != 0 && cfg.show_sofar == OTTO_TRUE)
			{
				duration = now - item->start;
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
		printf("%02ld:%02ld:%02ld\n", item->duration / 3600, item->duration / 60 % 60, item->duration % 60);
	}
}



//
// vim: ts=3 sw=3 ai
//
