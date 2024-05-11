#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "otto.h"

void write_insert_job_htmldtl(DYNBUF *b, JOB *item);



int
write_htmldtl(int fd, JOBLIST *joblist, ottohtml_query *q)
{
   int retval = OTTO_SUCCESS;
   int i;
   DYNBUF b;

   if(joblist == NULL)
      retval = OTTO_FAIL;

   if(retval == OTTO_SUCCESS)
   {
      memset(&b, 0, sizeof(b));

      bprintf(&b, "<html>\n");
      bprintf(&b, "<head>\n");
      bprintf(&b, "   <link rel=\"icon\" href=\"favicon.ico\" type=\"image/x-icon\">\n");
      bprintf(&b, "   <link rel=\"stylesheet\" href=\"otto.css\" type=\"text/css\">\n");
      bprintf(&b, "   <title>Otto HTTP</title>\n");
      bprintf(&b, "</head>\n");
      bprintf(&b, "<body>\n");
      bprintf(&b, "<center><br><br><table class=\"content-table\">\n");

      for(i=0; i<joblist->nitems; i++)
      {
         if(i > 0)
            bprintf(&b, "<tr><td>&nbsp;</td><td></td></tr>");

         switch(joblist->item[i].opcode)
         {
            case CREATE_JOB: write_insert_job_htmldtl(&b, &joblist->item[i]); break;
            case UPDATE_JOB:
            case DELETE_JOB:
            case DELETE_BOX:
            default:                                                  break;
         }
      }

      // output html page footer
      bprintf(&b, "</table>\n");
      bprintf(&b, "</body></html>\n");

      if(b.buffer != NULL)
      {
         ottohtml_send(fd, "200", "OK", "text/html", b.buffer, b.eob);
         free(b.buffer);
      }
      else
      {
         ottohtml_send_error(fd, "detail", "500", "Internal Server Error", "Internal Server Error");
      }
   }

   return(retval);
}



void
write_insert_job_htmldtl(DYNBUF *b, JOB *item)
{
   JOBTVAL tval;

   ottojob_prepare_txt_values(&tval, item, AS_HTML);

   bprintf(b, "<tr><td colspan=\"2\">---------------</td></tr>\n");
   bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n",     "id",              tval.id);
   bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n",     "level",           tval.level);
   bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n",     "linkage",         tval.linkage);
   bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n",     "name",            tval.name);
   bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n",     "type",            tval.type);
   bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n",     "box_name",        tval.box_name);
   bprintf(b, "<tr><td>%s</td><td>\"%s\"</td></tr>\n", "description",     tval.description);
   bprintf(b, "<tr><td>%s</td><td>\"%s\"</td></tr>\n", "environment",     tval.environment);

   bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n",     "command",         tval.command);
   bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n",     "condition",       tval.condition);
   bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n",     "date_conditions", tval.date_conditions);

   bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n",     "days_of_week",    tval.days_of_week);
   bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n",     "start_minutes",   tval.start_minutes);
   bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n",     "start_times",     tval.start_times);
   if(item->condition[0] != '\0')
   {
      bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n",  "expression1",     tval.expression);
      bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n",  "expression2",     tval.expression2);
      bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n",  "expr_fail",       tval.expr_fail);
   }
   bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n",     "status",          tval.status);
   bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n",     "autohold",        tval.autohold);
   bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n",     "on_autohold",     tval.on_autohold);
   bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n",     "autonoexec",      tval.autonoexec);
   bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n",     "on_noexec",       tval.on_noexec);
   bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n",     "loopname",        tval.loopname);
   bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n",     "loopmin",         tval.loopmin);
   bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n",     "loopmax",         tval.loopmax);
   bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n",     "loopnum",         tval.loopnum);
   if(item->type != OTTO_BOX)
      bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n",  "pid",             tval.pid);
   bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n",     "start",           tval.start);
   bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n",     "finish",          tval.finish);
   bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n",     "duration",        tval.duration);
   bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n",     "exit_status",     tval.exit_status);
}



