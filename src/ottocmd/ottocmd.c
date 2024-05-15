#include <ctype.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "otto.h"

OTTOLOG *logp = NULL;

int ottosysd_socket = -1;
ottoipc_simple_pdu_st send_pdu;


// ottocmd function prototypes
void  ottocmd_exit(int signum);
int   ottocmd_getargs (int argc, char **argv);
void  ottocmd_usage(void);
int   ottocmd (void);
int   ottocmd_copy_event(ottoipc_simple_pdu_st *pdu, char *s);
int   ottocmd_copy_jobname(ottoipc_simple_pdu_st *pdu, char *s);
int   ottocmd_copy_signum(ottoipc_simple_pdu_st *pdu, char *s);
int   ottocmd_copy_status(ottoipc_simple_pdu_st *pdu, char *s);
int   ottocmd_copy_count(ottoipc_simple_pdu_st *pdu, char *s);
int   ottocmd_copy_iterator(ottoipc_simple_pdu_st *pdu, char *s);

void report_events(char *msg);
void report_statuses(char *msg);


int
main(int argc, char *argv[])
{
   int retval = OTTO_SUCCESS;
   int i;
   int loglevel = INFO;
   int report_initialization = OTTO_TRUE;

   // quick scan args to filter PING events
   for(i=1; i<argc; i++)
   {
      if(strcmp(argv[i], "-E") == 0 && i+1 < argc && strcmp(argv[i+1], "PING") == 0)
      {
         report_initialization = OTTO_FALSE;
         loglevel = NO_LOG;
         break;
      }
   }

   if(retval == OTTO_SUCCESS)
      retval = init_cfg(argc, argv);

   if(retval == OTTO_SUCCESS)
      init_signals(ottocmd_exit);

   if(retval == OTTO_SUCCESS)
      if((logp = ottolog_init(cfg.env_ottolog, cfg.progname, loglevel, 0)) == NULL)
         retval = OTTO_FAIL;

   if(retval == OTTO_SUCCESS && report_initialization == OTTO_TRUE)
   {
      log_initialization();
      log_args(argc, argv);
   }

   if(retval == OTTO_SUCCESS)
      retval = read_cfgfile();

   if(retval == OTTO_SUCCESS)
      retval = ottocmd_getargs(argc, argv);

   if(retval == OTTO_SUCCESS)
   {
      if((ottosysd_socket = init_client_ipc(cfg.server_addr, cfg.ottosysd_port)) == -1)
         retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS)
      retval = ottocmd();

   if(ottosysd_socket != -1)
   {
      close(ottosysd_socket);
   }

   return(retval);
}



void
ottocmd_exit(int signum)
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
ottocmd_getargs(int argc, char **argv)
{
   int retval = OTTO_SUCCESS;
   int i_opt;
   int event_reported  = OTTO_FALSE;
   int status_reported = OTTO_FALSE;
   int iterator_found  = OTTO_FALSE;

   // initialize values
   memset(&send_pdu, 0, sizeof(ottoipc_simple_pdu_st));

   // Suppress getopt from printing an error.
   opterr = 0;

   // Get all parameters.
   while((i_opt = getopt(argc, argv, ":c:C:e:E:hHi:I:j:J:k:K:s:S:")) != -1 && retval != OTTO_FAIL)
   {
      switch(tolower(i_opt))
      {
         case 'c':
            retval = ottocmd_copy_count(&send_pdu, optarg);
            break;
         case 'e':
            if((retval = ottocmd_copy_event(&send_pdu, optarg)) == OTTO_FALSE)
               event_reported = OTTO_TRUE;
            break;
         case 'h':
            ottocmd_usage();
            break;
         case 'i':
            if((retval = ottocmd_copy_iterator(&send_pdu, optarg)) == OTTO_SUCCESS)
               iterator_found = OTTO_TRUE;
            break;
         case 'j':
            retval = ottocmd_copy_jobname(&send_pdu, optarg);
            break;
         case 'k':
            retval = ottocmd_copy_signum(&send_pdu, optarg);
            break;
         case 's':
            if((retval = ottocmd_copy_status(&send_pdu, optarg)) == OTTO_FALSE)
               status_reported = OTTO_TRUE;
            break;
         case ':' :
            ottocmd_usage();
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
         case BREAK_LOOP:
         case CHANGE_STATUS:
         case FORCE_START_JOB:
         case JOB_ON_AUTOHOLD:
         case JOB_ON_AUTONOEXEC:
         case JOB_ON_HOLD:
         case JOB_ON_NOEXEC:
         case JOB_OFF_AUTOHOLD:
         case JOB_OFF_AUTONOEXEC:
         case JOB_OFF_HOLD:
         case JOB_OFF_NOEXEC:
         case KILL_JOB:
         case MOVE_JOB_BOTTOM:
         case MOVE_JOB_DOWN:
         case MOVE_JOB_TOP:
         case MOVE_JOB_UP:
         case RESET_JOB:
         case SEND_SIGNAL:
         case SET_LOOP:
         case START_JOB:
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

   if(retval          == OTTO_SUCCESS  &&
      send_pdu.opcode == CHANGE_STATUS &&
      send_pdu.option == NO_STATUS)
   {
      if(status_reported == OTTO_FALSE)
         report_statuses("CHANGE_STATUS event requires a STATUS.");

      retval = OTTO_FAIL;
   }

   if(retval          == OTTO_SUCCESS &&
      send_pdu.opcode == SEND_SIGNAL  &&
      send_pdu.option == 0)
   {
      fprintf(stderr, "\nSEND_SIGNAL event requires a signal (-k).\n");

      retval = OTTO_FAIL;
   }

   if(retval          == OTTO_SUCCESS &&
      send_pdu.opcode == SET_LOOP     &&
      iterator_found  == OTTO_FALSE)
   {
      fprintf(stderr, "\nSET_LOOP event requires an iterator value(-i).\n");

      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS &&
      (send_pdu.opcode == MOVE_JOB_UP || send_pdu.opcode == MOVE_JOB_DOWN) &&
      send_pdu.option == 0)
   {
      send_pdu.option = 1;
   }

   if(retval == OTTO_FAIL)
      fprintf(stderr, "\n");

   return(retval);
}



void
ottocmd_usage(void)
{

   printf("   NAME\n");
   printf("      ottocmd - Command line interface to control Otto jobs\n");
   printf("\n");
   printf("   SYNOPSIS\n");
   printf("      ottocmd -E event [-J job_name] [-k signal_number] [-s status] [-c count] [-h]\n");
   printf("\n");
   printf("   DESCRIPTION\n");
   printf("      Sends events to manipulate jobs in Otto's job database.\n");
   printf("\n");
   printf("   OPTIONS\n");
   printf("      -c count\n");
   printf("         Specifies the count of steps to move a job down or up in its\n");
   printf("         containing box using the MOVE_JOB_DOWN or MOVE_JOB_UP events:\n");
   printf("\n");
   printf("      -E event\n");
   printf("         Specifies the event to be sent.  This option is required.\n");
   printf("         Any one of the following values may be specified:\n");
   printf("\n");
   printf("      Events affecting jobs:\n");
   printf("\n");
   printf("         BREAK_LOOP - Stop the execution of the specified  box's loop\n");
   printf("         before the loop's end condition is met.  The loop's current\n");
   printf("         iteration will complete.\n");
   printf("\n");
   printf("         CHANGE_STATUS - Change the specified job status to the\n");
   printf("         status specified with the -s option.\n");
   printf("\n");
   printf("         FORCE_START_JOB - Start the job specified in job_name\n");
   printf("         regardless  of  whether  the  starting  conditions  are\n");
   printf("         satisfied.\n");
   printf("\n");
   printf("         JOB_OFF_AUTOHOLD - Take the job specified with the -J option\n");
   printf("         off autohold.  If the starting conditions are met the job\n");
   printf("         will start.\n");
   printf("\n");
   printf("         JOB_OFF_AUTONOEXEC - Take the job specified with the -J option\n");
   printf("         off autonoexec.  If the starting conditions are met the job\n");
   printf("         will run.\n");
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
   printf("         JOB_ON_AUTONOEXEC - Put the job specified with the -J option\n");
   printf("         on autonoexec.\n");
   printf("\n");
   printf("         JOB_ON_HOLD - Put the job specified with the -J option on\n");
   printf("         hold.\n");
   printf("\n");
   printf("         JOB_ON_NOEXEC - Put the job specified with the -J option on\n");
   printf("         noexec.  A job on noexec will not run but will be marked as\n");
   printf("         successful when its starting conditions are met.\n");
   printf("\n");
   printf("         KILL_JOB - Kill the job specified with the -J option.\n");
   printf("\n");
   printf("         MOVE_JOB_BOTTOM - Change the position in the internal linked list of\n");
   printf("         the job specified with the -J option.  Move the job to the bottom of\n");
   printf("         its containing box.  This is cosmetic and has no impact on starting conditions.\n");
   printf("\n");
   printf("         MOVE_JOB_DOWN - Change the position in the internal linked list of the job\n");
   printf("         specified with the -J option.  Move the job down in its containing box by the\n");
   printf("         number of steps specified with the -c option.  This is cosmetic and has no\n");
   printf("         impact on starting conditions.\n");
   printf("\n");
   printf("         MOVE_JOB_TOP - Change the position in the internal linked list of the job\n");
   printf("         specified with the -J option.  Move the job to the top of its containing box.\n");
   printf("         This is cosmetic and has no impact on starting conditions.\n");
   printf("\n");
   printf("         MOVE_JOB_UP - Change the position in the internal linked list of the job\n");
   printf("         specified with the -J option.  Move the job up in its containing box by the\n");
   printf("         number of steps specified with the -c option.  This is cosmetic and has no\n");
   printf("         impact on starting conditions.\n");
   printf("\n");
   printf("         RESET_JOB - Set the job specified with the -J option back to\n");
   printf("         initial conditions as if it was newly inserted into the\n");
   printf("         Otto job database using ottojil.\n");
   printf("\n");
   printf("         SEND_SIGNAL - Send a UNIX signal to a running job.\n");
   printf("\n");
   printf("         SET_LOOP - Set the loop iterator of the job specified with the -J option\n");
   printf("         to the number specified with the -i option.\n");
   printf("\n");
   printf("         START_JOB - Activate the job specified in job_name,  The job\n");
   printf("         will run when its starting  conditions  are satisfied.\n");
   printf("\n");
   printf("      Events affecting the Otto server process:\n");
   printf("\n");
   printf("         DEBUG_OFF - Cause ottosysd to stop writing disgnostic information.\n");
   printf("         to its log file.\n");
   printf("\n");
   printf("         DEBUG_ON - Cause ottosysd to write additional diagnoostic\n");
   printf("         information to its log file.\n");
   printf("\n");
   printf("         PAUSE_DAEMON - Prevent ottosysd starting jobs while allowing the\n");
   printf("         daemon to capture currently running job results.  Use this to perform\n");
   printf("         an orderly shutdown.\n");
   printf("\n");
   printf("         PING - Test whether the ottosysd process is able to receive and send\n");
   printf("         IPC messages.\n");
   printf("\n");
   printf("         REFRESH - Re-evaluate the starting conditions for all jobs.\n");
   printf("\n");
   printf("         RESUME_DAEMON - Restart a paused ottosysd process.\n");
   printf("\n");
   printf("         STOP_DAEMON - Immediately stop the ottosysd process.\n");
   printf("\n");
   printf("         VERIFY_DB - Compare the inode of the Otto database file as known by ottosysd\n");
   printf("         to the inode of the file as determined by the sending process.\n");
   printf("\n");
   printf("      -h Print this help and exit.\n");
   printf("\n");
   printf("      -i iterator value\n");
   printf("         Specifies the new loop iterator value to set for a job.\n");
   printf("\n");
   printf("      -J job_name\n");
   printf("         Identifies the job the specified event should be applied to.\n");
   printf("\n");
   printf("      -k signal_number\n");
   printf("         Specifies the signal number to send to a job using the\n");
   printf("         SEND_SIGNAL event.\n");
   printf("\n");
   printf("      -s status\n");
   printf("         Specifies the status to which to change a job.  You can\n");
   printf("         set the job to one of the following statuses:\n");
   printf("         FAILURE, INACTIVE, RUNNING, SUCCESS, and TERMINATED.\n");
   exit(0);

}



int
ottocmd(void)
{
   int retval = OTTO_SUCCESS;
   ottoipc_simple_pdu_st *pdu;

   ottoipc_initialize_send();
   ottoipc_enqueue_simple_pdu(&send_pdu);

   if((retval = ottoipc_send_all(ottosysd_socket)) == OTTO_FAIL)
   {
      lprintf(logp, MAJR, "send failed.\n");
   }
   else
   {
      if((retval = ottoipc_recv_all(ottosysd_socket)) == OTTO_SUCCESS)
      {
         if(ottoipc_dequeue_pdu((void **)&pdu) == OTTO_SUCCESS)
         {
            if(cfg.debug == OTTO_TRUE)
            {
               log_received_pdu(pdu);
            }

            // pings can be very verbose.  they don't need to be logged
            // but some output to the screen is required
            if(pdu->opcode == PING && pdu->option == ACK)
            {
               printf("%s\n", pdu->name);
            }

            // otherwise if the pdu response is NOT ACK
            // print a message to the screen
            if(pdu->option != ACK)
            {
               print_received_pdu(pdu);
            }
         }
      }
   }

   return(retval);
}



int
ottocmd_copy_event(ottoipc_simple_pdu_st *pdu, char *s)
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
   // Autosys command aliases
   if(strcmp("FORCE_STARTJOB", s) == 0) { i = FORCE_START_JOB; pdu->opcode = i; }
   if(strcmp("STARTJOB",       s) == 0) { i = START_JOB;       pdu->opcode = i; }
   if(strcmp("KILLJOB",        s) == 0) { i = KILL_JOB;        pdu->opcode = i; }
   if(strcmp("STOP_DEMON",     s) == 0) { i = STOP_DAEMON;     pdu->opcode = i; }
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
ottocmd_copy_jobname(ottoipc_simple_pdu_st *pdu, char *s)
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
ottocmd_copy_signum(ottoipc_simple_pdu_st *pdu, char *s)
{
   int retval = OTTO_SUCCESS;
   int i;

   i = atoi(s);
   if(i < 1 || i > 64)
   {
      fprintf(stderr, "Invalid signal.  Signal number must be 1 to 64 \n");
      pdu->option = 0;

      retval = OTTO_FAIL;
   }
   else
   {
      pdu->option = i;
   }

   return(retval);
}



int
ottocmd_copy_status(ottoipc_simple_pdu_st *pdu, char *s)
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



int
ottocmd_copy_count(ottoipc_simple_pdu_st *pdu, char *s)
{
   int retval = OTTO_SUCCESS;
   int i;

   i = atoi(s);
   if(i < 1 || i > 255)
   {
      fprintf(stderr, "Invalid count.  Count must be 1 to 255 \n");
      pdu->option = 0;

      retval = OTTO_FAIL;
   }
   else
   {
      pdu->option = i;
   }

   return(retval);
}



int
ottocmd_copy_iterator(ottoipc_simple_pdu_st *pdu, char *s)
{
   int retval = OTTO_SUCCESS;
   int i;

   i = atoi(s);
   if(i < 0 || i > 99)
   {
      fprintf(stderr, "Invalid iterator value.  Iterator must be 0 to 99 \n");
      pdu->option = 0;

      retval = OTTO_FAIL;
   }
   else
   {
      pdu->option = i;
   }

   return(retval);
}



