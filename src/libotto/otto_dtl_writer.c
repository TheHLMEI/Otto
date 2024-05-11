#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "otto.h"

void write_insert_job_dtl(JOB *item);



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
            default:                                                  break;
         }
      }
   }

   return(retval);
}



void
write_insert_job_dtl(JOB *item)
{
   JOBTVAL tval;

   ottojob_prepare_txt_values(&tval, item, AS_ASCII);

   printf("---------------\n");
   printf("id             : %s\n", tval.id);
   printf("level          : %s\n", tval.level);
   printf("linkage        : %s\n", tval.linkage);
   printf("name           : %s\n", tval.name);
   printf("type           : %s\n", tval.type);
   printf("box_name       : %s\n", tval.box_name);
   printf("description    : %s\n", tval.description);
   printf("environment    : %s\n", tval.environment);

   printf("command        : %s\n", tval.command);
   printf("condition      : %s\n", tval.condition);
   printf("date_conditions: %s\n", tval.date_conditions);

   printf("days_of_week   : %s\n", tval.days_of_week);
   printf("start_mins     : %s\n", tval.start_minutes);
   printf("start_times    : %s\n", tval.start_times);
   if(item->condition[0] != '\0')
   {
      printf("expression1    : %s\n", tval.expression);
      printf("expression2    : %s\n", tval.expression2);
      printf("expr_fail      : %s\n", tval.expr_fail);
   }
   printf("status         : %s\n", tval.status);
   printf("autohold       : %s\n", tval.autohold);
   printf("on_autohold    : %s\n", tval.on_autohold);
   printf("autonoexec     : %s\n", tval.autonoexec);
   printf("on_noexec      : %s\n", tval.on_noexec);
   printf("loopname       : %s\n", tval.loopname);
   printf("loopmin        : %s\n", tval.loopmin);
   printf("loopmax        : %s\n", tval.loopmax);
   printf("loopnum        : %s\n", tval.loopnum);
   if(item->type != OTTO_BOX)
      printf("pid            : %s\n", tval.pid);
   printf("start          : %s\n", tval.start);
   printf("finish         : %s\n", tval.finish);
   printf("duration       : %s\n", tval.duration);
   printf("exit_status    : %s\n", tval.exit_status);

   printf("\n");
}



