#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "otto.h"



int
write_htmlmspdi(int fd, JOBLIST *joblist, ottohtml_query *q)
{
   int retval = OTTO_SUCCESS;
   DYNBUF b;
   int autoschedule = OTTO_FALSE;

   if(joblist == NULL)
      retval = OTTO_FAIL;

   if(retval == OTTO_SUCCESS)
   {
      memset(&b, 0, sizeof(b));

      buffer_mspdi(&b, joblist, autoschedule);

      if(b.buffer != NULL)
      {
         ottohtml_send_attachment(fd, "OttoMSP.xml", b.buffer, b.eob);
         free(b.buffer);
      }
      else
      {
         ottohtml_send_error(fd, "OttoMSP.xml", "500", "Internal Server Error", "Internal Server Error");
      }
   }

   return(retval);
}



