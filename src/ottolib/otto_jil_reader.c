#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ottobits.h"
#include "ottoipc.h"

#include "otto_jil_reader.h"

enum JIL_KEYWORDS
{
	JIL_UNKNOWN,
	JIL_AUTOHLD,
	JIL_BOXNAME,
	JIL_COMMAND,
	JIL_CONDITN,
	JIL_DESCRIP,
	JIL_DEL_BOX,
	JIL_DEL_JOB,
	JIL_DTECOND,
	JIL_DYSOFWK,
	JIL_INS_JOB,
	JIL_JOBTYPE,
	JIL_STRTMIN,
	JIL_STRTTIM,
	JIL_UPD_JOB,
	JIL_UNSUPPD
};



int  jil_reserved_word(char *s);
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
int  validate_and_copy_jil_auto_hold(JOB *item, char *auto_holdp);
int  validate_and_copy_jil_date_conditions(JOB *item, char *date_conditionsp, char *days_of_weekp, char *start_minsp, char *start_timesp);



int
parse_jil(DYNBUF *b, JOBLIST *joblist)
{
	int retval = OTTO_SUCCESS;
	int i = 0;

	if(joblist == NULL)
		retval = OTTO_FAIL;

	// count items
	if(retval == OTTO_SUCCESS)
	{
		memset(joblist, 0, sizeof(JOBLIST));

		b->line = 0;
		b->s    = b->buffer;
		while(b->s[0] != '\0')
		{
			switch(jil_reserved_word(b->s))
			{
				case JIL_INS_JOB:
				case JIL_UPD_JOB:
				case JIL_DEL_BOX:
				case JIL_DEL_JOB:
					joblist->nitems++;
					break;
				default:
					break;
			}
			advance_word(b);
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
	char    *date_conditionsp     = NULL;
	char    *days_of_weekp        = NULL;
	char    *start_minsp          = NULL;
	char    *start_timesp         = NULL;

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
			case JIL_AUTOHLD: advance_jilword(b); auto_holdp       = b->s; break;
			case JIL_DTECOND: advance_jilword(b); date_conditionsp = b->s; break;
			case JIL_DYSOFWK: advance_jilword(b); days_of_weekp    = b->s; break;
			case JIL_STRTMIN: advance_jilword(b); start_minsp      = b->s; break;
			case JIL_STRTTIM: advance_jilword(b); start_timesp     = b->s; break;
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
	if(validate_and_copy_jil_auto_hold(item, auto_holdp) != OTTO_SUCCESS)
		retval = OTTO_FAIL;
	if(validate_and_copy_jil_date_conditions(item, date_conditionsp, days_of_weekp, start_minsp, start_timesp) != OTTO_SUCCESS)
		retval = OTTO_FAIL;

	// check valid combinations of attributes
	// TODO

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
	char    *date_conditionsp     = NULL;
	char    *days_of_weekp        = NULL;
	char    *start_minsp          = NULL;
	char    *start_timesp         = NULL;

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
			case JIL_AUTOHLD: advance_jilword(b); auto_holdp       = b->s; break;
			case JIL_DTECOND: advance_jilword(b); date_conditionsp = b->s; break;
			case JIL_DYSOFWK: advance_jilword(b); days_of_weekp    = b->s; break;
			case JIL_STRTMIN: advance_jilword(b); start_minsp      = b->s; break;
			case JIL_STRTTIM: advance_jilword(b); start_timesp     = b->s; break;
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
	if(validate_and_copy_jil_auto_hold(item, auto_holdp) != OTTO_SUCCESS)
		retval = OTTO_FAIL;
	if(validate_and_copy_jil_date_conditions(item, date_conditionsp, days_of_weekp, start_minsp, start_timesp) != OTTO_SUCCESS)
		retval = OTTO_FAIL;

	// check valid combinations of attributes
	// TODO

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
			if(strncmp(s, "auto_hold",        9) == 0) { retval = JIL_AUTOHLD; c =  9; };
			break;
		case 'b':
			if(strncmp(s, "box_name",         8) == 0) { retval = JIL_BOXNAME; c =  8; };
			break;
		case 'c':
			if(strncmp(s, "command",          7) == 0) { retval = JIL_COMMAND; c =  7; };
			if(strncmp(s, "condition",        9) == 0) { retval = JIL_CONDITN; c =  9; };
			break;
		case 'd':
			if(strncmp(s, "date_conditions", 15) == 0) { retval = JIL_DTECOND; c = 15; };
			if(strncmp(s, "days_of_week",    12) == 0) { retval = JIL_DYSOFWK; c = 12; };
			if(strncmp(s, "description",     11) == 0) { retval = JIL_DESCRIP; c = 11; };
			if(strncmp(s, "delete_box",      10) == 0) { retval = JIL_DEL_BOX; c = 10; };
			if(strncmp(s, "delete_job",      10) == 0) { retval = JIL_DEL_JOB; c = 10; };
			if(strncmp(s, "delete_blob",     11) == 0) { retval = JIL_UNSUPPD; c = 11; };
			if(strncmp(s, "delete_glob",     11) == 0) { retval = JIL_UNSUPPD; c = 11; };
			if(strncmp(s, "delete_job_type", 15) == 0) { retval = JIL_UNSUPPD; c = 15; };
			if(strncmp(s, "delete_machine",  14) == 0) { retval = JIL_UNSUPPD; c = 14; };
			if(strncmp(s, "delete_monbro",   13) == 0) { retval = JIL_UNSUPPD; c = 13; };
			if(strncmp(s, "delete_resource", 15) == 0) { retval = JIL_UNSUPPD; c = 15; };
			if(strncmp(s, "delete_xinst",    12) == 0) { retval = JIL_UNSUPPD; c = 12; };
			break;
		case 'i':
			if(strncmp(s, "insert_job",      10) == 0) { retval = JIL_INS_JOB; c = 10; };
			if(strncmp(s, "insert_blob",     11) == 0) { retval = JIL_UNSUPPD; c = 11; };
			if(strncmp(s, "insert_glob",     11) == 0) { retval = JIL_UNSUPPD; c = 11; };
			if(strncmp(s, "insert_job_type", 15) == 0) { retval = JIL_UNSUPPD; c = 15; };
			if(strncmp(s, "insert_machine",  14) == 0) { retval = JIL_UNSUPPD; c = 14; };
			if(strncmp(s, "insert_monbro",   13) == 0) { retval = JIL_UNSUPPD; c = 13; };
			if(strncmp(s, "insert_resource", 15) == 0) { retval = JIL_UNSUPPD; c = 15; };
			if(strncmp(s, "insert_xinst",    12) == 0) { retval = JIL_UNSUPPD; c = 12; };
			break;
		case 'j':
			if(strncmp(s, "job_type",         8) == 0) { retval = JIL_JOBTYPE; c =  8; };
			break;
		case 'o':
			if(strncmp(s, "override_job",    12) == 0) { retval = JIL_UNSUPPD; c = 12; };
			break;
		case 's':
			if(strncmp(s, "start_mins",      10) == 0) { retval = JIL_STRTMIN; c = 10; };
			if(strncmp(s, "start_times",     11) == 0) { retval = JIL_STRTTIM; c = 11; };
			break;
		case 'u':
			if(strncmp(s, "update_job",      10) == 0) { retval = JIL_UPD_JOB; c = 10; };
			if(strncmp(s, "update_job_type", 15) == 0) { retval = JIL_UNSUPPD; c = 15; };
			if(strncmp(s, "update_machine",  14) == 0) { retval = JIL_UNSUPPD; c = 14; };
			if(strncmp(s, "update_monbro",   13) == 0) { retval = JIL_UNSUPPD; c = 13; };
			if(strncmp(s, "update_resource", 15) == 0) { retval = JIL_UNSUPPD; c = 15; };
			if(strncmp(s, "update_xinst",    12) == 0) { retval = JIL_UNSUPPD; c = 12; };
			break;
		default:                                        retval = JIL_UNKNOWN;
	}

	if(retval != JIL_UNKNOWN && retval != JIL_UNSUPPD)
	{
		// autosys allows whitespace between a keyword and the following
		// colon.  it's easier to parse if it's adjacent so fix it here
		// look ahead for the colon, if found move it to c if necessary
		for(i=c; s[i] != '\0'; i++)
		{
			if(!isspace(s[i]) && s[i] != ':')
			{
				// printf(error)
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

	if(namep == NULL)
	{
		retval = OTTO_FAIL;
	}

	if(retval == OTTO_SUCCESS && jil_reserved_word(namep) != JIL_UNKNOWN)
	{
		retval = OTTO_FAIL;
	}

	if(retval == OTTO_SUCCESS)
	{
		if((rc = ottojob_copy_name(item->name, namep, NAMLEN)) != OTTO_SUCCESS)
			retval = OTTO_FAIL;
		if(rc & OTTO_INVALID_NAME_CHARS)
		{
			fprintf(stderr, "ERROR for %s: %s < %sname contains one or more invalid characters or symbols >.\n", action, item->name, kind);
		}

		if(rc & OTTO_INVALID_NAME_LENGTH)
		{
			fprintf(stderr, "ERROR for %s: %s < %sname exceeds allowed length (%d bytes) >.\n", action, item->name, kind, NAMLEN);
		}
	}

	return(retval);
}



int
validate_and_copy_jil_type(JOB *item, char *typep)
{
	int retval = OTTO_SUCCESS;
	int rc;

	if(typep != NULL)
	{
		item->attributes = HAS_TYPE;

		if(jil_reserved_word(typep) == JIL_UNKNOWN)
		{
			switch(item->opcode)
			{
				case CREATE_JOB:
					if((rc = ottojob_copy_type(&item->type, typep, TYPLEN)) != OTTO_SUCCESS)
						retval = OTTO_FAIL;
					if(rc == OTTO_INVALID_VALUE)
					{
						fprintf(stderr, "ERROR for insert_job: %s < invalid value \"%c\" for JIL keyword: \"job_type\" >.\n", item->name, typep[0]);
					}
					break;
				case UPDATE_JOB: 
					fprintf(stderr, "ERROR for update_job: %s < invalid use of JIL keyword: \"job_type\" >.\n", item->name);
					retval = OTTO_FAIL;
					break;
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

	if(box_namep != NULL)
	{
		item->attributes = HAS_BOX_NAME;

		if(jil_reserved_word(box_namep) == JIL_UNKNOWN)
		{
			if((rc = ottojob_copy_name(item->box_name, box_namep, NAMLEN)) != OTTO_SUCCESS)
				retval = OTTO_FAIL;
			if(rc & OTTO_INVALID_NAME_CHARS)
			{
				fprintf(stderr, "ERROR for %s: %s < box_name %s contains one or more invalid characters or symbols >.\n", action, item->name, item->box_name);
			}

			if(rc & OTTO_INVALID_NAME_LENGTH)
			{
				fprintf(stderr, "ERROR for %s: %s < box_name %s exceeds allowed length (%d bytes) >.\n", action, item->name, item->box_name, NAMLEN);
			}

			if(item->type == OTTO_BOX && strcmp(item->name, item->box_name) == 0)
			{
				fprintf(stderr, "ERROR for %s: %s < job_name and box_name cannot be same >.\n", action, item->name);
				retval = OTTO_FAIL;
			}
		}
		else
		{
			// only a problem if creating a job
			if(item->opcode == CREATE_JOB)
				retval = OTTO_FAIL;
		}
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

	if(commandp != NULL)
	{
		item->attributes = HAS_COMMAND;

		if(jil_reserved_word(commandp) == JIL_UNKNOWN)
		{
			if((rc = ottojob_copy_command(item->command, commandp, CMDLEN)) != OTTO_SUCCESS)
				retval = OTTO_FAIL;
		}
		if(item->type == OTTO_CMD && item->command[0] == '\0')
		{
			fprintf(stderr, "ERROR for %s: %s < required JIL keyword \"command\" is missing >.\n", action, item->name);
			retval = OTTO_FAIL;
		}
	}

	return(retval);
}



int
validate_and_copy_jil_condition(JOB *item, char *conditionp)
{
	int retval = OTTO_SUCCESS;
	char *action = "action";

	switch(item->opcode)
	{
		case CREATE_JOB: action = "insert_job"; break;
		case UPDATE_JOB: action = "update_job"; break;
	}

	if(conditionp != NULL)
	{
		item->attributes = HAS_CONDITION;

		if(jil_reserved_word(conditionp) == JIL_UNKNOWN)
		{
			if(ottojob_copy_condition(item->condition, conditionp, CNDLEN) != OTTO_SUCCESS)
			{
				fprintf(stderr, "ERROR for %s: %s < condition statement contains errors >.\n", action, item->name);
				retval = OTTO_FAIL;
			}
		}
		else
		{
			// only a problem if creating a job
			if(item->opcode == CREATE_JOB)
				retval = OTTO_FAIL;
		}
	}

	return(retval);
}



int
validate_and_copy_jil_description(JOB *item, char *descriptionp)
{
	int retval = OTTO_SUCCESS;

	if(descriptionp != NULL)
	{
		item->attributes = HAS_DESCRIPTION;

		if(jil_reserved_word(descriptionp) == JIL_UNKNOWN)
		{
			if(ottojob_copy_description(item->description, descriptionp, DSCLEN) != OTTO_SUCCESS)
				retval = OTTO_FAIL;
		}
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

	if(auto_holdp != NULL)
	{
		item->attributes = HAS_AUTO_HOLD;

		if(jil_reserved_word(auto_holdp) == JIL_UNKNOWN)
		{
			if((rc = ottojob_copy_flag(&item->autohold, auto_holdp, FLGLEN)) != OTTO_SUCCESS)
				retval = OTTO_FAIL;
			if(rc == OTTO_INVALID_VALUE)
			{
				fprintf(stderr, "ERROR for %s: %s < invalid value \"%c\" for JIL keyword: \"auto_hold\" >.\n", action, item->name, auto_holdp[0]);
			}
			item->on_autohold = item->autohold;
		}
		else
		{
			// only a problem if creating a job
			if(item->opcode == CREATE_JOB)
				retval = OTTO_FAIL;
		}
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
	int     rc;

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
				fprintf(stderr, "ERROR for %s: %s < invalid application of date conditions >.\n", action, item->name);
				fprintf(stderr, "Otto only allows date conditions on objects at level 0. (i.e. no parent box).\n");
				retval = OTTO_FAIL;
			}
			else
			{
				parse_date_conditions = OTTO_TRUE;
			}
			break;
		default:
			fprintf(stderr, "ERROR for %s: %s < invalid combination of date conditions >.\n", action, item->name);
			fprintf(stderr, "You must specify date_conditions, days_of_week, and either start_mins or start_times.\n");
			retval = OTTO_FAIL;
			break;
	}

	if(parse_date_conditions == OTTO_TRUE)
	{
		if((rc = ottojob_copy_flag(&date_conditions, date_conditionsp, FLGLEN)) != OTTO_SUCCESS)
			retval = OTTO_FAIL;
		if(rc == OTTO_INVALID_VALUE)
		{
			fprintf(stderr, "ERROR for %s: %s < invalid value \"%c\" for JIL keyword: \"date_conditions\" >.\n", action, item->name, date_conditionsp[0]);
		}

		if((rc = ottojob_copy_days_of_week(&days_of_week, days_of_weekp)) != OTTO_SUCCESS)
			retval = OTTO_FAIL;
		if(rc == OTTO_INVALID_VALUE)
		{
			fprintf(stderr, "ERROR for %s: %s < days_of_week attribute contains errors >.\n", action, item->name);
		}

		if(start_minsp != NULL)
		{
			if((rc = ottojob_copy_start_minutes(&start_minutes, start_minsp)) != OTTO_SUCCESS)
				retval = OTTO_FAIL;
			if(rc == OTTO_INVALID_VALUE)
			{
				fprintf(stderr, "ERROR for %s: %s < start_mins attribute contains errors >.\n", action, item->name);
			}
		}

		if(start_timesp != NULL)
		{
			if((rc = ottojob_copy_start_times(start_times, start_timesp)) != OTTO_SUCCESS)
				retval = OTTO_FAIL;
			if(rc == OTTO_INVALID_VALUE)
			{
				fprintf(stderr, "ERROR for %s: %s < start_times attribute contains errors >.\n", action, item->name);
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
			item->attributes    = HAS_DATE_CONDITIONS;
			item->days_of_week  = days_of_week;
			item->start_minutes = start_minutes;
			memcpy(item->start_times, start_times, sizeof(start_times));
		}
	}

	return(retval);
}



//
// vim: ts=3 sw=3 ai
//
