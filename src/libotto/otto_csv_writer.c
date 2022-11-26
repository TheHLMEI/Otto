#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "otto.h"

void buffer_csv_summary_header(DYNBUF *b);
void buffer_job_csv(DYNBUF *b, JOB *item);


int
write_csv(JOBLIST *joblist)
{
   int retval = OTTO_SUCCESS;
   DYNBUF b;

   if(joblist == NULL)
      retval = OTTO_FAIL;

   if(retval == OTTO_SUCCESS)
   {
      memset(&b, 0, sizeof(b));

      buffer_csv(&b, joblist);

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
buffer_csv(DYNBUF *b, JOBLIST *joblist)
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
            buffer_csv_summary_header(b);

         buffer_job_csv(b, &joblist->item[i]);
      }
   }

   return(retval);
}



void
buffer_csv_summary_header(DYNBUF *b)
{
   bprintf(b, "action,");
   bprintf(b, "name,");
   bprintf(b, "type,");
   bprintf(b, "box_name,");
   bprintf(b, "description,");
   bprintf(b, "environment,");
   bprintf(b, "command,");
   bprintf(b, "condition,");
   bprintf(b, "date_conditions,");
   bprintf(b, "days_of_week,");
   bprintf(b, "start_mins,");
   bprintf(b, "start_times,");
   bprintf(b, "status,");
   bprintf(b, "autohold,");
   bprintf(b, "start,");
   bprintf(b, "finish,");
   bprintf(b, "duration,");
   bprintf(b, "exit_status,");
   bprintf(b, "loop");
   bprintf(b, "\r\n");
}



void
buffer_job_csv(DYNBUF *b, JOB *item)
{
   JOBTVAL tval;
   char loop[512];

   ottojob_prepare_txt_values(&tval, item, AS_CSV);

   // reset tval.date_conditions if it has a null
   if(tval.date_conditions[0] == '0')
   {
      tval.date_conditions[0] = '\0';
      tval.days_of_week[0]    = '\0';
      tval.start_minutes[0]   = '\0';
      tval.start_times[0]     = '\0';
   }

   // create a blank.loop expression if the item is not a box
   if(tval.type[0] != OTTO_BOX || item->loopname[0] == '\0')
   {
      loop[0] = '\0';
   }
   else
   {
      strcpy(loop, "\"FOR ");
      strcat(loop, tval.loopname);
      strcat(loop, "=");
      strcat(loop, tval.loopmin);
      strcat(loop, ",");
      strcat(loop, tval.loopmax);
      strcat(loop, "\"");
   }

   bprintf(b, "insert,");
   bprintf(b, "%s,", tval.name);
   bprintf(b, "%s,", tval.type);
   bprintf(b, "%s,", tval.box_name);
   bprintf(b, "%s,", tval.description);
   bprintf(b, "%s,", tval.environment);
   bprintf(b, "%s,", tval.command);
   bprintf(b, "%s,", tval.condition);
   bprintf(b, "%s,", tval.date_conditions);
   bprintf(b, "%s,", tval.days_of_week);
   bprintf(b, "%s,", tval.start_minutes);
   bprintf(b, "%s,", tval.start_times);
   bprintf(b, "%s,", tval.status);
   bprintf(b, "%s,", tval.autohold);
   bprintf(b, "%s,", tval.start);
   bprintf(b, "%s,", tval.finish);
   bprintf(b, "%s,", tval.duration);
   bprintf(b, "%s,", tval.exit_status);
   bprintf(b, "%s",  loop);
   bprintf(b, "\r\n");
}

