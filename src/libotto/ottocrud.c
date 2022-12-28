#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "otto.h"

void delete_box_chain(int id, DBCTX *ctx);



int
create_job(ottoipc_create_job_pdu_st *pdu, DBCTX *ctx)
{
   int retval = OTTO_SUCCESS;
   int     i, rc;
   int16_t id;
   int16_t box = -1;

   ottoipc_initialize_send();

   copy_jobwork(ctx);

   if(find_jobname(ctx, pdu->name) != -1)
   {
      pdu->option = JOB_ALREADY_EXISTS;
      ottoipc_enqueue_simple_pdu((ottoipc_simple_pdu_st *)pdu);
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS &&
      pdu->box_name[0] != '\0' &&
      (box=find_jobname(ctx, pdu->box_name)) == -1)
   {
      pdu->option = BOX_NOT_FOUND;
      ottoipc_enqueue_simple_pdu((ottoipc_simple_pdu_st *)pdu);
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS)
   {
      rc = validate_dependencies(ctx, pdu->name, pdu->condition);

      // make sure the job isn't directly referencing itself (fatal)
      if(rc & SELF_REFERENCE)
      {
         pdu->option = JOB_DEPENDS_ON_ITSELF;
         ottoipc_enqueue_simple_pdu((ottoipc_simple_pdu_st *)pdu);
         retval = OTTO_FAIL;
      }

      // check whether a referenced job exists (non-fatal)
      if(rc & MISS_REFERENCE)
      {
         pdu->option = JOB_DEPENDS_ON_MISSING_JOB;
         ottoipc_enqueue_simple_pdu((ottoipc_simple_pdu_st *)pdu);
      }

      if(pdu->type == OTTO_BOX && pdu->command[0] != '\0')
      {
         memset(pdu->command, 0, sizeof(pdu->command));
         pdu->option = BOX_COMMAND;
         ottoipc_enqueue_simple_pdu((ottoipc_simple_pdu_st *)pdu);
      }
   }

   // reset the work space if necessary
   sort_jobwork(ctx, BY_NAME);

   if(retval == OTTO_SUCCESS)
   {
      // assume the last job is the best job to pick based on sort criteria
      id = cfg.ottodb_maxjobs-1;
      if(ctx->job[id].name[0] != '\0')
      {
         pdu->option = NO_SPACE_AVAILABLE;
         ottoipc_enqueue_simple_pdu((ottoipc_simple_pdu_st *)pdu);
         retval = OTTO_FAIL;
      }
   }

   if(retval == OTTO_SUCCESS)
   {
      // get actual indexes before re-sorting
      id = ctx->job[id].id;
      if(box != -1)
         box = ctx->job[box].id;

      sort_jobwork(ctx, BY_ID);

      // copy pdu values to job storage
      clear_jobwork(ctx, id);

      memcpy(ctx->job[id].name,        pdu->name,        NAMLEN+1);
      memcpy(ctx->job[id].box_name,    pdu->box_name,    NAMLEN+1);
      memcpy(ctx->job[id].description, pdu->description, DSCLEN+1);
      memcpy(ctx->job[id].command,     pdu->command,     CMDLEN+1);
      memcpy(ctx->job[id].condition,   pdu->condition,   CNDLEN+1);
      memcpy(ctx->job[id].environment, pdu->environment, ENVLEN+1);
      memcpy(ctx->job[id].loopname,    pdu->loopname,    VARLEN+1);
      memset(ctx->job[id].expression,  0,                CNDLEN+1);
      ctx->job[id].type            = tolower(pdu->type);
      ctx->job[id].autohold        = pdu->autohold;
      ctx->job[id].date_conditions = pdu->date_conditions;
      ctx->job[id].days_of_week    = pdu->days_of_week;
      ctx->job[id].start_minutes   = pdu->start_minutes;
      ctx->job[id].status          = STAT_IN;
      ctx->job[id].on_autohold     = pdu->autohold;
      ctx->job[id].loopmin         = pdu->loopmin;
      ctx->job[id].loopmax         = pdu->loopmax;
      ctx->job[id].loopsgn         = pdu->loopsgn;
      ctx->job[id].loopnum         = ctx->job[id].loopmin - 1;
      for(i=0; i<24; i++)
         ctx->job[id].start_times[i] = pdu->start_times[i];

      // link the new job into the job stream
      ctx->job[id].box  = box;
      ctx->job[id].head = -1;
      ctx->job[id].tail = -1;
      ctx->job[id].prev = -1;
      ctx->job[id].next = -1;

      // add the job at the tail of its parent box
      if(box == -1)
      {
         if(root_job->head == -1)
         {
            root_job->head = id;
         }
         else
         {
            ctx->job[id].prev = root_job->tail;
            ctx->job[root_job->tail].next = id;
         }
         root_job->tail = id;
      }
      else
      {
         if(ctx->job[box].head == -1)
         {
            ctx->job[box].head = id;
         }
         else
         {
            ctx->job[id].prev = ctx->job[box].tail;
            ctx->job[ctx->job[box].tail].next = id;
         }
         ctx->job[box].tail = id;
      }

      save_jobwork(ctx);
      pdu->option = JOB_CREATED;
      ottoipc_enqueue_simple_pdu((ottoipc_simple_pdu_st *)pdu);
   }

   return(retval);
}



int
report_job(ottoipc_simple_pdu_st *pdu, DBCTX *ctx)
{
   int     retval = OTTO_SUCCESS;
   JOBLIST joblist;
   int     level_limit = MAXLVL;
   int     h, i;
   ottoipc_report_job_pdu_st response;

   if((joblist.item = (JOB *)calloc(cfg.ottodb_maxjobs, sizeof(JOB))) == NULL)
      retval = OTTO_FAIL;

   joblist.nitems = 0;
   level_limit    = pdu->option;
   if(level_limit > MAXLVL)
      level_limit = MAXLVL;

   build_joblist(&joblist, ctx, pdu->name, root_job->head, 0, pdu->option, OTTO_TRUE);

   if(joblist.nitems == 0)
   {
      pdu->option = BOX_NOT_FOUND;
      ottoipc_enqueue_simple_pdu((ottoipc_simple_pdu_st *)pdu);
   }
   else
   {
      for(i=0; i<joblist.nitems; i++)
      {
         memset(&response, 0, sizeof(response));

         response.pdu_type = pdu->pdu_type;
         response.opcode   = pdu->opcode;

         memcpy(response.name,        joblist.item[i].name,        NAMLEN+1);
         memcpy(response.box_name,    joblist.item[i].box_name,    NAMLEN+1);
         memcpy(response.description, joblist.item[i].description, DSCLEN+1);
         memcpy(response.environment, joblist.item[i].environment, ENVLEN+1);
         memcpy(response.command,     joblist.item[i].command,     CMDLEN+1);
         memcpy(response.condition,   joblist.item[i].condition,   CNDLEN+1);

         response.id              = joblist.item[i].id;
         response.level           = joblist.item[i].level;
         response.box             = joblist.item[i].box;
         response.head            = joblist.item[i].head;
         response.tail            = joblist.item[i].tail;
         response.prev            = joblist.item[i].prev;
         response.next            = joblist.item[i].next;
         response.type            = joblist.item[i].type;
         response.date_conditions = joblist.item[i].date_conditions;
         response.days_of_week    = joblist.item[i].days_of_week;
         response.start_minutes   = joblist.item[i].start_minutes;
         for(h=0; h<24; h++)
            response.start_times[h] = joblist.item[i].start_times[h];
         response.autohold        = joblist.item[i].autohold;
         response.expr_fail       = joblist.item[i].expr_fail;
         response.status          = joblist.item[i].status;
         response.on_autohold     = joblist.item[i].on_autohold;
         response.on_noexec       = joblist.item[i].on_noexec;
         response.pid             = joblist.item[i].pid;
         response.start           = joblist.item[i].start;
         response.finish          = joblist.item[i].finish;
         response.duration        = joblist.item[i].duration;
         response.exit_status     = joblist.item[i].exit_status;

         ottoipc_enqueue_report_job(&response);
      }
   }

   return(retval);
}



int
update_job(ottoipc_update_job_pdu_st *pdu, DBCTX *ctx)
{
   int retval = OTTO_SUCCESS;
   int16_t id, box, new_box, tmp_id;
   int     i;
   int     rc;

   copy_jobwork(ctx);

   ottoipc_initialize_send();

   if((id = find_jobname(ctx, pdu->name)) == -1)
   {
      pdu->option = JOB_NOT_FOUND;
      ottoipc_enqueue_simple_pdu((ottoipc_simple_pdu_st *)pdu);
   }
   else
   {
      // get actual indexes before re-sorting
      box = ctx->job[id].box;
      id  = ctx->job[id].id;

      // get new box index if this pdu requests a box change
      if(pdu->attributes & HAS_BOX_NAME)
      {
         if(pdu->box_name[0] == '\0')
         {
            new_box = -1;
         }
         else
         {
            new_box = find_jobname(ctx, pdu->box_name);
            if(new_box == -1)
            {
               pdu->option = BOX_NOT_FOUND;
               ottoipc_enqueue_simple_pdu((ottoipc_simple_pdu_st *)pdu);
            }
            else
            {
               new_box = ctx->job[new_box].id;
            }
         }
      }

      // check validity of requested changes
      if(pdu->attributes & HAS_NEW_NAME)
      {
         // check to see if the new name is already in use
         if((tmp_id = find_jobname(ctx, pdu->new_name)) != -1)
         {
            pdu->option = NEW_NAME_ALREADY_EXISTS;
            ottoipc_enqueue_simple_pdu((ottoipc_simple_pdu_st *)pdu);
            retval = OTTO_FAIL;
         }
         else
         {
            sort_jobwork(ctx, BY_ID);
            // copy the new job name into the job workspace so the checks below can use it
            memcpy(ctx->job[id].name, pdu->new_name, sizeof(ctx->job[id].name));
         }
      }

      sort_jobwork(ctx, BY_ID);

      if(pdu->attributes & HAS_BOX_NAME)
      {
         // check to see if the new box is actually a child of the
         // current job in some way
         if(ctx->job[id].type == OTTO_BOX)
         {
            tmp_id = ctx->job[new_box].box;
            while(tmp_id != -1)
            {
               if(tmp_id == box)
               {
                  pdu->option = GRANDFATHER_PARADOX;
                  ottoipc_enqueue_simple_pdu((ottoipc_simple_pdu_st *)pdu);
                  retval = OTTO_FAIL;
                  break;
               }
               tmp_id = ctx->job[tmp_id].box;
            }
         }
      }

      if(pdu->attributes & HAS_COMMAND && ctx->job[id].type == OTTO_BOX)
      {
         pdu->attributes ^= HAS_COMMAND;
         pdu->option = BOX_COMMAND;
         ottoipc_enqueue_simple_pdu((ottoipc_simple_pdu_st *)pdu);
      }

      if(pdu->attributes & HAS_CONDITION)
      {
         rc = validate_dependencies(ctx, pdu->name, pdu->condition);

         // make sure the job isn't directly referencing itself (fatal)
         if(rc & SELF_REFERENCE)
         {
            pdu->option = JOB_DEPENDS_ON_ITSELF;
            ottoipc_enqueue_simple_pdu((ottoipc_simple_pdu_st *)pdu);
            retval = OTTO_FAIL;
         }

         // check whether a referenced job exists (non-fatal)
         if(rc & MISS_REFERENCE)
         {
            pdu->option = JOB_DEPENDS_ON_MISSING_JOB;
            ottoipc_enqueue_simple_pdu((ottoipc_simple_pdu_st *)pdu);
         }
      }

      if(pdu->attributes & HAS_LOOP && ctx->job[id].type == OTTO_CMD)
      {
         pdu->attributes ^= HAS_LOOP;
         pdu->option = CMD_LOOP;
         ottoipc_enqueue_simple_pdu((ottoipc_simple_pdu_st *)pdu);
      }


      // update fields specifically appearing in the request according
      // to the pdu->attributes bitmask
      if(retval == OTTO_SUCCESS)
      {
         if(pdu->attributes & HAS_BOX_NAME)
         {
            // unlink the job from its current parent and siblings
            if(ctx->job[id].prev != -1) { ctx->job[ctx->job[id].prev].next = ctx->job[id].next; }
            if(ctx->job[id].next != -1) { ctx->job[ctx->job[id].next].prev = ctx->job[id].prev; }

            if(box == -1)
            {
               if(root_job->head == id) { root_job->head = ctx->job[id].next; }
               if(root_job->tail == id) { root_job->tail = ctx->job[id].prev; }
            }
            else
            {
               if(ctx->job[box].head == id) { ctx->job[box].head = ctx->job[id].next; }
               if(ctx->job[box].tail == id) { ctx->job[box].tail = ctx->job[id].prev; }
            }

            // initialize job's new linkage
            ctx->job[id].box  = new_box;
            ctx->job[id].prev = -1;
            ctx->job[id].next = -1;

            // link the job to its new parent and siblings
            if(new_box == -1)
            {
               if(root_job->head == -1)
               {
                  root_job->head = id;
               }
               else
               {
                  ctx->job[id].prev = root_job->tail;
                  ctx->job[root_job->tail].next = id;
               }
               root_job->tail = id;
            }
            else
            {
               if(ctx->job[new_box].head == -1)
               {
                  ctx->job[new_box].head = id;
               }
               else
               {
                  ctx->job[id].prev = ctx->job[new_box].tail;
                  ctx->job[ctx->job[new_box].tail].next = id;
               }
               ctx->job[new_box].tail = id;
            }

            // update box name
            memcpy(ctx->job[id].box_name, pdu->box_name, sizeof(ctx->job[id].box_name));
         }

         if(pdu->attributes & HAS_DESCRIPTION)
         {
            memcpy(ctx->job[id].description, pdu->description, sizeof(ctx->job[id].description));
         }

         if(pdu->attributes & HAS_COMMAND)
         {
            memcpy(ctx->job[id].command, pdu->command, sizeof(ctx->job[id].command));
         }

         if(pdu->attributes & HAS_CONDITION)
         {
            memcpy(ctx->job[id].condition, pdu->condition, sizeof(ctx->job[id].condition));
         }

         if(pdu->attributes & HAS_DATE_CONDITIONS)
         {
            ctx->job[id].date_conditions = pdu->date_conditions;
            ctx->job[id].days_of_week    = pdu->days_of_week;
            ctx->job[id].start_minutes   = pdu->start_minutes;
            for(i=0; i<24; i++)
               ctx->job[id].start_times[i] = pdu->start_times[i];
         }

         if(pdu->attributes & HAS_AUTO_HOLD)
         {
            ctx->job[id].autohold    = pdu->autohold;
            ctx->job[id].on_autohold = pdu->autohold;
         }

         if(pdu->attributes & HAS_LOOP)
         {
            memcpy(ctx->job[id].loopname, pdu->loopname, sizeof(ctx->job[id].loopname));
            ctx->job[id].loopmin = pdu->loopmin;
            ctx->job[id].loopmax = pdu->loopmax;
            ctx->job[id].loopsgn = pdu->loopsgn;
            ctx->job[id].loopnum = ctx->job[id].loopmin - 1;
         }

         if(pdu->attributes & HAS_ENVIRONMENT)
         {
            memcpy(ctx->job[id].environment, pdu->environment, sizeof(ctx->job[id].environment));
         }

         if(pdu->attributes & HAS_START)
         {
            ctx->job[id].start = pdu->start;
         }

         if(pdu->attributes & HAS_FINISH)
         {
            ctx->job[id].finish = pdu->finish;
         }

         save_jobwork(ctx);
         pdu->option = JOB_UPDATED;
         ottoipc_enqueue_simple_pdu((ottoipc_simple_pdu_st *)pdu);
      }
   }

   return(retval);
}



void
delete_job(ottoipc_simple_pdu_st *pdu, DBCTX *ctx)
{
   int16_t id, box;

   copy_jobwork(ctx);

   ottoipc_initialize_send();

   if((id = find_jobname(ctx, pdu->name)) == -1 || ctx->job[id].type == OTTO_BOX)
   {
      pdu->option = JOB_NOT_FOUND;
      ottoipc_enqueue_simple_pdu(pdu);
   }
   else
   {
      // get actual indexes before re-sorting
      box = ctx->job[id].box;
      id  = ctx->job[id].id;

      sort_jobwork(ctx, BY_ID);

      if(ctx->job[id].prev != -1) { ctx->job[ctx->job[id].prev].next = ctx->job[id].next; }
      if(ctx->job[id].next != -1) { ctx->job[ctx->job[id].next].prev = ctx->job[id].prev; }

      if(box == -1)
      {
         if(root_job->head == id) { root_job->head = ctx->job[id].next; }
         if(root_job->tail == id) { root_job->tail = ctx->job[id].prev; }
      }
      else
      {
         if(ctx->job[box].head == id) { ctx->job[box].head = ctx->job[id].next; }
         if(ctx->job[box].tail == id) { ctx->job[box].tail = ctx->job[id].prev; }
      }

      clear_jobwork(ctx, id);
      save_jobwork(ctx);
      pdu->option = JOB_DELETED;
      ottoipc_enqueue_simple_pdu(pdu);
   }
}



void
delete_box(ottoipc_simple_pdu_st *pdu, DBCTX *ctx)
{
   int16_t id, box, head;

   ottoipc_initialize_send();

   copy_jobwork(ctx);

   if((id = find_jobname(ctx, pdu->name)) == -1 || ctx->job[id].type != OTTO_BOX)
   {
      pdu->option = JOB_NOT_FOUND;
      ottoipc_enqueue_simple_pdu(pdu);
   }
   else
   {
      // get actual indexes before re-sorting
      box = ctx->job[id].box;
      id  = ctx->job[id].id;

      sort_jobwork(ctx, BY_ID);

      if(ctx->job[id].prev != -1) { ctx->job[ctx->job[id].prev].next = ctx->job[id].next; }
      if(ctx->job[id].next != -1) { ctx->job[ctx->job[id].next].prev = ctx->job[id].prev; }

      if(box == -1)
      {
         if(root_job->head == id) { root_job->head = ctx->job[id].next; }
         if(root_job->tail == id) { root_job->tail = ctx->job[id].prev; }
      }
      else
      {
         if(ctx->job[box].head == id) { ctx->job[box].head = ctx->job[id].next; }
         if(ctx->job[box].tail == id) { ctx->job[box].tail = ctx->job[id].prev; }
      }

      head = ctx->job[id].head;
      clear_jobwork(ctx, id);
      pdu->option = BOX_DELETED;
      ottoipc_enqueue_simple_pdu(pdu);
      delete_box_chain(head, ctx);
      save_jobwork(ctx);
   }
}



void
delete_box_chain(int id, DBCTX *ctx)
{
   int next, head, type;
   ottoipc_simple_pdu_st pdu;

   while(id != -1)
   {
      // save values for use after they're cleared
      next = ctx->job[id].next;
      head = ctx->job[id].head;
      type = ctx->job[id].type;

      memcpy(pdu.name, ctx->job[id].name, sizeof(pdu.name));
      pdu.opcode  = DELETE_JOB;
      pdu.option  = JOB_DELETED;
      if(type == OTTO_BOX)
      {
         pdu.opcode  = DELETE_BOX;
         pdu.option  = BOX_DELETED;
      }
      ottoipc_enqueue_simple_pdu(&pdu);

      clear_jobwork(ctx, id);

      if(type == OTTO_BOX)
         delete_box_chain(head, ctx);

      id = next;
   }
}



