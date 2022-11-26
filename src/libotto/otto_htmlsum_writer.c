#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "otto.h"

void write_htmlsummary_header(DYNBUF *b);
void write_htmljob_summary(DYNBUF *b, JOB *item, ottohtml_query *q);



int
write_htmlsum(int fd, JOBLIST *joblist, ottohtml_query *q)
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
         if(i == 0)
            write_htmlsummary_header(&b);

         write_htmljob_summary(&b, &joblist->item[i], q);
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
         ottohtml_send_error(fd, "summary", "500", "Internal Server Error", "Internal Server Error");
      }
   }

   return(retval);
}



void
write_htmlsummary_header(DYNBUF *b)
{
   bprintf(b, "<tr class=\"summary-header\"><td>Job Name</td><td>Last Start</td><td>Last End</td><td>ST</td><td>Runtime</td></tr>\n");

}




void
write_htmljob_summary(DYNBUF *b, JOB *item, ottohtml_query *q)
{
   JOBTVAL tval;
   char *indent      = "";
   char *rowclass    = "";
   char *statusclass = "";

   switch(item->status)
   {
      case STAT_IN: if(q->show_IN != OTTO_TRUE) return; break;
      case STAT_AC: if(q->show_AC != OTTO_TRUE) return; break;
      case STAT_RU: if(q->show_RU != OTTO_TRUE) return; break;
      case STAT_SU: if(q->show_SU != OTTO_TRUE) return; break;
      case STAT_FA: if(q->show_FA != OTTO_TRUE) return; break;
      case STAT_TE: if(q->show_TE != OTTO_TRUE) return; break;
      case STAT_OH: if(q->show_OH != OTTO_TRUE) return; break;
      case STAT_BR: if(q->show_BR != OTTO_TRUE) return; break;
      default:                                  return; break;
   }

   ottojob_prepare_txt_values(&tval, item, AS_HTML);

   indent = get_html_indent(item->level);

   switch(item->status)
   {
      case STAT_OH:
      case STAT_IN: rowclass = " class=\"muted-job\"";       break;
      case STAT_RU: rowclass = " class=\"highlighted-job\""; break;
      default:      rowclass = " class=\"normal-job\"";      break;
   }

   switch(item->status)
   {
      case STAT_IN: statusclass = " class=\"stat-in\""; break;
      case STAT_AC: statusclass = " class=\"stat-ac\""; break;
      case STAT_RU: statusclass = " class=\"stat-ru\""; break;
      case STAT_SU: statusclass = " class=\"stat-su\""; break;
      case STAT_FA: statusclass = " class=\"stat-fa\""; break;
      case STAT_TE: statusclass = " class=\"stat-te\""; break;
      case STAT_OH: statusclass = " class=\"stat-oh\""; break;
      case STAT_BR: statusclass = " class=\"stat-br\""; break;
      default:      statusclass = "";                   break;
   }

   bprintf(b, "<tr%s><td>%s%s</td><td>%s</td><td>%s</td><td%s>%s</td><td>%s</td></tr>\n",
           rowclass, indent, tval.name, tval.start, tval.finish, statusclass, tval.status, tval.duration);

}



