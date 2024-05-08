#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "otto.h"

#define MSP_WIDTHMAX 255

#define cdeActive__     ((i64)'A'<<56 | (i64)'C'<<48 | (i64)'T'<<40 | (i64)'I'<<32 | 'V'<<24 | 'E'<<16 | ' '<<8 | ' ')
#define cdeAlias___     ((i64)'A'<<56 | (i64)'L'<<48 | (i64)'I'<<40 | (i64)'A'<<32 | 'S'<<24 | ' '<<16 | ' '<<8 | ' ')
#define cde_Extende     ((i64)'/'<<56 | (i64)'E'<<48 | (i64)'X'<<40 | (i64)'T'<<32 | 'E'<<24 | 'N'<<16 | 'D'<<8 | 'E')
#define cdeExtended     ((i64)'E'<<56 | (i64)'X'<<48 | (i64)'T'<<40 | (i64)'E'<<32 | 'N'<<24 | 'D'<<16 | 'E'<<8 | 'D')
#define cdeFieldID_     ((i64)'F'<<56 | (i64)'I'<<48 | (i64)'E'<<40 | (i64)'L'<<32 | 'D'<<24 | 'I'<<16 | 'D'<<8 | ' ')
#define cdeID______     ((i64)'I'<<56 | (i64)'D'<<48 | (i64)' '<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeName____     ((i64)'N'<<56 | (i64)'A'<<48 | (i64)'M'<<40 | (i64)'E'<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeOutlineL     ((i64)'O'<<56 | (i64)'U'<<48 | (i64)'T'<<40 | (i64)'L'<<32 | 'I'<<24 | 'N'<<16 | 'E'<<8 | 'L')
#define cde_Predece     ((i64)'/'<<56 | (i64)'P'<<48 | (i64)'R'<<40 | (i64)'E'<<32 | 'D'<<24 | 'E'<<16 | 'C'<<8 | 'E')
#define cdePredeces     ((i64)'P'<<56 | (i64)'R'<<48 | (i64)'E'<<40 | (i64)'D'<<32 | 'E'<<24 | 'C'<<16 | 'E'<<8 | 'S')
#define cdeSummary_     ((i64)'S'<<56 | (i64)'U'<<48 | (i64)'M'<<40 | (i64)'M'<<32 | 'A'<<24 | 'R'<<16 | 'Y'<<8 | ' ')
#define cde_Task___     ((i64)'/'<<56 | (i64)'T'<<48 | (i64)'A'<<40 | (i64)'S'<<32 | 'K'<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeTask____     ((i64)'T'<<56 | (i64)'A'<<48 | (i64)'S'<<40 | (i64)'K'<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cde_Tasks__     ((i64)'/'<<56 | (i64)'T'<<48 | (i64)'A'<<40 | (i64)'S'<<32 | 'K'<<24 | 'S'<<16 | ' '<<8 | ' ')
#define cdeTasks___     ((i64)'T'<<56 | (i64)'A'<<48 | (i64)'S'<<40 | (i64)'K'<<32 | 'S'<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeType____     ((i64)'T'<<56 | (i64)'Y'<<48 | (i64)'P'<<40 | (i64)'E'<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeUID_____     ((i64)'U'<<56 | (i64)'I'<<48 | (i64)'D'<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeValue___     ((i64)'V'<<56 | (i64)'A'<<48 | (i64)'L'<<40 | (i64)'U'<<32 | 'E'<<24 | ' '<<16 | ' '<<8 | ' ')

char    extenddef[EXTDEF_TOTAL][15]={"", "", "", "", "", "", "", ""};

#pragma pack(1)
typedef struct _MSP_TASK
{
   int     uid;
   int     id;
   char   *name;
   int     active;
   int     level;
   int     summary;
   char   *extend[EXTDEF_TOTAL];
   int     depend[512];
   char    dtype[512];
   int     ndep;
} MSP_TASK;

typedef struct _mspdi_tasklist
{
   int          nitems;
   MSP_TASK *item;
} MSP_TASKLIST;
#pragma pack()


int   parse_mspdi_attributes(DYNBUF *b);
char *get_extend_def(char *s);
int   parse_mspdi_tasks(DYNBUF *b, MSP_TASKLIST *tasklist);
char *add_extend_ptr(MSP_TASK *tmptask, char *s);
char *add_depend_uid(MSP_TASK *tmptask, char *s);
int   resolve_mspdi_dependencies();
int   validate_and_copy_mspdi();
int   cmp_mspdi_joblist(const void * a, const void * b);



int
parse_mspdi(DYNBUF *b, JOBLIST *joblist)
{
   int retval = OTTO_SUCCESS;
   MSP_TASKLIST tasklist;

   if(joblist == NULL)
      retval = OTTO_FAIL;

   if(retval == OTTO_SUCCESS)
      retval = parse_mspdi_attributes(b);

   if(retval == OTTO_SUCCESS)
      retval = parse_mspdi_tasks(b, &tasklist);

   if(retval == OTTO_SUCCESS)
      retval = resolve_mspdi_dependencies(&tasklist);

   if(retval == OTTO_SUCCESS)
   {
      joblist->nitems = tasklist.nitems;
      if((joblist->item = (JOB *)calloc(joblist->nitems, sizeof(JOB))) == NULL)
         retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS)
      retval = validate_and_copy_mspdi(&tasklist, joblist);

   return(retval);
}



int
parse_mspdi_attributes(DYNBUF *b)
{
   int retval = OTTO_SUCCESS;
   int  intasks = OTTO_FALSE;
   char code[9];
   char *s = b->buffer;

   while(*s != '\0' && intasks == OTTO_FALSE)
   {
      if(*s == '<')
      {
         // advance to first character of the tag
         s++;

         // convert first 8 characters of tag to a "code" and switch on that
         // when a desired tag is found mark it
         mkcode(code, s);
         switch(cde8int(code))
         {
            case cdeTasks___: intasks = OTTO_TRUE; break;
            case cdeExtended: s = get_extend_def(s); break;
            default: break;
         }
      }

      s++;
   }

   return(retval);
}



char *
get_extend_def(char *s)
{
   int   loop=OTTO_TRUE;
   char *alias=NULL, *fieldid=NULL;
   char code[9];

   // now associate file tags with task structure members
   while(loop == OTTO_TRUE && *s != '\0')
   {
      if(*s == '<')
      {
         // advance to first character of the tag
         s++;

         // convert first 8 characters of tag to a "code" and switch on that
         // when a desired tag is found mark it
         mkcode(code, s);
         switch(cde8int(code))
         {
            case cdeAlias___: s += 6; alias   = s; break;
            case cdeFieldID_: s += 8; fieldid = s; break;
            case cde_Extende: loop = OTTO_FALSE;   break;
            default: break;
         }
      }

      s++;
   }

   if(alias != NULL && fieldid != NULL)
   {
      if(strncasecmp(alias, "command",          7) == 0) xmlcpy(extenddef[COMMAND],         fieldid);
      if(strncasecmp(alias, "description",     11) == 0) xmlcpy(extenddef[DESCRIPTION],     fieldid);
      if(strncasecmp(alias, "environment",     11) == 0) xmlcpy(extenddef[ENVIRONMENT],     fieldid);
      if(strncasecmp(alias, "auto_hold",        8) == 0) xmlcpy(extenddef[AUTOHOLD],        fieldid);
      if(strncasecmp(alias, "date_conditions", 15) == 0) xmlcpy(extenddef[DATE_CONDITIONS], fieldid);
      if(strncasecmp(alias, "days_of_week",    12) == 0) xmlcpy(extenddef[DAYS_OF_WEEK],    fieldid);
      if(strncasecmp(alias, "start_mins",      10) == 0) xmlcpy(extenddef[START_MINUTES],   fieldid);
      if(strncasecmp(alias, "start_times",     11) == 0) xmlcpy(extenddef[START_TIMES],     fieldid);
      if(strncasecmp(alias, "loop",             4) == 0) xmlcpy(extenddef[LOOP],            fieldid);
   }

   return(s);
}



int
parse_mspdi_tasks(DYNBUF *b, MSP_TASKLIST *tasklist)
{
   int retval = OTTO_SUCCESS;
   int        id, intasks=OTTO_FALSE, max_id=-1;
   char       code[9];
   MSP_TASK   tmptask;
   char *     s = b->buffer, *tasksp = NULL;

   // find highest <ID> value inside <Tasks> container to allocate task array
   while(*s != '\0')
   {
      if(*s == '<')
      {
         // advance to first character of the tag
         s++;

         // convert first 8 characters of tag to a "code" and switch on that
         // when a desired tag is found mark it
         mkcode(code, s);
         switch(cde8int(code))
         {
            case cdeTasks___: intasks=OTTO_TRUE; tasksp = s-1; break;
            case cdeID______: if(intasks == OTTO_TRUE) { s+=3; id = xmltoi(s); if(max_id < id) max_id = id; } break;
            case cde_Tasks__: intasks=OTTO_FALSE; break;
            default: break;
         }
      }

      s++;
   }

   if(max_id == -1)
   {
      printf("No tasks found.\n");
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS)
   {
      // allocate task array
      tasklist->nitems = max_id + 1;
      if((tasklist->item = (MSP_TASK *)calloc(tasklist->nitems, sizeof(MSP_TASK))) == NULL)
      {
         printf("Can't allocate tasklist.\n");
         retval = OTTO_FAIL;
      }
   }

   if(retval == OTTO_SUCCESS)
   {
      // now associate MSP Task tags with MSP_TASK structure members
      s       = tasksp;
      intasks = OTTO_FALSE;
      while(*s != '\0')
      {
         if(*s == '<')
         {
            // advance to first character of the tag
            s++;

            // convert first 8 characters of tag to a "code" and switch on that
            // when a desired tag is found mark it
            mkcode(code, s);
            switch(intasks)
            {
               case OTTO_FALSE:
                  switch(cde8int(code))
                  {
                     case cdeTasks___: intasks = OTTO_TRUE; break;
                     default: break;
                  }
                  break;
               case OTTO_TRUE:
                  switch(cde8int(code))
                  {
                     case cde_Tasks__: intasks = OTTO_FALSE; break;
                     case cdeTask____: memset(&tmptask,          0,        sizeof(MSP_TASK)); break;
                     case cde_Task___: memcpy(&tasklist->item[tmptask.id], &tmptask, sizeof(MSP_TASK)); break;
                     case cdeUID_____: s+= 4;  tmptask.uid      = xmltoi(s);     break;
                     case cdeID______: s+= 3;  tmptask.id       = xmltoi(s);     break;
                     case cdeName____: s+= 5;  tmptask.name     = s;             break;
                     case cdeActive__: s+= 5;  tmptask.active   = xmltoi(s);     break;
                     case cdeOutlineL: s+= 13; tmptask.level    = xmltoi(s) -1;  break;
                     case cdeSummary_: s+= 8;  tmptask.summary  = xmltoi(s);     break;
                     case cdeExtended: s = add_extend_ptr(&tmptask, s);          break;
                     case cdePredeces: s = add_depend_uid(&tmptask, s);          break;
                     default: break;
                  }
                  break;
            }
         }

         s++;
      }
   }

   return(retval);
}



char *
add_extend_ptr(MSP_TASK *tmptask, char *s)
{
   int   loop=OTTO_TRUE;
   char *fieldid=NULL, *value=NULL;
   char code[9];

   // associate file tags with task structure members
   while(loop == OTTO_TRUE && *s != '\0')
   {
      if(*s == '<')
      {
         // advance to first character of the tag
         s++;

         // convert first 8 characters of tag to a "code" and switch on that
         // when a desired tag is found mark it
         mkcode(code, s);
         switch(cde8int(code))
         {
            case cdeFieldID_: s += 8; fieldid = s; break;
            case cdeValue___: s += 6; value   = s; break;
            case cde_Extende: loop = OTTO_FALSE; break;
            default: break;
         }
      }

      s++;
   }

   if(fieldid != NULL && value != NULL)
   {
      if(strncmp(fieldid, extenddef[COMMAND],         9) == 0) tmptask->extend[COMMAND]         = value;
      if(strncmp(fieldid, extenddef[DESCRIPTION],     9) == 0) tmptask->extend[DESCRIPTION]     = value;
      if(strncmp(fieldid, extenddef[ENVIRONMENT],     9) == 0) tmptask->extend[ENVIRONMENT]     = value;
      if(strncmp(fieldid, extenddef[AUTOHOLD],        9) == 0) tmptask->extend[AUTOHOLD]        = value;
      if(strncmp(fieldid, extenddef[DATE_CONDITIONS], 9) == 0) tmptask->extend[DATE_CONDITIONS] = value;
      if(strncmp(fieldid, extenddef[DAYS_OF_WEEK],    9) == 0) tmptask->extend[DAYS_OF_WEEK]    = value;
      if(strncmp(fieldid, extenddef[START_MINUTES],   9) == 0) tmptask->extend[START_MINUTES]   = value;
      if(strncmp(fieldid, extenddef[START_TIMES],     9) == 0) tmptask->extend[START_TIMES]     = value;
      if(strncmp(fieldid, extenddef[LOOP],            9) == 0) tmptask->extend[LOOP]            = value;
   }

   return(s);
}



char *
add_depend_uid(MSP_TASK *tmptask, char *s)
{
   int   loop=OTTO_TRUE;
   char *uid=NULL, *type=NULL;
   char code[9];

   // parse the XML - quick and dirty because we know what we're looking for

   // now associate file tags with task structure members
   while(loop == OTTO_TRUE && *s != '\0')
   {
      if(*s == '<')
      {
         // advance to first character of the tag
         s++;

         // convert first 8 characters of tag to a "code" and switch on that
         // when a desired tag is found mark it
         mkcode(code, s);
         switch(cde8int(code))
         {
            case cdePredeces: s+= 15; uid  = s;  break;
            case cdeType____: s+=  5; type = s;  break;
            case cde_Predece: if(strncasecmp(s, "/PredecessorLink", 16) == 0) loop = OTTO_FALSE; break;
            default: break;
         }
      }

      s++;
   }

   if(uid != NULL)
   {
      tmptask->depend[tmptask->ndep] = xmltoi(uid);
      tmptask->dtype[tmptask->ndep]  = xmltoi(type);
      tmptask->ndep++;
   }

   return(s);
}



int
resolve_mspdi_dependencies(MSP_TASKLIST *tasklist)
{
   int retval = OTTO_SUCCESS;
   int i, j, max_uid=-1;
   int *uid;

   // find max_uid
   for(i=0; i<tasklist->nitems; i++)
   {
      if(max_uid < tasklist->item[i].uid)
         max_uid = tasklist->item[i].uid;
   }

   // allocate uid array
   if((uid = (int *)calloc(max_uid+1, sizeof(int))) == NULL)
   {
      printf("uid couldn't be allocated\n");
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS)
   {
      // fill uid array
      for(i=0; i<tasklist->nitems; i++)
      {
         uid[tasklist->item[i].uid] = tasklist->item[i].id;
      }

      // replace any dependency uid values with corresponding id values
      for(i=0; i<tasklist->nitems; i++)
      {
         for(j=0; j<tasklist->item[i].ndep; j++)
         {
            tasklist->item[i].depend[j] = uid[tasklist->item[i].depend[j]];
         }
      }

      free(uid);
   }

   return(retval);
}



int
validate_and_copy_mspdi(MSP_TASKLIST *tasklist, JOBLIST *joblist)
{
   int  retval = OTTO_SUCCESS;
   int  errcount=0, i, j = 0;
   int  date_check = 0, parse_date_conditions = OTTO_FALSE;
   char namstr[(MSP_WIDTHMAX+1)*2];  // more than enough space
   char tmpstr[(MSP_WIDTHMAX+1)*3];  // more than enough space
   char tmpcond[(CNDLEN+1)*3];       // more than enough space?
   char *tmpstrp;
   char *kind;
   char *levelbox[9];
   int  boxlevel = 0;
   int  rc = OTTO_SUCCESS;


   // HIGH LEVEL file format checks
   {
      if(extenddef[COMMAND][0] == '\0')
      {
         printf("The 'Command' column was not found.\n");
         errcount++;
      }

      if(extenddef[DESCRIPTION][0] == '\0')
      {
         printf("The 'Description' column was not found.\n");
         errcount++;
      }

      if(extenddef[AUTOHOLD][0] == '\0')
      {
         printf("The 'auto_hold' column was not found.\n");
         errcount++;
      }
   }

   if(errcount == 0)
   {
      // first pass through tasks checks task attributes that
      // don't relate to other tasks
      for(i=1; i<tasklist->nitems; i++)
      {
         joblist->item[i].id = -1;
         joblist->item[i].opcode = NOOP;
         if(tasklist->item[i].name == NULL)
            continue;

         // move each XML attribute into its corresponding job field
         // performing sanity checks along the way

         // all tasks in a MSP XML file resolve to CREATE_JOB actions
         joblist->item[i].opcode = CREATE_JOB;

         // set the id
         joblist->item[i].id = i;

         // transfer the MSP XML level attribute directly to the joblist item
         joblist->item[i].level = tasklist->item[i].level;

         // some of these steps aren't actually required because of the way
         // the project files are used but they're here for consistency's sake
         kind = "job";
         if(tasklist->item[i].summary == OTTO_TRUE)
         {
            kind = "box";
         }

         // NAME copy and verification
         {
            xmlcpy(namstr, tasklist->item[i].name);
            if((rc = ottojob_copy_name(joblist->item[i].name, namstr, NAMLEN)) != OTTO_SUCCESS)
            {
               errcount += ottojob_print_name_errors(rc, "insert_job", namstr, kind);

               retval = OTTO_FAIL;
            }
         }


         // JOB TYPE copy
         {
            if(tasklist->item[i].summary == OTTO_TRUE)
            {
               tmpstrp = "b";
               levelbox[tasklist->item[i].level] = joblist->item[i].name;
            }
            else
            {
               tmpstrp = "c";
            }
            if((rc = ottojob_copy_type(&joblist->item[i].type, tmpstrp, TYPLEN)) != OTTO_SUCCESS)
            {
               errcount += ottojob_print_name_errors(rc, "insert_job", namstr, tmpstrp);

               retval = OTTO_FAIL;
            }
         }


         // BOX NAME copy
         {
            if((tasklist->item[i].level - boxlevel) > 0)
            {
               rc = ottojob_copy_name(joblist->item[i].box_name, levelbox[tasklist->item[i].level-1], NAMLEN);
               if(strcmp(joblist->item[i].box_name, namstr) == 0)
               {
                  rc |= OTTO_SAME_JOB_BOX_NAMES;
               }
               if(rc != OTTO_SUCCESS)
               {
                  errcount += ottojob_print_box_name_errors(rc, "insert_job", namstr, levelbox[tasklist->item[i].level-1]);

                  retval = OTTO_FAIL;
               }
            }
         }


         // COMMAND copy and verification
         {
            if(tasklist->item[i].summary == OTTO_FALSE)
            {
               if(tasklist->item[i].extend[COMMAND] == NULL)
               {
                  rc = OTTO_MISSING_REQUIRED_VALUE;
               }
               else
               {
                  tmpstr[0] = '\0';
                  if(tasklist->item[i].extend[COMMAND] != NULL)
                  {
                     xmlcpy(tmpstr, tasklist->item[i].extend[COMMAND]);
                  }
                  rc = ottojob_copy_command(joblist->item[i].command, tmpstr, CMDLEN);
               }
               if(rc != OTTO_SUCCESS)
               {
                  errcount += ottojob_print_command_errors(rc, "insert_job", namstr, CMDLEN);

                  retval = OTTO_FAIL;
               }
            }
         }


         // DESCRIPTION copy and verification
         {
            if(i > 0)
            {
               if(tasklist->item[i].extend[DESCRIPTION] != NULL)
               {
                  xmlcpy(tmpstr, tasklist->item[i].extend[DESCRIPTION]);
                  if((rc = ottojob_copy_description(joblist->item[i].description, tmpstr, DSCLEN)) != OTTO_SUCCESS)
                  {
                     errcount += ottojob_print_description_errors(rc, "insert_job", namstr, DSCLEN);

                     retval = OTTO_FAIL;
                  }
               }
            }
         }


         // ENVIRONMENT copy and verification
         {
            if(i > 0)
            {
               if(tasklist->item[i].extend[ENVIRONMENT] != NULL)
               {
                  xmlcpy(tmpstr, tasklist->item[i].extend[ENVIRONMENT]);
                  if((rc = ottojob_copy_environment(joblist->item[i].environment, tmpstr, ENVLEN)) != OTTO_SUCCESS)
                  {
                     errcount += ottojob_print_environment_errors(rc, "insert_job", namstr, ENVLEN);

                     retval = OTTO_FAIL;
                  }
               }
            }
         }


         // AUTO HOLD copy
         {
            if(tasklist->item[i].extend[AUTOHOLD] != NULL && tasklist->item[i].extend[AUTOHOLD][0] == '1')
            {
               xmlcpy(tmpstr, tasklist->item[i].extend[AUTOHOLD]);
               if((rc = ottojob_copy_flag(&joblist->item[i].autohold, tmpstr, FLGLEN)) != OTTO_SUCCESS)
               {
                  errcount += ottojob_print_auto_hold_errors(rc, "insert_job", namstr, tmpstr);

                  retval = OTTO_FAIL;
               }
               joblist->item[i].on_autohold = joblist->item[i].autohold;
            }
         }


         // DATE CONDITIONS copy and verification
         {
            // check if a valid combination of date conditions attributes was specified
            rc = OTTO_SUCCESS;
            date_check = 0;
            if(tasklist->item[i].extend[DATE_CONDITIONS] != NULL) date_check |= DTECOND_CHECK;
            if(tasklist->item[i].extend[DAYS_OF_WEEK]    != NULL) date_check |= DYSOFWK_CHECK;
            if(tasklist->item[i].extend[START_MINUTES]   != NULL) date_check |= STRTMNS_CHECK;
            if(tasklist->item[i].extend[START_TIMES]     != NULL) date_check |= STRTTMS_CHECK;

            parse_date_conditions = OTTO_FALSE;
            switch(date_check)
            {
               case 0:
                  // do nothing
                  break;
               case (DTECOND_CHECK | DYSOFWK_CHECK | STRTMNS_CHECK):
               case (DTECOND_CHECK | DYSOFWK_CHECK | STRTTMS_CHECK):
                  if(tasklist->item[i].level > 0)
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
               errcount += ottojob_print_date_conditions_errors(rc, "insert_job", namstr, "1");

               retval = OTTO_FAIL;
            }

            if(parse_date_conditions == OTTO_TRUE)
            {
               xmlcpy(tmpstr, tasklist->item[i].extend[DAYS_OF_WEEK]);
               if((rc = ottojob_copy_days_of_week(&joblist->item[i].days_of_week, tmpstr)) != OTTO_SUCCESS)
               {
                  errcount += ottojob_print_days_of_week_errors(rc, "insert_job", namstr);

                  retval = OTTO_FAIL;
               }

               xmlcpy(tmpstr, tasklist->item[i].extend[START_MINUTES]);
               if((rc = ottojob_copy_start_minutes(&joblist->item[i].start_minutes, tmpstr)) != OTTO_SUCCESS)
               {
                  errcount += ottojob_print_start_mins_errors(rc, "insert_job", namstr);

                  retval = OTTO_FAIL;
               }

               xmlcpy(tmpstr, tasklist->item[i].extend[START_TIMES]);
               if((rc = ottojob_copy_start_times(joblist->item[i].start_times, tmpstr)) != OTTO_SUCCESS)
               {
                  errcount += ottojob_print_start_times_errors(rc, "insert_job", namstr);

                  retval = OTTO_FAIL;
               }

               // assume the task is using start times
               joblist->item[i].date_conditions = OTTO_USE_START_TIMES;

               // modify if it's using start_mins
               if(date_check & STRTMNS_CHECK)
               {
                  joblist->item[i].date_conditions = OTTO_USE_START_MINUTES;
                  for(j=0; j<24; j++)
                     joblist->item[i].start_times[j] = joblist->item[i].start_minutes;
               }
            }
         }


         // LOOP copy
         {
            if(tasklist->item[i].extend[LOOP] != NULL)
            {
               xmlcpy(tmpstr, tasklist->item[i].extend[LOOP]);
               if((rc = ottojob_copy_loop(joblist->item[i].loopname,
                                          &joblist->item[i].loopmin,
                                          &joblist->item[i].loopmax,
                                          &joblist->item[i].loopsgn,
                                          tmpstr)) != OTTO_SUCCESS)
               {
                  errcount += ottojob_print_loop_errors(rc, "insert_job", namstr);

                  retval = OTTO_FAIL;
               }
            }
         }
      }


      // second pass through tasks checks task dependencies
      // on other tasks
      for(i=1; i<tasklist->nitems; i++)
      {
         xmlcpy(namstr, tasklist->item[i].name);

         // NAME DUPLICATION checks
         {
            // linear comparison of task names - not the most efficient but not complicated either
            for(j=i+1; j<tasklist->nitems; j++)
            {
               rc = OTTO_SUCCESS;
               if(strcmp(namstr, joblist->item[j].name) == 0)
               {
                  rc = OTTO_SAME_NAME;
                  if(rc != OTTO_SUCCESS)
                  {
                     errcount += ottojob_print_task_name_errors(rc, "insert_job", namstr, i, j);
                  }
               }
            }
         }


         // CONDITION construction
         {
            tmpcond[0] = '\0';
            for(j=0; j<tasklist->item[i].ndep; j++)
            {
               if(joblist->item[tasklist->item[i].depend[j]].name[0] != '\0')
               {
                  if(j>0)
                     strcat(tmpcond,  " & ");
                  if(tasklist->item[i].dtype[j] == 3)
                     strcat(tmpcond,  "r(");
                  else
                     strcat(tmpcond,  "s(");
                  strcat(tmpcond,  joblist->item[tasklist->item[i].depend[j]].name);
                  strcat(tmpcond,  ")");
               }
            }
            if(tmpcond[0] != '\0')
            {
               if((rc = ottojob_copy_condition(joblist->item[i].condition, tmpcond, CNDLEN)) != OTTO_SUCCESS)
               {
                  errcount += ottojob_print_condition_errors(rc, "insert_job", namstr, CMDLEN);

                  retval = OTTO_FAIL;
               }

            }
         }
      }

      // sort the new joblist to remove empty jobs
      qsort(joblist->item, joblist->nitems, sizeof(JOB), cmp_mspdi_joblist);

      // reset the nuber of items in the list - discarding marked jobs at the end
      // conveniently this is just the id accumulator
      for(i=0; i<tasklist->nitems; i++)
      {
         if(joblist->item[i].id == -1)
            break;

         joblist->nitems = i + 1;
      }

   }


   if(errcount > 0)
   {
      fprintf(stderr,"%d error%s occurred.\n", errcount, errcount == 1 ? "" : "s");
      retval = OTTO_FAIL;
   }

   return(retval);
}



int
cmp_mspdi_joblist(const void * a, const void * b)
{
   JOB *p1 = (JOB *)a;
   JOB *p2 = (JOB *)b;

   if(p1->id == -1) { return( 1); }
   if(p2->id == -1) { return(-1); }

   return(p1->id - p2->id);
}




