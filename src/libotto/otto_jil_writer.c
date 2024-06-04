#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "otto.h"

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
            default:                                                  break;
         }
      }
   }

   return(retval);
}



void
write_insert_job_jil(JOB *item)
{
   char indent[128];
   JOBTVAL tval;
   int c;
   char loop[512];

   ottojob_prepare_txt_values(&tval, item, AS_ASCII);

   otto_sprintf(indent, "%*.*s", item->level, item->level, " ");

   printf("%s/* ----------------- %s ----------------- */\n", indent, tval.name);
   printf("\n");

   printf("%sinsert_job:      %s\n", indent, tval.name);
   printf("%sjob_type:        %s\n", indent, tval.type);

   if(item->box_name[0] != '\0')
      printf("%sbox_name:        %s\n", indent, tval.box_name);

   if(item->command[0] != '\0')
      printf("%scommand:         %s\n", indent, tval.command);

   if(item->condition[0] != '\0')
      printf("%scondition:       %s\n", indent, tval.condition);

   if(item->description[0] != '\0')
   {
      printf("%sdescription:     \"",     indent);
      for(c=0; c<sizeof(tval.description); c++)
      {
         if(tval.description[c] == '\0')
            break;
         else if(tval.description[c] == '"')
            printf("\\\"");
         else
            printf("%c", tval.description[c]);
      }
      printf("\"\n");
   }

   if(item->environment[0] != '\0')
      printf("%senvironment:     %s\n", indent, tval.environment);

   if(item->autohold == OTTO_TRUE)
      printf("%sauto_hold:       %s\n", indent, tval.autohold);

   if(item->autonoexec == OTTO_TRUE)
      printf("%sauto_noexec:     %s\n", indent, tval.autonoexec);

   if(item->date_conditions != OTTO_FALSE)
   {
      printf("%sdate_conditions: %s\n", indent, tval.date_conditions);

      printf("%sdays_of_week:    %s\n", indent, tval.days_of_week);

      switch(item->date_conditions)
      {
         case OTTO_USE_START_MINUTES:
            printf("%sstart_mins:      %s\n", indent, tval.start_minutes);
            break;
         case OTTO_USE_START_TIMES:
            printf("%sstart_times:     %s\n", indent, tval.start_times);
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
      printf("%sloop:            %s\n", indent, loop);
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



