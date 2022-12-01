#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "otto.h"



int
write_htmlcsv(int fd, JOBLIST *joblist, ottohtml_query *q)
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
         ottohtml_send_attachment(fd, "OttoCSV.csv", b.buffer, b.eob);
         free(b.buffer);
      }
      else
      {
         ottohtml_send_error(fd, "OttoCSV.csv", "500", "Internal Server Error", "Internal Server Error");
      }
   }

   return(retval);
}



