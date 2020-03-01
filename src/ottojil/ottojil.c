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

#include "otto_jil_reader.h"


int server_socket = -1;
ottoipc_simple_pdu_st send_pdu;

// ottojil function prototypes
void  ottojil_exit(int signum);
int   ottojil_getargs (int argc, char **argv);
void  ottojil_usage(void);
int   ottojil (void);
int   send_create_job(JOB *i);
int   send_update_job(JOB *i);
int   send_delete_item(JOB *i);



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
	JOBLIST joblist;
	int i;

	memset(&joblist, 0, sizeof(JOBLIST));

	retval = read_stdin(&b, "ottojil");

	if(retval == OTTO_SUCCESS)
		retval = parse_jil(&b, &joblist);

	if(retval == OTTO_SUCCESS)
	{
		for(i=0; i<joblist.nitems; i++)
		{
			switch(joblist.item[i].opcode)
			{
				case CREATE_JOB: if(send_create_job(&joblist.item[i])  != OTTO_SUCCESS) retval = OTTO_FAIL; break;
				case UPDATE_JOB: if(send_update_job(&joblist.item[i])  != OTTO_SUCCESS) retval = OTTO_FAIL; break;
				case DELETE_BOX: if(send_delete_item(&joblist.item[i]) != OTTO_SUCCESS) retval = OTTO_FAIL; break;
				case DELETE_JOB: if(send_delete_item(&joblist.item[i]) != OTTO_SUCCESS) retval = OTTO_FAIL; break;
				default: break;
			}
		}
	}

	if(b.buf != NULL)
		free(b.buf);

	if(joblist.item != NULL)
		free(joblist.item);

	return(retval);
}



int
send_create_job(JOB *item)
{
	int retval = OTTO_SUCCESS;
	int feedback_level;
	ottoipc_pdu_header_st *hdr = (ottoipc_pdu_header_st *)recv_header;
	ottoipc_create_job_pdu_st pdu;
	ottoipc_simple_pdu_st *response;
	int i;

	if(cfg.verbose == OTTO_TRUE)
		feedback_level = MAJR;
	else
		feedback_level = INFO;

	memset(&pdu, 0, sizeof(pdu));

	strcpy(pdu.name,        item->name);
	strcpy(pdu.box_name,    item->box_name);
	strcpy(pdu.description, item->description);
	strcpy(pdu.command,     item->command);
	strcpy(pdu.condition,   item->condition);

	pdu.type            = item->type;
	pdu.auto_hold       = item->auto_hold;
	pdu.date_conditions = item->date_conditions;
	pdu.days_of_week    = item->days_of_week;
	pdu.start_mins      = item->start_mins;
	for(i=0; i<24; i++)
		pdu.start_times[i]  = item->start_times[i];
	pdu.opcode          = item->opcode;

	lprintf(feedback_level, "Inserting job: %s.\n", pdu.name);
	pdu.opcode = CREATE_JOB;

	ottoipc_initialize_send();
	ottoipc_queue_create_job(&pdu);
	retval = ottoipc_send_all(server_socket);

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

	return(retval);
}



int
send_update_job(JOB *item)
{
	int retval = OTTO_SUCCESS;

	return(retval);
}



int
send_delete_item(JOB *item)
{
	int retval = OTTO_SUCCESS;
	int feedback_level;
	char *kind = "";
	ottoipc_pdu_header_st *hdr = (ottoipc_pdu_header_st *)recv_header;
	ottoipc_simple_pdu_st pdu, *response;

	if(cfg.verbose == OTTO_TRUE)
		feedback_level = MAJR;
	else
		feedback_level = INFO;

	memset(&pdu, 0, sizeof(pdu));

	strcpy(pdu.name, item->name);
	pdu.opcode = item->opcode;

	switch(pdu.opcode)
	{
		case DELETE_BOX: kind = "box"; break;
		case DELETE_JOB: kind = "job"; break;
	}

	lprintf(feedback_level, "Deleting %s: %s.\n", kind, pdu.name);

	ottoipc_initialize_send();
	ottoipc_queue_simple_pdu(&pdu);
	retval = ottoipc_send_all(server_socket);

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



//
// vim: ts=3 sw=3 ai
//
