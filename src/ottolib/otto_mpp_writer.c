#include <stdio.h>
#include <string.h>
#include <time.h>

#include "ottobits.h"
#include "ottocfg.h"
#include "ottoutil.h"
#include "otto_mpp_bits.h"
#include "otto_mpp_writer.h"

typedef struct _extdef
{
	char *fieldid;
	char *fieldname;
	char *alias;
} EXTDEF;

EXTDEF EXT[EXTDEF_TOTAL] = {
	{"188743731",  "Text1", "Job Script"},
	{"188743734",  "Text2", "Parameters"},
	{"188743737",  "Text3", "description"},
	{"188743752",  "Flag1", "auto_hold"},
	{"188743753",  "Flag2", "date_conditions"},
	{"188743740",  "Text4", "days_of_week"},
	{"188743743",  "Text5", "start_mins"},
	{"188743746",  "Text6", "start_times"}
};


void write_mpp_xml_preamble();
void write_mpp_xml_tasks(JOBLIST *joblist, int autoschedule);
void write_mpp_xml_task(JOB *item, char *outline, int autoschedule);
void write_mpp_xml_predecessors(char *name, char *expression);
void write_mpp_xml_postamble();
int  compile_expressions(JOBLIST *joblist);



int
write_mpp_xml(JOBLIST *joblist, int autoschedule)
{
	int retval = OTTO_SUCCESS;

	if(joblist == NULL)
		retval = OTTO_FAIL;

	// compile expressions against the jobs specified in the joblist
	// so no broken dependencies are produced
	if(retval == OTTO_SUCCESS)
		retval = compile_expressions(joblist);

	if(retval == OTTO_SUCCESS)
	{
		write_mpp_xml_preamble(joblist, autoschedule);
		write_mpp_xml_tasks(joblist, autoschedule);
		write_mpp_xml_postamble();
	}

	return(retval);
}



void
write_mpp_xml_preamble(JOBLIST *joblist, int autoschedule)
{
	int       i;
	time_t    t;
	struct tm *now;
	int       start = -1;

	// find item in joblist with the earliest start time
	for(i=0; i<joblist->nitems; i++)
	{
		if(joblist->item[i].start != 0)
		{
			if(start == -1)
			{
				start = i;
			}
		}
		else if(joblist->item[i].start < joblist->item[start].start)
		{
			start = i;
		}
	}

	printf("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n");
	printf("<Project xmlns=\"http://schemas.microsoft.com/project\">\n");
	printf("	<SaveVersion>14</SaveVersion>\n");
	printf("	<Name>OttoMSP.xml</Name>\n");
	printf("	<Title>OttoMSP</Title>\n");

	if(autoschedule == OTTO_TRUE || start == -1)
	{
		printf("	<ScheduleFromStart>1</ScheduleFromStart>\n");
		time(&t);
		now = localtime(&t);
		now->tm_hour = 8; now->tm_min = 0; now->tm_sec = 0;
		t = mktime(now);
	}
	else
	{
		t = joblist->item[start].start;
	}
	printf("	<StartDate>%s</StartDate>\n", format_timestamp(t));
	printf("	<CalendarUID>3</CalendarUID>\n");
	printf("	<DefaultStartTime>08:00:00</DefaultStartTime>\n");
	printf("	<MinutesPerDay>1440</MinutesPerDay>\n");
	printf("	<MinutesPerWeek>10080</MinutesPerWeek>\n");
	printf("	<DaysPerMonth>31</DaysPerMonth>\n");

	if(autoschedule == OTTO_TRUE)
	{
		printf("	<DurationFormat>3</DurationFormat>\n");
	}
	else
	{
		printf("	<DurationFormat>4</DurationFormat>\n");
	}

	printf("	<WeekStartDay>0</WeekStartDay>\n");
	printf("	<UpdateManuallyScheduledTasksWhenEditingLinks>1</UpdateManuallyScheduledTasksWhenEditingLinks>\n");
	printf("	<ExtendedAttributes>\n");

	for(i=0; i<8; i++)
	{
		printf("		<ExtendedAttribute>\n");
		printf("			<FieldID>%s</FieldID>\n",      EXT[i].fieldid);
		printf("			<FieldName>%s</FieldName>\n",  EXT[i].fieldname);
		printf("			<Alias>%s</Alias>\n",          EXT[i].alias);
		printf("		</ExtendedAttribute>\n");
	}

	printf("	</ExtendedAttributes>\n");
	printf("	<Calendars>\n");
	printf("		<Calendar>\n");
	printf("			<UID>1</UID>\n");
	printf("			<Name>Standard</Name>\n");
	printf("			<IsBaseCalendar>1</IsBaseCalendar>\n");
	printf("			<IsBaselineCalendar>0</IsBaselineCalendar>\n");
	printf("			<BaseCalendarUID>0</BaseCalendarUID>\n");
	printf("			<WeekDays>\n");
	printf("				<WeekDay>\n");
	printf("					<DayType>1</DayType>\n");
	printf("					<DayWorking>0</DayWorking>\n");
	printf("				</WeekDay>\n");

	for(i=2; i<7; i++)
	{
		printf("				<WeekDay>\n");
		printf("					<DayType>%d</DayType>\n", i);
		printf("					<DayWorking>1</DayWorking>\n");
		printf("					<WorkingTimes>\n");
		printf("						<WorkingTime><FromTime>08:00:00</FromTime><ToTime>12:00:00</ToTime></WorkingTime>\n");
		printf("						<WorkingTime><FromTime>13:00:00</FromTime><ToTime>17:00:00</ToTime></WorkingTime>\n");
		printf("					</WorkingTimes>\n");
		printf("				</WeekDay>\n");
	}

	printf("				<WeekDay>\n");
	printf("					<DayType>7</DayType>\n");
	printf("					<DayWorking>0</DayWorking>\n");
	printf("				</WeekDay>\n");
	printf("			</WeekDays>\n");
	printf("		</Calendar>\n");
	printf("		<Calendar>\n");
	printf("			<UID>3</UID>\n");
	printf("			<Name>24 Hours</Name>\n");
	printf("			<IsBaseCalendar>1</IsBaseCalendar>\n");
	printf("			<IsBaselineCalendar>0</IsBaselineCalendar>\n");
	printf("			<BaseCalendarUID>0</BaseCalendarUID>\n");
	printf("			<WeekDays>\n");

	for(i=1; i<8; i++)
	{
		printf("				<WeekDay>\n");
		printf("					<DayType>%d</DayType>\n", i);
		printf("					<DayWorking>1</DayWorking>\n");
		printf("					<WorkingTimes>\n");
		printf("						<WorkingTime><FromTime>00:00:00</FromTime><ToTime>00:00:00</ToTime></WorkingTime>\n");
		printf("					</WorkingTimes>\n");
		printf("				</WeekDay>\n");
	}

	printf("			</WeekDays>\n");
	printf("		</Calendar>\n");
	printf("	</Calendars>\n");
	printf("	<Tasks>\n");
}



void
write_mpp_xml_tasks(JOBLIST *joblist, int autoschedule)
{
	int  i, l;
   char outline[128], tmpstr[128];
	int  levels[99] = { 0 };
	
	for(i=0; i<joblist->nitems; i++)
	{
		// increment outline count for the current level
		levels[joblist->item[i].level]++;

		// create an outline string
		sprintf(outline, "%d", levels[0]);

		for(l=1; l<joblist->item[i].level; l++)
		{
			sprintf(tmpstr, ".%d", levels[l]);
			strcat(outline, tmpstr);
		}

		// output a single MPP xml task
		write_mpp_xml_task(&joblist->item[i], outline, autoschedule);

		// reset outline count for the next level
		if(joblist->item[i].type == OTTO_BOX)
			levels[joblist->item[i].level+1] = 0;
	}
}



void
write_mpp_xml_task(JOB *item, char *outline, int autoschedule)
{
	int d, h, m, one_printed;
	char *s;
	char wrapper[CMDLEN+1];
	char jobscript[CMDLEN+1];
	char parameters[CMDLEN+1];
	char description[DSCLEN+1];
	
	wrapper[0]    = '\0';
	jobscript[0]  = '\0';
	parameters[0] = '\0';
	
	printf("		<Task>\n");
	printf("			<UID>%d</UID>\n",                     item->id);
	printf("			<ID>%d</ID>\n",                       item->id);
	printf("			<Name>%s</Name>\n",                   item->name);
	printf("			<OutlineLevel>%d</OutlineLevel>\n",   item->level+1);
	printf("			<OutlineNumber>%s</OutlineNumber>\n", outline);

	printf("			<Active>1</Active>\n");
	printf("			<Type>0</Type>\n");
	if(autoschedule != OTTO_TRUE && item->start != 0)
	{
		printf("			<ActualStart>%s</ActualStart>\n", format_timestamp(item->start));
	}
	printf("			<RemainingDuration>%s</RemainingDuration>\n", format_duration(item->duration));

	if(autoschedule == OTTO_TRUE)
	{
		printf("			<DurationFormat>3</DurationFormat>\n");
	}
	else
	{
		printf("			<DurationFormat>4</DurationFormat>\n");
	}
	printf("			<Summary>%c</Summary>\n", item->type == OTTO_BOX ? '1' : '0');
	printf("			<Rollup>0</Rollup>\n");
			
	if(item->condition[0] != '\0')
	{
		write_mpp_xml_predecessors(item->name, item->expression);
	}

	if(item->type != OTTO_BOX)
	{
		s = item->command;
		s = copy_word(wrapper, s);
		if(cfg.wrapper_script != NULL && cfg.wrapper_script[0] != '\0' && strcmp(cfg.wrapper_script, wrapper) != 0)
		{
			strcpy(jobscript, wrapper);
			wrapper[0] = '\0';
		}
		else
		{
			s = copy_word(jobscript, s);
		}
		copy_eol(parameters, s);

		if(jobscript[0] != '\0')
		{
			printf("			<ExtendedAttribute>\n");
			printf("				<FieldID>%s</FieldID>\n", EXT[JOBSCRIPT].fieldid);
			printf("				<Value>");
			output_xml(jobscript);
			printf("</Value>\n");
			printf("			</ExtendedAttribute>\n");
		}

		if(parameters[0] != '\0')
		{
			printf("			<ExtendedAttribute>\n");
			printf("				<FieldID>%s</FieldID>\n", EXT[PARAMETERS].fieldid);
			printf("				<Value>");
			output_xml(parameters);
			printf("</Value>\n");
			printf("			</ExtendedAttribute>\n");
		}
	}

	if(item->description[0] != '\0')
	{
		printf("			<ExtendedAttribute>\n");
		printf("				<FieldID>%s</FieldID>\n", EXT[DESCRIPTION].fieldid);
		printf("				<Value>");
		if(item->description[0] == '"')
		{
			strcpy(description, &item->description[1]);
			for(d=strlen(description)-1; d>0; d--)
			{
				if(description[d] == '"')
				{
					description[d] = '\0';
					break;
				}
			}
		}
		else
		{
			strcpy(description, item->description);
		}
		if(strlen(description) > 256)
		{
			description[252] = '.';
			description[253] = '.';
			description[254] = '.';
			description[255] = '\0';
		}
		output_xml(description);
		printf("</Value>\n");
		printf("			</ExtendedAttribute>\n");
	}

	printf("			<ExtendedAttribute>\n");
	printf("				<FieldID>%s</FieldID>\n", EXT[AUTOHOLD].fieldid);
	printf("				<Value>");
	if(item->autohold == OTTO_TRUE)
		printf("1");
	else
		printf("0");

	printf("</Value>\n");

	printf("			</ExtendedAttribute>\n");

	if(item->date_conditions != OTTO_FALSE)
	{
		printf("			<ExtendedAttribute>\n");
		printf("				<FieldID>%s</FieldID>\n", EXT[DATE_CONDITIONS].fieldid);
		printf("				<Value>1</Value>\n");
		printf("			</ExtendedAttribute>\n");


		printf("			<ExtendedAttribute>\n");
		printf("				<FieldID>%s</FieldID>\n", EXT[DAYS_OF_WEEK].fieldid);
		printf("				<Value>");
		if(item->days_of_week == OTTO_ALL)
		{
			printf("all");
		}
		else
		{
			one_printed = OTTO_FALSE;
			for(d=0; d<7; d++)
			{
				if(item->days_of_week & (1L << d))
				{
					if(one_printed == OTTO_TRUE)
						printf(",");
					switch((1L << d))
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
		printf("</Value>\n");
		printf("			</ExtendedAttribute>\n");


		switch(item->date_conditions)
		{
			case OTTO_USE_START_MINUTES:
				printf("			<ExtendedAttribute>\n");
				printf("				<FieldID>%s</FieldID>\n", EXT[START_MINUTES].fieldid);
				printf("				<Value>");
				one_printed = OTTO_FALSE;
				for(m=0; m<60; m++)
				{
					if(item->start_minutes & (1L << m))
					{
						if(one_printed == OTTO_TRUE)
							printf(",");
						printf("%d", m);
						one_printed = OTTO_TRUE;
					}
				}
				printf("</Value>\n");
				printf("			</ExtendedAttribute>\n");
				break;
			case OTTO_USE_START_TIMES:
				printf("			<ExtendedAttribute>\n");
				printf("				<FieldID>%s</FieldID>\n", EXT[START_TIMES].fieldid);
				printf("				<Value>");
				one_printed = OTTO_FALSE;
				for(h=0; h<24; h++)
				{
					for(m=0; m<60; m++)
					{
						if(item->start_times[h] & (1L << m))
						{
							if(one_printed == OTTO_TRUE)
								printf(",");
							printf("%d:%02d", h, m);
							one_printed = OTTO_TRUE;
						}
					}
				}
				printf("</Value>\n");
				printf("			</ExtendedAttribute>\n");
				break;
		}
	}

	printf("		</Task>\n");
}



void
write_mpp_xml_predecessors(char *name, char *expression)
{
	char *s = expression;
	int16_t index;
	char *i;

	while(*s != '\0')
	{
		switch(*s)
		{
			default:
				break;

			case 'd':
			case 'f':
			case 'n':
			case 't':
				fprintf(stderr, "WARNING for job: %s < Condition '%c' is inconsistent with MS Project predecessor concept >.\n",
						name, *s);
				break;
			case 'r':
				s++;
				i = (char *)&index;
				*i++ = *s++;
				*i   = *s;
				if(index >= 0)
				{
					printf("			<PredecessorLink>\n");
					printf("				<PredecessorUID>%d</PredecessorUID>\n", index);
					printf("				<Type>3</Type>\n");
					printf("			</PredecessorLink>\n");
				}
				break;
			case 's':
				s++;
				i = (char *)&index;
				*i++ = *s++;
				*i   = *s;
				if(index >= 0)
				{
					printf("			<PredecessorLink>\n");
					printf("				<PredecessorUID>%d</PredecessorUID>\n", index);
					printf("				<Type>1</Type>\n");
					printf("			</PredecessorLink>\n");
				}
				break;
		}

		s++;
	}
}



void
write_mpp_xml_postamble()
{
	printf("	</Tasks>\n");
	printf("</Project>\n");
}



int
compile_expressions(JOBLIST *joblist)
{
	int retval = OTTO_SUCCESS;
	int exit_loop;
	char name[NAMLEN+1];
	char *idx, *n, *s, *t;
	int16_t index;
	int l, i, item_ok;

	for(i=0; i<joblist->nitems; i++)
	{

		if(joblist->item[i].condition[0] != '\0')
		{
			s = joblist->item[i].condition;
			t = joblist->item[i].expression;
			l = CNDLEN;;
			item_ok = OTTO_TRUE;

			while(l > 0 && *s != '\0' && item_ok == OTTO_TRUE)
			{
				switch(*s)
				{
					case '(':
					case ')':
					case '&':
					case '|':
						*t++ = *s;
						l--;
						break;
					case 'd':
					case 'f':
					case 'n':
					case 'r':
					case 's':
					case 't':
						*t++ = *s++;
						l--;
						if(l > 2)
						{
							n = name;
							exit_loop = OTTO_FALSE;
							while(exit_loop == OTTO_FALSE && *s != '\0' && item_ok == OTTO_TRUE)
							{
								switch(*s)
								{
									case '(':
										break;
									case ')':
										*n = '\0';
										exit_loop = OTTO_TRUE;
										break;
									default:
										*n++ = *s;
										*n   = '\0';
										if(n - name == NAMLEN)
										{
											fprintf(stderr, "Condition compile name exceeds allowed length (%d bytes).\n", NAMLEN);
											item_ok = OTTO_FALSE;
											retval = OTTO_FAIL;
										}
										break;
								}
								s++;
							}

							// search for name
							for(index = 0; index < joblist->nitems; index++)
							{
								if(strcmp(name, joblist->item[index].name) == 0)
									break;
							}
							if(index == joblist->nitems)
							{
								index = -1;
							}
							idx = (char *)&index;
							*t++ = *idx++;
							*t++ = *idx;
							l -= 2;
						}
						else
						{
							fprintf(stderr, "Condition exceeds compile buffer (%d bytes).\n", CNDLEN);
							item_ok = OTTO_FALSE;
							retval = OTTO_FAIL;
						}
						break;
				}

				s++;
			}
			*t = '\0';
		}
	}

	return(retval);
}




//
// vim: ts=3 sw=3 ai
//
