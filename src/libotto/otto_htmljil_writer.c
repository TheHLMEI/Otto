#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "otto.h"

void write_insert_job_htmljil(DYNBUF *b, JOB *item);
void write_update_job_htmljil(DYNBUF *b, JOB *item);
void write_delete_job_htmljil(DYNBUF *b, JOB *item);
void write_delete_box_htmljil(DYNBUF *b, JOB *item);



int
write_htmljil(int fd, JOBLIST *joblist, ottohtml_query *q)
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
            case CREATE_JOB: write_insert_job_htmljil(&b, &joblist->item[i]); break;
            case UPDATE_JOB: write_update_job_htmljil(&b, &joblist->item[i]); break;
            case DELETE_JOB: write_delete_job_htmljil(&b, &joblist->item[i]); break;
            case DELETE_BOX: write_delete_box_htmljil(&b, &joblist->item[i]); break;
            default:                                                          break;
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
         ottohtml_send_error(fd, "query", "500", "Internal Server Error", "Internal Server Error");
      }
   }

   return(retval);
}



void
write_insert_job_htmljil(DYNBUF *b, JOB *item)
{
   char *indent;
   JOBTVAL tval;
   char loop[512];

   ottojob_prepare_txt_values(&tval, item, AS_HTML);

   indent = get_html_indent(item->level);

   bprintf(b, "<tr><td colspan=\"2\">%s/* ----------------- %s ----------------- */</td></tr>\n", indent, tval.name);

   bprintf(b, "<tr><td>%s%s</td><td>%s</td></tr>\n", indent, "insert_job:",           tval.name);
   bprintf(b, "<tr><td>%s%s</td><td>%s</td></tr>\n", indent, "job_type:",             tval.type);

   if(item->box_name[0] != '\0')
      bprintf(b, "<tr><td>%s%s</td><td>%s</td></tr>\n", indent, "box_name:",          tval.box_name);

   if(item->command[0] != '\0')
      bprintf(b, "<tr><td>%s%s</td><td>%s</td></tr>\n", indent, "command:",           tval.command);

   if(item->condition[0] != '\0')
      bprintf(b, "<tr><td>%s%s</td><td>%s</td></tr>\n", indent, "condition:",         tval.condition);

   if(item->description[0] != '\0')
      bprintf(b, "<tr><td>%s%s</td><td>\"%s\"</td></tr>\n", indent, "description:",   tval.description);

   if(item->environment[0] != '\0')
      bprintf(b, "<tr><td>%s%s</td><td>\"%s\"</td></tr>\n", indent, "environment:",   tval.environment);

   if(item->autohold == OTTO_TRUE)
      bprintf(b, "<tr><td>%s%s</td><td>%s</td></tr>\n", indent, "auto_hold:",         tval.autohold);

   if(item->autonoexec == OTTO_TRUE)
      bprintf(b, "<tr><td>%s%s</td><td>%s</td></tr>\n", indent, "auto_noexec:",       tval.autonoexec);

   if(item->date_conditions != OTTO_FALSE)
   {
      bprintf(b, "<tr><td>%s%s</td><td>%s</td></tr>\n", indent, "date_conditions:",   tval.date_conditions);
      bprintf(b, "<tr><td>%s%s</td><td>%s</td></tr>\n", indent, "days_of_week:",      tval.days_of_week);
      switch(item->date_conditions)
      {
         case OTTO_USE_START_MINUTES:
            bprintf(b, "<tr><td>%s%s</td><td>%s</td></tr>\n", indent, "start_mins:",  tval.start_minutes);
            break;
         case OTTO_USE_START_TIMES:
            bprintf(b, "<tr><td>%s%s</td><td>%s</td></tr>\n", indent, "start_times:", tval.start_times);
            break;
      }
   }

   // create a blank.loop expression if the item is not a box
   if(item->loopname[0] != '\0')
   {
      strcpy(loop, "&quot;FOR ");
      strcat(loop, tval.loopname);
      strcat(loop, "=");
      strcat(loop, tval.loopmin);
      strcat(loop, ",");
      strcat(loop, tval.loopmax);
      strcat(loop, "&quot;");
      bprintf(b, "<tr><td>%s%s</td><td>%s</td></tr>\n", indent, "loop:",            loop);
   }
}



void
write_update_job_htmljil(DYNBUF *b, JOB *item)
{
   bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n", "date_job:", item->name);
}



void
write_delete_job_htmljil(DYNBUF *b, JOB *item)
{
   bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n", "lete_job:", item->name);
}



void
write_delete_box_htmljil(DYNBUF *b, JOB *item)
{
   bprintf(b, "<tr><td>%s</td><td>%s</td></tr>\n", "lete_box:", item->name);
}



