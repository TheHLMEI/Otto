#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "otto.h"

void write_job_cnd(JOB *item, JOBLIST *deplist);
void get_next_start_time(char *next_date, char date_conditions, char days_of_week, int64_t *start_times, int64_t *start_mins);



int
write_cnd(JOBLIST *joblist, DBCTX *ctx)
{
   int retval = OTTO_SUCCESS;
   int i;
   JOBLIST    deplist;

   if(joblist == NULL)
      retval = OTTO_FAIL;

   if(retval == OTTO_SUCCESS &&
      (deplist.item = (JOB *)calloc(cfg.ottodb_maxjobs, sizeof(JOB))) == NULL)
      retval = OTTO_FAIL;

   for(i=0; retval == OTTO_SUCCESS && i<joblist->nitems; i++)
   {
      deplist.nitems = 0;
      build_deplist(&deplist, ctx, &joblist->item[i]);

      write_job_cnd(&joblist->item[i], &deplist);
   }

   if(deplist.item != NULL)
      free(deplist.item);

   return(retval);
}



void
write_job_cnd(JOB *item, JOBLIST *deplist)
{
   JOBTVAL tval;
   char *status, next_date[20], *start_cond = "No", *dep_jobs = "No";
   int16_t index;
   int i, value;
   char *s, *x;
   ecnd_st c;
   char state;
   char atomic_condition[128];

   ottojob_prepare_txt_values(&tval, item, AS_ASCII);

   switch(item->status)
   {
      case STAT_IN: status = "INACTIVE";   break;
      case STAT_AC: status = "ACTIVATED";  break;
      case STAT_RU: status = "RUNNING";    break;
      case STAT_SU: status = "SUCCESS";    break;
      case STAT_FA: status = "FAILURE";    break;
      case STAT_TE: status = "TERMINATED"; break;
      case STAT_OH: status = "ON_HOLD";    break;
      case STAT_BR: status = "BRROKEN";    break;
   }

   get_next_start_time(next_date, item->date_conditions, item->days_of_week, item->start_times, &item->start_minutes);
   if(tval.condition[0] != '\0')
      start_cond = "Yes";
   if(deplist->nitems > 0)
      dep_jobs = "Yes";


   printf("\n");
   printf("_______________________________________________________________________________\n");
   printf("                                                                                                  Start   Dependent\n");
   printf("Job Name                                                         Status         Date Cond?        Cond?     Jobs?\n");
   printf("--------                                                         -------     -----------------    -----   ---------\n");
   printf("%-64.64s %-10.10s  %-17.17s    %-3.3s     %-3.3s\n", tval.name, status, next_date, start_cond, dep_jobs);
   printf("\n");

   if(tval.condition[0] != '\0')
   {
      printf("   Condition:  %s\n", tval.condition);
      printf("\n");
      printf("   Atomic Condition                                                              Current Status T/F\n");
      printf("   ----------------                                                              -------------- ---\n");
      s          = item->expression;
      while(*s != '\0')
      {
         switch(*s)
         {
            case '&':
            case '|':
            case '(':
            case ')':
            default:
               // do nothing
               ;
               break;

            case 'd':
            case 'f':
            case 'n':
            case 'r':
            case 's':
            case 't':
               switch(*s)
               {
                  case 'd': status = "DONE";       break;
                  case 'f': status = "FAILURE";    break;
                  case 'n': status = "NOTRUNNING"; break;
                  case 'r': status = "RUNNING";    break;
                  case 's': status = "SUCCESS";    break;
                  case 't': status = "TERMINATED"; break;
               }

               x = (char *)&index;
               *x++ = *(s+1);
               *x   = *(s+2);

               otto_sprintf(atomic_condition, "%s(%s)", status, job[index].name);

               switch(job[index].status)
               {
                  case STAT_IN: status = "INACTIVE";   break;
                  case STAT_AC: status = "ACTIVATED";  break;
                  case STAT_RU: status = "RUNNING";    break;
                  case STAT_SU: status = "SUCCESS";    break;
                  case STAT_FA: status = "FAILURE";    break;
                  case STAT_TE: status = "TERMINATED"; break;
                  case STAT_OH: status = "ON_HOLD";    break;
                  case STAT_BR: status = "BRROKEN";    break;
               }

               c.s = s;
               value = evaluate_stat(&c);
               switch(value)
               {
                  case OTTO_TRUE:  state = 'T'; break;
                  case OTTO_FALSE: state = 'F'; break;
                  case OTTO_FAIL:  state = '?'; break;
                  default:         state = '-'; break;
               }

               printf("   %-76.76s  %-14.14s %c\n", atomic_condition, status, state);

               s += 2;
               break;
         }

         s++;
      }
      printf("\n");
   }

   if(deplist->nitems > 0)
   {
      printf("   Dependent Job Name                                                            Condition\n");
      printf("   ------------------                                                            ---------\n");
      for(i=0; i<deplist->nitems; i++)
      {
         switch(deplist->item[i].opcode)
         {
            case 'd': status = "DONE";       break;
            case 'f': status = "FAILURE";    break;
            case 'n': status = "NOTRUNNING"; break;
            case 'r': status = "RUNNING";    break;
            case 's': status = "SUCCESS";    break;
            case 't': status = "TERMINATED"; break;
         }
         printf("   %-64.64s              %s(%s)\n", deplist->item[i].name, status, tval.name);
      }
      printf("\n");
   }

   printf("_______________________________________________________________________________\n");
   printf("\n");
}



void
get_next_start_time(char *next_date, char date_conditions, char days_of_week, int64_t *start_times, int64_t *start_mins)
{
   time_t now;
   struct tm *parts;
   int m, h;

   if(date_conditions != OTTO_USE_START_TIMES && date_conditions != OTTO_USE_START_MINUTES)
   {
      strcpy(next_date, "No");
   }
   else
   {
      next_date[0] = '\0';
      now = time(0);
      parts = localtime(&now);

      // check the remainder of today
      if(days_of_week & (1L << parts->tm_wday))
      {
         // check remainder of this hour
         if(start_times[parts->tm_hour] != 0)
         {
            for(m=parts->tm_min; m<60; m++)
            {
               if(start_times[parts->tm_hour] & (1L << m))
               {
                  otto_sprintf(next_date, "%02d/%02d/%04d  %02d:%02d", parts->tm_mon+1, parts->tm_mday, parts->tm_year+1900, parts->tm_hour, m);
                  break;
               }
            }
         }

         // check remaining hours in this day
         for(h=parts->tm_hour+1; next_date[0] == '\0' && h<24; h++)
         {
            for(m=0; m<60; m++)
            {
               if(start_times[parts->tm_hour] & (1L << m))
               {
                  otto_sprintf(next_date, "%02d/%02d/%04d  %02d:%02d", parts->tm_mon+1, parts->tm_mday, parts->tm_year+1900, h, m);
                  break;
               }
            }
         }
      }

      if(next_date[0] == '\0')
      {
         now = now - (now%86400) + 10;
         // check days for the next week
         while(next_date[0] == '\0')
         {
            now += 86400;
            parts = localtime(&now);
            if(days_of_week & (1L << parts->tm_wday))
            {
               for(h=0; next_date[0] == '\0' && h<24; h++)
               {
                  if(start_times[h] != 0)
                  {
                     for(m=0; m<60; m++)
                     {
                        if(start_times[h] & (1L << m))
                        {
                           otto_sprintf(next_date, "%02d/%02d/%04d  %02d:%02d", parts->tm_mon+1, parts->tm_mday, parts->tm_year+1900, h, m);
                           break;
                        }
                     }
                  }
               }
            }
         }
      }
   }
}
