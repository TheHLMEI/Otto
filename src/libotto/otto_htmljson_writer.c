#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "otto.h"
#include "otto_json.h"

// Helper functions
int should_include_job(JOB *job, ottohtml_query *q);
int create_filtered_joblist(JOBLIST *dest, JOBLIST *src, ottohtml_query *q);


int
write_htmljson(int fd, JOBLIST *joblist, ottohtml_query *q)
{
   int retval = OTTO_SUCCESS;
   JOBLIST filtered_joblist;
   DYNBUF b;

   if(joblist == NULL)
      retval = OTTO_FAIL;

   // Initialize filtered joblist
   memset(&filtered_joblist, 0, sizeof(filtered_joblist));

   if(retval == OTTO_SUCCESS)
   {
      // Create filtered joblist based on status parameters
      retval = create_filtered_joblist(&filtered_joblist, joblist, q);
   }

   if(retval == OTTO_SUCCESS)
   {
      memset(&b, 0, sizeof(b));

      buffer_json(&b, &filtered_joblist);

      if(b.buffer != NULL)
      {
         // Send as JSON with correct content type and length
         ottohtml_send(fd, "200", "OK", "application/json; charset=utf-8", b.buffer, b.eob);
         free(b.buffer);
      }
      else
      {
         ottohtml_send_error(fd, "OttoJSON.json", "500", "Internal Server Error", "Internal Server Error");
      }
   }

   // Cleanup filtered joblist
   if(filtered_joblist.item != NULL)
      free(filtered_joblist.item);

   return(retval);
}


int
should_include_job(JOB *job, ottohtml_query *q)
{
   // Apply status filtering - same logic as otto_htmlsum_writer.c
   switch(job->status)
   {
      case STAT_IN: if(q->show_IN != OTTO_TRUE) return OTTO_FALSE; break;
      case STAT_AC: if(q->show_AC != OTTO_TRUE) return OTTO_FALSE; break;
      case STAT_RU: if(q->show_RU != OTTO_TRUE) return OTTO_FALSE; break;
      case STAT_SU: if(q->show_SU != OTTO_TRUE) return OTTO_FALSE; break;
      case STAT_FA: if(q->show_FA != OTTO_TRUE) return OTTO_FALSE; break;
      case STAT_TE: if(q->show_TE != OTTO_TRUE) return OTTO_FALSE; break;
      case STAT_OH: if(q->show_OH != OTTO_TRUE) return OTTO_FALSE; break;
      case STAT_BR: if(q->show_BR != OTTO_TRUE) return OTTO_FALSE; break;
      default:      return OTTO_FALSE; break;
   }
   
   return OTTO_TRUE;
}


int
create_filtered_joblist(JOBLIST *dest, JOBLIST *src, ottohtml_query *q)
{
   int retval = OTTO_SUCCESS;
   int i, filtered_count = 0;

   if(dest == NULL || src == NULL || q == NULL)
      return OTTO_FAIL;

   // Handle empty source list
   if(src->nitems <= 0)
   {
      dest->item = NULL;
      dest->nitems = 0;
      return OTTO_SUCCESS;
   }

   // Allocate memory for filtered list (worst case: same size as source)
   if((dest->item = (JOB *)calloc(src->nitems, sizeof(JOB))) == NULL)
      return OTTO_FAIL;

   // Apply status filtering to each job
   for(i = 0; i < src->nitems; i++)
   {
      if(should_include_job(&src->item[i], q))
      {
         // Copy the job to filtered list
         memcpy(&dest->item[filtered_count], &src->item[i], sizeof(JOB));
         filtered_count++;
      }
   }
   
   dest->nitems = filtered_count;
   
   // If no jobs passed the filter, still keep the allocated memory with zero items
   // This is safer for buffer_json to handle
   
   return retval;
}



