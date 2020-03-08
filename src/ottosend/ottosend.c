#include <ctype.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ottobits.h"
#include "ottocfg.h"
#include "ottoipc.h"
#include "ottojob.h"
#include "signals.h"
#include "simplelog.h"

SIMPLELOG *logp = NULL;

int server_socket = -1;
ottoipc_simple_pdu_st send_pdu;


// ottosend function prototypes
void  ottosend_exit(int signum);
int   ottosend_getargs (int argc, char **argv);
void  ottosend_usage(void);
int   ottosend (void);
int   ottosend_copy_event(ottoipc_simple_pdu_st *pdu, char *s);
int   ottosend_copy_jobname(ottoipc_simple_pdu_st *pdu, char *s);
int   ottosend_copy_signum(ottoipc_simple_pdu_st *pdu, char *s);
int   ottosend_copy_status(ottoipc_simple_pdu_st *pdu, char *s);

void report_events(char *msg);
void report_statuses(char *msg);


int
main(int argc, char *argv[])
{
	int retval = init_cfg(argc, argv);

	if(retval == OTTO_SUCCESS)
		init_signals(ottosend_exit);

	if(retval == OTTO_SUCCESS)
		if((logp = simplelog_init(cfg.env_ottolog, cfg.progname, INFO, 0)) == NULL)
			retval = OTTO_FAIL;

	if(retval == OTTO_SUCCESS)
	{
		log_initialization();
		log_args(argc, argv);
	}

	if(retval == OTTO_SUCCESS)
		retval = read_cfgfile();

	if(retval == OTTO_SUCCESS)
		retval = ottosend_getargs(argc, argv);

	if(retval == OTTO_SUCCESS)
	{
		if((server_socket = init_client_ipc(cfg.server_addr, cfg.server_port)) == -1)
			retval = OTTO_FAIL;
	}

	if(retval == OTTO_SUCCESS)
		retval = ottosend();

	if(server_socket != -1)
	{
		close(server_socket);
	}

	return(retval);
}



void
ottosend_exit(int signum)
{
	int j, nptrs;
	void *buffer[100];
	char **strings;

	if(signum > 0)
	{
		lprintf(logp, MAJR, "Shutting down. Caught signal %d (%s).\n", signum, strsignal(signum));

		nptrs   = backtrace(buffer, 100);
		strings = backtrace_symbols(buffer, nptrs);
		if(strings == NULL)
		{
			lprintf(logp, MAJR, "Error getting backtrace symbols.\n");
		}
		else
		{
			lsprintf(logp, INFO, "Backtrace:\n");
			for (j = 0; j < nptrs; j++)
				lsprintf(logp, CATI, "  %s\n", strings[j]);
			lsprintf(logp, END, "");
		}
	}
	else
	{
		lprintf(logp, MAJR, "Shutting down. Reason code %d.\n", signum);
	}

	exit(-1);
}



int
ottosend_getargs(int argc, char **argv)
{
	int retval = OTTO_SUCCESS;
	int i_opt;
	int event_reported = OTTO_FALSE;
	int status_reported = OTTO_FALSE;

	// initialize values
	memset(&send_pdu, 0, sizeof(ottoipc_simple_pdu_st));

	// Suppress getopt from printing an error.
	opterr = 0;

	// Get all parameters.
	while((i_opt = getopt(argc, argv, ":e:E:hHj:J:k:K:s:S:")) != -1 && retval != OTTO_FAIL)
	{
		switch(tolower(i_opt))
		{
			case 'e':
				if((retval = ottosend_copy_event(&send_pdu, optarg)) == OTTO_FALSE)
					event_reported = OTTO_TRUE;
				break;
			case 'h':
				ottosend_usage();
				break;
			case 'j':
				retval = ottosend_copy_jobname(&send_pdu, optarg);
				break;
			case 'k':
				retval = ottosend_copy_signum(&send_pdu, optarg);
				break;
			case 's':
				if((retval = ottosend_copy_status(&send_pdu, optarg)) == OTTO_FALSE)
					status_reported = OTTO_TRUE;
				break;
			case ':' :
				ottosend_usage();
				retval = OTTO_FAIL;
				break;
		}
		if(retval == OTTO_FAIL)
			break;
	}

	// check parameter combinations
	if(retval == OTTO_SUCCESS && send_pdu.opcode == NOOP)
	{
		if(event_reported == OTTO_FALSE)
			report_events("No EVENT specified.");

		retval = OTTO_FAIL;
	}

	if(retval == OTTO_SUCCESS)
	{
		switch(send_pdu.opcode)
		{
			case START_JOB:
			case KILL_JOB:
			case SEND_SIGNAL:
			case RESET_JOB:
			case CHANGE_STATUS:
			case JOB_ON_AUTOHOLD:
			case JOB_OFF_AUTOHOLD:
			case JOB_ON_HOLD:
			case JOB_OFF_HOLD:
			case JOB_ON_NOEXEC:
			case JOB_OFF_NOEXEC:
				if(send_pdu.name[0] == '\0')
				{
					fprintf(stderr, "\n%s event requires a job name\n", stropcode(send_pdu.opcode));
					retval = OTTO_FAIL;
				}
				break;
			default:
				break;
		}
	}

	if(retval == OTTO_SUCCESS      &&
		send_pdu.opcode == CHANGE_STATUS &&
		send_pdu.option == NO_STATUS)
	{
		if(status_reported == OTTO_FALSE)
			report_statuses("CHANGE_STATUS event requires a STATUS.");

		retval = OTTO_FAIL;
	}

	if(retval == OTTO_SUCCESS &&
		send_pdu.opcode == SEND_SIGNAL &&
		send_pdu.option == 0)
	{
		fprintf(stderr, "\nSEND_SIGNAL event requires a signal (-k).\n");

		retval = OTTO_FAIL;
	}

	if(retval == OTTO_FAIL)
		fprintf(stderr, "\n");

	return(retval);
}



void
ottosend_usage(void)
{

	printf("   NAME\n");
	printf("      ottosend - Command line interface to control Otto jobs\n");
	printf("\n");
	printf("   SYNOPSIS\n");
	printf("      ottosend -E event [-J job_name] [-k signal_number] [-s status] [-h]\n");
	printf("\n");
	printf("   DESCRIPTION\n");
	printf("      Sends events to manipulate jobs in Otto's job database.\n");
	printf("\n");
	printf("   OPTIONS\n");
	printf("      -E event\n");
	printf("         Specifies the event to be sent.  This option is required.\n");
	printf("         Any one of the following values may be specified:\n");
	printf("\n");
	printf("         CHANGE_STATUS - Change the specified job status to the\n");
	printf("         status specified with the -s option.\n");
	printf("\n");
	printf("         FORCE_STARTJOB - Start the job specified in job_name,\n");
	printf("         regardless  of  whether  the  starting  conditions  are\n");
	printf("         satisfied or not.\n");
	printf("\n");
	printf("         JOB_OFF_AUTOHOLD - Take the job specified with the -J option\n");
	printf("         off autohold.  If the starting conditions are met the job\n");
	printf("         will start.\n");
	printf("\n");
	printf("         JOB_OFF_HOLD - Take the job specified with the -J option off\n");
	printf("         hold.  If the starting conditions are met the job will start.\n");
	printf("\n");
	printf("         JOB_OFF_NOEXEC - Take the job specified with the -J option off\n");
	printf("         autoexec.\n");
	printf("\n");
	printf("         JOB_ON_AUTOHOLD - Put the job specified with the -J option\n");
	printf("         on autohold.\n");
	printf("\n");
	printf("         JOB_ON_HOLD - Put the job specified with the -J option on\n");
	printf("         hold.\n");
	printf("\n");
	printf("         JOB_ON_NOEXEC - Put the job specified with the -J option on\n");
	printf("         noexec.  A job on noexec will not run but will be marked as\n");
	printf("         successful when its starting conditions are met.\n");
	printf("\n");
	printf("         KILLJOB - Kill the job specified with the -J option.\n");
	printf("\n");
	printf("         REFRESH - Re-evaluate the starting conditions for the job\n");
	printf("         specified with the -J option.\n");
	printf("\n");
	printf("         RESET - Set the job specified with the -J option back to\n");
	printf("         initial conditions as if it was newly inserted into the\n");
	printf("         Otto job database using ottojil.\n");
	printf("\n");
	printf("         SEND_SIGNAL - Send a UNIX signal to a running job.\n");
	printf("\n");
	printf("         STOP_DEMON - Stop ottosysd.\n");
	printf("\n");
	printf("      -h Print this help and exit.\n");
	printf("\n");
	printf("      -J job_name\n");
	printf("         Identifies the job to which to send the specified event.\n");
	printf("\n");
	printf("      -k signal_number\n");
	printf("         Specifies the signal number to send to a job using the\n");
	printf("         KILLJOB or SEND_SIGNAL events.\n");
	printf("\n");
	printf("      -s status\n");
	printf("         Specifies the status to which to change a job.  You can\n");
	printf("         set the job to one of the following statuses:\n");
	printf("         FAILURE, INACTIVE, RUNNING, SUCCESS, and TERMINATED.\n");
	exit(0);

}



int
ottosend(void)
{
	int retval = OTTO_SUCCESS;
	ottoipc_simple_pdu_st *pdu;

	ottoipc_initialize_send();
	ottoipc_enqueue_simple_pdu(&send_pdu);

	if((retval = ottoipc_send_all(server_socket)) == OTTO_FAIL)
	{
		lprintf(logp, MAJR, "send failed.\n");
	}
	else
	{
		if((retval = ottoipc_recv_all(server_socket)) == OTTO_SUCCESS)
		{
			if(ottoipc_dequeue_pdu((void **)&pdu) == OTTO_SUCCESS)
			{
				if(cfg.debug == OTTO_TRUE)
				{
					log_received_pdu(pdu);
				}

				if(pdu->opcode == PING && pdu->option == ACK)
				{
					lprintf(logp, MAJR, "ottosysd is responding\n");
				}
			}
		}
	}

	return(retval);
}



int
ottosend_copy_event(ottoipc_simple_pdu_st *pdu, char *s)
{
	int retval = OTTO_SUCCESS;
	int i;

	for(i=CRUD_TOTAL+1; i<OPCODE_TOTAL; i++)
	{
		if(i == SCHED_TOTAL || i == VERIFY_DB)
			continue;
		if(strcmp(stropcode(i), s) == 0)
		{
			pdu->opcode = i;
			break;
		}
	}
	// Autosys JIL aliases
	if(strcmp("STARTJOB", s) == 0) { i = START_JOB; pdu->opcode = i; }
	if(strcmp("KILLJOB",  s) == 0) { i = KILL_JOB;  pdu->opcode = i; }
	if(i == OPCODE_TOTAL)
	{
		if(strcmp("JOB_ON_ICE",  s) == 0 ||
			strcmp("JOB_OFF_ICE", s) == 0)
		{
			fprintf(stderr, "JOB_ON_ICE/JOB_OFF_ICE are not supported. Use JOB_ON_NOEXEC/JOB_OFF_NOEXEC instead.\n");
		}

		report_events("Unsupported EVENT");

		retval = OTTO_FAIL;
	}

	return(retval);
}



void
report_events(char *msg)
{
	int i;

	fprintf(stderr, "\n%s.\nEvent must be one of:", msg);
	for(i=CRUD_TOTAL+1; i<OPCODE_TOTAL; i++)
	{
		if(i == SCHED_TOTAL || i == VERIFY_DB)
			continue;
		if(i > CRUD_TOTAL+1)
			fprintf(stderr, ",");
		fprintf(stderr, "\n   %s", stropcode(i));
	}
}



int
ottosend_copy_jobname(ottoipc_simple_pdu_st *pdu, char *s)
{
	int retval = OTTO_SUCCESS;

	if((retval = ottojob_copy_name(pdu->name, s, (int)sizeof(pdu->name)-1)) != OTTO_SUCCESS)
	{
		if(retval & OTTO_INVALID_NAME_LENGTH)
			fprintf(stderr, "Job name too long.  Job name must be <= %d characters\n", (int)sizeof(pdu->name)-1);

		if(retval & OTTO_INVALID_NAME_CHARS)
			fprintf(stderr, "Job name contains invalid characters\n");

		retval = OTTO_FAIL;
	}

	return(retval);
}



int
ottosend_copy_signum(ottoipc_simple_pdu_st *pdu, char *s)
{
	int retval = OTTO_SUCCESS;

	pdu->option = atoi(optarg);
	if(pdu->option < 1 || pdu->option > 64)
	{
		fprintf(stderr, "Invalid signal.  Signal number must be 1 to 64 \n");
		pdu->option = 0;

		retval = OTTO_FAIL;
	}

	return(retval);
}



int
ottosend_copy_status(ottoipc_simple_pdu_st *pdu, char *s)
{
	int retval = OTTO_SUCCESS;
	int i;

	for(i=NO_STATUS+1; i<STATUS_TOTAL; i++)
	{
		if(strcmp(strstatus(i), s) == 0)
		{
			pdu->option = i;
			break;
		}
	}
	if(i == STATUS_TOTAL)
	{
		report_statuses("Unsupported STATUS");

		retval = OTTO_FAIL;
	}

	return(retval);
}



void
report_statuses(char *msg)
{
	int i;

	fprintf(stderr, "\n%s.\nStatus must be one of:\n", msg);
	for(i=NO_STATUS+1; i<STATUS_TOTAL; i++)
	{
		if(i > NO_STATUS+1)
			fprintf(stderr, ",");
		fprintf(stderr, "\n   %s", strstatus(i));
	}
}



//
// vim: ts=3 sw=3 ai
//
