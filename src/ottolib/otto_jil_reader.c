#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ottobits.h"
#include "ottoipc.h"

#include "otto_jil_reader.h"

// ottojil defines
#define cdeSU______     ((i64)'S'<<56 | (i64)'U'<<48 | (i64)' '<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeMO______     ((i64)'M'<<56 | (i64)'O'<<48 | (i64)' '<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeTU______     ((i64)'T'<<56 | (i64)'U'<<48 | (i64)' '<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeWE______     ((i64)'W'<<56 | (i64)'E'<<48 | (i64)' '<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeTH______     ((i64)'T'<<56 | (i64)'H'<<48 | (i64)' '<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeFR______     ((i64)'F'<<56 | (i64)'R'<<48 | (i64)' '<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeSA______     ((i64)'S'<<56 | (i64)'A'<<48 | (i64)' '<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeALL_____     ((i64)'A'<<56 | (i64)'L'<<48 | (i64)'L'<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')

#define DTECOND_BIT      (1L)
#define DYSOFWK_BIT (1L << 1)
#define STRTMNS_BIT (1L << 2)
#define STRTTMS_BIT (1L << 3)

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



int jil_reserved_word(BUF_st *b);
int add_insert_job(JOB *item, BUF_st *b);
int add_update_job(JOB *item, BUF_st *b);
int add_delete_item(JOB *item, BUF_st *b, char *kind);



int
parse_jil(BUF_st *b, JOBLIST *joblist)
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
		b->s    = b->buf;
		while(b->s[0] != '\0')
		{
			switch(jil_reserved_word(b))
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
		b->s    = b->buf;
		while(b->s[0] != '\0')
		{
			switch(jil_reserved_word(b))
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

	return(retval);
}



int
add_insert_job(JOB *item, BUF_st *b)
{
	int     retval       = OTTO_SUCCESS;
	int     exit_loop    = OTTO_FALSE;
	int     rc;
	char    *namep            = NULL;
	char    *typep            = NULL;
	char    *auto_holdp       = NULL;
	char    *box_namep        = NULL;
	char    *commandp         = NULL;
	char    *conditionp       = NULL;
	char    *descriptionp     = NULL;
	char    date_conditions   = 0;
	char    *date_conditionsp = NULL;
	char    days_of_week      = 0;
	char    *days_of_weekp    = NULL;
	char    *start_minsp      = NULL;
	char    *start_timesp     = NULL;
	int64_t start_mins=0;
	int64_t start_times[24]={0};
	int i, date_check = 0, parse_date_conditions = OTTO_FALSE;

	advance_jilword(b);
	namep = b->s;

	while(exit_loop == OTTO_FALSE)
	{
		switch(jil_reserved_word(b))
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
			case JIL_UNSUPPD: exit_loop = OTTO_TRUE; break;
			default:          advance_word(b);       break;
		}

		if(b->s[0] == '\0')
			exit_loop = OTTO_TRUE;
	}

	if((rc = ottojob_copy_name(item->name, namep, NAMLEN)) != OTTO_SUCCESS) retval = OTTO_FAIL;
	if(rc & OTTO_INVALID_NAME_CHARS)
	{
		fprintf(stderr, "ERROR for job: %s < job_name contains one or more invalid characters or symbols >.\n", item->name);
		retval = OTTO_FAIL;
	}

	if(rc & OTTO_INVALID_NAME_LENGTH)
	{
		fprintf(stderr, "ERROR for job: %s < job_name exceeds allowed length (%d bytes) >.\n", item->name, NAMLEN);
		retval = OTTO_FAIL;
	}

	if((rc = ottojob_copy_type(&item->type, typep, TYPLEN)) != OTTO_SUCCESS) retval = OTTO_FAIL;
	if(rc == OTTO_INVALID_VALUE)
	{
		fprintf(stderr, "ERROR for job: %s < invalid value \"%c\" for JIL keyword: \"job_type\" >.\n", item->name, typep[0]);
		retval = OTTO_FAIL;
	}

	if((rc = ottojob_copy_name(item->box_name, box_namep, NAMLEN)) != OTTO_SUCCESS) retval = OTTO_FAIL;
	if(rc & OTTO_INVALID_NAME_CHARS)
	{
		fprintf(stderr, "ERROR for job: %s < box_name %s contains one or more invalid characters or symbols >.\n", item->name, item->box_name);
		retval = OTTO_FAIL;
	}

	if(rc & OTTO_INVALID_NAME_LENGTH)
	{
		fprintf(stderr, "ERROR for job: %s < box_name %s exceeds allowed length (%d bytes) >.\n", item->name, item->box_name, NAMLEN);
		retval = OTTO_FAIL;
	}

	if(item->type == 'b' && strcmp(item->name, item->box_name) == '\0')
	{
		fprintf(stderr, "ERROR for job: %s < job_name and box_name cannot be same >.\n", item->name);
		retval = OTTO_FAIL;
	}

	if((rc = ottojob_copy_command(item->command, commandp, CMDLEN)) != OTTO_SUCCESS) retval = OTTO_FAIL;
	if(item->type == 'c' && item->command[0] == '\0')
	{
		fprintf(stderr, "ERROR for job: %s < required JIL keyword \"command\" is missing >.\n", item->name);
		retval = OTTO_FAIL;
	}

	if((rc = ottojob_copy_condition(item->condition, conditionp, CNDLEN)) != OTTO_SUCCESS) retval = OTTO_FAIL;
	if(rc != OTTO_SUCCESS)
	{
		fprintf(stderr, "ERROR for job: %s < condition statement contains errors >.\n", item->name);
		retval = OTTO_FAIL;
	}

	if((rc = ottojob_copy_description(item->description, descriptionp, DSCLEN)) != OTTO_SUCCESS) retval = OTTO_FAIL;

	if((rc = ottojob_copy_flag(&item->auto_hold, auto_holdp, FLGLEN)) != OTTO_SUCCESS) retval = OTTO_FAIL;
	if(rc == OTTO_INVALID_VALUE)
	{
		fprintf(stderr, "ERROR for job: %s < invalid value \"%c\" for JIL keyword: \"auto_hold\" >.\n", item->name, auto_holdp[0]);
		retval = OTTO_FAIL;
	}

	// check if a valid comination of date conditions attributes was specified
	if(date_conditionsp != NULL) date_check |= DTECOND_BIT;
	if(days_of_weekp    != NULL) date_check |= DYSOFWK_BIT;
	if(start_minsp      != NULL) date_check |= STRTMNS_BIT;
	if(start_timesp     != NULL) date_check |= STRTTMS_BIT;

	switch(date_check)
	{
		case 0:
			// do nothing
			break;
		case (DTECOND_BIT | DYSOFWK_BIT | STRTMNS_BIT):
		case (DTECOND_BIT | DYSOFWK_BIT | STRTTMS_BIT):
			if(box_namep != NULL)
			{
				fprintf(stderr, "ERROR for job: %s < invalid application of date conditions >.\n", item->name);
				fprintf(stderr, "Otto only allows date conditions on objects at level 0. (i.e. no parent box).\n");
			}
			else
			{
				parse_date_conditions = OTTO_TRUE;
			}
			break;
		default:
			fprintf(stderr, "ERROR for job: %s < invalid combination of date conditions >.\n", item->name);
			fprintf(stderr, "You must specify date_conditions, days_of_week, and either start_mins or start_times.\n");
			break;
	}

	if(parse_date_conditions == OTTO_TRUE)
	{
		if((rc = ottojob_copy_flag(&date_conditions, date_conditionsp, FLGLEN)) != OTTO_SUCCESS) retval = OTTO_FAIL;
		if(rc == OTTO_INVALID_VALUE)
		{
			fprintf(stderr, "ERROR for job: %s < invalid value \"%c\" for JIL keyword: \"date_conditions\" >.\n", item->name, date_conditionsp[0]);
			retval = OTTO_FAIL;
		}

		if((rc = ottojob_copy_days_of_week(&days_of_week, days_of_weekp)) != OTTO_SUCCESS) retval = OTTO_FAIL;
		if(rc == OTTO_INVALID_VALUE)
		{
			fprintf(stderr, "ERROR for job: %s < days_of_week attribute contains errors >.\n", item->name);
			retval = OTTO_FAIL;
		}

		if((rc = ottojob_copy_start_mins(&start_mins, start_minsp)) != OTTO_SUCCESS) retval = OTTO_FAIL;
		if(rc == OTTO_INVALID_VALUE)
		{
			fprintf(stderr, "ERROR for job: %s < start_mins attribute contains errors >.\n", item->name);
			retval = OTTO_FAIL;
		}

		if((rc = ottojob_copy_start_times(start_times, start_timesp)) != OTTO_SUCCESS) retval = OTTO_FAIL;
		if(rc == OTTO_INVALID_VALUE)
		{
			fprintf(stderr, "ERROR for job: %s < start_times attribute contains errors >.\n", item->name);
			retval = OTTO_FAIL;
		}
	}

	if(parse_date_conditions == OTTO_TRUE)
	{
		// assume we're using start times
		item->date_conditions = OTTO_USE_STARTTIMES;

		// modify if we're using start_mins
		if(start_minsp != NULL)
		{
			item->date_conditions = OTTO_USE_STARTMINS;
			for(i=0; i<24; i++)
				start_times[i] = start_mins;
		}

		// copy the data to the structure
		item->days_of_week = days_of_week;
		item->start_mins   = start_mins;
		memcpy(item->start_times, start_times, sizeof(start_times));
	}

	item->opcode = CREATE_JOB;

	regress_word(b);

	return(retval);
}



int
add_update_job(JOB *item, BUF_st *b)
{
	int retval = OTTO_FAIL;

	return(retval);
}



int
add_delete_item(JOB *item, BUF_st *b, char *kind)
{
	int retval = OTTO_SUCCESS;
	int rc;

	advance_jilword(b);
	if((rc = ottojob_copy_name(item->name, b->s, NAMLEN)) != OTTO_SUCCESS)
	{
		if(rc & OTTO_INVALID_NAME_CHARS)
		{
			fprintf(stderr, "ERROR for job: %s < %s name contains one or more invalid characters or symbols >.\n", kind, item->name);
			retval = OTTO_FAIL;
		}

		if(rc & OTTO_INVALID_NAME_LENGTH)
		{
			fprintf(stderr, "ERROR for job: %s < %s name exceeds allowed length (%d bytes) >.\n", kind, item->name, NAMLEN);
			retval = OTTO_FAIL;
		}
	}

	if(strcmp(kind, "box") == 0)
		item->opcode = DELETE_BOX;
	else
		item->opcode = DELETE_JOB;

	return(retval);
}



int
jil_reserved_word(BUF_st *b)
{
	int retval = JIL_UNKNOWN;
	int c, i;
	char *s = b->s;

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
		// autosys allows whitespace between a keeyword and the following
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



//
// vim: ts=3 sw=3 ai
//
