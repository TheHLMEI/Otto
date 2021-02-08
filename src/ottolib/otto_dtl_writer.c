#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "ottobits.h"
#include "ottocond.h"
#include "ottoipc.h"
#include "otto_dtl_writer.h"



void write_insert_job_dtl(JOB *item);
void write_expression_dtl(char *s);
void write_expression_status_dtl(char *s);



int
write_dtl(JOBLIST *joblist)
{
	int retval = OTTO_SUCCESS;
	int i;

	if(joblist == NULL)
		retval = OTTO_FAIL;

	if(retval == OTTO_SUCCESS)
	{
		for(i=0; i<joblist->nitems; i++)
		{
			switch(joblist->item[i].opcode)
			{
				case CREATE_JOB: write_insert_job_dtl(&joblist->item[i]); break;
				case UPDATE_JOB:
				case DELETE_JOB:
				case DELETE_BOX:
				default:
									  break;
			}
		}
	}

	return(retval);
}



void
write_insert_job_dtl(JOB *item)
{
	time_t     tmp_time_t;
	struct tm *tm_time;
	char       c_time[25];
	int        i, j, one_printed;

	printf("---------------\n");
	printf("id             : %d\n",  item->id);
	printf("level          : %d\n",  item->level);
	printf("linkage        : box %d, head %d, tail %d, prev %d, next %d\n",
			                   item->box, item->head, item->tail, item->prev, item->next);
	printf("name           : %s\n",  item->name);
	printf("type           : %c\n",  item->type);
	printf("box_name       : %s\n",  item->box_name);
	printf("description    : %s\n",  item->description);
	if(item->type != OTTO_BOX)
		printf("command        : %s\n",  item->command);
	printf("condition      : %s\n", item->condition);
	if(item->date_conditions == 0)
		printf("date_conditions: 0\n");
	else
		printf("date_conditions: 1\n");

	printf("days_of_week   : ");
	if(item->days_of_week == OTTO_ALL)
	{
		printf("all");
	}
	else
	{
		one_printed = OTTO_FALSE;
		for(i=0; i<7; i++)
		{
			if(item->days_of_week & (1L << i))
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

	switch(item->date_conditions)
	{
		default:
			printf("start_mins     :\n");
			printf("start_times    :\n");
			break;
		case OTTO_USE_START_MINUTES:
			printf("start_minutes     : ");
			one_printed = OTTO_FALSE;
			for(i=0; i<60; i++)
			{
				if(item->start_minutes & (1L << i))
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
		case OTTO_USE_START_TIMES:
			printf("start_mins     :\n");
			printf("start_times    : \"");
			one_printed = OTTO_FALSE;
			for(i=0; i<24; i++)
			{
				for(j=0; j<60; j++)
				{
					if(item->start_times[i] & (1L << j))
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

	if(item->condition[0] != '\0')
	{
		printf("expression1    : ");
		write_expression_dtl(item->expression);
		printf("\n");
		printf("expression2    : ");
		write_expression_status_dtl(item->expression);
		printf("\n");
		printf("expr_fail      : %d\n",  item->expr_fail);
	}
	printf("status         : ");
	switch(item->status)
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
	printf("autohold       : %d\n",  item->autohold);
	printf("on_autohold    : %d\n",  item->on_autohold);
	printf("on_noexec      : %d\n",  item->on_noexec);
	if(item->type != OTTO_BOX)
		printf("pid            : %d\n",  item->pid);
	printf("start          : ");
	if(item->start == 0)
	{
		printf("-\n");
	}
	else
	{
		tmp_time_t = item->start;
		tm_time = localtime(&tmp_time_t);
		strftime(c_time, 256, "%m/%d/%Y %T", tm_time);
		printf("%s\n", c_time);
	}

	printf("finish         : ");
	if(item->finish == 0)
	{
		printf("-\n");
	}
	else
	{
		tmp_time_t = item->finish;
		tm_time = localtime(&tmp_time_t);
		strftime(c_time, 256, "%m/%d/%Y %T", tm_time);
		printf("%s\n", c_time);
	}

	printf("duration       : ");
	if(item->duration == 0 && (item->start == 0 || item->finish == 0))
	{
		printf("-\n");
	}
	else
	{
		printf("%02ld:%02ld:%02ld\n", item->duration / 3600, item->duration / 60 % 60, item->duration % 60);
	}
	printf("exit_status    : %d\n",  item->exit_status);

	printf("\n");
}



void
write_expression_dtl(char *s)
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
write_expression_status_dtl(char *s)
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



//
// vim: ts=3 sw=3 ai
//
