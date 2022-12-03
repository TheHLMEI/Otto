#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "otto.h"

typedef struct _csvcol
{
   char *start;
   char *end;
   int   quoted;
   int   type;
} CSVCOL;

typedef struct _csvrow
{
   CSVCOL *col;
   int     ncol;
   int     acol;
   int     action_index;
   int     type_index;
} CSVROW;


#define CSV_INSERT 1
#define CSV_UPDATE 2
#define CSV_DELETE 3

#define WITH_QUOTES    1
#define WITHOUT_QUOTES 2

enum CSV_KEYWORDS
{
   CSV_ACTION,
   CSV_NAME,
   CSV_TYPE,
   CSV_BOX_NAME,
   CSV_DESCRIPTION,
   CSV_ENVIRONMENT,
   CSV_COMMAND,
   CSV_CONDITION,
   CSV_DATE_CONDITIONS,
   CSV_DAYS_OF_WEEK,
   CSV_START_MINS,
   CSV_START_TIMES,
   CSV_STATUS,
   CSV_AUTOHOLD,
   CSV_START,
   CSV_FINISH,
   CSV_DURATION,
   CSV_EXIT_STATUS,
   CSV_LOOP,
   CSV_NEW_NAME,
   CSV_UNSUPPD
};



int  add_csv_insert(JOB *item, CSVROW *row, CSVROW *hdr);
int  add_csv_update(JOB *item, CSVROW *row, CSVROW *hdr);
int  add_csv_delete(JOB *item, CSVROW *row, CSVROW *hdr);
void set_csv_joblist_levels(JOBLIST *joblist);
int  validate_and_copy_csv_name(JOB *item, CSVCOL *namep);
int  validate_and_copy_csv_type(JOB *item, CSVCOL *typep);
int  validate_and_copy_csv_box_name(JOB *item, CSVCOL *box_namep);
int  validate_and_copy_csv_command(JOB *item, CSVCOL *commandp);
int  validate_and_copy_csv_condition(JOB *item, CSVCOL *conditionp);
int  validate_and_copy_csv_description(JOB *item, CSVCOL *descriptionp);
int  validate_and_copy_csv_environment(JOB *item, CSVCOL *environmentr);
int  validate_and_copy_csv_auto_hold(JOB *item, CSVCOL *auto_holdp);
int  validate_and_copy_csv_date_conditions(JOB *item, CSVCOL *date_conditionsp, CSVCOL *days_of_weekp, CSVCOL *start_minsp, CSVCOL *start_timesp);
int  validate_and_copy_csv_loop(JOB *item, CSVCOL *loopp);
int  validate_and_copy_csv_new_name(JOB *item, CSVCOL *new_namep);
int  validate_and_copy_csv_start(JOB *item, CSVCOL *startp);
int  validate_and_copy_csv_finish(JOB *item, CSVCOL *finishp);

int  csv_gethdr(CSVROW *hdr, DYNBUF *b);
int  csv_getrow(CSVROW *row, DYNBUF *b);
void csv_getcol(char *t, size_t tlen, CSVCOL *c, int style);
int  csv_get_action(CSVCOL *actionp);
int  csv_get_type(CSVCOL *typep);
int  csv_preprow(CSVROW *r);
int  csv_addcol(CSVROW *r, CSVCOL *c);
void remove_carriage_returns(DYNBUF *b);


int
parse_csv(DYNBUF *b, JOBLIST *joblist)
{
   int retval = OTTO_SUCCESS;
   int i = 0;
   char *itemstart = NULL;
   CSVROW hdr, row;

   if(joblist == NULL)
      retval = OTTO_FAIL;

   // remove unguarded carriage returns from buffer to make parsing easier later
   remove_carriage_returns(b);

   // read CSV header
   if(retval == OTTO_SUCCESS)
   {
      b->line = 0;
      b->s    = b->buffer;
      retval = csv_gethdr(&hdr, b);
   }

   // count items
   if(retval == OTTO_SUCCESS)
   {
      // save start of data
      itemstart = b->s;

      memset(joblist, 0, sizeof(JOBLIST));

      while(b->s[0] != '\0')
      {
         if(csv_getrow(&row, b) == OTTO_SUCCESS)
         {
            // check column count
            if(row.ncol == hdr.ncol)
            {
               joblist->nitems++;
            }
            else
            {
               // TODO report this
               printf("Fail ncol. Want %d got %d\n", hdr.ncol, row.ncol);
               retval = OTTO_FAIL;
            }
         }
         else
         {
            // TODO report this
            printf("Fail getrow\n");
            retval = OTTO_FAIL;
         }
      }

      if(joblist->nitems == 0)
         retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS)
   {
      // allocate joblist
      if((joblist->item = (JOB *)calloc(joblist->nitems, sizeof(JOB))) == NULL)
         retval = OTTO_FAIL;
   }

   // add items to joblist
   if(retval == OTTO_SUCCESS)
   {
      b->line = 0;
      b->s    = itemstart;
      while(b->s[0] != '\0')
      {
         csv_getrow(&row, b);
         switch(csv_get_action(&row.col[hdr.action_index]))
         {
            case CSV_INSERT:
               retval = add_csv_insert(&joblist->item[i++], &row, &hdr);
               break;
            case CSV_UPDATE:
               retval = add_csv_update(&joblist->item[i++], &row, &hdr);
               break;
            case CSV_DELETE:
               retval = add_csv_delete(&joblist->item[i++], &row, &hdr);
               break;
            default:
               break;
         }
      }
   }

   if(retval == OTTO_SUCCESS)
      set_csv_joblist_levels(joblist);

   return(retval);
}



int
add_csv_insert(JOB *item, CSVROW *row, CSVROW *hdr)
{
   int     retval           = OTTO_SUCCESS;
   int     c;
   CSVCOL *namep            = NULL;
   CSVCOL *typep            = NULL;
   CSVCOL *auto_holdp       = NULL;
   CSVCOL *box_namep        = NULL;
   CSVCOL *commandp         = NULL;
   CSVCOL *conditionp       = NULL;
   CSVCOL *descriptionp     = NULL;
   CSVCOL *environmentp     = NULL;
   CSVCOL *date_conditionsp = NULL;
   CSVCOL *days_of_weekp    = NULL;
   CSVCOL *start_minsp      = NULL;
   CSVCOL *start_timesp     = NULL;
   CSVCOL *loopp            = NULL;

   for(c=0; c<hdr->ncol; c++)
   {
      switch(hdr->col[c].type)
      {
         case CSV_NAME:            namep            = &row->col[c]; break;
         case CSV_TYPE:            typep            = &row->col[c]; break;
         case CSV_BOX_NAME:        box_namep        = &row->col[c]; break;
         case CSV_DESCRIPTION:     descriptionp     = &row->col[c]; break;
         case CSV_ENVIRONMENT:     environmentp     = &row->col[c]; break;
         case CSV_COMMAND:         commandp         = &row->col[c]; break;
         case CSV_CONDITION:       conditionp       = &row->col[c]; break;
         case CSV_DATE_CONDITIONS: date_conditionsp = &row->col[c]; break;
         case CSV_DAYS_OF_WEEK:    days_of_weekp    = &row->col[c]; break;
         case CSV_START_MINS:      start_minsp      = &row->col[c]; break;
         case CSV_START_TIMES:     start_timesp     = &row->col[c]; break;
         case CSV_AUTOHOLD:        auto_holdp       = &row->col[c]; break;
         case CSV_LOOP:            loopp            = &row->col[c]; break;
         default:
                                   break;
      }
   }

   memset(item, 0, sizeof(JOB));
   item->opcode = CREATE_JOB;

   if(validate_and_copy_csv_name(item, namep) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_csv_type(item, typep) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_csv_box_name(item, box_namep) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_csv_command(item, commandp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_csv_condition(item, conditionp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_csv_description(item, descriptionp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_csv_environment(item, environmentp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_csv_auto_hold(item, auto_holdp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_csv_date_conditions(item, date_conditionsp, days_of_weekp, start_minsp, start_timesp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_csv_loop(item,loopp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;

   return(retval);
}



int
add_csv_update(JOB *item, CSVROW *row, CSVROW *hdr)
{
   int     retval           = OTTO_SUCCESS;
   int     c;
   CSVCOL *namep            = NULL;
   CSVCOL *typep            = NULL;
   CSVCOL *auto_holdp       = NULL;
   CSVCOL *box_namep        = NULL;
   CSVCOL *commandp         = NULL;
   CSVCOL *conditionp       = NULL;
   CSVCOL *descriptionp     = NULL;
   CSVCOL *environmentp     = NULL;
   CSVCOL *date_conditionsp = NULL;
   CSVCOL *days_of_weekp    = NULL;
   CSVCOL *start_minsp      = NULL;
   CSVCOL *start_timesp     = NULL;
   CSVCOL *loopp            = NULL;
   CSVCOL *new_namep        = NULL;
   CSVCOL *startp           = NULL;
   CSVCOL *finishp          = NULL;

   for(c=0; c<hdr->ncol; c++)
   {
      switch(hdr->col[c].type)
      {
         case CSV_NAME:            namep            = &row->col[c]; break;
         case CSV_TYPE:            typep            = &row->col[c]; break;
         case CSV_BOX_NAME:        box_namep        = &row->col[c]; break;
         case CSV_DESCRIPTION:     descriptionp     = &row->col[c]; break;
         case CSV_ENVIRONMENT:     environmentp     = &row->col[c]; break;
         case CSV_COMMAND:         commandp         = &row->col[c]; break;
         case CSV_CONDITION:       conditionp       = &row->col[c]; break;
         case CSV_DATE_CONDITIONS: date_conditionsp = &row->col[c]; break;
         case CSV_DAYS_OF_WEEK:    days_of_weekp    = &row->col[c]; break;
         case CSV_START_MINS:      start_minsp      = &row->col[c]; break;
         case CSV_START_TIMES:     start_timesp     = &row->col[c]; break;
         case CSV_AUTOHOLD:        auto_holdp       = &row->col[c]; break;
         case CSV_LOOP:            loopp            = &row->col[c]; break;
         case CSV_NEW_NAME:        new_namep        = &row->col[c]; break;
         case CSV_START:           startp           = &row->col[c]; break;
         case CSV_FINISH:          finishp          = &row->col[c]; break;
         default:
                                   break;
      }
   }

   memset(item, 0, sizeof(JOB));
   item->opcode = UPDATE_JOB;

   if(validate_and_copy_csv_name(item, namep) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_csv_type(item, typep) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_csv_box_name(item, box_namep) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_csv_command(item, commandp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_csv_condition(item, conditionp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_csv_description(item, descriptionp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_csv_environment(item, environmentp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_csv_auto_hold(item, auto_holdp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_csv_date_conditions(item, date_conditionsp, days_of_weekp, start_minsp, start_timesp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_csv_loop(item, loopp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_csv_new_name(item, new_namep) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_csv_start(item, startp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_csv_finish(item, finishp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;

   return(retval);
}



int
add_csv_delete(JOB *item, CSVROW *row, CSVROW *hdr)
{
   int     retval = OTTO_SUCCESS;
   int     c;
   CSVCOL *namep  = NULL;
   CSVCOL *typep  = NULL;

   for(c=0; c<hdr->ncol; c++)
   {
      switch(hdr->col[c].type)
      {
         case CSV_NAME: namep = &row->col[c]; break;
         case CSV_TYPE: typep = &row->col[c]; break;
         default:
                        break;
      }
   }

   memset(item, 0, sizeof(JOB));

   switch(csv_get_type(typep))
   {
      case OTTO_BOX: item->opcode = DELETE_BOX; break;
      case OTTO_CMD: item->opcode = DELETE_JOB; break;
      default:  retval       = OTTO_FAIL;  break;
   }

   if(validate_and_copy_csv_name(item, namep) != OTTO_SUCCESS)
      retval = OTTO_FAIL;

   return(retval);
}



void
set_csv_joblist_levels(JOBLIST *joblist)
{
   int b, current_level=0, i, one_changed=OTTO_TRUE;

   if(joblist != NULL)
   {
      // set up initial level states
      for(i=0; i<joblist->nitems; i++)
      {
         switch(joblist->item[i].opcode)
         {
            case CREATE_JOB:
               if(joblist->item[i].box_name[0] == '\0')
                  joblist->item[i].level = 0;
               else
                  joblist->item[i].level = -1;
               break;
            case UPDATE_JOB:
            case DELETE_BOX:
            case DELETE_JOB:
               joblist->item[i].level = 0;
               break;
         }
      }

      while(one_changed == OTTO_TRUE)
      {
         one_changed = OTTO_FALSE;
         // look for a box at the current level and try to find its child jobs
         for(b=0; b<joblist->nitems; b++)
         {
            if(joblist->item[b].opcode == CREATE_JOB &&
               joblist->item[b].type   == OTTO_BOX   &&
               joblist->item[b].level  == current_level)
            {
               // found a box, look for its children
               for(i=0; i<joblist->nitems; i++)
               {
                  if(i != b &&
                     joblist->item[i].opcode == CREATE_JOB &&
                     strcmp(joblist->item[i].box_name, joblist->item[b].name) == 0)
                  {
                     joblist->item[i].level = current_level + 1;
                     one_changed = OTTO_TRUE;
                  }
               }
            }
         }
         current_level++;
      }
   }
}



int
validate_and_copy_csv_name(JOB *item, CSVCOL *namep)
{
   int retval   = OTTO_SUCCESS;
   char *action = "action";
   char *kind   = "";
   int rc;
   char name[NAMLEN+5];

   switch(item->opcode)
   {
      case CREATE_JOB: action = "insert"; kind = "job "; break;
      case UPDATE_JOB: action = "update"; kind = "job "; break;
      case DELETE_JOB: action = "delete"; kind = "job "; break;
      case DELETE_BOX: action = "delete"; kind = "box "; break;
   }

   // reset pointer if its first character is ','
   if(namep != NULL && *namep->start == ',') namep = NULL;

   if(namep == NULL)
   {
      retval = OTTO_FAIL;
   }
   else
   {
      // unpack the value - convert to what it would look like in JIL
      csv_getcol(name, sizeof(name), namep, WITHOUT_QUOTES);
   }

   // compatibility check with JIL
   if(retval == OTTO_SUCCESS && jil_reserved_word(name) != JIL_UNKNOWN)
   {
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS)
   {
      if((rc = ottojob_copy_name(item->name, name, NAMLEN)) != OTTO_SUCCESS)
      {
         ottojob_print_name_errors(rc, action, item->name, kind);

         retval = OTTO_FAIL;
      }
   }

   return(retval);
}



int
validate_and_copy_csv_type(JOB *item, CSVCOL *typep)
{
   int retval = OTTO_SUCCESS;
   char *action = "action";
   int rc;
   char type[10];

   switch(item->opcode)
   {
      case CREATE_JOB: action = "insert_job"; break;
      case UPDATE_JOB: action = "update_job"; break;
   }

   // reset pointer if its first character is ','
   if(typep != NULL && *typep->start == ',') typep = NULL;

   if(typep == NULL)  // must have a type
   {
      retval = OTTO_FAIL;
   }
   else
   {
      // unpack the value - convert to what it would look like in JIL
      csv_getcol(type, sizeof(type), typep, WITHOUT_QUOTES);

      if(type[0] != '\0')
      {
         item->attributes |= HAS_TYPE;

         if(jil_reserved_word(type) == JIL_UNKNOWN)
         {
            if((rc = ottojob_copy_type(&item->type, type, TYPLEN)) != OTTO_SUCCESS)
            {
               ottojob_print_type_errors(rc, action, item->name, type);

               retval = OTTO_FAIL;
            }
         }
      }
      else
      {
         retval = OTTO_FAIL;
      }
   }

   return(retval);
}



int
validate_and_copy_csv_box_name(JOB *item, CSVCOL *box_namep)
{
   int retval = OTTO_SUCCESS;
   char *action = "action";
   int rc;
   char box_name[NAMLEN+5];

   switch(item->opcode)
   {
      case CREATE_JOB: action = "insert_job"; break;
      case UPDATE_JOB: action = "update_job"; break;
   }

   // reset pointer if its first character is ','
   if(box_namep != NULL && *box_namep->start == ',') box_namep = NULL;

   if(box_namep != NULL)
   {
      // unpack the value - convert to what it would look like in JIL
      csv_getcol(box_name, sizeof(box_name), box_namep, WITHOUT_QUOTES);

      if(box_name[0] != '\0')
      {
         item->attributes |= HAS_BOX_NAME;

         // only try to copy data if the first character isn't a bang
         if(box_name[0] != '!')
         {
            if(jil_reserved_word(box_name) == JIL_UNKNOWN)
            {
               rc = ottojob_copy_name(item->box_name, box_name, NAMLEN);
               if(item->type == OTTO_BOX && strcmp(item->name, item->box_name) == 0)
               {
                  rc |= OTTO_SAME_JOB_BOX_NAMES;
               }

               if(rc != OTTO_SUCCESS)
               {
                  ottojob_print_box_name_errors(rc, action, item->name, item->box_name);

                  retval = OTTO_FAIL;
               }
            }
         }
      }
   }

   return(retval);
}



int
validate_and_copy_csv_command(JOB *item, CSVCOL *commandp)
{
   int retval = OTTO_SUCCESS;
   char *action = "action";
   int rc;
   char command[CMDLEN+5];

   switch(item->opcode)
   {
      case CREATE_JOB: action = "insert_job"; break;
      case UPDATE_JOB: action = "update_job"; break;
   }

   // reset pointer if its first character is ','
   if(commandp != NULL && *commandp->start == ',') commandp = NULL;

   if(commandp != NULL)
   {
      // unpack the value - convert to what it would look like in JIL
      csv_getcol(command, sizeof(command), commandp, WITHOUT_QUOTES);

      if(command[0] != '\0')
      {
         item->attributes |= HAS_COMMAND;

         // only try to copy data if the first character isn't a bang
         if(command[0] != '!')
         {
            if(jil_reserved_word(command) == JIL_UNKNOWN)
            {
               rc = ottojob_copy_command(item->command, command, CMDLEN);
            }
         }
         else
         {
            if(item->type == OTTO_CMD)
            {
               rc = OTTO_MISSING_REQUIRED_VALUE;
            }
         }
         if(rc != OTTO_SUCCESS)
         {
            ottojob_print_command_errors(rc, action, item->name, CMDLEN);

            retval = OTTO_FAIL;
         }
      }
   }

   return(retval);
}



int
validate_and_copy_csv_condition(JOB *item, CSVCOL *conditionp)
{
   int retval = OTTO_SUCCESS;
   char *action = "action";
   int rc;
   char condition[CNDLEN+5]; // maximum number of characters in an excel cell + 1

   switch(item->opcode)
   {
      case CREATE_JOB: action = "insert_job"; break;
      case UPDATE_JOB: action = "update_job"; break;
   }

   // reset pointer if its first character is ','
   if(conditionp != NULL && *conditionp->start == ',') conditionp = NULL;

   if(conditionp != NULL)
   {
      // unpack the value - convert to what it would look like in JIL
      csv_getcol(condition, sizeof(condition), conditionp, WITHOUT_QUOTES);

      if(condition[0] != '\0')
      {
         item->attributes |= HAS_CONDITION;

         // only try to copy data if the first character isn't a bang
         if(condition[0] != '!')
         {
            if(jil_reserved_word(condition) == JIL_UNKNOWN)
            {
               if((rc = ottojob_copy_condition(item->condition, condition, CNDLEN)) != OTTO_SUCCESS)
               {
                  ottojob_print_condition_errors(rc, action, item->name, CMDLEN);

                  retval = OTTO_FAIL;
               }
            }
         }
      }
   }

   return(retval);
}



int
validate_and_copy_csv_description(JOB *item, CSVCOL *descriptionp)
{
   int retval = OTTO_SUCCESS;
   char *action = "action";
   int rc;
   char description[DSCLEN+5];

   switch(item->opcode)
   {
      case CREATE_JOB: action = "insert_job"; break;
      case UPDATE_JOB: action = "update_job"; break;
   }

   // reset pointer if its first character is ','
   if(descriptionp != NULL && *descriptionp->start == ',') descriptionp = NULL;

   if(descriptionp != NULL)
   {
      // unpack the value - convert to what it would look like in JIL
      csv_getcol(description, sizeof(description), descriptionp, WITH_QUOTES);

      if(description[0] != '\0')
      {
         item->attributes |= HAS_DESCRIPTION;

         // only try to copy data if the first character isn't a bang
         if(description[0] != '!')
         {
            if(jil_reserved_word(description) == JIL_UNKNOWN)
            {
               if((rc = ottojob_copy_description(item->description, description, DSCLEN)) != OTTO_SUCCESS)
               {
                  ottojob_print_description_errors(rc, action, item->name, DSCLEN);

                  retval = OTTO_FAIL;
               }
            }
         }
      }
   }

   return(retval);
}



int
validate_and_copy_csv_environment(JOB *item, CSVCOL *environmentp)
{
   int retval = OTTO_SUCCESS;
   char *action = "action";
   int rc;
   char environment[ENVLEN+5];

   switch(item->opcode)
   {
      case CREATE_JOB: action = "insert_job"; break;
      case UPDATE_JOB: action = "update_job"; break;
   }

   // reset pointer if its first character is ','
   if(environmentp != NULL && *environmentp->start == ',') environmentp = NULL;

   if(environmentp != NULL)
   {
      // unpack the value - convert to what it would look like in JIL
      csv_getcol(environment, sizeof(environment), environmentp, WITHOUT_QUOTES);

      if(environment[0] != '\0')
      {
         item->attributes |= HAS_ENVIRONMENT;

         // only try to copy data if the first character isn't a bang
         if(environment[0] != '!')
         {
            if(jil_reserved_word(environment) == JIL_UNKNOWN)
            {
               if((rc = ottojob_copy_environment(item->environment, environment, ENVLEN)) != OTTO_SUCCESS)
               {
                  ottojob_print_environment_errors(rc, action, item->name, DSCLEN);

                  retval = OTTO_FAIL;
               }
            }
         }
      }
   }

   return(retval);
}



int
validate_and_copy_csv_auto_hold(JOB *item, CSVCOL *auto_holdp)
{
   int retval = OTTO_SUCCESS;
   char *action = "action";
   int rc;
   char auto_hold[10]; // maximum number of characters in an excel cell + 1

   switch(item->opcode)
   {
      case CREATE_JOB: action = "insert_job"; break;
      case UPDATE_JOB: action = "update_job"; break;
   }

   // reset pointer if its first character is ','
   if(auto_holdp != NULL && *auto_holdp->start == ',') auto_holdp = NULL;

   if(auto_holdp != NULL)
   {
      // unpack the value - convert to what it would look like in JIL
      csv_getcol(auto_hold, sizeof(auto_hold), auto_holdp, WITHOUT_QUOTES);

      if(auto_hold[0] != '\0')
      {
         item->attributes |= HAS_AUTO_HOLD;

         // only try to copy data if the first character isn't a bang
         if(auto_hold[0] != '!')
         {
            if(jil_reserved_word(auto_hold) == JIL_UNKNOWN)
            {
               if((rc = ottojob_copy_flag(&item->autohold, auto_hold, FLGLEN)) != OTTO_SUCCESS)
               {
                  ottojob_print_auto_hold_errors(rc, action, item->name, auto_hold);

                  retval = OTTO_FAIL;
               }
               item->on_autohold = item->autohold;
            }
         }
      }
   }

   return(retval);
}



int
validate_and_copy_csv_date_conditions(JOB *item, CSVCOL *date_conditionsp, CSVCOL *days_of_weekp, CSVCOL *start_minsp, CSVCOL *start_timesp)
{
   int     retval                = OTTO_SUCCESS;
   char   *action                = "action";
   char    date_conditions       = 0;
   char    days_of_week          = 0;
   int64_t start_minutes         = 0;
   int64_t start_times[24]       = {0};
   int     date_check            = 0;
   int     parse_date_conditions = OTTO_FALSE;
   int     i;
   int     rc = OTTO_SUCCESS;
   char s_date_conditions[10];
   char s_days_of_week[128];
   char s_start_mins[2048];
   char s_start_times[2048];

   switch(item->opcode)
   {
      case CREATE_JOB: action = "insert_job"; break;
      case UPDATE_JOB: action = "update_job"; break;
   }

   // reset pointers if any of their first characters is ','
   if(date_conditionsp != NULL && *date_conditionsp->start == ',') date_conditionsp = NULL;
   if(days_of_weekp    != NULL && *days_of_weekp->start    == ',') days_of_weekp    = NULL;
   if(start_minsp      != NULL && *start_minsp->start      == ',') start_minsp      = NULL;
   if(start_timesp     != NULL && *start_timesp->start     == ',') start_timesp     = NULL;

   if(date_conditionsp != NULL) csv_getcol(s_date_conditions, sizeof(s_date_conditions), date_conditionsp, WITHOUT_QUOTES);
   if(days_of_weekp    != NULL) csv_getcol(s_days_of_week,    sizeof(s_days_of_week),    days_of_weekp,    WITH_QUOTES);
   if(start_minsp      != NULL) csv_getcol(s_start_mins,      sizeof(s_start_mins),      start_minsp,      WITH_QUOTES);
   if(start_timesp     != NULL) csv_getcol(s_start_times,     sizeof(s_start_times),     start_timesp,     WITH_QUOTES);

   // check if a valid comination of date conditions attributes was specified
   if(date_conditionsp != NULL && jil_reserved_word(s_date_conditions) == JIL_UNKNOWN) date_check |= DTECOND_BIT;
   if(days_of_weekp    != NULL && jil_reserved_word(s_days_of_week)    == JIL_UNKNOWN) date_check |= DYSOFWK_BIT;
   if(start_minsp      != NULL && jil_reserved_word(s_start_mins)      == JIL_UNKNOWN) date_check |= STRTMNS_BIT;
   if(start_timesp     != NULL && jil_reserved_word(s_start_times)     == JIL_UNKNOWN) date_check |= STRTTMS_BIT;

   switch(date_check)
   {
      case 0:
         ; // do nothing
         break;
      case (DTECOND_BIT | DYSOFWK_BIT | STRTMNS_BIT):
      case (DTECOND_BIT | DYSOFWK_BIT | STRTTMS_BIT):
         if(item->box_name[0] != '\0')
         {
            rc = OTTO_INVALID_APPLICATION;
         }
         else
         {
            parse_date_conditions = OTTO_TRUE;
         }
         break;
      default:
         rc = OTTO_INVALID_COMBINATION;
         break;
   }
   if(rc != OTTO_SUCCESS)
   {
      ottojob_print_date_conditions_errors(rc, action, item->name, s_date_conditions);

      retval = OTTO_FAIL;
   }

   if(parse_date_conditions == OTTO_TRUE)
   {
      if((rc = ottojob_copy_flag(&date_conditions, s_date_conditions, FLGLEN)) != OTTO_SUCCESS)
      {
         ottojob_print_date_conditions_errors(rc, action, item->name, s_date_conditions);

         retval = OTTO_FAIL;
      }

      if((rc = ottojob_copy_days_of_week(&days_of_week, s_days_of_week)) != OTTO_SUCCESS)
      {
         ottojob_print_days_of_week_errors(rc, action, item->name);

         retval = OTTO_FAIL;
      }

      if(start_minsp != NULL)
      {
         if((rc = ottojob_copy_start_minutes(&start_minutes, s_start_mins)) != OTTO_SUCCESS)
         {
            ottojob_print_start_mins_errors(rc, action, item->name);

            retval = OTTO_FAIL;
         }
      }

      if(start_timesp != NULL)
      {
         if((rc = ottojob_copy_start_times(start_times, s_start_times)) != OTTO_SUCCESS)
         {
            ottojob_print_start_times_errors(rc, action, item->name);

            retval = OTTO_FAIL;
         }
      }

      if(retval == OTTO_SUCCESS)
      {
         // assume the job uses start times
         item->date_conditions = OTTO_USE_START_TIMES;

         // modify if it's using start_minutes
         if(start_minsp != NULL)
         {
            item->date_conditions = OTTO_USE_START_MINUTES;
            for(i=0; i<24; i++)
               start_times[i] = start_minutes;
         }

         // copy the data to the structure
         item->attributes    |= HAS_DATE_CONDITIONS;
         item->days_of_week   = days_of_week;
         item->start_minutes  = start_minutes;
         memcpy(item->start_times, start_times, sizeof(start_times));
      }
   }

   return(retval);
}



int
validate_and_copy_csv_loop(JOB *item, CSVCOL *loopp)
{
   int retval = OTTO_SUCCESS;
   char *action = "action";
   int rc;
   char loop[10];

   switch(item->opcode)
   {
      case CREATE_JOB: action = "insert_job"; break;
      case UPDATE_JOB: action = "update_job"; break;
   }

   // reset pointer if its first character is ','
   if(loopp != NULL && *loopp->start == ',') loopp = NULL;

   if(loopp != NULL)
   {
      // unpack the value - convert to what it would look like in JIL
      csv_getcol(loop, sizeof(loop), loopp, WITHOUT_QUOTES);

      if(loop[0] != '\0')
      {
         item->attributes |= HAS_LOOP;

         // only try to copy data if the first character isn't a bang
         if(loop[0] != '!')
         {
            if(jil_reserved_word(loop) == JIL_UNKNOWN)
            {
               if((rc = ottojob_copy_loop(item->loopname, &item->loopmin, &item->loopmax,  &item->loopsgn, loop)) != OTTO_SUCCESS)
               {
                  ottojob_print_loop_errors(rc, action, item->name);

                  retval = OTTO_FAIL;
               }
            }
         }
      }
   }

   return(retval);
}



int
validate_and_copy_csv_new_name(JOB *item, CSVCOL *new_namep)
{
   int retval = OTTO_SUCCESS;
   char *action = "update_job";
   int rc;
   char new_name[NAMLEN+5];

   // don't perform a name change on anything but an update
   if(item->opcode == UPDATE_JOB)
   {
      // reset pointer if its first character is ','
      if(new_namep != NULL && *new_namep->start == ',') new_namep = NULL;

      if(new_namep != NULL)
      {
         item->attributes |= HAS_NEW_NAME;

         // unpack the value - convert to what it would look like in JIL
         csv_getcol(new_name, sizeof(new_name), new_namep, WITHOUT_QUOTES);

         if(jil_reserved_word(new_name) == JIL_UNKNOWN)
         {
            rc = ottojob_copy_name(item->expression, new_name, NAMLEN);
            if(strcmp(item->name, item->expression) == 0)
            {
               rc |= OTTO_SAME_NAME;
            }
            if(strcmp(item->box_name, item->expression) == 0)
            {
               rc |= OTTO_SAME_JOB_BOX_NAMES;
            }

            if((rc = ottojob_copy_name(item->expression, new_name, NAMLEN)) != OTTO_SUCCESS)
            {
               ottojob_print_new_name_errors(rc, action, item->name, item->expression);

               retval = OTTO_FAIL;
            }
         }
      }
   }

   return(retval);
}



int
validate_and_copy_csv_start(JOB *item, CSVCOL *startp)
{
   int retval = OTTO_SUCCESS;
   char *action = "update_job";
   int rc;
   char start[30];

   // don't perform a start time change on anything but an update
   if(item->opcode == UPDATE_JOB)
   {
      // reset pointer if its first character is ','
      if(startp != NULL && *startp->start == ',') startp = NULL;

      if(startp != NULL)
      {
         item->attributes |= HAS_START;

         // unpack the value - convert to what it would look like in JIL
         csv_getcol(start, sizeof(start), startp, WITH_QUOTES);

         if(jil_reserved_word(start) == JIL_UNKNOWN)
         {
            if((rc = ottojob_copy_time(&item->start, start)) != OTTO_SUCCESS)
            {
               ottojob_print_start_errors(rc, action, item->name);

               retval = OTTO_FAIL;
            }
         }
      }
   }

   return(retval);
}



int
validate_and_copy_csv_finish(JOB *item, CSVCOL *finishp)
{
   int retval = OTTO_SUCCESS;
   char *action = "update_job";
   int rc;
   char finish[30];

   // don't perform a finish time change on anything but an update
   if(item->opcode == UPDATE_JOB)
   {
      // reset pointer if its first character is ','
      if(finishp != NULL && *finishp->start == ',') finishp = NULL;

      if(finishp != NULL)
      {
         item->attributes |= HAS_FINISH;

         // unpack the value - convert to what it would look like in JIL
         csv_getcol(finish, sizeof(finish), finishp, WITH_QUOTES);

         if(jil_reserved_word(finish) == JIL_UNKNOWN)
         {
            if((rc = ottojob_copy_time(&item->finish, finish)) != OTTO_SUCCESS)
            {
               ottojob_print_finish_errors(rc, action, item->name);

               retval = OTTO_FAIL;
            }
         }
      }
   }

   return(retval);
}



int
csv_gethdr(CSVROW *hdr, DYNBUF *b)
{
   int  retval = OTTO_SUCCESS;
   int  c;
   int  action_count          = 0,
        name_count            = 0,
        type_count            = 0,
        box_name_count        = 0,
        description_count     = 0,
        command_count         = 0,
        condition_count       = 0,
        date_conditions_count = 0,
        days_of_week_count    = 0,
        start_mins_count      = 0,
        start_times_count     = 0,
        autohold_count        = 0,
        start_count           = 0,
        finish_count          = 0,
        loop_count            = 0,
        new_name_count        = 0;
   char v[128];

   csv_getrow(hdr, b);
   for(c=0; c<hdr->ncol; c++)
   {
      csv_getcol(v, sizeof(v), &hdr->col[c], WITHOUT_QUOTES);

      if(strcasecmp(v, "action")          == 0) {hdr->col[c].type = CSV_ACTION;          action_count++;          continue;}
      if(strcasecmp(v, "name")            == 0) {hdr->col[c].type = CSV_NAME;            name_count++;            continue;}
      if(strcasecmp(v, "type")            == 0) {hdr->col[c].type = CSV_TYPE;            type_count++;            continue;}
      if(strcasecmp(v, "box_name")        == 0) {hdr->col[c].type = CSV_BOX_NAME;        box_name_count++;        continue;}
      if(strcasecmp(v, "description")     == 0) {hdr->col[c].type = CSV_DESCRIPTION;     description_count++;     continue;}
      if(strcasecmp(v, "command")         == 0) {hdr->col[c].type = CSV_COMMAND;         command_count++;         continue;}
      if(strcasecmp(v, "condition")       == 0) {hdr->col[c].type = CSV_CONDITION;       condition_count++;       continue;}
      if(strcasecmp(v, "date_conditions") == 0) {hdr->col[c].type = CSV_DATE_CONDITIONS; date_conditions_count++; continue;}
      if(strcasecmp(v, "days_of_week")    == 0) {hdr->col[c].type = CSV_DAYS_OF_WEEK;    days_of_week_count++;    continue;}
      if(strcasecmp(v, "start_mins")      == 0) {hdr->col[c].type = CSV_START_MINS;      start_mins_count++;      continue;}
      if(strcasecmp(v, "start_times")     == 0) {hdr->col[c].type = CSV_START_TIMES;     start_times_count++;     continue;}
      if(strcasecmp(v, "autohold")        == 0) {hdr->col[c].type = CSV_AUTOHOLD;        autohold_count++;        continue;}
      if(strcasecmp(v, "start")           == 0) {hdr->col[c].type = CSV_START;           start_count++;           continue;}
      if(strcasecmp(v, "finish")          == 0) {hdr->col[c].type = CSV_FINISH;          finish_count++;          continue;}
      if(strcasecmp(v, "loop")            == 0) {hdr->col[c].type = CSV_LOOP;            loop_count++;            continue;}
      if(strcasecmp(v, "new_name")        == 0) {hdr->col[c].type = CSV_NEW_NAME;        new_name_count++;        continue;}

      hdr->col[c].type = CSV_UNSUPPD;
   };

   // error check multiply defined columns
   if(action_count          > 1 ||
      name_count            > 1 ||
      type_count            > 1 ||
      box_name_count        > 1 ||
      description_count     > 1 ||
      command_count         > 1 ||
      condition_count       > 1 ||
      date_conditions_count > 1 ||
      days_of_week_count    > 1 ||
      start_mins_count      > 1 ||
      start_times_count     > 1 ||
      autohold_count        > 1 ||
      start_count           > 1 ||
      finish_count          > 1 ||
      loop_count            > 1 ||
      new_name_count        > 1)
   {
      fprintf(stderr, "Multiple occurrences of header column names.\n");
      retval = OTTO_FAIL;
   }

   // error check minimum required columns
   if(action_count == 0 ||
      name_count   == 0 ||
      type_count   == 0)
   {
      fprintf(stderr, "Missing required header column names (action, name, type).\n");
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS)
   {
      for(c=0; c<hdr->ncol; c++)
      {
         if(hdr->col[c].type == CSV_ACTION) hdr->action_index = c;
         if(hdr->col[c].type == CSV_TYPE)   hdr->type_index   = c;
      }
   }

   return(retval);
}



int
csv_getrow(CSVROW *row, DYNBUF *b)
{
   int retval = OTTO_SUCCESS;
   char *c;
   CSVCOL col;

   if(row == NULL || b == NULL)
      retval = OTTO_FAIL;

   // clear or allocate columns
   if(retval == OTTO_SUCCESS)
      retval = csv_preprow(row);

   while(retval == OTTO_SUCCESS && *(b->s) != '\0')
   {
      // read a column
      c = b->s;
      memset(&col, 0, sizeof(col));
      col.start = c;

      while(retval == OTTO_SUCCESS && *c != ',' && *c != '\n' && *c != '\0')
      {
         switch(*c)
         {
            default:
               ; // just a "normal" character, do nothing
               break;
            case '"':
               if(c == col.start)     // ONLY okay to find an unguarded quote at the start of a column
               {
                  col.quoted = OTTO_TRUE;
                  c++;
                  // read to the closing quote
                  while(*c != '\0')
                  {
                     if(*c == '"' && *(c+1) != '"')
                     {
                        // this unguarded quote is the end of the column
                        // if the next character isn't a comma, newline, or null terminator
                        // the file is malformed
                        if(*(c+1) != ',' && *(c+1) != '\n' && *(c+1) != '\0')
                        {
                           // TODO report this
                           retval = OTTO_FAIL;
                        }
                        break;
                     }

                     c++;
                  }
               }
               else  // a quote in the middle of a column is malformed
               {
                  // TODO report this
                  retval = OTTO_FAIL;
               }
               break;
         }

         c++;
      }

      if(retval == OTTO_SUCCESS)
      {
         // the column ends on the delimiter
         col.end = c;

         // add to the row
         retval = csv_addcol(row, &col);

         // set the buffer s pointer to the character after the delimiter
         // unless the delimiter is the null terminator
         if(*c != '\0')
            b->s = c+1;
         else
            b->s = c;

         // break from the loop if the delimiter is a newline
         if(*c == '\n')
            break;
      }
   }

   return(retval);
}



int
csv_get_action(CSVCOL *c)
{
   int  retval = OTTO_UNKNOWN;
   char v[128];

   csv_getcol(v, sizeof(v), c, WITHOUT_QUOTES);

   if(strncasecmp(v, "insert", strlen(v)) == 0) retval = CSV_INSERT;
   if(strncasecmp(v, "update", strlen(v)) == 0) retval = CSV_UPDATE;
   if(strncasecmp(v, "delete", strlen(v)) == 0) retval = CSV_DELETE;

   return(retval);
}



int
csv_get_type(CSVCOL *c)
{
   int  retval = OTTO_UNKNOWN;
   char v[128];

   csv_getcol(v, sizeof(v), c, WITHOUT_QUOTES);

   if(strncasecmp(v, "box", strlen(v)) == 0) retval = OTTO_BOX;
   if(strncasecmp(v, "cmd", strlen(v)) == 0) retval = OTTO_CMD;

   return(retval);
}



int
csv_preprow(CSVROW *r)
{
   int retval = OTTO_SUCCESS;

   if(r == NULL)
      retval = OTTO_FAIL;

   // allocate space for the new column if it doesn't already exist
   if(retval == OTTO_SUCCESS)
   {
      if(r->acol == 0 || r->col == NULL)
      {
         if((r->col = (CSVCOL *)calloc(CSV_UNSUPPD, sizeof(CSVCOL))) != NULL)
         {
            r->acol = CSV_UNSUPPD;
            r->ncol = 0;
         }
         else
         {
            retval = OTTO_FAIL;
         }
      }
      else
      {
         if(r->col != NULL)
         {
            memset(r->col, 0, (r->acol * sizeof(CSVCOL)));
            r->ncol = 0;
         }
         else
         {
            retval = OTTO_FAIL;
         }
      }
   }

   return(retval);
}



int
csv_addcol(CSVROW *r, CSVCOL *c)
{
   int retval = OTTO_SUCCESS;

   if(r == NULL || c == NULL)
      retval = OTTO_FAIL;

   // allocate space for the new column if it doesn't already exist
   if(retval == OTTO_SUCCESS)
   {
      if(r->acol < (r->ncol + 1))
      {
         if((r->col = (CSVCOL *)realloc(r->col, sizeof(CSVCOL)*(r->ncol + 1))) != NULL)
         {
            r->acol = (r->ncol + 1);
         }
         else
         {
            retval = OTTO_FAIL;
         }
      }
   }

   // transfer information from submitted col structure to the new structure
   if(retval == OTTO_SUCCESS)
   {
      memcpy(&r->col[r->ncol], c, sizeof(CSVCOL));
      r->ncol++;
   }

   return(retval);
}



void
csv_getcol(char *t, size_t tlen, CSVCOL *c, int style)
{
   char *s;
   char *e;
   int space_to_fill = tlen - 1;  // leave room for null terminator

   if(t != NULL && tlen > 0 && c != NULL)
   {
      s = c->start;
      e = c->end;

      // remove quotes for quoted strings
      if(c->quoted == OTTO_TRUE)
      {
         s++;
         e--;
      }

      if(style == WITH_QUOTES)
      {
         space_to_fill -= 2; // leave room for requested quotes
         *t++ = '"';
      }


      // now inside the column data move a character at a time
      while(s < e && space_to_fill > 0)
      {
         if(*s == '"' && *(s+1) == '"')
            s++;

         *t++ = *s++;
         space_to_fill--;
      }

      if(style == WITH_QUOTES)
      {
         *t++ = '"';
      }

      // null terminate
      *t = '\0';
   }
}



void
remove_carriage_returns(DYNBUF *b)
{
   char *s, *t;
   int  in_quote = OTTO_FALSE;

   s = b->buffer;
   t = b->buffer;

   // consume leading whitespace
   while(*s != '\0' && isspace(*s))
      s++;

   while(*s != '\0')
   {
      *t = *s;

      if(*t == '\r' && in_quote == OTTO_FALSE)
         t--;

      if(*s == '"')
      {
         if(in_quote == OTTO_FALSE)
            in_quote = OTTO_TRUE;
         else
            in_quote = OTTO_FALSE;
      }

      t++;
      s++;
   }

   *t = '\0';
}

