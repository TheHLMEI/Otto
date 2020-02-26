#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ottobits.h"
#include "ottojob.h"
#include "otto_mpp_bits.h"
#include "otto_mpp_reader.h"


#define MPP_WIDTHMAX 255

#define DTECOND_BIT      (1L)
#define DYSOFWK_BIT (1L << 1)
#define STRTMNS_BIT (1L << 2)
#define STRTTMS_BIT (1L << 3)

#define cdeAMP_____     ((i64)'A'<<56 | (i64)'M'<<48 | (i64)'P'<<40 | (i64)';'<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeAPOS____     ((i64)'A'<<56 | (i64)'P'<<48 | (i64)'O'<<40 | (i64)'S'<<32 | ';'<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeGT______     ((i64)'G'<<56 | (i64)'T'<<48 | (i64)';'<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeLT______     ((i64)'L'<<56 | (i64)'T'<<48 | (i64)';'<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeQUOT____     ((i64)'Q'<<56 | (i64)'U'<<48 | (i64)'O'<<40 | (i64)'T'<<32 | ';'<<24 | ' '<<16 | ' '<<8 | ' ')

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
typedef struct _MPP_TASK
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
} MPP_TASK;

typedef struct _mpp_tasklist
{
	int          nitems;
	MPP_TASK *item;
} MPP_TASKLIST;
#pragma pack()


int   parse_mpp_xml_attributes(BUF_st *b);
char *get_extend_def(char *s);
int   parse_mpp_xml_tasks(BUF_st *b, MPP_TASKLIST *tasklist);
char *add_extend_ptr(MPP_TASK *tmptask, char *s);
char *add_depend_uid(MPP_TASK *tmptask, char *s);
int   resolve_mpp_xml_dependencies();
int   validate_and_copy_mpp_xml();



int
parse_mpp_xml(BUF_st *b, JOBLIST *joblist)
{
	int retval = OTTO_SUCCESS;
	MPP_TASKLIST tasklist;;

	if(joblist == NULL)
		retval = OTTO_FAIL;

	if(retval == OTTO_SUCCESS)
		retval = parse_mpp_xml_attributes(b);

	if(retval == OTTO_SUCCESS)
		retval = parse_mpp_xml_tasks(b, &tasklist);

	if(retval == OTTO_SUCCESS)
		retval = resolve_mpp_xml_dependencies(&tasklist);

	if(retval == OTTO_SUCCESS)
	{
		joblist->nitems = tasklist.nitems;
		if((joblist->item = (JOB *)calloc(joblist->nitems, sizeof(JOB))) == NULL)
			retval = OTTO_FAIL;
	}

	if(retval == OTTO_SUCCESS)
		retval = validate_and_copy_mpp_xml(&tasklist, joblist);

	return(retval);
}



int
parse_mpp_xml_attributes(BUF_st *b)
{
	int retval = OTTO_SUCCESS;
	int  intasks = OTTO_FALSE;
	char code[9];
	char *s = b->buf;

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
		if(strncasecmp(alias, "job script",      10) == 0) xmlcpy(extenddef[JOBSCRIPT],       fieldid);
		if(strncasecmp(alias, "jobscript",        9) == 0) xmlcpy(extenddef[JOBSCRIPT],       fieldid);
		if(strncasecmp(alias, "parameters",      10) == 0) xmlcpy(extenddef[PARAMETERS],      fieldid);
		if(strncasecmp(alias, "description",     11) == 0) xmlcpy(extenddef[DESCRIPTION],     fieldid);
		if(strncasecmp(alias, "auto_hold",        8) == 0) xmlcpy(extenddef[AUTOHOLD],        fieldid);
		if(strncasecmp(alias, "date_conditions", 15) == 0) xmlcpy(extenddef[DATE_CONDITIONS], fieldid);
		if(strncasecmp(alias, "days_of_week",    12) == 0) xmlcpy(extenddef[DAYS_OF_WEEK],    fieldid);
		if(strncasecmp(alias, "start_mins",      10) == 0) xmlcpy(extenddef[START_MINS],      fieldid);
		if(strncasecmp(alias, "start_times",     11) == 0) xmlcpy(extenddef[START_TIMES],     fieldid);
	}

	return(s);
}



int
parse_mpp_xml_tasks(BUF_st *b, MPP_TASKLIST *tasklist)
{
	int retval = OTTO_SUCCESS;
	int        id, intasks=OTTO_FALSE, max_id=-1;
	char       code[9];
	MPP_TASK   tmptask;
	char *     s = b->buf, *tasksp = NULL;

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
		tasklist->nitems = max_id;
		if((tasklist->item = (MPP_TASK *)calloc(tasklist->nitems+1, sizeof(MPP_TASK))) == NULL)
		{
			printf("Can't allocate tasklist.\n");
			retval = OTTO_FAIL;
		}
	}

	if(retval == OTTO_SUCCESS)
	{
		// now associate MPP Task tags with MPP_TASK structure members
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
							case cdeTask____: memset(&tmptask,          0,        sizeof(MPP_TASK)); break;
							case cde_Task___: memcpy(&tasklist->item[tmptask.id], &tmptask, sizeof(MPP_TASK)); break;
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
add_extend_ptr(MPP_TASK *tmptask, char *s)
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
		if(strncmp(fieldid, extenddef[JOBSCRIPT],       9) == 0) tmptask->extend[JOBSCRIPT]       = value;
		if(strncmp(fieldid, extenddef[PARAMETERS],      9) == 0) tmptask->extend[PARAMETERS]      = value;
		if(strncmp(fieldid, extenddef[DESCRIPTION],     9) == 0) tmptask->extend[DESCRIPTION]     = value;
		if(strncmp(fieldid, extenddef[AUTOHOLD],        9) == 0) tmptask->extend[AUTOHOLD]        = value;
		if(strncmp(fieldid, extenddef[DATE_CONDITIONS], 9) == 0) tmptask->extend[DATE_CONDITIONS] = value;
		if(strncmp(fieldid, extenddef[DAYS_OF_WEEK],    9) == 0) tmptask->extend[DAYS_OF_WEEK]    = value;
		if(strncmp(fieldid, extenddef[START_MINS],      9) == 0) tmptask->extend[START_MINS]      = value;
		if(strncmp(fieldid, extenddef[START_TIMES],     9) == 0) tmptask->extend[START_TIMES]     = value;
	}

	return(s);
}



char *
add_depend_uid(MPP_TASK *tmptask, char *s)
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
resolve_mpp_xml_dependencies(MPP_TASKLIST *tasklist)
{
	int retval = OTTO_SUCCESS;
	int i, j, max_uid=-1;
	int *uid;

	// find max_uid
	for(i=0; i<=tasklist->nitems; i++)
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
		for(i=0; i<=tasklist->nitems; i++)
		{
			uid[tasklist->item[i].uid] = tasklist->item[i].id;
		}

		// replace any dependency uid values with corresponding id values
		for(i=0; i<=tasklist->nitems; i++)
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
validate_and_copy_mpp_xml(MPP_TASKLIST *tasklist, JOBLIST *joblist)
{
	int  retval = OTTO_SUCCESS;
	int  errcount=0, i, j, wrncount=0;
   int  date_check = 0, parse_date_conditions = OTTO_FALSE;
	char namstr[(MPP_WIDTHMAX+1)*2];  // more than enough space
	char tmpstr[(MPP_WIDTHMAX+1)*2];  // more than enough space
	char *levelbox[9];
	int  boxlevel = 0;
	int  rc;


	// HIGH LEVEL file format checks
	{
		if(extenddef[JOBSCRIPT][0] == '\0')
		{
			printf("The 'Job Script' column was not found.\n");
			errcount++;
		}

		if(extenddef[PARAMETERS][0] == '\0')
		{
			printf("The 'Parameters' column was not found.\n");
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
		for(i=1; i<=tasklist->nitems; i++)
		{
			// move each XML attribute into its corresponding job field
			// performing sanity checks along the way

			// NAME copy and verification
			{
				xmlcpy(namstr, tasklist->item[i].name);
				if((rc = ottojob_copy_name(joblist->item[i].name, namstr, NAMLEN)) != OTTO_SUCCESS) retval = OTTO_FAIL;
				if(rc & OTTO_INVALID_NAME_CHARS)
				{
					printf("ERROR for task: %s < task name contains one or more invalid characters or symbols >.\n", namstr);
					errcount++;
				}

				if(rc & OTTO_INVALID_NAME_LENGTH)
				{
					printf("ERROR for task: %s < task name exceeds allowed length (%d bytes) >.\n", namstr, NAMLEN);
					errcount++;
				}
			}


			// JOB TYPE copy
			{
				if(tasklist->item[i].summary == OTTO_TRUE)
				{
					joblist->item[i].type = OTTO_BOX;
					levelbox[tasklist->item[i].level] = joblist->item[i].name;
				}
				else
				{
					joblist->item[i].type = OTTO_CMD;
				}
			}


			// BOX NAME copy
			{
				if((tasklist->item[i].level - boxlevel) > 0)
				{
					strcpy(joblist->item[i].box_name, levelbox[tasklist->item[i].level-1]);
				}
			}


			// COMMAND copy and verification
			{
				if(tasklist->item[i].summary == OTTO_FALSE)
				{
					if(tasklist->item[i].extend[JOBSCRIPT] == NULL)
					{
						printf("ERROR for task: %s < task looks like a command but doesn't specify a job script.\n", namstr);
						errcount++;
					}
					else
					{
						strcpy(joblist->item[i].command, "cnv_autojob.sh ");
						xmlcpy(tmpstr, tasklist->item[i].extend[JOBSCRIPT]);
						strcat(joblist->item[i].command, tmpstr);
						if(tasklist->item[i].extend[PARAMETERS] != NULL)
						{
							strcat(joblist->item[i].command, " ");
							xmlcpy(tmpstr, tasklist->item[i].extend[PARAMETERS]);
							strcat(joblist->item[i].command, tmpstr);
						}
					}
				}
			}


			// DESCRIPTION copy and verification
			{
				if(i > 0)
				{
					if(tasklist->item[i].extend[DESCRIPTION] == NULL)
					{
						printf("WARNING for task: %s < task doesn't have a description.\n", namstr);
						wrncount++;
					}
					else
					{
						xmlcpy(joblist->item[i].description, tasklist->item[i].extend[DESCRIPTION]);
						if(xmlchr(joblist->item[i].description, '"') == OTTO_TRUE)
						{
							printf("ERROR for task: %s < task has quotes in its description.\n", namstr);
							errcount++;
						}
					}
				}
			}


			// AUTO HOLD copy
			{
				if(tasklist->item[i].extend[AUTOHOLD] != NULL && tasklist->item[i].extend[AUTOHOLD][0] == '1')
				{
					joblist->item[i].auto_hold = OTTO_TRUE;
				}
			}


			// DATE CONDITIONS copy and verification
			{
				// check if a valid combination of date conditions attributes was specified
				date_check = 0;
				if(tasklist->item[i].extend[DATE_CONDITIONS] != NULL) date_check |= DTECOND_BIT;
				if(tasklist->item[i].extend[DAYS_OF_WEEK]    != NULL) date_check |= DYSOFWK_BIT;
				if(tasklist->item[i].extend[START_MINS]      != NULL) date_check |= STRTMNS_BIT;
				if(tasklist->item[i].extend[START_TIMES]     != NULL) date_check |= STRTTMS_BIT;

				parse_date_conditions = OTTO_FALSE;
				switch(date_check)
				{
					case 0:
						// do nothing
						break;
					case (DTECOND_BIT | DYSOFWK_BIT | STRTMNS_BIT):
					case (DTECOND_BIT | DYSOFWK_BIT | STRTTMS_BIT):
						if(tasklist->item[i].level > 0)
						{
							printf("ERROR for task: %s < task has an invalid application of date conditions.\n", namstr);
							printf("   Otto only allows date conditions on objects at level 0. (i.e. no parent box).\n");
							errcount++;
						}
						else
						{
							parse_date_conditions = OTTO_TRUE;
						}
						break;
					default:
						printf("ERROR for task: %s < task has an invalid combination of date conditions.\n", namstr);
						printf("   You must specify date_conditions, days_of_week, and either start_mins or start_times.\n");
						errcount++;
						break;
				}

				if(parse_date_conditions == OTTO_TRUE)
				{
					if((rc = ottojob_copy_days_of_week(&joblist->item[i].days_of_week, tasklist->item[i].extend[DAYS_OF_WEEK])) != OTTO_SUCCESS)
						retval = OTTO_FAIL;
					if(rc == OTTO_INVALID_VALUE)
					{
						printf("ERROR for task: %s < days_of_week attribute contains errors >.\n", namstr);
						errcount++;
					}

					if((rc = ottojob_copy_start_mins(&joblist->item[i].start_mins, tasklist->item[i].extend[START_MINS])) != OTTO_SUCCESS)
						retval = OTTO_FAIL;
					if(rc == OTTO_INVALID_VALUE)
					{
						printf("ERROR for task: %s < start_mins attribute contains errors >.\n", namstr);
						errcount++;
					}

					if((rc = ottojob_copy_start_times(joblist->item[i].start_times, tasklist->item[i].extend[START_TIMES])) != OTTO_SUCCESS)
						retval = OTTO_FAIL;
					if(rc == OTTO_INVALID_VALUE)
					{
						printf("ERROR for task: %s < start_times attribute contains errors >.\n", namstr);
						errcount++;
					}

					// assume the task is using start times
					joblist->item[i].date_conditions = OTTO_USE_STARTTIMES;

					// modify if it's using start_mins
					if(date_check & STRTMNS_BIT)
					{
						joblist->item[i].date_conditions = OTTO_USE_STARTMINS;
						for(j=0; j<24; j++)
							joblist->item[i].start_times[j] = joblist->item[i].start_mins;
					}
				}
			}
		}


		// second pass through tasks checks task dependencies
		// on other tasks
		for(i=1; i<=tasklist->nitems; i++)
		{
			xmlcpy(namstr, tasklist->item[i].name);

			// NAME DUPLICATION checks
			{
				// linear comparison of task names - not the most efficient but not complicated either
				for(j=i+1; j<=tasklist->nitems; j++)
				{
					if(strcmp(namstr, joblist->item[j].name) == 0)
					{
						printf("ERROR for task: %s < name is duplicated between tasks %d and %d >.\n", namstr, i, j);
						errcount++;
					}
				}
			}


			// CONDITION construction
			{
				for(j=0; j<tasklist->item[i].ndep; j++)
				{
					if(joblist->item[tasklist->item[i].depend[j]].name != NULL)
					{
						if(j>0)
							strcat(joblist->item[i].condition, " & ");
						if(tasklist->item[i].dtype[j] == 3)
							strcat(joblist->item[i].condition, "r(");
						else
							strcat(joblist->item[i].condition, "s(");
						strcat(joblist->item[i].condition, joblist->item[tasklist->item[i].depend[j]].name);
						strcat(joblist->item[i].condition, ")");
					}
				}
			}
		}
	}


	if(wrncount > 0)
	{
		fprintf(stderr,"%d warning%s occurred.\n", wrncount, wrncount == 1 ? "" : "s");
	}

	if(errcount > 0)
	{
		fprintf(stderr,"%d error%s occurred.\n", errcount, errcount == 1 ? "" : "s");
		retval = OTTO_FAIL;
	}

	return(retval);
}



//
// vim: ts=3 sw=3 ai
//
