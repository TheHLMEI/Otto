#include <ctype.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ottobits.h"
#include "ottocfg.h"
#include "ottodb.h"
#include "ottoipc.h"
#include "ottolog.h"
#include "ottojob.h"
#include "ottosignal.h"
#include "ottoutil.h"

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

enum keywords
{
	OTTO_AUTOHLD=1,
	OTTO_BOXNAME,
	OTTO_COMMAND,
	OTTO_CONDITN,
	OTTO_DESCRIP,
	OTTO_DEL_BOX,
	OTTO_DEL_JOB,
	OTTO_DTECOND,
	OTTO_DYSOFWK,
	OTTO_INS_JOB,
	OTTO_JOBTYPE,
	OTTO_STRTMIN,
	OTTO_STRTTIM,
	OTTO_UPD_JOB,
	OTTO_UNSUPPD
};

int server_socket = -1;
ottoipc_simple_pdu_st send_pdu;

// ottojil function prototypes
void  ottojil_exit(int signum);
int   ottojil_getargs (int argc, char **argv);
void  ottojil_usage(void);
int   ottojil (void);
int   parse_jobs (BUF_st *b);
int   delete_item (BUF_st *b, char *item);
int   insert_job (BUF_st *b);
void  get_cword (char *t, char *s, int tlen);
int   is_status (char *word);
int   reserved_word (BUF_st *b);



int
main(int argc, char *argv[])
{
	int retval = init_cfg(argc, argv);

	if(retval == OTTO_SUCCESS)
		retval = init_signals(ottojil_exit);

	if(retval == OTTO_SUCCESS)
		retval = init_log(cfg.env_ottolog, cfg.progname, INFO, 0);

	if(retval == OTTO_SUCCESS)
	{
		log_initialization();
		log_args(argc, argv);
	}

	if(retval == OTTO_SUCCESS)
		retval = read_cfgfile();

	if(retval == OTTO_SUCCESS)
		retval = ottojil_getargs(argc, argv);

	if(retval == OTTO_SUCCESS)
	{
		if((server_socket = init_client_ipc(cfg.server_addr, cfg.server_port)) == -1)
			retval = OTTO_FAIL;
	}

	if(retval == OTTO_SUCCESS)
		retval = ottojil();

	if(server_socket != -1)
	{
		close(server_socket);
	}

	return(retval);
}



void
ottojil_exit(int signum)
{
	int j, nptrs;
	void *buffer[100];
	char **strings;

	if(signum > 0)
	{
		lprintf(MAJR, "Shutting down. Caught signal %d (%s).\n", signum, strsignal(signum));

		nptrs   = backtrace(buffer, 100);
		strings = backtrace_symbols(buffer, nptrs);
		if(strings == NULL)
		{
			lprintf(MAJR, "Error getting backtrace symbols.\n");
		}
		else
		{
			lsprintf(INFO, "Backtrace:\n");
			for (j = 0; j < nptrs; j++)
				lsprintf(CATI, "  %s\n", strings[j]);
			lsprintf(END, "");
		}
	}
	else
	{
		lprintf(MAJR, "Shutting down. Reason code %d.\n", signum);
	}

	exit(-1);
}




/*------------------------------- ottojil functions -------------------------------*/
int
ottojil_getargs(int argc, char **argv)
{
	int retval = OTTO_SUCCESS;
	int i_opt;

	// Suppress getopt from printing an error.
	opterr = 0;

	// Get all parameters.
	while((i_opt = getopt(argc, argv, ":hH")) != -1 && retval != OTTO_FAIL)
	{
		switch(tolower(i_opt))
		{
			default:
				ottojil_usage();
				break;
		}
	}

	return(retval);
}



void
ottojil_usage(void)
{

	printf("   NAME\n");
	printf("      ottojil - Command line interface to define Otto jobs and objects\n");
	printf("\n");
	printf("   SYNOPSIS\n");
	printf("      ottojil [-h]\n");
	printf("\n");
	printf("   DESCRIPTION\n");
	printf("      Runs Otto's Job Information Language (JIL) processor to manipulate\n");
	printf("      jobs in the Otto job database.\n");
	printf("\n");
	printf("      You can use the ottojil command in two ways:\n");
	printf("\n");
	printf("      *  To automatically apply JIL commands to the database by\n");
	printf("         redirecting a JIL script file to the ottojil command. A JIL\n");
	printf("         script file contains one or more subcommands (such as insert_job).\n");
	printf("\n");
	printf("      *  To interactively apply JIL commands to the database by\n");
	printf("         issuing the ottojil command only and entering JIL subcommands at the\n");
	printf("         prompts. To exit interactive mode press Ctrl+D.\n");
	printf("\n");
	printf("   OPTIONS\n");
	printf("      -h Print this help and exit.\n");
	exit(0);

}



int
ottojil(void)
{
	int retval = OTTO_SUCCESS;
	BUF_st b;

	retval = read_stdin(&b, "ottojil");

	if(retval == OTTO_SUCCESS)
	{
		remove_jil_comments(&b);

		retval = parse_jobs(&b);
	}

	return(retval);
}



int
parse_jobs(BUF_st *b)
{
	int retval = OTTO_SUCCESS;

	b->line = 0;
	b->s    = b->buf;

	while(b->s[0] != '\0')
	{
		switch(reserved_word(b))
		{
			case OTTO_DEL_BOX:
				retval = delete_item(b, "box");
				break;
			case OTTO_DEL_JOB:
				retval = delete_item(b, "job");
				break;
			case OTTO_INS_JOB:
				retval = insert_job(b);
				break;
			default:
				break;
		}
		advance_word(b);
	}

	return(retval);
}



int
delete_item(BUF_st *b, char *item)
{
	int retval = OTTO_SUCCESS;
	int feedback_level;
	int rc;
	ottoipc_pdu_header_st *hdr = (ottoipc_pdu_header_st *)recv_header;
	ottoipc_simple_pdu_st pdu, *response;

	memset(&pdu, 0, sizeof(pdu));

	if(cfg.verbose == OTTO_TRUE)
		feedback_level = MAJR;
	else
		feedback_level = INFO;

	advance_jilword(b);
	if((rc = ottojob_copy_name(pdu.name, b->s, NAMLEN)) != OTTO_SUCCESS)
	{
		if(rc & OTTO_INVALID_NAME_CHARS)
		{
			lprintf(MAJR, "ERROR for job: %s < %s name contains one or more invalid characters or symbols >.\n", item, pdu.name);
			retval = OTTO_FAIL;
		}

		if(rc & OTTO_INVALID_NAME_LENGTH)
		{
			lprintf(MAJR, "ERROR for job: %s < %s name exceeds allowed length (%d bytes) >.\n", item, pdu.name, NAMLEN);
			retval = OTTO_FAIL;
		}
	}

	if(retval == OTTO_SUCCESS)
	{
		lprintf(feedback_level, "Deleting %s: %s.\n", item, pdu.name);
		if(strcmp(item, "box") == 0)
			pdu.opcode = DELETE_BOX;
		else
			pdu.opcode = DELETE_JOB;

		ottoipc_initialize_send();
		ottoipc_queue_simple_pdu(&pdu);
		retval = ottoipc_send_all(server_socket);
	}

	if(retval == OTTO_SUCCESS)
	{
		retval = ottoipc_recv_all(server_socket);
	}

	if(retval == OTTO_SUCCESS)
	{
		// loop over all returned PDUs reporting status
		response = (ottoipc_simple_pdu_st *)recv_pdus;
		while(response < (ottoipc_simple_pdu_st *)(recv_pdus + hdr->payload_length))
		{
			if(cfg.debug == OTTO_TRUE)
				log_pdu((ottoipc_pdu_header_st *)recv_header, (ottoipc_simple_pdu_st *)response);
			switch(response->option)
			{
				case JOB_NOT_FOUND:
					lprintf(feedback_level, "Invalid job name: %s Delete not performed.\n", response->name);
					retval = OTTO_FAIL;
					break;
				case JOB_DELETED:
					lprintf(feedback_level, "Deleting job: %s.\n", response->name);
					retval = OTTO_FAIL;
					break;
				case BOX_NOT_FOUND:
					lprintf(feedback_level, "Invalid job name: %s Delete not performed.\n", response->name);
					retval = OTTO_FAIL;
					break;
				case BOX_DELETED:
					lprintf(feedback_level, "Deleting box: %s.\n", response->name);
					retval = OTTO_FAIL;
					break;
			}

			response = (ottoipc_simple_pdu_st *)((char *)response + ottoipc_pdu_size(response));
		}
	}

	if(retval == OTTO_SUCCESS)
		lprintf(feedback_level, "Delete was successful.\n");
	else
		lprintf(feedback_level, "Delete was not successful.\n");

	return(retval);
}



int
insert_job(BUF_st *b)
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
	int feedback_level;
	int i, date_check = 0, parse_date_conditions = OTTO_FALSE;
	ottoipc_pdu_header_st *hdr = (ottoipc_pdu_header_st *)recv_header;
	ottoipc_create_job_pdu_st pdu;
	ottoipc_simple_pdu_st *response;

	memset(&pdu, 0, sizeof(pdu));

	if(cfg.verbose == OTTO_TRUE)
		feedback_level = MAJR;
	else
		feedback_level = INFO;

	advance_jilword(b);
	namep = b->s;

	while(exit_loop == OTTO_FALSE)
	{
		switch(reserved_word(b))
		{
			case OTTO_JOBTYPE: advance_jilword(b); typep            = b->s; break;
			case OTTO_BOXNAME: advance_jilword(b); box_namep        = b->s; break;
			case OTTO_COMMAND: advance_jilword(b); commandp         = b->s; break;
			case OTTO_CONDITN: advance_jilword(b); conditionp       = b->s; break;
			case OTTO_DESCRIP: advance_jilword(b); descriptionp     = b->s; break;
			case OTTO_AUTOHLD: advance_jilword(b); auto_holdp       = b->s; break;
			case OTTO_DTECOND: advance_jilword(b); date_conditionsp = b->s; break;
			case OTTO_DYSOFWK: advance_jilword(b); days_of_weekp    = b->s; break;
			case OTTO_STRTMIN: advance_jilword(b); start_minsp      = b->s; break;
			case OTTO_STRTTIM: advance_jilword(b); start_timesp     = b->s; break;
			case OTTO_DEL_BOX:
			case OTTO_DEL_JOB:
			case OTTO_INS_JOB:
			case OTTO_UNSUPPD: exit_loop = OTTO_TRUE;               break;
			default:           advance_word(b);                     break;
		}

		if(b->s[0] == '\0')
			exit_loop = OTTO_TRUE;
	}

	if((rc = ottojob_copy_name(pdu.name, namep, NAMLEN)) != OTTO_SUCCESS) retval = OTTO_FAIL;
	if(rc & OTTO_INVALID_NAME_CHARS)
	{
		lprintf(MAJR, "ERROR for job: %s < job_name contains one or more invalid characters or symbols >.\n", pdu.name);
		retval = OTTO_FAIL;
	}

	if(rc & OTTO_INVALID_NAME_LENGTH)
	{
		lprintf(MAJR, "ERROR for job: %s < job_name exceeds allowed length (%d bytes) >.\n", pdu.name, NAMLEN);
		retval = OTTO_FAIL;
	}

	if((rc = ottojob_copy_type(&pdu.type, typep, TYPLEN)) != OTTO_SUCCESS) retval = OTTO_FAIL;
	if(rc == OTTO_INVALID_VALUE)
	{
		lprintf(MAJR, "ERROR for job: %s < invalid value \"%c\" for JIL keyword: \"job_type\" >.\n", pdu.name, typep[0]);
		retval = OTTO_FAIL;
	}

	if((rc = ottojob_copy_name(pdu.box_name, box_namep, NAMLEN)) != OTTO_SUCCESS) retval = OTTO_FAIL;
	if(rc & OTTO_INVALID_NAME_CHARS)
	{
		lprintf(MAJR, "ERROR for job: %s < box_name %s contains one or more invalid characters or symbols >.\n", pdu.name, pdu.box_name);
		retval = OTTO_FAIL;
	}

	if(rc & OTTO_INVALID_NAME_LENGTH)
	{
		lprintf(MAJR, "ERROR for job: %s < box_name %s exceeds allowed length (%d bytes) >.\n", pdu.name, pdu.box_name, NAMLEN);
		retval = OTTO_FAIL;
	}

	if(pdu.type == 'b' && strcmp(pdu.name, pdu.box_name) == '\0')
	{
		lprintf(MAJR, "ERROR for job: %s < job_name and box_name cannot be same >.\n", pdu.name);
		retval = OTTO_FAIL;
	}

	if((rc = ottojob_copy_command(pdu.command, commandp, CMDLEN)) != OTTO_SUCCESS) retval = OTTO_FAIL;
	if(pdu.type == 'c' && pdu.command[0] == '\0')
	{
		lprintf(MAJR, "ERROR for job: %s < required JIL keyword \"command\" is missing >.\n", pdu.name);
		retval = OTTO_FAIL;
	}

	if((rc = ottojob_copy_condition(pdu.condition, conditionp, CNDLEN)) != OTTO_SUCCESS) retval = OTTO_FAIL;
	if(rc != OTTO_SUCCESS)
	{
		lprintf(MAJR, "ERROR for job: %s < condition statement contains errors >.\n", pdu.name);
		retval = OTTO_FAIL;
	}

	if((rc = ottojob_copy_description(pdu.description, descriptionp, DSCLEN)) != OTTO_SUCCESS) retval = OTTO_FAIL;

	if((rc = ottojob_copy_flag(&pdu.auto_hold, auto_holdp, FLGLEN)) != OTTO_SUCCESS) retval = OTTO_FAIL;
	if(rc == OTTO_INVALID_VALUE)
	{
		lprintf(MAJR, "ERROR for job: %s < invalid value \"%c\" for JIL keyword: \"auto_hold\" >.\n", pdu.name, auto_holdp[0]);
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
				lsprintf(MAJR, "ERROR for job: %s < invalid application of date conditions >.\n", pdu.name);
				lsprintf(CATI, "Otto only allows date conditions on objects at level 0. (i.e. no parent box).\n");
				lsprintf(END, "");
			}
			else
			{
				parse_date_conditions = OTTO_TRUE;
			}
			break;
		default:
			lsprintf(MAJR, "ERROR for job: %s < invalid combination of date conditions >.\n", pdu.name);
			lsprintf(CATI, "You must specify date_conditions, days_of_week, and either start_mins or start_times.\n");
			lsprintf(END, "");
			break;
	}

	if(parse_date_conditions == OTTO_TRUE)
	{
		if((rc = ottojob_copy_flag(&date_conditions, date_conditionsp, FLGLEN)) != OTTO_SUCCESS) retval = OTTO_FAIL;
		if(rc == OTTO_INVALID_VALUE)
		{
			lprintf(MAJR, "ERROR for job: %s < invalid value \"%c\" for JIL keyword: \"date_conditions\" >.\n", pdu.name, date_conditionsp[0]);
			retval = OTTO_FAIL;
		}

		if((rc = ottojob_copy_days_of_week(&days_of_week, days_of_weekp)) != OTTO_SUCCESS) retval = OTTO_FAIL;
		if(rc == OTTO_INVALID_VALUE)
		{
			lprintf(MAJR, "ERROR for job: %s < days_of_week attribute contains errors >.\n", pdu.name);
			retval = OTTO_FAIL;
		}

		if((rc = ottojob_copy_start_mins(&start_mins, start_minsp)) != OTTO_SUCCESS) retval = OTTO_FAIL;
		if(rc == OTTO_INVALID_VALUE)
		{
			lprintf(MAJR, "ERROR for job: %s < start_mins attribute contains errors >.\n", pdu.name);
			retval = OTTO_FAIL;
		}

		if((rc = ottojob_copy_start_times(start_times, start_timesp)) != OTTO_SUCCESS) retval = OTTO_FAIL;
		if(rc == OTTO_INVALID_VALUE)
		{
			lprintf(MAJR, "ERROR for job: %s < start_times attribute contains errors >.\n", pdu.name);
			retval = OTTO_FAIL;
		}
	}

	if(parse_date_conditions == OTTO_TRUE)
	{
		// assume we're using start times
		pdu.date_conditions = OTTO_USE_STARTTIMES;

		// modify if we're using start_mins
		if(start_minsp != NULL)
		{
			pdu.date_conditions = OTTO_USE_STARTMINS;
			for(i=0; i<24; i++)
				start_times[i] = start_mins;
		}

		// copy the data to the structure
		pdu.days_of_week = days_of_week;
		pdu.start_mins   = start_mins;
		memcpy(pdu.start_times, start_times, sizeof(start_times));
	}

	if(retval == OTTO_SUCCESS)
	{
		lprintf(feedback_level, "Inserting job: %s.\n", pdu.name);
		pdu.opcode = CREATE_JOB;

		ottoipc_initialize_send();
		ottoipc_queue_create_job(&pdu);
		retval = ottoipc_send_all(server_socket);
	}

	if(retval == OTTO_SUCCESS)
	{
		retval = ottoipc_recv_all(server_socket);
	}

	if(retval == OTTO_SUCCESS)
	{
		// loop over all returned PDUs reporting status
		response = (ottoipc_simple_pdu_st *)recv_pdus;
		while(response < (ottoipc_simple_pdu_st *)(recv_pdus + hdr->payload_length))
		{
			switch(response->option)
			{
				case JOB_ALREADY_EXISTS:
					lprintf(feedback_level, "The job already exists.\n");
					retval = OTTO_FAIL;
					break;
				case BOX_NOT_FOUND:
					lprintf(feedback_level, "The parent job was not found.\n");
					retval = OTTO_FAIL;
					break;
				case NO_SPACE_AVAILABLE:
					lprintf(feedback_level, "No space is available in the job database.\n");
					retval = OTTO_FAIL;
					break;
				case JOB_DEPENDS_ON_MISSING_JOB:
					lprintf(feedback_level, "The job depends on a missing job.\n");
					break;
				case JOB_DEPENDS_ON_ITSELF:
					lprintf(feedback_level, "The job depends on itself.\n");
					retval = OTTO_FAIL;
					break;
			}

			response = (ottoipc_simple_pdu_st *)((char *)response + ottoipc_pdu_size(response));
		}
	}

	if(retval == OTTO_SUCCESS)
	{
		lprintf(feedback_level, "Insert was successful.\n");
	}
	else
	{
		lprintf(feedback_level, "Insert was not successful.\n");
	}

	regress_word(b);

	return(retval);
}



int
reserved_word(BUF_st *b)
{
	int retval = OTTO_UNKNOWN;
	int c, i;
	char *s = b->s;

	switch(s[0])
	{
		case 'a':
			if(strncmp(s, "auto_hold",        9) == 0) { retval = OTTO_AUTOHLD; c =  9; };
			break;
		case 'b':
			if(strncmp(s, "box_name",         8) == 0) { retval = OTTO_BOXNAME; c =  8; };
			break;
		case 'c':
			if(strncmp(s, "command",          7) == 0) { retval = OTTO_COMMAND; c =  7; };
			if(strncmp(s, "condition",        9) == 0) { retval = OTTO_CONDITN; c =  9; };
			break;
		case 'd':
			if(strncmp(s, "date_conditions", 15) == 0) { retval = OTTO_DTECOND; c = 15; };
			if(strncmp(s, "days_of_week",    12) == 0) { retval = OTTO_DYSOFWK; c = 12; };
			if(strncmp(s, "description",     11) == 0) { retval = OTTO_DESCRIP; c = 11; };
			if(strncmp(s, "delete_box",      10) == 0) { retval = OTTO_DEL_BOX; c = 10; };
			if(strncmp(s, "delete_job",      10) == 0) { retval = OTTO_DEL_JOB; c = 10; };
			if(strncmp(s, "delete_blob",     11) == 0) { retval = OTTO_UNSUPPD; c = 11; };
			if(strncmp(s, "delete_glob",     11) == 0) { retval = OTTO_UNSUPPD; c = 11; };
			if(strncmp(s, "delete_job_type", 15) == 0) { retval = OTTO_UNSUPPD; c = 15; };
			if(strncmp(s, "delete_machine",  14) == 0) { retval = OTTO_UNSUPPD; c = 14; };
			if(strncmp(s, "delete_monbro",   13) == 0) { retval = OTTO_UNSUPPD; c = 13; };
			if(strncmp(s, "delete_resource", 15) == 0) { retval = OTTO_UNSUPPD; c = 15; };
			if(strncmp(s, "delete_xinst",    12) == 0) { retval = OTTO_UNSUPPD; c = 12; };
			break;
		case 'i':
			if(strncmp(s, "insert_job",      10) == 0) { retval = OTTO_INS_JOB; c = 10; };
			if(strncmp(s, "insert_blob",     11) == 0) { retval = OTTO_UNSUPPD; c = 11; };
			if(strncmp(s, "insert_glob",     11) == 0) { retval = OTTO_UNSUPPD; c = 11; };
			if(strncmp(s, "insert_job_type", 15) == 0) { retval = OTTO_UNSUPPD; c = 15; };
			if(strncmp(s, "insert_machine",  14) == 0) { retval = OTTO_UNSUPPD; c = 14; };
			if(strncmp(s, "insert_monbro",   13) == 0) { retval = OTTO_UNSUPPD; c = 13; };
			if(strncmp(s, "insert_resource", 15) == 0) { retval = OTTO_UNSUPPD; c = 15; };
			if(strncmp(s, "insert_xinst",    12) == 0) { retval = OTTO_UNSUPPD; c = 12; };
			break;
		case 'j':
			if(strncmp(s, "job_type",         8) == 0) { retval = OTTO_JOBTYPE; c =  8; };
			break;
		case 'o':
			if(strncmp(s, "override_job",    12) == 0) { retval = OTTO_UNSUPPD; c = 12; };
			break;
		case 's':
			if(strncmp(s, "start_mins",      10) == 0) { retval = OTTO_STRTMIN; c = 10; };
			if(strncmp(s, "start_times",     11) == 0) { retval = OTTO_STRTTIM; c = 11; };
			break;
		case 'u':
			if(strncmp(s, "update_job",      10) == 0) { retval = OTTO_UPD_JOB; c = 10; };
			if(strncmp(s, "update_job_type", 15) == 0) { retval = OTTO_UNSUPPD; c = 15; };
			if(strncmp(s, "update_machine",  14) == 0) { retval = OTTO_UNSUPPD; c = 14; };
			if(strncmp(s, "update_monbro",   13) == 0) { retval = OTTO_UNSUPPD; c = 13; };
			if(strncmp(s, "update_resource", 15) == 0) { retval = OTTO_UNSUPPD; c = 15; };
			if(strncmp(s, "update_xinst",    12) == 0) { retval = OTTO_UNSUPPD; c = 12; };
			break;
		default:                                        retval = OTTO_UNKNOWN;
	}

	if(retval != OTTO_UNKNOWN && retval != OTTO_UNSUPPD)
	{
		// autosys allows whitespace between a keeyword and the following
		// colon.  it's easier to parse if it's adjacent so fix it here
		// look ahead for the colon, if found move it to c if necessary
		for(i=c; s[i] != '\0'; i++)
		{
			if(!isspace(s[i]) && s[i] != ':')
			{
				// printf(error)
				retval = OTTO_UNKNOWN;
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
