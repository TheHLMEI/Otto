#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "otto.h"

typedef struct _extdef
{
   char *fieldid;
   char *fieldname;
   char *alias;
} EXTDEF;


EXTDEF EXT[EXTDEF_TOTAL] = {
   {"188743731",  "Text1",   "command"},
   {"188743734",  "Text2",   "description"},
   {"188743737",  "Text3",   "environment"},
   {"188743740",  "Text4",   "days_of_week"},
   {"188743743",  "Text5",   "start_mins"},
   {"188743746",  "Text6",   "start_times"},
   {"188743747",  "Text7",   "loop"},
   {"188743752",  "Flag1",   "auto_hold"},
   {"188743753",  "Flag2",   "date_conditions"}
};


void buffer_mspdi_preamble(DYNBUF *b, JOBLIST *joblist, int *autoschedule);
void buffer_mspdi_tasks(DYNBUF *b, JOBLIST *joblist, int autoschedule);
void buffer_mspdi_task(DYNBUF *b, JOB *item, char *outline, int autoschedule);
void buffer_mspdi_predecessors(DYNBUF *b, char *name, char *expression);
void buffer_mspdi_postamble(DYNBUF *b);
void prep_mspdi_tasks(JOBLIST *joblist);
int  cmp_joblist_by_id(const void * a, const void * b);
int  compile_mspdi_expressions(JOBLIST *joblist);
char *format_mspdi_timestamp(time_t t);
char *format_mspdi_duration(time_t t, int autoschedule);



int
write_mspdi(JOBLIST *joblist, int autoschedule)
{
   int retval = OTTO_SUCCESS;
   DYNBUF b;

   if(joblist == NULL)
      retval = OTTO_FAIL;

   if(retval == OTTO_SUCCESS)
   {
      memset(&b, 0, sizeof(b));

      buffer_mspdi(&b, joblist, autoschedule);

      if(b.buffer != NULL)
      {
         fprintf(stdout, "%s", b.buffer);
         free(b.buffer);
      }
      else
      {
         fprintf(stdout, "Error preparing output\n");
      }
   }

   return(retval);
}



int
buffer_mspdi(DYNBUF *b, JOBLIST *joblist, int autoschedule)
{
   int retval = OTTO_SUCCESS;

   if(b == NULL || joblist == NULL)
      retval = OTTO_FAIL;

   // renumber joblist jobs, set levels, and
   // compile expressions against the jobs specified in the joblist
   // so no broken dependencies are produced
   if(retval == OTTO_SUCCESS)
   {
      prep_mspdi_tasks(joblist);
      retval = compile_mspdi_expressions(joblist);
   }

   if(retval == OTTO_SUCCESS)
   {
      buffer_mspdi_preamble(b, joblist, &autoschedule);
      buffer_mspdi_tasks(b, joblist, autoschedule);
      buffer_mspdi_postamble(b);
   }

   return(retval);
}



void
buffer_mspdi_preamble(DYNBUF *b, JOBLIST *joblist, int *autoschedule)
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
   if(start == -1)
   {
      *autoschedule = OTTO_TRUE;
   }
   if(*autoschedule == OTTO_TRUE)
   {
      time(&t);
      now = localtime(&t);
      now->tm_hour = 8; now->tm_min = 0; now->tm_sec = 0;
      t = mktime(now);
   }
   else
   {
      t = joblist->item[start].start;
   }

   bprintf(b, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n");
   bprintf(b, "<Project xmlns=\"http://schemas.microsoft.com/project\">\n");
   bprintf(b, "   <SaveVersion>14</SaveVersion>\n");
   bprintf(b, "   <Name>OttoMSP.xml</Name>\n");
   bprintf(b, "   <Title>OttoMSP</Title>\n");
   bprintf(b, "   <ScheduleFromStart>1</ScheduleFromStart>\n");
   bprintf(b, "   <StartDate>%s</StartDate>\n", format_mspdi_timestamp(t));
   bprintf(b, "   <CalendarUID>3</CalendarUID>\n");
   bprintf(b, "   <DefaultStartTime>08:00:00</DefaultStartTime>\n");
   bprintf(b, "   <MinutesPerDay>1440</MinutesPerDay>\n");
   bprintf(b, "   <MinutesPerWeek>10080</MinutesPerWeek>\n");
   bprintf(b, "   <DaysPerMonth>31</DaysPerMonth>\n");
   bprintf(b, "   <DurationFormat>%c</DurationFormat>\n", *autoschedule == OTTO_TRUE ? '3' : '4');
   bprintf(b, "   <WeekStartDay>0</WeekStartDay>\n");
   bprintf(b, "   <UpdateManuallyScheduledTasksWhenEditingLinks>1</UpdateManuallyScheduledTasksWhenEditingLinks>\n");
   bprintf(b, "   <ExtendedAttributes>\n");

   for(i=0; i<EXTDEF_TOTAL; i++)
   {
      bprintf(b, "      <ExtendedAttribute>\n");
      bprintf(b, "         <FieldID>%s</FieldID>\n",      EXT[i].fieldid);
      bprintf(b, "         <FieldName>%s</FieldName>\n",  EXT[i].fieldname);
      bprintf(b, "         <Alias>%s</Alias>\n",          EXT[i].alias);
      bprintf(b, "      </ExtendedAttribute>\n");
   }

   bprintf(b, "   </ExtendedAttributes>\n");
   bprintf(b, "   <Calendars>\n");
   bprintf(b, "      <Calendar>\n");
   bprintf(b, "         <UID>1</UID>\n");
   bprintf(b, "         <Name>Standard</Name>\n");
   bprintf(b, "         <IsBaseCalendar>1</IsBaseCalendar>\n");
   bprintf(b, "         <IsBaselineCalendar>0</IsBaselineCalendar>\n");
   bprintf(b, "         <BaseCalendarUID>0</BaseCalendarUID>\n");
   bprintf(b, "         <WeekDays>\n");
   bprintf(b, "            <WeekDay>\n");
   bprintf(b, "               <DayType>1</DayType>\n");
   bprintf(b, "               <DayWorking>0</DayWorking>\n");
   bprintf(b, "            </WeekDay>\n");

   for(i=2; i<7; i++)
   {
   bprintf(b, "            <WeekDay>\n");
   bprintf(b, "               <DayType>%d</DayType>\n", i);
   bprintf(b, "               <DayWorking>1</DayWorking>\n");
   bprintf(b, "               <WorkingTimes>\n");
   bprintf(b, "                  <WorkingTime><FromTime>08:00:00</FromTime><ToTime>12:00:00</ToTime></WorkingTime>\n");
   bprintf(b, "                  <WorkingTime><FromTime>13:00:00</FromTime><ToTime>17:00:00</ToTime></WorkingTime>\n");
   bprintf(b, "               </WorkingTimes>\n");
   bprintf(b, "            </WeekDay>\n");
   }

   bprintf(b, "            <WeekDay>\n");
   bprintf(b, "               <DayType>7</DayType>\n");
   bprintf(b, "               <DayWorking>0</DayWorking>\n");
   bprintf(b, "            </WeekDay>\n");
   bprintf(b, "         </WeekDays>\n");
   bprintf(b, "      </Calendar>\n");
   bprintf(b, "      <Calendar>\n");
   bprintf(b, "         <UID>3</UID>\n");
   bprintf(b, "         <Name>24 Hours</Name>\n");
   bprintf(b, "         <IsBaseCalendar>1</IsBaseCalendar>\n");
   bprintf(b, "         <IsBaselineCalendar>0</IsBaselineCalendar>\n");
   bprintf(b, "         <BaseCalendarUID>0</BaseCalendarUID>\n");
   bprintf(b, "         <WeekDays>\n");

   for(i=1; i<8; i++)
   {
   bprintf(b, "            <WeekDay>\n");
   bprintf(b, "               <DayType>%d</DayType>\n", i);
   bprintf(b, "               <DayWorking>1</DayWorking>\n");
   bprintf(b, "               <WorkingTimes>\n");
   bprintf(b, "                  <WorkingTime><FromTime>00:00:00</FromTime><ToTime>00:00:00</ToTime></WorkingTime>\n");
   bprintf(b, "               </WorkingTimes>\n");
   bprintf(b, "            </WeekDay>\n");
   }

   bprintf(b, "         </WeekDays>\n");
   bprintf(b, "      </Calendar>\n");
   bprintf(b, "   </Calendars>\n");
}



void
buffer_mspdi_tasks(DYNBUF *b, JOBLIST *joblist, int autoschedule)
{
   int  i, l;
   char outline[128], tmpstr[128];
   int  levels[99] = { 0 };

   bprintf(b, "   <Tasks>\n");
   for(i=0; i<joblist->nitems; i++)
   {
      // increment outline count for the current level
      levels[joblist->item[i].level]++;

      // create an outline string
      sprintf(outline, "%d", levels[1]);

      for(l=2; l<=joblist->item[i].level; l++)
      {
         sprintf(tmpstr, ".%d", levels[l]);
         strcat(outline, tmpstr);
      }

      // output a single MPP xml task
      buffer_mspdi_task(b, &joblist->item[i], outline, autoschedule);

      // reset outline count for the next level
      if(joblist->item[i].type == OTTO_BOX)
         levels[joblist->item[i].level+1] = 0;
   }
   bprintf(b, "   </Tasks>\n");
}



void
buffer_mspdi_task(DYNBUF *b, JOB *item, char *outline, int autoschedule)
{
   JOBTVAL tval;
   char loop[512];

   ottojob_prepare_txt_values(&tval, item, AS_MSPDI);

   bprintf(b, "   <Task>\n");
   bprintf(b, "      <UID>%d</UID>\n",                     item->id);
   bprintf(b, "      <ID>%d</ID>\n",                       item->id);
   bprintf(b, "      <Name>%s</Name>\n",                   item->name);
   bprintf(b, "      <OutlineLevel>%d</OutlineLevel>\n",   item->level);
   bprintf(b, "      <OutlineNumber>%s</OutlineNumber>\n", outline);

   bprintf(b, "      <Active>1</Active>\n");
   bprintf(b, "      <Type>0</Type>\n");
   if(autoschedule != OTTO_TRUE && item->start != 0)
   {
      bprintf(b, "      <ActualStart>%s</ActualStart>\n", format_mspdi_timestamp(item->start));
   }

   if(autoschedule != OTTO_TRUE && item->finish != 0)
   {
      if(item->duration < 6)
         bprintf(b, "      <ActualFinish>%s</ActualFinish>\n", format_mspdi_timestamp(item->start+6));
      else
         bprintf(b, "      <ActualFinish>%s</ActualFinish>\n", format_mspdi_timestamp(item->finish));

      bprintf(b, "      <ActualDuration>%s</ActualDuration>\n", format_mspdi_duration(item->duration, autoschedule));
   }
   else
   {
      bprintf(b, "      <RemainingDuration>%s</RemainingDuration>\n", format_mspdi_duration(item->duration, autoschedule));
   }

   bprintf(b, "      <DurationFormat>%c</DurationFormat>\n", autoschedule == OTTO_TRUE ? '3' : '4');
   bprintf(b, "      <Summary>%c</Summary>\n",               item->type   == OTTO_BOX  ? '1' : '0');
   bprintf(b, "      <Rollup>0</Rollup>\n");

   if(item->condition[0] != '\0')
   {
      buffer_mspdi_predecessors(b, item->name, item->expression);
   }

   if(item->type != OTTO_BOX)
   {
      bprintf(b, "         <ExtendedAttribute>\n");
      bprintf(b, "            <FieldID>%s</FieldID>\n", EXT[COMMAND].fieldid);
      bprintf(b, "            <Value>%s</Value>\n", tval.command);
      bprintf(b, "         </ExtendedAttribute>\n");
   }

   if(item->description[0] != '\0')
   {
      bprintf(b, "         <ExtendedAttribute>\n");
      bprintf(b, "            <FieldID>%s</FieldID>\n", EXT[DESCRIPTION].fieldid);
      bprintf(b, "            <Value>%s</Value>\n", tval.description);
      bprintf(b, "         </ExtendedAttribute>\n");
   }

   if(item->environment[0] != '\0')
   {
      bprintf(b, "         <ExtendedAttribute>\n");
      bprintf(b, "            <FieldID>%s</FieldID>\n", EXT[ENVIRONMENT].fieldid);
      bprintf(b, "            <Value>%s</Value>\n", tval.environment);
      bprintf(b, "         </ExtendedAttribute>\n");
   }

   bprintf(b, "         <ExtendedAttribute>\n");
   bprintf(b, "            <FieldID>%s</FieldID>\n", EXT[AUTOHOLD].fieldid);
   bprintf(b, "            <Value>%c</Value>\n", item->autohold == OTTO_TRUE ? '1' : '0');
   bprintf(b, "         </ExtendedAttribute>\n");

   if(item->date_conditions != OTTO_FALSE)
   {
      bprintf(b, "         <ExtendedAttribute>\n");
      bprintf(b, "            <FieldID>%s</FieldID>\n", EXT[DATE_CONDITIONS].fieldid);
      bprintf(b, "            <Value>1</Value>\n");
      bprintf(b, "         </ExtendedAttribute>\n");


      bprintf(b, "         <ExtendedAttribute>\n");
      bprintf(b, "            <FieldID>%s</FieldID>\n", EXT[DAYS_OF_WEEK].fieldid);
      bprintf(b, "            <Value>%s</Value>\n", tval.days_of_week);
      bprintf(b, "         </ExtendedAttribute>\n");


      switch(item->date_conditions)
      {
         case OTTO_USE_START_MINUTES:
            bprintf(b, "         <ExtendedAttribute>\n");
            bprintf(b, "            <FieldID>%s</FieldID>\n", EXT[START_MINUTES].fieldid);
            bprintf(b, "            <Value>%s</Value>\n", tval.start_minutes);
            bprintf(b, "         </ExtendedAttribute>\n");
            break;
         case OTTO_USE_START_TIMES:
            bprintf(b, "         <ExtendedAttribute>\n");
            bprintf(b, "            <FieldID>%s</FieldID>\n", EXT[START_TIMES].fieldid);
            bprintf(b, "            <Value>%s</Value>\n", tval.start_times);
            bprintf(b, "         </ExtendedAttribute>\n");
            break;
      }
   }

   if(item->loopname[0] != '\0')
   {
      strcpy(loop, "FOR ");
      strcat(loop, tval.loopname);
      strcat(loop, "=");
      strcat(loop, tval.loopmin);
      strcat(loop, ",");
      strcat(loop, tval.loopmax);
      bprintf(b, "       <ExtendedAttribute>\n");
      bprintf(b, "          <FieldID>%s</FieldID>\n", EXT[LOOP].fieldid);
      bprintf(b, "          <Value>%s</Value>\n", loop);
      bprintf(b, "       </ExtendedAttribute>\n");
   }


   bprintf(b, "      </Task>\n");
}



void
buffer_mspdi_predecessors(DYNBUF *b, char *name, char *expression)
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
               bprintf(b, "         <PredecessorLink>\n");
               bprintf(b, "            <PredecessorUID>%d</PredecessorUID>\n", index);
               bprintf(b, "            <Type>3</Type>\n");
               bprintf(b, "         </PredecessorLink>\n");
            }
            break;
         case 's':
            s++;
            i = (char *)&index;
            *i++ = *s++;
            *i   = *s;
            if(index >= 0)
            {
               bprintf(b, "         <PredecessorLink>\n");
               bprintf(b, "            <PredecessorUID>%d</PredecessorUID>\n", index);
               bprintf(b, "            <Type>1</Type>\n");
               bprintf(b, "         </PredecessorLink>\n");
            }
            break;
      }

      s++;
   }
}



void
buffer_mspdi_postamble(DYNBUF *b)
{
   bprintf(b, "</Project>\n");
}



void
prep_mspdi_tasks(JOBLIST *joblist)
{
   int i;
   int id = 0;

   // the joblist already contains a sorted list of jobs that satisfy the
   // requested job criteria so simply loop through them with no recursion
   for(i=0; i<joblist->nitems; i++)
   {
      // by default mark the job to be discarded.  MSPDI only supports
      // CREATE_JOB opcodes
      joblist->item[i].id = -1;

      if(joblist->item[i].opcode == CREATE_JOB)
      {
         // MS Project task numbers should start at 1 but the requested job(s)
         // could come from anywhere in the DB so renumber the list here
         joblist->item[i].id = ++id;

         // similarly Otto stores levels starting at 0 and MS Project needs them
         // to start at 1 so add one to the level here
         joblist->item[i].level++;
      }
   }

   // re-sort the joblist on id sending all -1s to the end of the list
   qsort(joblist->item, joblist->nitems, sizeof(JOB), cmp_joblist_by_id);

   // reset the nuber of items in the list - discarding marked jobs at the end
   // conveniently this is just the id accumulator
   joblist->nitems = id;

}



int
cmp_joblist_by_id(const void * a, const void * b)
{
   JOB *p1 = (JOB *)a;
   JOB *p2 = (JOB *)b;

   if(p1->id == -1) { return( 1); }
   if(p2->id == -1) { return(-1); }

   return(p1->id - p2->id);
}



int
compile_mspdi_expressions(JOBLIST *joblist)
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
         l = CNDLEN;
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
                        idx = (char *)&index;
                     }
                     else
                     {
                        // use the adjusted ID here
                        idx = (char *)&(joblist->item[index].id);
                     }
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



char *
format_mspdi_timestamp(time_t t)
{
   struct tm *now;
   static char timestamp[128];

   now = localtime(&t);
   sprintf(timestamp, "%4d-%02d-%02dT%02d:%02d:%02d",
           (now->tm_year + 1900), (now->tm_mon + 1), now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);

   return(timestamp);
}




char *
format_mspdi_duration(time_t t, int autoschedule)
{
   int32_t seconds = (int32_t)t;
   int32_t hours;
   int32_t minutes;
   static char duration[128];

   // adjust duration to minimum granularity of MS Project depending on scheduling mode
   if(autoschedule == OTTO_TRUE)
   {
      if(seconds < 60)
         seconds = 60;
   }
   else
   {
      if(seconds < 6)
         seconds = 6;
   }

   // convert seconds to minutes
   minutes = seconds/60;

   // convert minutes to hours
   hours = minutes/60;

   // normalize minutes and seconds
   minutes %= 60;
   seconds %= 60;

   // create the duration string
   sprintf(duration, "PT%dH%dM%dS", hours, minutes, seconds);

   return(duration);
}




