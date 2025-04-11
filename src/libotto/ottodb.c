#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "otto.h"

extern OTTOLOG *logp;

JOB   *job = NULL;
JOB   *root_job = NULL;
ino_t  ottodb_inode = 0;
int    ottodbfd;


int   cmp_job_by_active (const void *a, const void *b);
int   cmp_job_by_id (const void *a, const void *b);
int   cmp_job_by_name (const void *a, const void *b);
int   cmp_job_by_pid (const void *a, const void *b);
int   cmp_job_by_date_cond (const void *a, const void *b);
void  sort_jobwork_by_linked_list(DBCTX *ctx);
void  add_job_to_tmpwork(JOB *tmpwork, int place, DBCTX *ctx, int id);
int   migrate_db(int16_t dbversion);



/*--------------------------- ottodb support functions ---------------------------*/
void
copy_jobwork(DBCTX *ctx)
{
   if(ctx->job == NULL)
   {
      if((ctx->job = (JOB *)calloc(cfg.ottodb_maxjobs, sizeof(JOB))) == NULL)
      {
         lprintf(logp, MAJR, "Couldn't allocate work space job array.\n");
         exit(-1);
      }
   }
   memcpy(ctx->job, job, sizeof(JOB)*cfg.ottodb_maxjobs);
   ctx->lastsort = BY_UNKNOWN;
}



void
sort_jobwork(DBCTX *ctx, int sort_mode)
{
   if(ctx != NULL)
   {
      ctx->lastsort = sort_mode;

      switch(sort_mode)
      {
         case BY_ACTIVE:      qsort(ctx->job, cfg.ottodb_maxjobs, sizeof(JOB), cmp_job_by_active);    break;
         case BY_ID:          qsort(ctx->job, cfg.ottodb_maxjobs, sizeof(JOB), cmp_job_by_id);        break;
         case BY_LINKED_LIST: sort_jobwork_by_linked_list(ctx);                                       break;
         case BY_NAME:        qsort(ctx->job, cfg.ottodb_maxjobs, sizeof(JOB), cmp_job_by_name);      break;
         case BY_PID:         qsort(ctx->job, cfg.ottodb_maxjobs, sizeof(JOB), cmp_job_by_pid);       break;
         case BY_DATE_COND:   qsort(ctx->job, cfg.ottodb_maxjobs, sizeof(JOB), cmp_job_by_date_cond); break;
         default:             ctx->lastsort = BY_UNKNOWN; break;
      }
   }
}



int
cmp_job_by_active(const void * a, const void * b)
{
   JOB *p1 = (JOB *)a;
   JOB *p2 = (JOB *)b;
   int diff;

   if(p1->name[0] == '\0' && p2->name[0] == '\0')
   {
      return(p2->id - p1->id);
   }

   if(p1->name[0] == '\0') { return( 1); }
   if(p2->name[0] == '\0') { return(-1); }

   if(p1->status == STAT_AC && p2->status != STAT_AC) { return(-1); }
   if(p1->status != STAT_AC && p2->status == STAT_AC) { return( 1); }

   diff = p1->type - p2->type;

   return(diff);
}



int
cmp_job_by_id(const void * a, const void * b)
{
   JOB *p1 = (JOB *)a;
   JOB *p2 = (JOB *)b;

   return(p1->id - p2->id);
}



int
cmp_job_by_name(const void * a, const void * b)
{
   JOB *p1 = (JOB *)a;
   JOB *p2 = (JOB *)b;

   if(p1->name[0] == '\0' && p2->name[0] == '\0')
   {
      return(p2->id - p1->id);
   }

   if(p1->name[0] == '\0') { return( 1); }
   if(p2->name[0] == '\0') { return(-1); }

   return(memcmp(p1->name, p2->name, NAMLEN));
}



int
cmp_job_by_pid(const void * a, const void * b)
{
   JOB *p1 = (JOB *)a;
   JOB *p2 = (JOB *)b;

   if(p1->name[0] == '\0' && p2->name[0] == '\0')
   {
      return(p2->id - p1->id);
   }

   if(p1->name[0] == '\0') { return( 1); }
   if(p2->name[0] == '\0') { return(-1); }

   return(p1->pid - p2->pid);
}



int
cmp_job_by_date_cond(const void * a, const void * b)
{
   JOB *p1 = (JOB *)a;
   JOB *p2 = (JOB *)b;

   return(p2->date_conditions - p1->date_conditions);
}



void
sort_jobwork_by_linked_list(DBCTX *ctx)
{
   JOB *tmpwork;

   if((tmpwork = (JOB *)calloc(cfg.ottodb_maxjobs, sizeof(JOB))) == NULL)
   {
      lprintf(logp, MAJR, "Couldn't allocate tmpwork array.\n");
      exit(-1);
   }

   add_job_to_tmpwork(tmpwork, 0, ctx, root_job->head);

   memcpy(ctx->job, tmpwork, sizeof(JOB)*cfg.ottodb_maxjobs);

   free(tmpwork);
}



void
add_job_to_tmpwork(JOB *tmpwork, int place, DBCTX *ctx, int id)
{
   while(id != -1)
   {
      memcpy(&tmpwork[place++], &ctx->job[id], sizeof(JOB));
      if(ctx->job[id].type == OTTO_BOX)
         add_job_to_tmpwork(tmpwork, place, ctx, ctx->job[id].head);

      id = ctx->job[id].next;
   }
}



void
save_jobwork(DBCTX *ctx)
{
   sort_jobwork(ctx, BY_ID);
   memcpy(job, ctx->job, sizeof(JOB)*cfg.ottodb_maxjobs);
   msync((void *)job, sizeof(JOB)*(cfg.ottodb_maxjobs+1), MS_SYNC|MS_INVALIDATE);
}



int
find_jobname(DBCTX *ctx, char *jobname)
{
   int i;
   int retval = -1;

   if(ctx->lastsort != BY_NAME)
      sort_jobwork(ctx, BY_NAME);

   for(i=0; i<cfg.ottodb_maxjobs; i++)
   {
      if(ctx->job[i].name[0] != '\0')
      {
         if(strcmp(jobname, ctx->job[i].name) == 0)
         {
            retval = i;
            break;
         }
      }
      else
      {
         // printf("find_jobname break at %d\n", i);
         break;
      }
   }

   return(retval);
}



void
clear_job(int job_index)
{
   memset(&job[job_index], 0, sizeof(JOB));
   job[job_index].id = job_index;
}


void
clear_jobwork(DBCTX *ctx, int job_index)
{
   int j;

   j = ctx->job[job_index].id;
   memset(&ctx->job[job_index], 0, sizeof(JOB));
   ctx->job[job_index].id = j;
}



int
open_ottodb(int type)
{
   int   retval = OTTO_SUCCESS;
   int   fd, i, initdb=OTTO_FALSE, testflags, mapflags, clear_from = -1;
   int   actual_size, desired_size;
   char *filename;
   int16_t dbversion = -1;
   struct stat statbuf;

   desired_size = sizeof(JOB)*(cfg.ottodb_maxjobs+1);

   switch(type)
   {
      case OTTO_SERVER: testflags = O_RDWR;   mapflags = PROT_READ|PROT_WRITE; break;
      default:          testflags = O_RDONLY; mapflags = PROT_READ; break;
   }

   // check environment variablw
   if((filename = getenv("OTTODB")) == NULL)
   {
      lprintf(logp, MAJR, "$OTTODB isn't defined.\n");
      retval = OTTO_FAIL;
   }

   // check if file exists and we can access it, get actual size and format
   if(retval != OTTO_FAIL)
   {
      switch(stat(filename, &statbuf))
      {
         case 0:
            actual_size = statbuf.st_size;
            if((fd = open(filename, testflags)) == -1)
            {
               printf("$OTTODB exists but can't be opened in read/write mode\n");
               retval = OTTO_FAIL;
            }
            else
            {
               if(actual_size >= sizeof(dbversion))
                  read(fd, &dbversion, sizeof(dbversion));
               close(fd);
            }
            break;
         default:
            actual_size = 0;
            if(errno == EACCES)
            {
               printf("$OTTODB exists but can't be opened in read/write mode\n");
               retval = OTTO_FAIL;
            }
            break;
      }
   }

   // create the file if necessary
   if(actual_size == 0)
   {
      if(type == OTTO_SERVER)
      {
         initdb = OTTO_TRUE;
         if((fd = open(filename,  O_RDWR|O_CREAT, S_IRWXU|S_IRWXG|S_IRWXO)) != -1)
         {
            close(fd);
            if(truncate(filename, (off_t)desired_size) == -1)
            {
               printf("Error sizing $OTTODB (%d)\n", desired_size);
               perror(filename);
               retval = OTTO_FAIL;
            }
            else
            {
               clear_from = actual_size/sizeof(JOB);
               actual_size = desired_size;
            }
         }
         else
         {
            printf("Error creating $OTTODB\n");
            retval = OTTO_FAIL;
         }
      }
      else
      {
         printf("$OTTODB is empty\n");
         retval = OTTO_FAIL;
      }
   }
   else
   {
      // migrate the format if required (only ottosysd)
      if(retval      != OTTO_FAIL           &&
         dbversion   != -1                  &&
         dbversion   != cfg.ottodb_version &&
         type        == OTTO_SERVER)
      {
         if((retval = migrate_db(dbversion)) != OTTO_FAIL)
         {
            // get the new actual size after migration
            stat(filename, &statbuf);
            actual_size = statbuf.st_size;
         }
      }
   }

   // check to see if the file needs to be re-sized (only ottosysd)
   if(retval      != OTTO_FAIL &&
      type        == OTTO_SERVER  &&
      actual_size <  desired_size)
   {
      if(truncate(filename, desired_size) == -1)
      {
         printf("Error re-sizing $OTTODB\n");
         retval = OTTO_FAIL;
      }
      else
      {
         clear_from = actual_size/sizeof(JOB);
         actual_size = desired_size;
      }
   }

   // check if the actual size is a whole number of records
   if(retval != OTTO_FAIL &&
      actual_size % sizeof(JOB) != 0)
   {
      printf("$OTTODB contains fractional rows\n");
      retval = OTTO_FAIL;
   }

   // map the file
   if(retval != OTTO_FAIL)
   {
      if((ottodbfd = open(filename, testflags)) != -1)
      {
         root_job = (JOB *)mmap((void *)job, actual_size,
                                mapflags, MAP_SHARED, ottodbfd, 0);

         if(root_job == NULL)
         {
            printf("Map failed %d\n", errno);
            fflush(stdout);
            switch(errno)
            {
               case ENOMEM:
                  lprintf(logp, MAJR, "Couldn't map $OTTODB (ENOMEM).\n");
                  break;
               case EAGAIN:
                  lprintf(logp, MAJR, "Couldn't map $OTTODB (EAGAIN).\n");
                  break;
               case EINVAL:
                  lprintf(logp, MAJR, "Couldn't map $OTTODB (EINVAL).\n");
                  break;
            }
            retval = OTTO_FAIL;
         }
         else
         {
            job = &root_job[1];

            if(initdb == OTTO_TRUE)
            {
               memset(root_job, 0, sizeof(JOB));
               otto_sprintf(root_job->name, "ottosysd %s", cfg.otto_version);
               root_job->id = cfg.ottodb_version;
               root_job->box  = -1;
               root_job->head = -1;
               root_job->tail = -1;
               root_job->prev = -1;
               root_job->next = -1;
               clear_from     =  0;
            }

            if(clear_from >= 0)
            {
               for(i=clear_from; i<cfg.ottodb_maxjobs; i++)
                  clear_job(i);
            }
         }
      }
      else
      {
         lprintf(logp, MAJR, "Couldn't map $OTTODB (bad open).\n");
         retval = OTTO_FAIL;
      }
   }

   if(retval != OTTO_FAIL)
   {
      retval = get_ottodb_inode();
   }

   return(retval);
}



int
get_ottodb_inode()
{
   int   retval = OTTO_SUCCESS;
   char *filename;
   struct stat statbuf;

   // check environment variablw
   if((filename = getenv("OTTODB")) == NULL)
   {
      lprintf(logp, MAJR, "$OTTODB isn't defined.\n");
      retval = OTTO_FAIL;
   }

   if(retval != OTTO_FAIL)
   {
      // store the inode of the ottodb file for verification
      stat(filename, &statbuf);
      ottodb_inode = statbuf.st_ino;
   }

   return(retval);
}



int
migrate_db(int16_t dbversion)
{
   int retval = OTTO_SUCCESS;

   switch(dbversion)
   {
      default:
         // print to log file
         lsprintf(logp, MAJR, "Automatic database migration from version %d to version %d is not supported.\n",
                  dbversion, cfg.ottodb_version);
         lsprintf(logp, CATI, "Check whether there is a supplemental migration program or delete and rebuild\n");
         lsprintf(logp, CATI, "the database using ottojil.\n");
         lsprintf(logp, END, "");

         // also print to stderr
         fprintf(stderr, "Automatic database migration from version %d to version %d is not supported.\nCheck whether there is a supplemental migration program or delete and rebuild\nthe database using ottojil.\n\n", dbversion, cfg.ottodb_version);

         retval = OTTO_FAIL;
         break;
   }

   return(retval);
}



