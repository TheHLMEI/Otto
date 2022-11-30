#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "otto.h"

void write_insert_job_htmldtl(DYNBUF *b, JOB *item);



int
write_htmlversion(int fd)
{
   int retval = OTTO_SUCCESS;
   DYNBUF b;

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

      bprintf(&b, "<center><small><small>Otto %s</small></small></center>", OTTO_VERSION_STRING);

      bprintf(&b, "</body></html>\n");

      if(b.buffer != NULL)
      {
         ottohtml_send(fd, "200", "OK", "text/html", b.buffer, b.eob);
         free(b.buffer);
      }
      else
      {
         ottohtml_send_error(fd, "version", "500", "Internal Server Error", "Internal Server Error");
      }
   }

   return(retval);
}



