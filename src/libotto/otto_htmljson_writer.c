#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "otto.h"
#include "otto_json.h"


int
write_htmljson(int fd, JOBLIST *joblist, ottohtml_query *q)
{
   int retval = OTTO_SUCCESS;
   DYNBUF b;

   if(joblist == NULL)
      retval = OTTO_FAIL;

   if(retval == OTTO_SUCCESS)
   {
      memset(&b, 0, sizeof(b));

      buffer_json(&b, joblist);

      if(b.buffer != NULL)
      {
         // Send as JSON with correct content type and length
         ottohtml_send(fd, "200", "OK", "application/json; charset=utf-8", b.buffer, b.eob);
         free(b.buffer);
      }
      else
      {
         ottohtml_send_error(fd, "OttoCSV.csv", "500", "Internal Server Error", "Internal Server Error");
      }
   }

   return(retval);
}



