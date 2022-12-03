#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "otto.h"



int  add_insert_job(JOB *item, DYNBUF *b);
int  add_update_job(JOB *item, DYNBUF *b);
int  add_delete_item(JOB *item, DYNBUF *b, char *kind);
void set_jil_joblist_levels(JOBLIST *joblist);
int  validate_and_copy_jil_name(JOB *item, char *namep);
int  validate_and_copy_jil_type(JOB *item, char *typep);
int  validate_and_copy_jil_box_name(JOB *item, char *box_namep);
int  validate_and_copy_jil_command(JOB *item, char *commandp);
int  validate_and_copy_jil_condition(JOB *item, char *conditionp);
int  validate_and_copy_jil_description(JOB *item, char *descriptionp);
int  validate_and_copy_jil_environment(JOB *item, char *environmentp);
int  validate_and_copy_jil_auto_hold(JOB *item, char *auto_holdp);
int  validate_and_copy_jil_date_conditions(JOB *item, char *date_conditionsp, char *days_of_weekp, char *start_minsp, char *start_timesp);
int  validate_and_copy_jil_loop(JOB *item, char *loopp);
int  validate_and_copy_jil_new_name(JOB *item, char *new_namep);
int  validate_and_copy_jil_start(JOB *item, char *startp);
int  validate_and_copy_jil_finish(JOB *item, char *finishp);

void advance_jilword(DYNBUF *b);
void remove_jil_comments(DYNBUF *b);



int
parse_jil(DYNBUF *b, JOBLIST *joblist)
{
   int retval = OTTO_SUCCESS;
   int i = 0;

   if(joblist == NULL)
      retval = OTTO_FAIL;

   // clean up the buffer before proceeding
   remove_jil_comments(b);

   // count items
   if(retval == OTTO_SUCCESS)
   {
      memset(joblist, 0, sizeof(JOBLIST));

      b->line = 0;
      b->s    = b->buffer;
      while(retval == OTTO_SUCCESS && b->s[0] != '\0')
      {
         switch(jil_reserved_word(b->s))
         {
            case JIL_INS_JOB:
            case JIL_UPD_JOB:
            case JIL_DEL_BOX:
            case JIL_DEL_JOB:
               joblist->nitems++;
               break;
            case JIL_BADWORD:
               retval = OTTO_FAIL;
               break;
            default:
               break;
         }
         advance_word(b);
      }

      if(retval == OTTO_SUCCESS && joblist->nitems == 0)
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
      b->s    = b->buffer;
      while(b->s[0] != '\0')
      {
         switch(jil_reserved_word(b->s))
         {
            case JIL_INS_JOB:
               retval = add_insert_job(&joblist->item[i++], b);
               break;
            case JIL_UPD_JOB:
               retval = add_update_job(&joblist->item[i++], b);
               break;
            case JIL_DEL_BOX:
               retval = add_delete_item(&joblist->item[i++], b, "box");
               break;
            case JIL_DEL_JOB:
               retval = add_delete_item(&joblist->item[i++], b, "job");
               break;
            default:
               break;
         }
         advance_word(b);
      }
   }

   if(retval == OTTO_SUCCESS)
      set_jil_joblist_levels(joblist);

   return(retval);
}



int
add_insert_job(JOB *item, DYNBUF *b)
{
   int     retval                = OTTO_SUCCESS;
   int     exit_loop             = OTTO_FALSE;
   char    *namep                = NULL;
   char    *typep                = NULL;
   char    *auto_holdp           = NULL;
   char    *box_namep            = NULL;
   char    *commandp             = NULL;
   char    *conditionp           = NULL;
   char    *descriptionp         = NULL;
   char    *environmentp         = NULL;
   char    *date_conditionsp     = NULL;
   char    *days_of_weekp        = NULL;
   char    *start_minsp          = NULL;
   char    *start_timesp         = NULL;
   char    *loopp                = NULL;

   advance_jilword(b);
   namep = b->s;

   while(exit_loop == OTTO_FALSE)
   {
      switch(jil_reserved_word(b->s))
      {
         case JIL_JOBTYPE: advance_jilword(b); typep            = b->s; break;
         case JIL_BOXNAME: advance_jilword(b); box_namep        = b->s; break;
         case JIL_COMMAND: advance_jilword(b); commandp         = b->s; break;
         case JIL_CONDITN: advance_jilword(b); conditionp       = b->s; break;
         case JIL_DESCRIP: advance_jilword(b); descriptionp     = b->s; break;
         case JIL_ENVIRON: advance_jilword(b); environmentp     = b->s; break;
         case JIL_AUTOHLD: advance_jilword(b); auto_holdp       = b->s; break;
         case JIL_DTECOND: advance_jilword(b); date_conditionsp = b->s; break;
         case JIL_DYSOFWK: advance_jilword(b); days_of_weekp    = b->s; break;
         case JIL_STRTMIN: advance_jilword(b); start_minsp      = b->s; break;
         case JIL_STRTTIM: advance_jilword(b); start_timesp     = b->s; break;
         case JIL_LOOP:    advance_jilword(b); loopp            = b->s; break;
         case JIL_DEL_BOX:
         case JIL_DEL_JOB:
         case JIL_INS_JOB:
         case JIL_UPD_JOB:
         case JIL_UNSUPPD: exit_loop = OTTO_TRUE; break;
         default:          advance_word(b);       break;
      }

      if(b->s[0] == '\0')
         exit_loop = OTTO_TRUE;
   }

   memset(item, 0, sizeof(JOB));
   item->opcode = CREATE_JOB;

   if(validate_and_copy_jil_name(item, namep) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_jil_type(item, typep) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_jil_box_name(item, box_namep) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_jil_command(item, commandp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_jil_condition(item, conditionp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_jil_description(item, descriptionp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_jil_environment(item, environmentp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_jil_auto_hold(item, auto_holdp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_jil_date_conditions(item, date_conditionsp, days_of_weekp, start_minsp, start_timesp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_jil_loop(item, loopp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;

   regress_word(b);

   return(retval);
}



int
add_update_job(JOB *item, DYNBUF *b)
{
   int     retval                = OTTO_SUCCESS;
   int     exit_loop             = OTTO_FALSE;
   char    *namep                = NULL;
   char    *typep                = NULL;
   char    *auto_holdp           = NULL;
   char    *box_namep            = NULL;
   char    *commandp             = NULL;
   char    *conditionp           = NULL;
   char    *descriptionp         = NULL;
   char    *environmentp         = NULL;
   char    *date_conditionsp     = NULL;
   char    *days_of_weekp        = NULL;
   char    *start_minsp          = NULL;
   char    *start_timesp         = NULL;
   char    *loopp                = NULL;
   char    *startp               = NULL;
   char    *finishp              = NULL;
   char    *new_namep            = NULL;

   advance_jilword(b);
   namep = b->s;

   while(exit_loop == OTTO_FALSE)
   {
      switch(jil_reserved_word(b->s))
      {
         case JIL_JOBTYPE: advance_jilword(b); typep            = b->s; break;
         case JIL_BOXNAME: advance_jilword(b); box_namep        = b->s; break;
         case JIL_COMMAND: advance_jilword(b); commandp         = b->s; break;
         case JIL_CONDITN: advance_jilword(b); conditionp       = b->s; break;
         case JIL_DESCRIP: advance_jilword(b); descriptionp     = b->s; break;
         case JIL_ENVIRON: advance_jilword(b); environmentp     = b->s; break;
         case JIL_AUTOHLD: advance_jilword(b); auto_holdp       = b->s; break;
         case JIL_DTECOND: advance_jilword(b); date_conditionsp = b->s; break;
         case JIL_DYSOFWK: advance_jilword(b); days_of_weekp    = b->s; break;
         case JIL_STRTMIN: advance_jilword(b); start_minsp      = b->s; break;
         case JIL_STRTTIM: advance_jilword(b); start_timesp     = b->s; break;
         case JIL_LOOP:    advance_jilword(b); loopp            = b->s; break;
         case JIL_START:   advance_jilword(b); startp           = b->s; break;
         case JIL_FINISH:  advance_jilword(b); finishp          = b->s; break;
         case JIL_NEWNAME: advance_jilword(b); new_namep        = b->s; break;
         case JIL_DEL_BOX:
         case JIL_DEL_JOB:
         case JIL_INS_JOB:
         case JIL_UPD_JOB:
         case JIL_UNSUPPD: exit_loop = OTTO_TRUE; break;
         default:          advance_word(b);       break;
      }

      if(b->s[0] == '\0')
         exit_loop = OTTO_TRUE;
   }

   memset(item, 0, sizeof(JOB));
   item->opcode = UPDATE_JOB;

   if(validate_and_copy_jil_name(item, namep) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_jil_type(item, typep) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_jil_box_name(item, box_namep) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_jil_command(item, commandp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_jil_condition(item, conditionp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_jil_description(item, descriptionp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_jil_environment(item, environmentp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_jil_auto_hold(item, auto_holdp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_jil_date_conditions(item, date_conditionsp, days_of_weekp, start_minsp, start_timesp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_jil_loop(item, loopp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_jil_start(item, startp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_jil_finish(item, finishp) != OTTO_SUCCESS)
      retval = OTTO_FAIL;
   if(validate_and_copy_jil_new_name(item, new_namep) != OTTO_SUCCESS)
      retval = OTTO_FAIL;

   regress_word(b);

   return(retval);
}



int
add_delete_item(JOB *item, DYNBUF *b, char *kind)
{
   int retval = OTTO_SUCCESS;
   char *namep = NULL;

   advance_jilword(b);
   namep = b->s;

   memset(item, 0, sizeof(JOB));
   if(strcmp(kind, "box") == 0)
      item->opcode = DELETE_BOX;
   else
      item->opcode = DELETE_JOB;

   if(validate_and_copy_jil_name(item, namep) != OTTO_SUCCESS)
      retval = OTTO_FAIL;

   return(retval);
}



int
jil_reserved_word(char *s)
{
   int retval = JIL_UNKNOWN;
   int c, i;

   switch(s[0])
   {
      case 'a':
         if(strncmp(s, "auto_hold",        9) == 0) { retval = JIL_AUTOHLD; c =  9; }
         break;
      case 'b':
         if(strncmp(s, "box_name",         8) == 0) { retval = JIL_BOXNAME; c =  8; }
         break;
      case 'c':
         if(strncmp(s, "command",          7) == 0) { retval = JIL_COMMAND; c =  7; }
         if(strncmp(s, "condition",        9) == 0) { retval = JIL_CONDITN; c =  9; }
         break;
      case 'd':
         if(strncmp(s, "date_conditions", 15) == 0) { retval = JIL_DTECOND; c = 15; }
         if(strncmp(s, "days_of_week",    12) == 0) { retval = JIL_DYSOFWK; c = 12; }
         if(strncmp(s, "description",     11) == 0) { retval = JIL_DESCRIP; c = 11; }
         if(strncmp(s, "delete_box",      10) == 0) { retval = JIL_DEL_BOX; c = 10; }
         if(strncmp(s, "delete_job",      10) == 0) { retval = JIL_DEL_JOB; c = 10; }
         if(strncmp(s, "delete_blob",     11) == 0) { retval = JIL_UNSUPPD; c = 11; }
         if(strncmp(s, "delete_glob",     11) == 0) { retval = JIL_UNSUPPD; c = 11; }
         if(strncmp(s, "delete_job_type", 15) == 0) { retval = JIL_UNSUPPD; c = 15; }
         if(strncmp(s, "delete_machine",  14) == 0) { retval = JIL_UNSUPPD; c = 14; }
         if(strncmp(s, "delete_monbro",   13) == 0) { retval = JIL_UNSUPPD; c = 13; }
         if(strncmp(s, "delete_resource", 15) == 0) { retval = JIL_UNSUPPD; c = 15; }
         if(strncmp(s, "delete_xinst",    12) == 0) { retval = JIL_UNSUPPD; c = 12; }
         break;
      case 'e':
         if(strncmp(s, "environment",     11) == 0) { retval = JIL_ENVIRON; c = 11; }
         break;
      case 'f':
         if(strncmp(s, "finish",           6) == 0) { retval = JIL_FINISH;  c =  6; }
         break;
      case 'i':
         if(strncmp(s, "insert_job",      10) == 0) { retval = JIL_INS_JOB; c = 10; }
         if(strncmp(s, "insert_blob",     11) == 0) { retval = JIL_UNSUPPD; c = 11; }
         if(strncmp(s, "insert_glob",     11) == 0) { retval = JIL_UNSUPPD; c = 11; }
         if(strncmp(s, "insert_job_type", 15) == 0) { retval = JIL_UNSUPPD; c = 15; }
         if(strncmp(s, "insert_machine",  14) == 0) { retval = JIL_UNSUPPD; c = 14; }
         if(strncmp(s, "insert_monbro",   13) == 0) { retval = JIL_UNSUPPD; c = 13; }
         if(strncmp(s, "insert_resource", 15) == 0) { retval = JIL_UNSUPPD; c = 15; }
         if(strncmp(s, "insert_xinst",    12) == 0) { retval = JIL_UNSUPPD; c = 12; }
         break;
      case 'j':
         if(strncmp(s, "job_type",         8) == 0) { retval = JIL_JOBTYPE; c =  8; }
         break;
      case 'l':
         if(strncmp(s, "loop",             4) == 0) { retval = JIL_LOOP;    c =  4; }
         break;
      case 'n':
         if(strncmp(s, "new_name",         8) == 0) { retval = JIL_NEWNAME; c =  8; }
         break;
      case 'o':
         if(strncmp(s, "override_job",    12) == 0) { retval = JIL_UNSUPPD; c = 12; }
         break;
      case 's':
         if(strncmp(s, "start",            5) == 0) { retval = JIL_START;   c =  5; }
         if(strncmp(s, "start_mins",      10) == 0) { retval = JIL_STRTMIN; c = 10; }
         if(strncmp(s, "start_times",     11) == 0) { retval = JIL_STRTTIM; c = 11; }
         break;
      case 'u':
         if(strncmp(s, "update_job",      10) == 0) { retval = JIL_UPD_JOB; c = 10; }
         if(strncmp(s, "update_job_type", 15) == 0) { retval = JIL_UNSUPPD; c = 15; }
         if(strncmp(s, "update_machine",  14) == 0) { retval = JIL_UNSUPPD; c = 14; }
         if(strncmp(s, "update_monbro",   13) == 0) { retval = JIL_UNSUPPD; c = 13; }
         if(strncmp(s, "update_resource", 15) == 0) { retval = JIL_UNSUPPD; c = 15; }
         if(strncmp(s, "update_xinst",    12) == 0) { retval = JIL_UNSUPPD; c = 12; }
         break;
      default:                                        retval = JIL_UNKNOWN;
   }

   // if the result is JIL_UNKNOWN and the word has a colon adjacent to it (perhaps separater
   // by spaces) the it is probably misspelled or intended to be a keyword Otto dopesn't
   // recognize.  Warn for that here.
   if(retval == JIL_UNKNOWN)
   {
      // consume the word
      for(i=0; s[i] != '\0'; i++)
      {
         if(isspace(s[i]) || s[i] == ':')
         {
            break;
         }
      }

      // consume potential spaces
      for(; s[i] != '\0'; i++)
      {
         if(!isspace(s[i]))
         {
            break;
         }
      }

      // if this is a potential keyword a colon should be the next character
      if(s[i] == ':')
      {
         fprintf(stderr, "ERROR Unrecognized keyword found: '");
         for(i=0; s[i] != '\0'; i++)
         {
            if(isspace(s[i]) || s[i] == ':')
            {
               break;
            }
            fprintf(stderr, "%c", s[i]);
         }
         fprintf(stderr, ":'\n");
         retval = JIL_BADWORD;
      }
   }

   if(retval != JIL_UNKNOWN && retval != JIL_UNSUPPD && retval != JIL_BADWORD)
   {
      // autosys allows whitespace between a keyword and the following
      // colon.  it's easier to parse if it's adjacent so fix it here
      // look ahead for the colon, if found move it to c if necessary
      for(i=c; s[i] != '\0'; i++)
      {
         if(!isspace(s[i]) && s[i] != ':')
         {
            // encountered something other than whitespace or a colon
            // so this must not be a supported JIL keyword
            retval = JIL_UNKNOWN;
            break;
         }
         if(s[i] == ':')
         {
            if(i != c)
            {
               s[c] = ':';
               s[i] = ' ';
            }
            break;
         }
      }
   }

   if(retval != JIL_UNKNOWN && retval != JIL_UNSUPPD && retval != JIL_BADWORD)
   {
      // a supported JIL keyword (or a manufactured one) was found
      // the jil parser works best with newlines at the end of each
      // keyword use but it's okay to type JIL all on one line
      // since JIL keywords have to be separated from other "words"
      // (multi-word strings enclosed in double quotes count as one word)
      // the white space preceding the found JIL keyword can be changed to
      // a newline
      s[-1] = '\n';
   }

   return(retval);
}



void
set_jil_joblist_levels(JOBLIST *joblist)
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
validate_and_copy_jil_name(JOB *item, char *namep)
{
   int retval   = OTTO_SUCCESS;
   char *action = "action";
   char *kind   = "";
   int rc;

   switch(item->opcode)
   {
      case CREATE_JOB: action = "insert_job"; kind = "job "; break;
      case UPDATE_JOB: action = "update_job"; kind = "job "; break;
      case DELETE_JOB: action = "delete_job"; kind = "job "; break;
      case DELETE_BOX: action = "delete_box"; kind = "box "; break;
   }

   if(namep == NULL || *namep == '\n')
   {
      rc = OTTO_MISSING_REQUIRED_VALUE;
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS && jil_reserved_word(namep) != JIL_UNKNOWN)
   {
      rc = OTTO_SYNTAX_ERROR;
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS)
   {
      if((rc = ottojob_copy_name(item->name, namep, NAMLEN)) != OTTO_SUCCESS)
      {
         retval = OTTO_FAIL;
      }
   }

   if(retval == OTTO_FAIL)
   {
      ottojob_print_name_errors(rc, action, item->name, kind);
   }

   return(retval);
}



int
validate_and_copy_jil_type(JOB *item, char *typep)
{
   int retval = OTTO_SUCCESS;
   char *action = "action";
   int rc;

   switch(item->opcode)
   {
      case CREATE_JOB: action = "insert_job"; break;
      case UPDATE_JOB: action = "update_job"; break;
   }

   // it's okay to not have this keyword on an update_job specification
   if(typep == NULL && item->opcode == CREATE_JOB)
   {
      rc = OTTO_MISSING_REQUIRED_VALUE;
      retval = OTTO_FAIL;
   }

   // it's never okay to have the keyword with nothing else on the line
   if(typep != NULL && *typep == '\n')
   {
      rc = OTTO_MISSING_REQUIRED_VALUE;
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS && jil_reserved_word(typep) != JIL_UNKNOWN)
   {
      rc = OTTO_SYNTAX_ERROR;
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS)
   {
      item->attributes |= HAS_TYPE;

      if(typep != NULL &&
         (rc = ottojob_copy_type(&item->type, typep, TYPLEN)) != OTTO_SUCCESS)
      {
         retval = OTTO_FAIL;
      }
   }

   if(retval != OTTO_SUCCESS)
   {
      ottojob_print_type_errors(rc, action, item->name, typep);
   }

   return(retval);
}



int
validate_and_copy_jil_box_name(JOB *item, char *box_namep)
{
   int retval = OTTO_SUCCESS;
   char *action = "action";
   int rc;

   switch(item->opcode)
   {
      case CREATE_JOB: action = "insert_job"; break;
      case UPDATE_JOB: action = "update_job"; break;
   }

   // it's okay for this to be NULL in any case but it's not
   // okay for it to be an empty line on an insert_job action
   if(box_namep != NULL && *box_namep == '\n' && item->opcode == CREATE_JOB)
   {
      rc = OTTO_MISSING_REQUIRED_VALUE;
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS && jil_reserved_word(box_namep) != JIL_UNKNOWN)
   {
      rc = OTTO_SYNTAX_ERROR;
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS && box_namep != NULL)
   {
      item->attributes |= HAS_BOX_NAME;

      if(*box_namep != '\n')
      {
         rc = ottojob_copy_name(item->box_name, box_namep, NAMLEN);
         if(item->type == OTTO_BOX && strcmp(item->name, item->box_name) == 0)
         {
            rc |= OTTO_SAME_JOB_BOX_NAMES;
         }
         if(rc != OTTO_SUCCESS)
         {
            retval = OTTO_FAIL;
         }
      }
   }

   if(retval != OTTO_SUCCESS)
   {
      ottojob_print_box_name_errors(rc, action, item->name, item->box_name);
   }

   return(retval);
}



int
validate_and_copy_jil_command(JOB *item, char *commandp)
{
   int retval = OTTO_SUCCESS;
   char *action = "action";
   int rc;

   switch(item->opcode)
   {
      case CREATE_JOB: action = "insert_job"; break;
      case UPDATE_JOB: action = "update_job"; break;
   }

   // it's okay for this to be NULL in any case since the requirements
   // check on job_type vs command will be done later. but it's not
   // okay for it to be an empty line on an insert_job action
   if(commandp != NULL && *commandp == '\n' && item->opcode == CREATE_JOB)
   {
      rc = OTTO_MISSING_REQUIRED_VALUE;
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS && jil_reserved_word(commandp) != JIL_UNKNOWN)
   {
      rc = OTTO_SYNTAX_ERROR;
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS && commandp != NULL)
   {
      item->attributes |= HAS_COMMAND;

      if(*commandp != '\n')
      {
         if((rc = ottojob_copy_command(item->command, commandp, CMDLEN)) != OTTO_SUCCESS)
         {
            retval = OTTO_FAIL;
         }
      }
   }

   if(rc != OTTO_SUCCESS)
   {
      ottojob_print_command_errors(rc, action, item->name, CMDLEN);
   }

   return(retval);
}



int
validate_and_copy_jil_condition(JOB *item, char *conditionp)
{
   int retval = OTTO_SUCCESS;
   char *action = "action";
   int rc;

   switch(item->opcode)
   {
      case CREATE_JOB: action = "insert_job"; break;
      case UPDATE_JOB: action = "update_job"; break;
   }

   // it's okay for this to be NULL in any case but it's not
   // okay for it to be an empty line on an insert_job action
   if(conditionp != NULL && *conditionp == '\n' && item->opcode == CREATE_JOB)
   {
      rc = OTTO_MISSING_REQUIRED_VALUE;
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS && jil_reserved_word(conditionp) != JIL_UNKNOWN)
   {
      rc = OTTO_SYNTAX_ERROR;
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS && conditionp != NULL)
   {
      item->attributes |= HAS_CONDITION;

      if(*conditionp != '\n')
      {
         if((rc = ottojob_copy_condition(item->condition, conditionp, CNDLEN)) != OTTO_SUCCESS)
         {
            retval = OTTO_FAIL;
         }
      }
   }

   if(retval != OTTO_SUCCESS)
   {
      ottojob_print_condition_errors(rc, action, item->name, CMDLEN);
   }

   return(retval);
}



int
validate_and_copy_jil_description(JOB *item, char *descriptionp)
{
   int retval = OTTO_SUCCESS;
   char *action = "action";
   int rc;

   switch(item->opcode)
   {
      case CREATE_JOB: action = "insert_job"; break;
      case UPDATE_JOB: action = "update_job"; break;
   }

   // it's okay for this to be NULL in any case but it's not
   // okay for it to be an empty line on an insert_job action
   if(descriptionp != NULL && *descriptionp == '\n' && item->opcode == CREATE_JOB)
   {
      rc = OTTO_MISSING_REQUIRED_VALUE;
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS && jil_reserved_word(descriptionp) != JIL_UNKNOWN)
   {
      rc = OTTO_SYNTAX_ERROR;
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS && descriptionp != NULL)
   {
      item->attributes |= HAS_DESCRIPTION;

      if(*descriptionp != '\n')
      {
         if((rc = ottojob_copy_description(item->description, descriptionp, DSCLEN)) != OTTO_SUCCESS)
         {
            retval = OTTO_FAIL;
         }
      }
   }

   if(retval != OTTO_SUCCESS)
   {
      ottojob_print_description_errors(rc, action, item->name, DSCLEN);
   }

   return(retval);
}



int
validate_and_copy_jil_environment(JOB *item, char *environmentp)
{
   int retval = OTTO_SUCCESS;
   char *action = "action";
   int rc;

   switch(item->opcode)
   {
      case CREATE_JOB: action = "insert_job"; break;
      case UPDATE_JOB: action = "update_job"; break;
   }

   // it's okay for this to be NULL in any case but it's not
   // okay for it to be an empty line on an insert_job action
   if(environmentp != NULL && *environmentp == '\n' && item->opcode == CREATE_JOB)
   {
      rc = OTTO_MISSING_REQUIRED_VALUE;
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS && jil_reserved_word(environmentp) != JIL_UNKNOWN)
   {
      rc = OTTO_SYNTAX_ERROR;
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS && environmentp != NULL)
   {
      item->attributes |= HAS_ENVIRONMENT;

      if(*environmentp != '\n')
      {
         if((rc = ottojob_copy_environment(item->environment, environmentp, ENVLEN)) != OTTO_SUCCESS)
         {
            retval = OTTO_FAIL;
         }
      }
   }

   if(retval != OTTO_SUCCESS)
   {
      ottojob_print_environment_errors(rc, action, item->name, ENVLEN);
   }

   return(retval);
}



int
validate_and_copy_jil_auto_hold(JOB *item, char *auto_holdp)
{
   int retval = OTTO_SUCCESS;
   char *action = "action";
   int rc;

   switch(item->opcode)
   {
      case CREATE_JOB: action = "insert_job"; break;
      case UPDATE_JOB: action = "update_job"; break;
   }

   // it's okay for this to be NULL in any case but it's never
   // okay for it to be an empty line
   if(auto_holdp != NULL && *auto_holdp == '\n')
   {
      rc = OTTO_MISSING_REQUIRED_VALUE;
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS && jil_reserved_word(auto_holdp) != JIL_UNKNOWN)
   {
      rc = OTTO_SYNTAX_ERROR;
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS)
   {
      item->attributes |= HAS_AUTO_HOLD;

      if((rc = ottojob_copy_flag(&item->autohold, auto_holdp, FLGLEN)) != OTTO_SUCCESS)
      {
         retval = OTTO_FAIL;
      }
      item->on_autohold = item->autohold;
   }

   if(retval != OTTO_SUCCESS)
   {
      ottojob_print_auto_hold_errors(rc, action, item->name, auto_holdp);
   }

   return(retval);
}



int
validate_and_copy_jil_date_conditions(JOB *item, char *date_conditionsp, char *days_of_weekp, char *start_minsp, char *start_timesp)
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

   switch(item->opcode)
   {
      case CREATE_JOB: action = "insert_job"; break;
      case UPDATE_JOB: action = "update_job"; break;
   }

   // check if a valid comination of date conditions attributes was specified
   if(date_conditionsp != NULL && jil_reserved_word(date_conditionsp) == JIL_UNKNOWN) date_check |= DTECOND_BIT;
   if(days_of_weekp    != NULL && jil_reserved_word(days_of_weekp)    == JIL_UNKNOWN) date_check |= DYSOFWK_BIT;
   if(start_minsp      != NULL && jil_reserved_word(start_minsp)      == JIL_UNKNOWN) date_check |= STRTMNS_BIT;
   if(start_timesp     != NULL && jil_reserved_word(start_timesp)     == JIL_UNKNOWN) date_check |= STRTTMS_BIT;

   switch(date_check)
   {
      case 0:
         // do nothing
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
      ottojob_print_date_conditions_errors(rc, action, item->name, date_conditionsp);

      retval = OTTO_FAIL;
   }


   if(parse_date_conditions == OTTO_TRUE)
   {
      if((rc = ottojob_copy_flag(&date_conditions, date_conditionsp, FLGLEN)) != OTTO_SUCCESS)
      {
         ottojob_print_date_conditions_errors(rc, action, item->name, date_conditionsp);

         retval = OTTO_FAIL;
      }

      if((rc = ottojob_copy_days_of_week(&days_of_week, days_of_weekp)) != OTTO_SUCCESS)
      {
         ottojob_print_days_of_week_errors(rc, action, item->name);

         retval = OTTO_FAIL;
      }

      if(start_minsp != NULL)
      {
         if((rc = ottojob_copy_start_minutes(&start_minutes, start_minsp)) != OTTO_SUCCESS)
         {
            ottojob_print_start_mins_errors(rc, action, item->name);

            retval = OTTO_FAIL;
         }
      }

      if(start_timesp != NULL)
      {
         if((rc = ottojob_copy_start_times(start_times, start_timesp)) != OTTO_SUCCESS)
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
validate_and_copy_jil_loop(JOB *item, char *loopp)
{
   int retval = OTTO_SUCCESS;
   char *action = "action";
   int rc;

   switch(item->opcode)
   {
      case CREATE_JOB: action = "insert_job"; break;
      case UPDATE_JOB: action = "update_job"; break;
   }

   // it's okay for this to be NULL in any case but it's not
   // okay for it to be an empty line on an insert_job action
   if(loopp != NULL && *loopp == '\n' && item->opcode == CREATE_JOB)
   {
      rc = OTTO_MISSING_REQUIRED_VALUE;
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS && jil_reserved_word(loopp) != JIL_UNKNOWN)
   {
      rc = OTTO_SYNTAX_ERROR;
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS && loopp != NULL)
   {
      item->attributes |= HAS_LOOP;

      if(*loopp != '\n')
      {
         if((rc = ottojob_copy_loop(item->loopname, &item->loopmin, &item->loopmax,  &item->loopsgn,  loopp)) != OTTO_SUCCESS)
         {
            ottojob_print_loop_errors(rc, action, item->name);

            retval = OTTO_FAIL;
         }
      }
   }

   if(retval != OTTO_SUCCESS)
   {
      ottojob_print_loop_errors(rc, action, item->name);
   }

   return(retval);
}



int
validate_and_copy_jil_new_name(JOB *item, char *new_namep)
{
   int retval = OTTO_SUCCESS;
   char *action = "update_job";
   int rc;

   // don't perform a name change on anything but an update
   if(item->opcode != UPDATE_JOB)
   {
      rc = OTTO_INVALID_APPLICATION;
      retval = OTTO_FAIL;
   }

   // it's okay for this to be NULL but it's not
   // okay for it to be an empty line
   if(new_namep != NULL && *new_namep == '\n')
   {
      rc = OTTO_MISSING_REQUIRED_VALUE;
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS && jil_reserved_word(new_namep) != JIL_UNKNOWN)
   {
      rc = OTTO_SYNTAX_ERROR;
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS && new_namep != NULL)
   {
      item->attributes |= HAS_NEW_NAME;

      rc = ottojob_copy_name(item->expression, new_namep, NAMLEN);
      if(strcmp(item->name, item->expression) == 0)
      {
         rc |= OTTO_SAME_NAME;
      }
      if(strcmp(item->box_name, item->expression) == 0)
      {
         rc |= OTTO_SAME_JOB_BOX_NAMES;
      }

      if(rc != OTTO_SUCCESS)
      {
         retval = OTTO_FAIL;
      }
   }

   if(retval != OTTO_SUCCESS)
   {
      ottojob_print_new_name_errors(rc, action, item->name, item->expression);
   }

   return(retval);
}



int
validate_and_copy_jil_start(JOB *item, char *startp)
{
   int retval = OTTO_SUCCESS;
   char *action = "update_job";
   int rc;

   // don't perform a start time change on anything but an update
   if(item->opcode != UPDATE_JOB)
   {
      rc = OTTO_INVALID_APPLICATION;
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS && jil_reserved_word(startp) != JIL_UNKNOWN)
   {
      rc = OTTO_SYNTAX_ERROR;
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS && startp != NULL)
   {
      item->attributes |= HAS_START;

      if(*startp != '\n')
      {
         if((rc = ottojob_copy_time(&item->start, startp)) != OTTO_SUCCESS)
         {
            retval = OTTO_FAIL;
         }
      }
   }

   if(retval != OTTO_SUCCESS)
   {
      ottojob_print_start_errors(rc, action, item->name);
   }

   return(retval);
}



int
validate_and_copy_jil_finish(JOB *item, char *finishp)
{
   int retval = OTTO_SUCCESS;
   char *action = "update_job";
   int rc;

   // don't perform a finish time change on anything but an update
   if(item->opcode != UPDATE_JOB)
   {
      rc = OTTO_INVALID_APPLICATION;
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS && jil_reserved_word(finishp) != JIL_UNKNOWN)
   {
      rc = OTTO_SYNTAX_ERROR;
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS && finishp != NULL)
   {
      item->attributes |= HAS_FINISH;

      if(*finishp != '\n')
      {
         if((rc = ottojob_copy_time(&item->finish, finishp)) != OTTO_SUCCESS)
         {
            retval = OTTO_FAIL;
         }
      }
   }

   if(retval != OTTO_SUCCESS)
   {
      ottojob_print_finish_errors(rc, action, item->name);
   }

   return(retval);
}



void
advance_jilword(DYNBUF *b)
{
   // consume non-space
   while(!isspace(*(b->s)) && *(b->s) != ':' && *(b->s) != '\0')
      b->s++;

   // consume space but break if a newline is found since that
   // would indicate an empty value for a jil keyword
   while((isspace(*(b->s)) || *(b->s) == ':') && *(b->s) != '\0')
   {
      if(*(b->s) == '\n')
      {
         b->line++;
         break;
      }
      b->s++;
   }
}



void
remove_jil_comments(DYNBUF *b)
{
   char *s, last_char;

   // remove comments from buffer
   last_char = '\n';

   s = b->buffer;
   while(*s != '\0')
   {
      if(last_char == '\n')
      {
         while(isspace(*s) && *s != '\n' && *s != '\0')
            s++;
      }

      switch(*s)
      {
         case '#':
            if(last_char == '\n')
            {
               // convert line to spaces
               while(*s != '\0' && *s != '\n')
                  *s++ = ' ';
            }
            break;
         case '"':
            // skip string literal
            s++;
            while(*s != '\0')
            {
               if(*s == '"' && *(s-1) != '\\')
                  break;
               s++;
            }
            break;
         case '*':
            if(last_char == '/')
            {
               // convert comment block to spaces
               *(s-1) = ' ';
               *s++   = ' ';
               while(*s != '\0')
               {
                  if(*s == '*' && *(s+1) == '/')
                  {
                     *s++ = ' ';
                     *s   = ' ';
                     break;
                  }
                  if(*s != '\n')
                     *s = ' ';
                  s++;
               }
            }
            break;
      }

      last_char = *s;
      s++;
   }
}




