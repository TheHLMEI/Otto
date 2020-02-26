#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "ottobits.h"
#include "ottoipc.h"
#include "otto_jil_writer.h"



void write_insert_job_jil(JOB *item);
void write_update_job_jil(JOB *item);
void write_delete_job_jil(JOB *item);
void write_delete_box_jil(JOB *item);



int
write_jil(JOBLIST *joblist)
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
				case CREATE_JOB: write_insert_job_jil(&joblist->item[i]); break;
				case UPDATE_JOB: write_update_job_jil(&joblist->item[i]); break;
				case DELETE_JOB: write_delete_job_jil(&joblist->item[i]); break;
				case DELETE_BOX: write_delete_box_jil(&joblist->item[i]); break;
				default:
					break;
			}
		}
	}

	return(retval);
}



void
write_insert_job_jil(JOB *item)
{
	char indent[128];
	int64_t one_printed = OTTO_FALSE, i = 0, j = 0;

	sprintf(indent, "%*.*s", item->level, item->level, " ");

	printf("%s/* ----------------- %s ----------------- */\n", indent, item->name);
	printf("\n");

	printf("%sinsert_job:      %s\n", indent, item->name);
	printf("%sjob_type:        %c\n", indent, item->type);

	if(item->box_name[0] != '\0')
		printf("%sbox_name:        %s\n", indent, item->box_name);

	if(item->command[0] != '\0')
		printf("%scommand:         %s\n", indent, item->command);

	if(item->condition[0] != '\0')
	{
		printf("%scondition:       %s\n", indent, item->condition);
	}

	if(item->description[0] != '\0')
		printf("%sdescription:     %s\n", indent, item->description);

	if(item->base_auto_hold == OTTO_TRUE)
		printf("%sauto_hold:       1\n", indent);

	if(item->date_conditions != OTTO_FALSE)
	{
		printf("%sdate_conditions: 1\n", indent);

		printf("%sdays_of_week:    ", indent);
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
			case OTTO_USE_STARTMINS:
				printf("%sstart_mins:      ", indent);
				one_printed = OTTO_FALSE;
				for(i=0; i<60; i++)
				{
					if(item->start_mins & (1L << i))
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
						if(item->start_times[i] & (1L << j))
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



void
write_update_job_jil(JOB *item)
{
	printf("update_job:      %s\n", item->name);

	printf("\n");
}



void
write_delete_job_jil(JOB *item)
{
	printf("delete_job:      %s\n", item->name);

	printf("\n");
}



void
write_delete_box_jil(JOB *item)
{
	printf("delete_box:      %s\n", item->name);

	printf("\n");
}



//
// vim: ts=3 sw=3 ai
//
