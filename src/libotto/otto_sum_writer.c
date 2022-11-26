#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "otto.h"

char *indentation[9] = {"", " ", "  ", "   ", "    ", "     ", "      ", "       ", "        "};


void write_summary_header(void);
void write_job_summary(JOB *item);



int
write_sum(JOBLIST *joblist)
{
   int retval = OTTO_SUCCESS;
   int i;

   if(joblist == NULL)
      retval = OTTO_FAIL;

   if(retval == OTTO_SUCCESS)
   {
      for(i=0; i<joblist->nitems; i++)
      {
         if(i == 0)
            write_summary_header();

         write_job_summary(&joblist->item[i]);
      }
   }

   return(retval);
}



void
write_summary_header()
{
   char *underline = "____________________________________________________________________________________";

   printf("\n");
   printf("%-64.64s %-20.20s %-20.20s %2.2s %-8.8s\n", "Job Name", "Last Start", "Last End", "ST",      "Runtime");
   printf("%64.64s %20.20s %20.20s %2.2s %8.8s\n", underline,  underline,    underline,  underline, underline);
}




void
write_job_summary(JOB *item)
{
   JOBTVAL tval;

   ottojob_prepare_txt_values(&tval, item, AS_ASCII);

   printf("%s%-*s ", indentation[item->level], 64-item->level, tval.name);
   printf("%-20s ",  tval.start);
   printf("%-20s ",  tval.finish);
   printf("%s ",     tval.status);
   printf("%s\n",    tval.duration);
}



