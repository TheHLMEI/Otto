#include <ctype.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "otto.h"

OTTOLOG *logp = NULL;

// ottoimp variables
int     import_type;

int ottosysd_socket = -1;
ottoipc_simple_pdu_st send_pdu;

// ottoimp function prototypes
void  ottoimp_exit(int signum);
int   ottoimp_getargs (int argc, char **argv);
void  ottoimp_usage(void);
int   ottoimp (void);
int   send_create_job(JOB *i);
int   send_update_job(JOB *i);
int   send_delete_item(JOB *i);



int
main(int argc, char *argv[])
{
   int retval = init_cfg(argc, argv);

   if(retval == OTTO_SUCCESS)
      init_signals(ottoimp_exit);

   if(retval == OTTO_SUCCESS)
      if((logp = ottolog_init(cfg.env_ottolog, cfg.progname, INFO, 0)) == NULL)
         retval = OTTO_FAIL;

   if(retval == OTTO_SUCCESS)
   {
      log_initialization();
      log_args(argc, argv);
   }

   if(retval == OTTO_SUCCESS)
      retval = read_cfgfile();

   if(retval == OTTO_SUCCESS)
      retval = ottoimp_getargs(argc, argv);

   if(retval == OTTO_SUCCESS)
   {
      if((ottosysd_socket = init_client_ipc(cfg.server_addr, cfg.ottosysd_port)) == -1)
         retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS)
      retval = ottoimp();

   if(ottosysd_socket != -1)
   {
      close(ottosysd_socket);
   }

   return(retval);
}



void
ottoimp_exit(int signum)
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




/*------------------------------- ottoimp functions -------------------------------*/
int
ottoimp_getargs(int argc, char **argv)
{
   int retval = OTTO_SUCCESS;
   int i_opt;
   char *import_optarg = NULL;

   // initialize values
   import_type = OTTO_UNKNOWN;

   // Suppress getopt from printing an error.
   opterr = 0;

   // Get all parameters.
   while((i_opt = getopt(argc, argv, ":f:F:hH")) != -1 && retval != OTTO_FAIL)
   {
      switch(tolower(i_opt))
      {
         case 'f' :
            if(optarg[0] != '-')
            {
               import_type = get_file_format(optarg);
               import_optarg = optarg;
            }
            break;
         case 'h':
            ottoimp_usage();
            break;
         case ':' :
            retval = OTTO_FAIL;
            break;
      }
   }

   if(import_type == OTTO_UNKNOWN)
   {
      if(import_optarg != NULL)
         fprintf(stderr, "Unknown import file type.  %s\n", import_optarg);
      else
         fprintf(stderr, "Missing import file type.\n");

      ottoimp_usage();
      retval = OTTO_FAIL;
   }

   return(retval);
}



void
ottoimp_usage(void)
{

   printf("   NAME\n");
   printf("      ottoimp - Command line interface to define Otto jobs and objects\n");
   printf("\n");
   printf("   SYNOPSIS\n");
   printf("      ottoimp -f <format> [-h]\n");
   printf("\n");
   printf("   DESCRIPTION\n");
   printf("      Runs Otto's import routines against a file in a supported format to\n");
   printf("      manipulate jobs in the Otto job database.\n");
   printf("\n");
   printf("      You can use the ottoimp command in two ways:\n");
   printf("\n");
   printf("      *  To automatically apply updates to the job database by\n");
   printf("         redirecting a file in a supported format to the ottoimp command\n");
   printf("\n");
   printf("      *  To interactively apply updates to the job database by\n");
   printf("         issuing the ottoimp command only and entering commands at the\n");
   printf("         prompts. To exit interactive mode t', or 'exit' and press <Enter>\n");
   printf("         or press <Ctrl+D>.\n");
   printf("\n");
   printf("   OPTIONS\n");
   printf("      -f <format> - Import updates in a supported format.\n");
   printf("         CSV - import from a csv file\n");
   printf("         JIL - import from supported  autosys JIL (and extensions)\n");
   printf("         MSP - import from MS Project Data Interchange compliant XML\n");
   printf("\n");
   printf("      -h Print this help and exit.\n");
   printf("\n");
   printf("   Common keywords between Otto and autosys JIL:\n");
   printf("      auto_hold\n");
   printf("      box_name\n");
   printf("      command\n");
   printf("      condition\n");
   printf("      date_conditions\n");
   printf("      days_of_week\n");
   printf("      delete_box\n");
   printf("      delete_job\n");
   printf("      description\n");
   printf("      insert_job\n");
   printf("      job_type\n");
   printf("      start_mins\n");
   printf("      start_times\n");
   printf("      update_job\n");
   printf("\n");
   printf("   Otto specific keywords:\n");
   printf("      environment\n");
   printf("      finish\n");
   printf("      loop\n");
   printf("      new_name\n");
   printf("      start\n");
   exit(0);

}



int
ottoimp(void)
{
   int retval = OTTO_SUCCESS;
   DYNBUF b;
   JOBLIST joblist;
   int i;
   char *prompt = NULL;

   memset(&joblist, 0, sizeof(JOBLIST));

   switch(import_type)
   {
      case OTTO_CSV:
         prompt = "ottocsv";
         break;
      case OTTO_JIL:
         prompt = "ottojil";
         break;
      case OTTO_MSP:
         prompt = "ottomsp";
         break;
      default:
         retval = OTTO_FAIL;
            break;
   }

   if(retval == OTTO_SUCCESS)
      retval = read_stdin(&b, prompt);

   if(retval == OTTO_SUCCESS)
   {
      switch(import_type)
      {
         case OTTO_CSV:
            retval = parse_csv(&b, &joblist);
            break;
         case OTTO_JIL:
            retval = parse_jil(&b, &joblist);
            break;
         case OTTO_MSP:
            retval = parse_mspdi(&b, &joblist);
            break;
      }
   }

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

   if(b.buffer != NULL)
      free(b.buffer);

   if(joblist.item != NULL)
      free(joblist.item);

   return(retval);
}



int
send_create_job(JOB *item)
{
   int retval = OTTO_SUCCESS;
   int feedback_level;
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
   strcpy(pdu.environment, item->environment);
   strcpy(pdu.command,     item->command);
   strcpy(pdu.condition,   item->condition);

   pdu.type            = item->type;
   pdu.autohold        = item->autohold;
   pdu.date_conditions = item->date_conditions;
   pdu.days_of_week    = item->days_of_week;
   pdu.start_minutes   = item->start_minutes;
   for(i=0; i<24; i++)
      pdu.start_times[i]  = item->start_times[i];
   pdu.opcode          = item->opcode;

   strcpy(pdu.loopname, item->loopname);
   pdu.loopmin = item->loopmin;
   pdu.loopmax = item->loopmax;
   pdu.loopsgn = item->loopsgn;

   lprintf(logp, feedback_level, "Inserting job: %s.\n", pdu.name);
   pdu.opcode = CREATE_JOB;

   ottoipc_initialize_send();
   ottoipc_enqueue_create_job(&pdu);
   retval = ottoipc_send_all(ottosysd_socket);

   if(retval == OTTO_SUCCESS)
   {
      retval = ottoipc_recv_all(ottosysd_socket);
   }

   if(retval == OTTO_SUCCESS)
   {
      // loop over all returned PDUs reporting status
      while(ottoipc_dequeue_pdu((void **)&response) == OTTO_SUCCESS)
      {
         if(cfg.debug == OTTO_TRUE)
            log_received_pdu(response);

         switch(response->option)
         {
            case JOB_ALREADY_EXISTS:
               lprintf(logp, feedback_level, "The job already exists.\n");
               retval = OTTO_FAIL;
               break;
            case JOB_DEPENDS_ON_MISSING_JOB:
               lprintf(logp, feedback_level, "The job depends on a missing job.\n");
               break;
            case JOB_DEPENDS_ON_ITSELF:
               lprintf(logp, feedback_level, "The job depends on itself.\n");
               retval = OTTO_FAIL;
               break;
            case BOX_NOT_FOUND:
               lprintf(logp, feedback_level, "The parent job was not found.\n");
               retval = OTTO_FAIL;
               break;
            case BOX_COMMAND:
               lprintf(logp, feedback_level, "The job is a box but specifies a command (ignored).\n");
               break;
            case CMD_LOOP:
               lprintf(logp, feedback_level, "The job is a command but specifies a loop (ignored).\n");
               break;
            case NO_SPACE_AVAILABLE:
               lprintf(logp, feedback_level, "No space is available in the job database.\n");
               retval = OTTO_FAIL;
               break;
         }
      }
   }

   if(retval == OTTO_SUCCESS)
   {
      lprintf(logp, feedback_level, "Insert was successful.\n");
   }
   else
   {
      lprintf(logp, feedback_level, "Insert was not successful.\n");
   }

   return(retval);
}



int
send_update_job(JOB *item)
{
   int retval = OTTO_SUCCESS;
   int feedback_level;
   ottoipc_update_job_pdu_st pdu;
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
   strcpy(pdu.environment, item->environment);
   strcpy(pdu.command,     item->command);
   strcpy(pdu.condition,   item->condition);
   // conditionally copy the new_name from the expression buffer
   if(item->attributes & HAS_NEW_NAME)
      strcpy(pdu.new_name,   item->expression);

   pdu.attributes      = item->attributes;
   pdu.autohold        = item->autohold;
   pdu.date_conditions = item->date_conditions;
   pdu.days_of_week    = item->days_of_week;
   pdu.start_minutes   = item->start_minutes;
   strcpy(pdu.loopname,  item->loopname);
   pdu.loopmin         = item->loopmin;
   pdu.loopmax         = item->loopmax;
   pdu.loopsgn         = item->loopsgn;
   pdu.start           = item->start;
   pdu.finish          = item->finish;
   for(i=0; i<24; i++)
      pdu.start_times[i]  = item->start_times[i];

   lprintf(logp, feedback_level, "Updating job: %s.\n", pdu.name);
   pdu.opcode = UPDATE_JOB;

   ottoipc_initialize_send();
   ottoipc_enqueue_update_job(&pdu);
   retval = ottoipc_send_all(ottosysd_socket);

   if(retval == OTTO_SUCCESS)
   {
      retval = ottoipc_recv_all(ottosysd_socket);
   }

   if(retval == OTTO_SUCCESS)
   {
      // loop over all returned PDUs reporting status
      while(ottoipc_dequeue_pdu((void **)&response) == OTTO_SUCCESS)
      {
         if(cfg.debug == OTTO_TRUE)
            log_received_pdu(response);

         switch(response->option)
         {
            case JOB_NOT_FOUND:
               lprintf(logp, feedback_level, "The job was not found.\n");
               retval = OTTO_FAIL;
               break;
            case JOB_DEPENDS_ON_MISSING_JOB:
               lprintf(logp, feedback_level, "The job depends on a missing job.\n");
               break;
            case JOB_DEPENDS_ON_ITSELF:
               lprintf(logp, feedback_level, "The job depends on itself.\n");
               retval = OTTO_FAIL;
               break;
            case BOX_NOT_FOUND:
               lprintf(logp, feedback_level, "The parent job was not found.\n");
               retval = OTTO_FAIL;
               break;
            case BOX_COMMAND:
               lprintf(logp, feedback_level, "The job is a box but specifies a command (ignored).\n");
               break;
            case CMD_LOOP:
               lprintf(logp, feedback_level, "The job is a command but specifies a loop (ignored).\n");
               break;
            case GRANDFATHER_PARADOX:
               lprintf(logp, feedback_level, "The job would become a child job of itself.\n");
               retval = OTTO_FAIL;
               break;
         }
      }
   }

   if(retval == OTTO_SUCCESS)
   {
      lprintf(logp, feedback_level, "Update was successful.\n");
   }
   else
   {
      lprintf(logp, feedback_level, "Update was not successful.\n");
   }

   return(retval);
}



int
send_delete_item(JOB *item)
{
   int retval = OTTO_SUCCESS;
   int feedback_level;
   char *kind = "";
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

   lprintf(logp, feedback_level, "Deleting %s: %s.\n", kind, pdu.name);

   ottoipc_initialize_send();
   ottoipc_enqueue_simple_pdu(&pdu);
   retval = ottoipc_send_all(ottosysd_socket);

   if(retval == OTTO_SUCCESS)
   {
      retval = ottoipc_recv_all(ottosysd_socket);
   }

   if(retval == OTTO_SUCCESS)
   {
      // loop over all returned PDUs reporting status
      while(ottoipc_dequeue_pdu((void **)&response) == OTTO_SUCCESS)
      {
         if(cfg.debug == OTTO_TRUE)
            log_received_pdu(response);

         switch(response->option)
         {
            case JOB_NOT_FOUND:
               lprintf(logp, feedback_level, "Invalid job name: %s. Delete not performed.\n", response->name);
               retval = OTTO_FAIL;
               break;
            case JOB_DELETED:
               lprintf(logp, feedback_level, "Deleted job: %s.\n", response->name);
               // retval = OTTO_SUCCESS;
               break;
            case BOX_NOT_FOUND:
               lprintf(logp, feedback_level, "Invalid job name: %s. Delete not performed.\n", response->name);
               retval = OTTO_FAIL;
               break;
            case BOX_DELETED:
               lprintf(logp, feedback_level, "Deleted box: %s.\n", response->name);
               // retval = OTTO_SUCCESS;
               break;
         }
      }
   }

   if(retval == OTTO_SUCCESS)
      lprintf(logp, feedback_level, "Delete was successful.\n");
   else
      lprintf(logp, feedback_level, "Delete was not successful.\n");

   return(retval);
}



