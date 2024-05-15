#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "otto.h"



#define NPOLLFDS 200
#define BUFSIZE 1024

// ottosysd context
OTTOLOG      *logp = NULL;
DBCTX         ctx  = {0};
int           ottosysd_socket = -1;
struct pollfd fds[NPOLLFDS];
int           nfds = 0;

// httpd context
OTTOLOG      *hlogp = NULL;
DBCTX         hctx  = {0};
JOBLIST       hjoblist;
int           ottohttpd_socket = -1;
struct pollfd hfds[NPOLLFDS];
int           nhfds = 0;

// internal representation of minutes during the week to check for a timed job
// updates each time dependencies are compiled
int64_t    check_times[7][24];
int        last_check_day=-1, last_check_hour=-1, last_check_min=-1;



/*---------------------------- program control functions --------------------------*/
void  ottosysd_exit(int signum);
int   ottosysd_getargs (int argc, char **argv);
void  ottosysd_usage(void);
int   ottosysd (void);
int   write_pidfile ();
void  delete_pidfile ();
/*------------------------------ event based functions ----------------------------*/
int   handle_ipc(int fd);
int   handle_crud_pdu(ottoipc_simple_pdu_st *pdu);
int   handle_scheduler_pdu(ottoipc_simple_pdu_st *pdu);
int   handle_daemon_pdu(ottoipc_simple_pdu_st *pdu);
void  check_dependencies ();
void  compile_dependencies ();
/*------------------------------ time based functions -----------------------------*/
void  handle_timeout();
int   get_msec_to_next_wake_time(void);
void  fire_this_minute(int this_wday, int this_hour, int this_min);
/*------------------------------ httpd based functions ----------------------------*/
int   fork_httpd(char **argv);
int   ottohttpd(void);
int   handle_http(RECVBUF *recvbuf);
/*------------------------------ job utility functions ----------------------------*/
void  activate_box             (int id);
void  activate_box_chain       (int id);
void  run_box                  (int id);
void  force_activate_box       (int id);
void  force_activate_box_chain (int id);
void  activate_job             (int id);
void  run_job                  (int id);
void  finish_box               (int box_id, time_t finish);
void  finish_job               (int sigNum);
void  force_start_job          (int id);
void  start_job                (int id);
void  kill_job                 (int id, int signal);
void  move_job                 (int id, int action, int count);
void  reset_job                (int id);
void  reset_job_chain          (int id);
int   check_reset              (int id);
int   check_reset_chain        (int id);
void  set_job_status           (int id, int status);
void  set_job_autohold         (int id, int action);
void  set_job_autonoexec       (int id, int action);
void  set_job_hold             (int id, int action);
void  set_job_noexec           (int id, int action);
void  set_job_noexec_chain     (int id, int action);
void  set_levels               (void);
void  set_levels_chain         (int id, int level);
void  break_loop               (int id);
void  set_loop                 (int id, int iterator, uint8_t *response);



int
main(int argc, char *argv[])
{
   int retval = init_cfg(argc, argv);

   if(retval == OTTO_SUCCESS)
      retval = write_pidfile();

   if(retval == OTTO_SUCCESS)
      init_signals(ottosysd_exit);

   if(retval == OTTO_SUCCESS)
      if((logp = ottolog_init(cfg.env_ottolog, cfg.progname, INFO, 0)) == NULL)
         retval = OTTO_FAIL;

   if(retval == OTTO_SUCCESS)
      retval = read_cfgfile();

   if(retval == OTTO_SUCCESS)
   {
      log_initialization();
      log_args(argc, argv);
   }

   if(retval == OTTO_SUCCESS)
      retval = ottosysd_getargs(argc, argv);

   if(retval == OTTO_SUCCESS)
      retval = open_ottodb(OTTO_SERVER);

   if(retval == OTTO_SUCCESS)
      log_cfg();

   if(retval == OTTO_SUCCESS)
      ottojob_log_job_layout();

   // fork and exec httpd
   if(retval == OTTO_SUCCESS && cfg.enable_httpd == OTTO_TRUE)
   {
      fork_httpd(argv);
   }

   if(retval == OTTO_SUCCESS)
   {
      if((ottosysd_socket = init_server_ipc(cfg.ottosysd_port, 32)) == -1)
         retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS)
      retval = ottosysd();

   delete_pidfile();

   return(retval);
}



void
ottosysd_exit(int signum)
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

   delete_pidfile();

   exit(-1);
}



int
ottosysd_getargs(int argc, char **argv)
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
            ottosysd_usage();
            break;
      }
   }

   return(retval);
}



void
ottosysd_usage(void)
{

   printf("   NAME\n");
   printf("      ottosysd - Otto job scheduler daemon\n");
   printf("\n");
   printf("   SYNOPSIS\n");
   printf("      ottosysd [-h]\n");
   printf("\n");
   printf("   DESCRIPTION\n");
   printf("      Executes and monitors the jobs in the Otto job database.\n");
   printf("\n");
   printf("      ottosysd is meant to be run as a server daemon.\n");
   printf("\n");
   printf("   OPTIONS\n");
   printf("      -h Print this help and exit.\n");
   exit(0);

}



int
ottosysd(void)
{
   int    retval = OTTO_SUCCESS;
   int    timeout;
   int    rc;
   int    new_sd = -1;
   int    end_server = OTTO_FALSE, compress_array = OTTO_FALSE;
   int    current_size = 0, i, j;

   // set signal handlers
   sig_set(SIGCLD,  finish_job);
   sig_set(SIGINT,  ottosysd_exit);

   // clean up before start
   compile_dependencies();
   check_dependencies();

   // Initialize the pollfd structure
   memset(fds, 0 , sizeof(fds));

   // Set up the initial listening socket
   fds[nfds].fd = ottosysd_socket;
   fds[nfds].events = POLLIN;
   nfds++;

   // Loop waiting for incoming connects, incoming data
   // on any connected sockets, or a timeout
   do
   {
      // get the number of milliseconds to the next wake time
      // technically this should be at zero seconds into the
      // minute but Otto only advertises support for timed
      // jobs at a 1 minute granularity and it's easier to
      // handle boundary conditions one second in so "wake
      // time" is always at HH:MM:01
      timeout = get_msec_to_next_wake_time();

      // Call poll()
      rc = poll(fds, nfds, timeout);

      // Check to see if the poll call failed.
      if(rc < 0)
      {
         if(errno == EINTR) // an interrupt isn't considered a failure
         {
            if(cfg.debug == OTTO_TRUE)
            {
               lprintf(logp, INFO, "errno == EINTR\n");
            }
            continue;
         }
         else
         {
            if(cfg.debug == OTTO_TRUE)
            {
               lprintf(logp, INFO, "poll failed\n");
            }
         }
         break;
      }

      // Check to see if the timer expired.
      if(rc == 0)
      {
         handle_timeout();
      }
      else
      {
         // One or more descriptors are readable.
         // Loop through to find the descriptors that returned POLLIN
         current_size = nfds;
         for(i=0; i<current_size; i++)
         {
            if(fds[i].revents == 0)
               continue;

            // If revents is not POLLIN, it's an unexpected result,
            // log it but continue
            if(fds[i].revents != POLLIN)
            {
               if(cfg.debug == OTTO_TRUE)
               {
                  lprintf(logp, INFO, "revents for connection %d %s%s%s", fds[i].fd,
                          fds[i].revents & POLLHUP  ? " POLLHUP"  : "",
                          fds[i].revents & POLLERR  ? " POLLERR"  : "",
                          fds[i].revents & POLLNVAL ? " POLLNVAL" : "");
               }
               if(fds[i].revents & POLLHUP || fds[i].revents & POLLERR)
               {
                  close(fds[i].fd);
                  fds[i].fd = -1;
                  compress_array = OTTO_TRUE;
               }
               continue;

            }

            // this descriptor has data
            if(i == 0)
            {
               // Accept all incoming connections that are
               // queued up on the listening socket before we
               // loop back and call poll again.
               do
               {
                  new_sd = accept(fds[0].fd, NULL, NULL);
                  if(new_sd < 0)
                  {
                     if(errno != EWOULDBLOCK)
                     {
                        if(cfg.debug == OTTO_TRUE)
                        {
                           lprintf(logp, INFO, "accept() failed");
                        }
                     }
                     break;
                  }

                  // Add the new incoming connection to the pollfd structure
                  if(cfg.debug == OTTO_TRUE)
                  {
                     lprintf(logp, INFO, "New incoming connection - %d\n", new_sd);
                  }
                  fds[nfds].fd     = new_sd;
                  fds[nfds].events = POLLIN;
                  nfds++;
               } while (new_sd != -1);
            }
            else
            {
               // this is a client
               // try to process data or determine if the socket needs to be closed
               switch(handle_ipc(fds[i].fd))
               {
                  case OTTO_CLOSE_CONN:
                     close(fds[i].fd);
                     fds[i].fd = -1;
                     compress_array = OTTO_TRUE;
                     break;
                  case OTTO_SUCCESS:
                     ottoipc_send_all(fds[i].fd);
                     break;
               }
            }
         }

         // clean up the pollfds array
         if(compress_array == OTTO_TRUE)
         {
            compress_array = OTTO_FALSE;
            for(i=0, j=0; i<nfds; i++, j++)
            {
               if(i != j)
                  fds[j].fd = fds[i].fd;
               if(fds[j].fd == -1)
                  j--;
            }
            nfds = j;
         }
      }

   } while (end_server == OTTO_FALSE); // End of serving running.

   return(retval);
}



int
get_msec_to_next_wake_time()
{
   int retval = 0;
   struct timeval now;
   time_t this_minute_in_seconds;
   time_t next_wake_time;

   this_minute_in_seconds = time(0);

   // the next wake time is the first second into the next minute
   next_wake_time = (((this_minute_in_seconds / 60) + 1) * 60) + 1;

   // get the time in microseconds right now
   gettimeofday(&now, 0);

   // compute how many milliseconds to wait for the next wake time
   retval  = (next_wake_time - now.tv_sec) * 1000;
   retval += (1000 - now.tv_usec/1000);

   if(cfg.debug == OTTO_TRUE)
      lprintf(logp, INFO, "msec to next wake time %d\n", retval);

   if(retval < 0 || retval > 61000)
   {
      lprintf(logp, INFO, "In msec to next wake time %d (%ld, %ld, %ld, %ld)\n",
              retval, this_minute_in_seconds, next_wake_time, now.tv_sec, now.tv_usec);
   }

   return(retval);
}



void
handle_timeout()
{
   time_t now;
   struct tm *parts;
   int jobwork_copied = OTTO_FALSE;
   int minutes_caught_up = 0;

   // Hold child signals while handling timer
   sig_hold(SIGCLD);

   now = time(0);
   parts = localtime(&now);

   // set last check variables if necessary
   if(last_check_min == -1)
   {
      last_check_day  = parts->tm_wday;
      last_check_hour = parts->tm_hour;
      last_check_min  = parts->tm_min - 1;
      if(last_check_min < 0)
      {
         last_check_min = 59;
         last_check_hour--;
         if(last_check_hour < 0)
         {
            last_check_hour = 23;
            last_check_day--;
            if(last_check_day < 0)
               last_check_day = 6;
         }
      }
   }

   if(cfg.debug == OTTO_TRUE)
   {
      lprintf(logp, INFO, "last_check_time = Day %d %02d:%02d  now = Day %d %02d:%02d\n",
              last_check_day, last_check_hour, last_check_min,
              parts->tm_wday, parts->tm_hour, parts->tm_min);
   }

   while(last_check_day  != parts->tm_wday ||
         last_check_hour != parts->tm_hour ||
         last_check_min  != parts->tm_min)
   {
      // increment last_check_variables
      last_check_min++;
      if(last_check_min > 59)
      {
         last_check_min = 0;
         last_check_hour++;
         if(last_check_hour > 23)
         {
            last_check_hour = 0;
            last_check_day++;
            if(last_check_day > 6)
               last_check_day = 0;
         }
      }

      if(cfg.debug == OTTO_TRUE)
      {
         lprintf(logp, INFO, "checking time Day %d %02d:%02d\n", last_check_day, last_check_hour, last_check_min);
      }

      // only do work if this minute on this day has a job to fire off
      if(check_times[last_check_day][last_check_hour] & (1L << last_check_min))
      {
         if(cfg.debug == OTTO_TRUE)
         {
            lprintf(logp, INFO, "something is scheduled for Day %d %02d:%02d\n", last_check_day, last_check_hour, last_check_min);
         }

         // only copy and sort on the first iteration
         if(jobwork_copied == OTTO_FALSE)
         {
            if(cfg.debug == OTTO_TRUE)
            {
               lprintf(logp, INFO, "copying jobwork\n");
            }

            copy_jobwork(&ctx);
            sort_jobwork(&ctx, BY_DATE_COND);
            jobwork_copied = OTTO_TRUE;
         }

         fire_this_minute(last_check_day, last_check_hour, last_check_min);
      }

      minutes_caught_up++;
   }

   if(minutes_caught_up > 1)
   {
      lprintf(logp, INFO, "Caught up %d minutes.", minutes_caught_up);
   }

   // Release the hold on the child signal to resume normal processing
   sig_rlse(SIGCLD);
}



void
fire_this_minute(int this_wday, int this_hour, int this_min)
{
   int i,id;
   int do_check = OTTO_FALSE;
   int expr_ret;

   // don't start any new jobs if the daemon is paused
   if(cfg.pause == OTTO_TRUE)
      return;

   // only do work if this hour on this minute has a job to fire off
   if(check_times[this_wday][this_hour] & (1L << this_min))
   {
      for(i=0; i<cfg.ottodb_maxjobs; i++)
      {
         if(cfg.debug == OTTO_TRUE)
         {
            lprintf(logp, INFO, "name %s date_conditions %d\n", ctx.job[i].name, ctx.job[i].date_conditions);
         }

         if(ctx.job[i].name[0] == '\0' || ctx.job[i].date_conditions == 0)
            break;

         id = ctx.job[i].id;

         if(job[id].days_of_week & (1L << this_wday))
         {

            switch(job[id].status)
            {
               case STAT_IN:
               case STAT_SU:
               case STAT_FA:
               case STAT_TE:
               case STAT_BR:
                  if(job[id].start_times[this_hour] & (1L << this_min))
                  {
                     expr_ret = OTTO_TRUE;
                     job[id].expr_fail = OTTO_FALSE;
                     if(job[id].expression[0] != '\0')
                     {
                        expr_ret = evaluate_expr(job[id].expression);
                     }
                     switch(expr_ret)
                     {
                        case OTTO_TRUE:
                           switch(job[id].type)
                           {
                              case OTTO_BOX:
                                 run_box(id);
                                 do_check = OTTO_TRUE;
                                 break;
                              case OTTO_CMD:
                                 run_job(id);
                                 break;
                           }
                           break;
                        case OTTO_FAIL:
                           job[id].expr_fail = OTTO_TRUE;
                           break;
                     }
                  }
                  break;
            }
         }
      }

      if(do_check == OTTO_TRUE)
      {
         check_dependencies();
      }
   }
}



int
handle_ipc(int fd)
{
   int retval         = OTTO_SUCCESS;
   int check_required = OTTO_FALSE;
   ottoipc_simple_pdu_st *pdu;

   // Hold child signals while reading from the socket
   sig_hold(SIGCLD);

   // read the incoming message
   if((retval = ottoipc_recv_all(fd)) == OTTO_SUCCESS)
   {
      if(ottoipc_dequeue_pdu((void **)&pdu) == OTTO_SUCCESS)
      {
         if(pdu->opcode > 0)
         {
            if(pdu->opcode != PING)
               log_received_pdu(pdu);

            if(pdu->opcode < CRUD_TOTAL)
            {
               check_required = handle_crud_pdu(pdu);
            }
            else if(pdu->opcode < SCHED_TOTAL)
            {
               check_required = handle_scheduler_pdu(pdu);
            }
            else
            {
               check_required = handle_daemon_pdu(pdu);
            }
            if(check_required == OTTO_TRUE)
            {
               compile_dependencies();
               check_dependencies();
            }
         }
      }
   }
   else
   {
      retval = OTTO_CLOSE_CONN;
   }

   // Release the hold on the child and alarm signals to resume normal processing
   sig_rlse(SIGCLD);

   return(retval);
}



int
handle_crud_pdu(ottoipc_simple_pdu_st *pdu)
{
   int check_required = OTTO_FALSE;

   switch(pdu->opcode)
   {
      case CREATE_JOB: create_job((ottoipc_create_job_pdu_st *)pdu, &ctx); break;
      case REPORT_JOB: report_job(pdu, &ctx); break;
      case UPDATE_JOB: update_job((ottoipc_update_job_pdu_st *)pdu, &ctx); break;
      case DELETE_JOB: delete_job(pdu, &ctx); break;
      case DELETE_BOX: delete_box(pdu, &ctx); break;
      default:
                       pdu->option = NOOP;
                       ottoipc_initialize_send();
                       ottoipc_enqueue_simple_pdu(pdu);
                       break;
   }

   if(pdu->opcode != REPORT_JOB)
   {
      set_levels();
      check_required = OTTO_TRUE;
   }

   return(check_required);
}



int
handle_scheduler_pdu(ottoipc_simple_pdu_st *pdu)
{
   int check_required = OTTO_FALSE;
   int id, jobwork_id, option;

   copy_jobwork(&ctx);
   sort_jobwork(&ctx, BY_NAME);

   jobwork_id = find_jobname(&ctx, pdu->name);
   if(jobwork_id >= 0 && jobwork_id < cfg.ottodb_maxjobs)
   {
      id = ctx.job[jobwork_id].id;

      // update pdu response in place then enqueue it to send
      option = pdu->option;
      pdu->option = ACK;
      switch(pdu->opcode)
      {
         case FORCE_START_JOB:    force_start_job (id);                       check_required = OTTO_TRUE; break;
         case START_JOB:          start_job       (id);                       check_required = OTTO_TRUE; break;
         case KILL_JOB:           kill_job        (id, SIGKILL);              check_required = OTTO_TRUE; break;
         case MOVE_JOB_TOP:       move_job        (id, pdu->opcode, option);                              break;
         case MOVE_JOB_UP:        move_job        (id, pdu->opcode, option);                              break;
         case MOVE_JOB_DOWN:      move_job        (id, pdu->opcode, option);                              break;
         case MOVE_JOB_BOTTOM:    move_job        (id, pdu->opcode, option);                              break;
         case RESET_JOB:          reset_job       (id);                       check_required = OTTO_TRUE; break;
         case SEND_SIGNAL:        kill_job        (id, option);               check_required = OTTO_TRUE; break;
         case CHANGE_STATUS:      set_job_status  (id, option);               check_required = OTTO_TRUE; break;
         case JOB_ON_AUTOHOLD:    set_job_autohold(id, pdu->opcode);          check_required = OTTO_TRUE; break;
         case JOB_OFF_AUTOHOLD:   set_job_autohold(id, pdu->opcode);          check_required = OTTO_TRUE; break;
         case JOB_ON_AUTONOEXEC:  set_job_autonoexec(id, pdu->opcode);        check_required = OTTO_TRUE; break;
         case JOB_OFF_AUTONOEXEC: set_job_autonoexec(id, pdu->opcode);        check_required = OTTO_TRUE; break;
         case JOB_ON_HOLD:        set_job_hold    (id, pdu->opcode);          check_required = OTTO_TRUE; break;
         case JOB_OFF_HOLD:       set_job_hold    (id, pdu->opcode);          check_required = OTTO_TRUE; break;
         case JOB_ON_NOEXEC:      set_job_noexec  (id, pdu->opcode);          check_required = OTTO_TRUE; break;
         case JOB_OFF_NOEXEC:     set_job_noexec  (id, pdu->opcode);          check_required = OTTO_TRUE; break;
         case BREAK_LOOP:         break_loop      (id);                       check_required = OTTO_TRUE; break;
         case SET_LOOP:           set_loop        (id, option, &pdu->option); check_required = OTTO_TRUE; break;
         default:                 pdu->option = NOOP;                                                     break;
      }
   }
   else
   {
      // update pdu response in place then enqueue it to send
      pdu->option = JOB_NOT_FOUND;
   }

   ottoipc_initialize_send();
   ottoipc_enqueue_simple_pdu(pdu);

   return(check_required);
}



int
handle_daemon_pdu(ottoipc_simple_pdu_st *pdu)
{
   int check_required = OTTO_FALSE;

   pdu->option = ACK;
   switch(pdu->opcode)
   {
      case PING:
         otto_sprintf(pdu->name, "ottosysd %s, %s, Debug %s, DB Layout v%d",
                      cfg.otto_version, cfg.pause ? "Paused" : "Running", cfg.debug ? "On" : "Off", cfg.ottodb_version);
         break;

      case VERIFY_DB:     otto_sprintf(pdu->name, "%ld", ottodb_inode);                        break;
      case DEBUG_ON:      cfg.debug = OTTO_TRUE;                                               break;
      case DEBUG_OFF:     cfg.debug = OTTO_FALSE;                                              break;
      case REFRESH:       compile_dependencies(&ctx);                                          break;
      case PAUSE_DAEMON:  cfg.pause = OTTO_TRUE;                                               break;
      case RESUME_DAEMON: cfg.pause = OTTO_FALSE;                  check_required = OTTO_TRUE; break;
      case STOP_DAEMON:   delete_pidfile(); exit(0);                                           break;
      default:            pdu->option = NOOP;                                                  break;
   }

   ottoipc_initialize_send();
   ottoipc_enqueue_simple_pdu(pdu);

   return(check_required);
}



void
check_dependencies()
{
   int box, i, id, recheck, expr_ret;

   recheck = OTTO_TRUE;
   while(recheck == OTTO_TRUE)
   {
      recheck = OTTO_FALSE;

      copy_jobwork(&ctx);
      sort_jobwork(&ctx, BY_ACTIVE);

      // jobwork is sorted by status (AC first), type (b then c)
      // linear search down the list of active jobs checking dependencies
      for(i=0; i<cfg.ottodb_maxjobs; i++)
      {
         if(ctx.job[i].name[0] == '\0' || ctx.job[i].status != STAT_AC)
            break;

         // make things easier to type
         id  = ctx.job[i].id;
         box = job[id].box;

         // only check jobs with no parent box or whose parent box is RUNNING
         if(box == -1 || (job[box].status == STAT_RU && job[box].loopstat != LOOP_BROKEN))
         {
            expr_ret = OTTO_TRUE;
            job[id].expr_fail = OTTO_FALSE;
            if(job[id].expression[0] != '\0')
            {
               expr_ret = evaluate_expr(job[id].expression);
            }
            switch(expr_ret)
            {
               case OTTO_TRUE:
                  // all dependencies are met so start the job
                  switch(job[id].type)
                  {
                     case OTTO_BOX:
                        run_box(id);
                        break;
                     case OTTO_CMD:
                        run_job(id);
                        break;
                  }

                  // only do additional checks if the daemon isn't paused
                  if(cfg.pause == OTTO_FALSE)
                     recheck = OTTO_TRUE;
                  break;
               case OTTO_FAIL:
                  job[id].expr_fail = OTTO_TRUE;
                  break;
            }
         }
      }
   }
}



void
compile_dependencies()
{
   int d, h, i, my_id;
   char minutes[1024], minute[10];
   int dmin;

   copy_jobwork(&ctx);
   sort_jobwork(&ctx, BY_NAME);

   // clear global areray of start times
   for(d=0; d<7; d++)
   {
      for(h=0; h<24; h++)
      {
         check_times[d][h] = 0;
      }
   }

   // compile new expressions
   for(i=0; i<cfg.ottodb_maxjobs; i++)
   {
      if(ctx.job[i].name[0] == '\0')
         break;

      my_id = ctx.job[i].id;
      memset(job[my_id].expression, 0, CNDLEN+1);

      if(ctx.job[i].condition[0] != '\0')
      {
         job[my_id].expr_fail = OTTO_FALSE;
         if(compile_expression(job[my_id].expression, &ctx, ctx.job[i].condition, CNDLEN) == OTTO_FAIL)
         {
            job[my_id].expr_fail = OTTO_TRUE;
         }
      }

      // add times to global array
      if(ctx.job[i].date_conditions != 0)
      {
         // loop over days
         for(d=0; d<7; d++)
         {
            if(ctx.job[i].days_of_week & (1L << d))
            {
               // bitwise OR the hours into the global array for this day
               for(h=0; h<24; h++)
               {
                  if(ctx.job[i].start_times[h] != 0)
                  {
                     check_times[d][h] |= ctx.job[i].start_times[h];
                     if(cfg.debug == OTTO_TRUE)
                     {
                        lprintf(logp, INFO, "Adding Day %d %02d to check_times for %s\n", d, h, ctx.job[i].name);
                        minutes[0] = '\0';
                        for(dmin=0; dmin<60; dmin++)
                        {
                           if(check_times[d][h] & (1L << dmin))
                           {
                              otto_sprintf(minute, " %d", dmin);
                              strcat(minutes, minute);
                           }
                        }
                        lprintf(logp, INFO, "check_times[%d][%d] minutes now%s\n", d, h, minutes);
                     }
                  }
               }
            }
         }
      }
   }
}



void
activate_box(int id)
{
   // only activate the box if it's not ON_HOLD
   if(job[id].status != STAT_OH)
   {
      // mark box as ACTIVATED
      job[id].status = STAT_AC;

      // handle box looping
      if(job[id].loopname[0] != '\0')
      {
         if(job[id].loopnum < job[id].loopmin ||
            job[id].loopnum > job[id].loopmax)
         {
            // reset the box's loop counter if it's >= the loop limit
            job[id].loopnum    = job[id].loopmin -1;
            job[id].loopstat   = LOOP_NOT_RUNNING;
            job[id].start      = 0;
            job[id].finish     = 0;
            job[id].duration   = 0;
         }
      }
      else
      {
         job[id].start    = 0;
         job[id].finish   = 0;
         job[id].duration = 0;
      }

      // activate the box's chain
      activate_box_chain(job[id].head);
   }
}



void
activate_box_chain(int id)
{
   while(id != -1)
   {
      switch(job[id].type)
      {
         case OTTO_BOX:
            // only activate the job if it's not ON_HOLD
            if(job[id].status != STAT_OH)
            {
               // mark box as ACTIVATED
               job[id].status     = STAT_AC;
               job[id].start      = 0;
               job[id].finish     = 0;
               job[id].duration   = 0;
               job[id].loopnum    = job[id].loopmin -1;
               job[id].loopstat   = LOOP_NOT_RUNNING;

               // activate the box's chain
               activate_box_chain(job[id].head);
            }
            break;
         case OTTO_CMD:
            if(job[id].status != STAT_OH)
            {
               job[id].pid      = 0;
               job[id].start    = 0;
               job[id].finish   = 0;
               job[id].duration = 0;
               if(job[id].on_autohold == OTTO_TRUE)
                  job[id].status   = STAT_OH;
               else
                  job[id].status   = STAT_AC;
            }
            break;
      }

      id = job[id].next;
   }
}



// slightly different behavior from a standard activation
// always activate the top box
void
force_activate_box(int id)
{
   // mark box as ACTIVATED
   job[id].status = STAT_AC;

   // reset all stats
   job[id].loopstat   = LOOP_NOT_RUNNING;
   job[id].start      = 0;
   job[id].finish     = 0;
   job[id].duration   = 0;

   // and force activate the box's chain
   force_activate_box_chain(job[id].head);
}



// slightly different behavior from a standard activation
// reset all status fields to 0 but leave OH status
void
force_activate_box_chain(int id)
{
   while(id != -1)
   {
      switch(job[id].type)
      {
         case OTTO_BOX:
            // only activate the job if it's not ON_HOLD
            if(job[id].status != STAT_OH)
            {
               // handle autohold
               if(job[id].on_autohold == OTTO_TRUE)
                  job[id].status   = STAT_OH;
               else
                  job[id].status   = STAT_AC;
            }
            // but always reset values
            job[id].start      = 0;
            job[id].finish     = 0;
            job[id].duration   = 0;
            job[id].loopstat   = LOOP_NOT_RUNNING;

            // and force activate the box's chain
            force_activate_box_chain(job[id].head);
            break;

         case OTTO_CMD:
            // only activate the job if it's not ON_HOLD
            if(job[id].status != STAT_OH)
            {
               // handle autohold
               if(job[id].on_autohold == OTTO_TRUE)
                  job[id].status   = STAT_OH;
               else
                  job[id].status   = STAT_AC;
            }
            // but always reset values
            job[id].pid      = 0;
            job[id].start    = 0;
            job[id].finish   = 0;
            job[id].duration = 0;
            break;
      }

      id = job[id].next;
   }
}



void
finish_box(int box_id, time_t finish)
{
   int fail_found=OTTO_FALSE, id, jobs_finished=OTTO_TRUE;

   id = job[box_id].head;

   while(id != -1)
   {
      if(job[id].status != STAT_SU &&
         job[id].status != STAT_FA &&
         job[id].status != STAT_TE &&
         job[id].status != STAT_BR)
      {
         jobs_finished = OTTO_FALSE;
      }

      if(job[id].status == STAT_FA ||
         job[id].status == STAT_TE ||
         job[id].status == STAT_BR)
      {
         fail_found = OTTO_TRUE;
      }

      id = job[id].next;
   }

   if(jobs_finished == OTTO_TRUE)
   {
      switch(job[box_id].loopstat)
      {
         case LOOP_RUNNING:
            printf("Loop running for %s\n", job[box_id].name);
            if(fail_found == OTTO_TRUE)
            {
               job[box_id].status = STAT_FA;
            }
            else
            {
               job[box_id].loopnum++;

               if(job[box_id].loopnum <= job[box_id].loopmax)
               {
                  // this box is looping and the limit hasn't been reached yet
                  // activate the box again
                  activate_box(box_id);

                  // and set its status back to RU so check dependencies picks it up
                  job[box_id].status = STAT_RU;
               }
               else
               {
                  job[box_id].status = STAT_SU;

                  job[box_id].finish = finish;
                  if(job[box_id].start != 0)
                     job[box_id].duration = finish - job[box_id].start;

                  if(job[box_id].box != -1)
                     finish_box(job[box_id].box, finish);
               }
            }
            break;

         case LOOP_BROKEN:
            printf("Loop broken for %s\n", job[box_id].name);
            job[box_id].status = STAT_BR;
            job[box_id].finish = finish;
            if(job[box_id].start != 0)
               job[box_id].duration = finish - job[box_id].start;

            if(job[box_id].box != -1)
               finish_box(job[box_id].box, finish);
            break;

         default:
            printf("Default for %s\n", job[box_id].name);
            job[box_id].loopstat = LOOP_NOT_RUNNING;
            if(fail_found == OTTO_TRUE)
               job[box_id].status = STAT_FA;
            else
               job[box_id].status = STAT_SU;

            job[box_id].finish = finish;
            if(job[box_id].start != 0)
               job[box_id].duration = finish - job[box_id].start;

            if(job[box_id].box != -1)
               finish_box(job[box_id].box, finish);
            break;
      }
   }
}



void
run_box(int id)
{
   int loop_just_started = OTTO_FALSE;

   // don't start any new jobs if the daemon is paused
   if(cfg.pause == OTTO_TRUE)
      return;

   // set the loop iterator to loopmin and set the state to RUNNING if necessary
   if(job[id].loopname[0] != '\0' && job[id].loopstat == LOOP_NOT_RUNNING)
   {
      if(job[id].loopnum < job[id].loopmin || job[id].loopnum > job[id].loopmax)
         job[id].loopnum = job[id].loopmin;
      job[id].loopstat = LOOP_RUNNING;
      loop_just_started = OTTO_TRUE;
   }

   // only set the start time on a box if it's not looping or it's the first iteration
   if(job[id].loopname[0] == '\0' || loop_just_started == OTTO_TRUE)
      job[id].start = time(0);
   job[id].finish   = 0;
   job[id].duration = 0;
   job[id].status   = STAT_RU;

   // activate the box's chain
   activate_box_chain(job[id].head);

}



void
run_job(int id)
{
   pid_t pid;
   char otto_jobname[NAMLEN+15];
   char otto_boxname[NAMLEN+15];
   char loopname[NAMLEN+15];
   int  box, fd;
   char *s, tmpenv[ENVLEN+1];
   int rc;

   // don't start any new jobs if the daemon is paused
   if(cfg.pause == OTTO_TRUE)
      return;

   if(job[id].on_noexec == OTTO_TRUE)
   {
      job[id].pid      = 0;
      job[id].start    = time(0);
      job[id].finish   = job[id].start;
      job[id].duration = 0;
      job[id].status   = STAT_SU;
      lprintf(logp, INFO, "%s started (on_noexec)\n", job[id].name);
      if(job[id].box != -1)
         finish_box(job[id].box, job[id].finish);

      return;
   }

   // rebuild the environment to be passed to the child job (if necessary)
   // Including HOME, LOGNAME, USER, PATH, and "envvar" lines read from OTTOCFG and OTTOENV
   rebuild_environment();

   pid = fork();       
   if(pid > 0)
   {
      // Make child process the leader of its own process group. This allows
      // signals to also be delivered to processes forked by the child process.
      job[id].pid         = pid;
      job[id].start       = time(0);
      job[id].finish      = 0;
      job[id].duration    = 0;
      job[id].exit_status = 0;
      job[id].status      = STAT_RU;
      setpgid(pid, 0); 
      lprintf(logp, INFO, "%s started\n", job[id].name);
   }
   else
   {
      if(pid < 0)
      {
         lprintf(logp, MAJR, "Couldn't create a child process for %s.\n", job[id].name);
      }
      else
      { 
         // change directory to $HOME before overlaying the process since PWD is inherited
         chdir(getenv("HOME"));

         // add the job name at the top of the list
         otto_sprintf(otto_jobname, "OTTO_JOBNAME=%s", job[id].name);
         cfg.envvar[0] = otto_jobname;

         // add the box name to the list
         if(job[id].box_name[0] != '\0')
         {
            otto_sprintf(otto_boxname, "OTTO_BOXNAME=%s", job[id].box_name);
            cfg.envvar[cfg.n_envvar] = otto_boxname;
            cfg.n_envvar++;
         }

         // ascend through parent boxes and add environment variables for each parent box
         box = job[id].box;
         while(box > -1)
         {
            // add per-job environment variables for the box
            if(job[box].environment[0] != '\0')
            {
               s = job[box].environment;
               s = copy_envvar_assignment(&rc, tmpenv, sizeof(tmpenv), s);
               while(tmpenv[0] != '\0')
               {
                  if((cfg.envvar[cfg.n_envvar] = strdup(tmpenv)) == NULL)
                  {
                     lprintf(logp, MAJR, "Error mallocing envvar %d.\n", cfg.n_envvar);
                  }
                  else
                  {
                     cfg.n_envvar++;
                  }

                  s = copy_envvar_assignment(&rc, tmpenv, sizeof(tmpenv), s);
               }
            }
            // also add environment variables for the box if it's looping
            if(job[box].loopname[0] != '\0')
            {
               otto_sprintf(loopname, "%s=%d", job[box].loopname, (int)(job[box].loopnum * job[box].loopsgn));
               if((cfg.envvar[cfg.n_envvar] = strdup(loopname)) == NULL)
               {
                  lprintf(logp, MAJR, "Error mallocing envvar %d.\n", cfg.n_envvar);
               }
               else
               {
                  cfg.n_envvar++;
               }
            }
            box = job[box].box;
         }

         // finally add per-job environment variables
         if(job[id].environment[0] != '\0')
         {
            s = job[id].environment;
            s = copy_envvar_assignment(&rc, tmpenv, sizeof(tmpenv), s);
            while(tmpenv[0] != '\0')
            {
               if((cfg.envvar[cfg.n_envvar] = strdup(tmpenv)) == NULL)
               {
                  lprintf(logp, MAJR, "Error mallocing envvar %d.\n", cfg.n_envvar);
               }
               else
               {
                  cfg.n_envvar++;
               }

               s = copy_envvar_assignment(&rc, tmpenv, sizeof(tmpenv), s);
            }
         }

         // ensure file descriptors are closed on exec
         fcntl(0,               F_SETFD, FD_CLOEXEC);  // stdin
         fcntl(1,               F_SETFD, FD_CLOEXEC);  // stdout
         fcntl(2,               F_SETFD, FD_CLOEXEC);  // stderr
         fcntl(ottodbfd,        F_SETFD, FD_CLOEXEC);  // otto database

         for(fd=0; fd<nfds; fd++)
            fcntl(fds[fd].fd, F_SETFD, FD_CLOEXEC);    // server and client sockets

         // overlay a shell to execute the command
         execle("/bin/sh", "/bin/sh", "-c", job[id].command, NULL, cfg.envvar);

         lprintf(logp, MAJR, "execle failed for %s.\n", job[id].name);
      }
   }
}



void
activate_job(int id)
{
   // only activate the job if it's not ON_HOLD and not already running
   if(job[id].status != STAT_OH &&
      job[id].status != STAT_RU)
   {
      // mark job as ACTIVATED
      job[id].status   = STAT_AC;
      job[id].start    = 0;
      job[id].finish   = 0;
      job[id].duration = 0;
   }
}



void
finish_job(int sigNum)
{
   int i, id;
   int status;
   pid_t pid;

   sig_hold(SIGCLD);

   copy_jobwork(&ctx);
   sort_jobwork(&ctx, BY_PID);

   while((pid = waitpid(-1, &status, WNOHANG)) > 0)
   {
      for(i=0; i<cfg.ottodb_maxjobs; i++)
      {
         if(ctx.job[i].name[0] == '\0')
            break;

         if(ctx.job[i].pid == pid)
         {
            id = ctx.job[i].id;
            if(WIFEXITED(status))
            {
               job[id].exit_status = WEXITSTATUS(status);
            }
            else if(WIFSIGNALED(status))
            {
               job[id].exit_status = WTERMSIG(status);
            }
            job[id].pid = 0;
            job[id].finish = time(0);
            job[id].duration = job[id].finish - job[id].start;
            if(job[id].exit_status == 0)
               job[id].status = STAT_SU;
            else
               job[id].status = STAT_FA;
            lprintf(logp, INFO, "%s ended with return code %d, duration %02ld:%02ld:%02ld\n", job[id].name, status,
            job[id].duration / 3600, job[id].duration / 60 % 60, job[id].duration % 60);

            finish_box(job[id].box, job[id].finish);

            check_dependencies();
            break;
         }
      }
   }

   sig_rlse(SIGCLD);
}



void
set_job_status(int id, int status)
{
   switch(status)
   {
      case FAILURE:
         job[id].status = STAT_FA;
         job[id].finish = time(0);
         if(job[id].start != 0)
            job[id].duration = job[id].finish - job[id].start;
         finish_box(job[id].box, job[id].finish);
         break;
      case INACTIVE:
         job[id].status = STAT_IN;
         break;
      case RUNNING:
         job[id].status = STAT_RU;
         break;
      case SUCCESS:
         job[id].status = STAT_SU;
         job[id].finish = time(0);
         if(job[id].start != 0)
            job[id].duration = job[id].finish - job[id].start;
         finish_box(job[id].box, job[id].finish);
         break;
      case TERMINATED:
         job[id].status = STAT_TE;
         job[id].finish = time(0);
         if(job[id].start != 0)
            job[id].duration = job[id].finish - job[id].start;
         finish_box(job[id].box, job[id].finish);
         break;
   }
}



void
set_job_autohold(int id, int action)
{
   switch(action)
   {
      // just set this one job on autohold no matter what type it is
      case JOB_ON_AUTOHOLD:
         job[id].on_autohold = OTTO_TRUE;
         break;

      case JOB_OFF_AUTOHOLD:
         // just set this one job off autohold no matter what type it is
         job[id].on_autohold = OTTO_FALSE;
         break;
      default:
         break;
   }
}



void
set_job_autonoexec(int id, int action)
{
   switch(action)
   {
      // just set this one job on autonoexec no matter what type it is
      case JOB_ON_AUTONOEXEC:
         job[id].autonoexec = OTTO_TRUE;
         break;

      case JOB_OFF_AUTONOEXEC:
         // just set this one job off autonoexec no matter what type it is
         job[id].autonoexec = OTTO_FALSE;
         break;
      default:
         break;
   }
}



void
set_job_hold(int id, int action)
{
   switch(action)
   {
      case JOB_ON_HOLD:
         // just set this one job on hold no matter what type it is
         if(job[id].status != STAT_OH && job[id].status != STAT_RU)
         {
            job[id].status = STAT_OH;
         }
         break;

      case JOB_OFF_HOLD:
         if(job[id].status == STAT_OH)
         {
            // check status of parent box
            switch(job[job[id].box].status)
            {
               case STAT_RU:
               case STAT_AC:
                  switch(job[id].type)
                  {
                     case OTTO_BOX:
                        // take the box off hold
                        job[id].status = STAT_AC;
                        // then activate the rest of the box
                        activate_box(id);
                        break;
                     case OTTO_CMD:
                        job[id].status = STAT_AC;
                        break;
                  }
                  break;
               default:
                  job[id].status = STAT_IN;
                  break;
            }
         }
         break;

      default:
         break;
   }
}



void
set_job_noexec(int id, int action)
{
   switch(action)
   {
      case JOB_ON_NOEXEC:
         if(job[id].on_noexec != OTTO_TRUE && job[id].status != STAT_RU)
         {
            job[id].on_noexec = OTTO_TRUE;

            if(job[id].type == OTTO_BOX)
               set_job_noexec_chain(action, job[id].head);
         }
         break;

      case JOB_OFF_NOEXEC:
         if(job[id].on_noexec == OTTO_TRUE)
         {
            // check whether parent box is on noexec
            if(job[id].box == -1 || job[job[id].box].on_noexec != OTTO_TRUE)
            {
               job[id].on_noexec = OTTO_FALSE;

               if(job[id].type == OTTO_BOX)
                  set_job_noexec_chain(action, job[id].head);
            }
         }
         break;

      default:
         break;
   }
}



void
set_job_noexec_chain(int id, int action)
{
   while(id != -1)
   {
      switch(action)
      {
         case JOB_ON_NOEXEC:
            job[id].on_noexec = OTTO_TRUE;
            break;

         case JOB_OFF_NOEXEC:
            job[id].on_noexec = OTTO_FALSE;
            break;

         default:
            break;
      }

      if(job[id].type == OTTO_BOX)
         set_job_noexec_chain(action, job[id].head);

      id = job[id].next;
   }
}



void
break_loop(int id)
{
   if(job[id].type == OTTO_BOX && job[id].loopname[0] != '\0')
   {
      job[id].loopstat = LOOP_BROKEN;
   }
}



void
set_loop(int id, int iterator, uint8_t *response)
{
   if(job[id].type == OTTO_BOX)
   {
      if(job[id].loopname[0] != '\0')
      {
         if(iterator >= job[id].loopmin && iterator <= job[id].loopmax)
         {
            job[id].loopnum = iterator;
         }
         else
         {
            *response = ITERATOR_OUT_OF_BOUNDS;
         }
      }
      else
      {
         *response = BOX_HAS_NO_LOOP;
      }
   }
   else
   {
      *response = JOB_IS_NOT_A_BOX;
   }
}



void
reset_job(int id)
{
   if(check_reset(id) == OTTO_TRUE)
   {
      job[id].on_autohold = job[id].autohold;
      job[id].on_noexec   = job[id].autonoexec;
      job[id].status      = STAT_IN;
      job[id].pid         = 0;
      job[id].exit_status = 0;
      job[id].start       = 0;
      job[id].finish      = 0;
      job[id].duration    = 0;
      job[id].loopnum     = job[id].loopmin - 1;
      job[id].loopstat    = LOOP_NOT_RUNNING;

      if(job[id].type == OTTO_BOX)
         reset_job_chain(job[id].head);
   }
}



void
reset_job_chain(int id)
{
   while(id != -1)
   {
      job[id].on_autohold = job[id].autohold;
      job[id].on_noexec   = job[id].autonoexec;
      job[id].status      = STAT_IN;
      job[id].pid         = 0;
      job[id].exit_status = 0;
      job[id].start       = 0;
      job[id].finish      = 0;
      job[id].duration    = 0;
      job[id].loopnum     = job[id].loopmin - 1;
      job[id].loopstat    = LOOP_NOT_RUNNING;

      if(job[id].type == OTTO_BOX)
         reset_job_chain(job[id].head);

      id = job[id].next;
   }
}



int
check_reset(int id)
{
   int retval = OTTO_TRUE;

   if(job[id].status == STAT_RU)
   {
      retval = OTTO_FALSE;
   }
   else
   {
      if(job[id].type == OTTO_BOX)
         retval = check_reset_chain(job[id].head);
   }

   return(retval);
}



int
check_reset_chain(int id)
{
   int retval = OTTO_TRUE;

   while(retval == OTTO_TRUE && id != -1)
   {
      if(job[id].status == STAT_RU)
      {
         retval = OTTO_FALSE;
      }
      else
      {
         if(job[id].type == OTTO_BOX)
            retval = check_reset_chain(job[id].head);
      }

      id = job[id].next;
   }

   return(retval);
}



void
force_start_job(int id)
{
   // don't start any new jobs if the daemon is paused
   if(cfg.pause == OTTO_TRUE)
      return;

   switch(job[id].type)
   {
      case OTTO_BOX:
         run_box(id);
         break;
      case OTTO_CMD:
         run_job(id);
         break;
   }
}



void
start_job(int id)
{
   // don't start any new jobs if the daemon is paused
   if(cfg.pause == OTTO_TRUE)
      return;

   if(job[id].status != STAT_OH &&
      job[id].status != STAT_RU)
   {
      switch(job[id].type)
      {
         case OTTO_BOX: activate_box(id); break;
         case OTTO_CMD: activate_job(id); break;
      }
   }
}



void
kill_job(int id, int signal)
{
   if(job[id].pid > 0)
   {
      // send signal to pid
      kill((-1*job[id].pid), signal);
   }
}



void
move_job(int id, int direction, int count)
{
   int steps  = cfg.ottodb_maxjobs; // assume the job will move the entire length of the DB
   int new_prev, new_next;

   lprintf(logp, INFO, "move_job. received count %d.\n", count);

   switch(direction)
   {
      case MOVE_JOB_UP:
         steps = count;    // set steps to pdu value and fall through
      case MOVE_JOB_TOP:
         while(steps > 0 && job[id].prev != -1)
         {
            // save new siblings
            new_next = job[id].prev;
            new_prev = job[new_next].prev;

            // remove job from list
            job[job[id].prev].next = job[id].next;
            job[job[id].next].prev = job[id].prev;

            // insert job between new siblings
            job[new_prev].next = id;
            job[id].prev       = new_prev;
            job[id].next       = new_next;
            job[new_next].prev = id;

            // update parent head/tail if necessary
            if(job[job[id].box].head == job[id].next)
               job[job[id].box].head = id;
            if(job[job[id].box].tail == id)
               job[job[id].box].tail = job[id].next;
            steps--;
         }
         break;
      case MOVE_JOB_DOWN:
         steps = count;    // set steps to pdu value and fall through
      case MOVE_JOB_BOTTOM:
         while(steps > 0 && job[id].next != -1)
         {
            // save new siblings
            new_prev = job[id].next;
            new_next = job[new_prev].next;

            // remove job from list
            job[job[id].prev].next = job[id].next;
            job[job[id].next].prev = job[id].prev;

            // insert job between new siblings
            job[new_prev].next = id;
            job[id].prev       = new_prev;
            job[id].next       = new_next;
            job[new_next].prev = id;

            // update parent head/tail if necessary
            if(job[job[id].box].head == id)
               job[job[id].box].head = job[id].prev;
            if(job[job[id].box].tail == job[id].prev)
               job[job[id].box].tail = id;
            steps--;
         }
         break;
   }
}



void
set_levels(void)
{
   copy_jobwork(&ctx);

   set_levels_chain(root_job->head, 0);

   save_jobwork(&ctx);
}



void
set_levels_chain(int id, int level)
{
   while(id != -1)
   {
      ctx.job[id].level = level;
      if(ctx.job[id].type == OTTO_BOX)
         set_levels_chain(ctx.job[id].head, level+1);

      id = ctx.job[id].next;
   }
}




int
write_pidfile()
{
   int retval = OTTO_SUCCESS;
   FILE *fp;

   char fname[PATH_MAX];

   strcpy(fname, getenv("OTTOLOG"));
   strcat(fname, "/ottosysd.pid");

   if((fp = fopen(fname, "w")) != NULL)
   {
      fprintf(fp, "%ld", (int64_t)getpid());
      fclose(fp);
   }
   else
   {
      retval = OTTO_FAIL;
   }

   return(retval);
}



void
delete_pidfile()
{
   char fname[PATH_MAX];

   strcpy(fname, getenv("OTTOLOG"));
   strcat(fname, "/ottosysd.pid");

   unlink(fname);
}



int
fork_httpd(char **argv)
{
   int pid = (int)fork();

   if(pid == 0)
   {
      char *name = strstr(argv[0], "ottosysd");
      memcpy(name, "ottohttp", 8);
      ottohttpd();
   }

   return(pid);
}



int
ottohttpd(void)
{
   int    retval = OTTO_SUCCESS;
   int    rc;
   int    new_sd = -1;
   int    end_server = OTTO_FALSE, compress_array = OTTO_FALSE;
   int    current_size = 0, i, j;

   // linux only set ottohttpd to die when ottosysd dies
   prctl(PR_SET_PDEATHSIG, SIGHUP);

   RECVBUF rbuf[NPOLLFDS];

   // open a log file
   if(retval == OTTO_SUCCESS)
      if((hlogp = ottolog_init(cfg.env_ottolog, "ottohttpd", INFO, 0)) == NULL)
         retval = OTTO_FAIL;

   // open the httpd server socket
   if(retval == OTTO_SUCCESS)
   {
      if((ottohttpd_socket = init_server_ipc(cfg.httpd_port, 32)) == -1)
         retval = OTTO_FAIL;
   }

   // allocate a workspace
   memset(&hjoblist, 0, sizeof(hjoblist));
   if(retval == OTTO_SUCCESS &&
      (hjoblist.item = (JOB *)calloc(cfg.ottodb_maxjobs, sizeof(JOB))) == NULL)
   {
      retval = OTTO_FAIL;
   }

   // Initialize the pollfd structure
   memset(hfds, 0 , sizeof(hfds));
   memset(rbuf, 0 , sizeof(rbuf));

   // Set up the initial listening socket
   hfds[nhfds].events = POLLIN;
   hfds[nhfds].fd     = ottohttpd_socket;
   rbuf[nhfds].fd     = hfds[nhfds].fd;;
   nhfds++;

   // Loop waiting for incoming connects or incoming data
   // on any connected sockets
   do
   {
      // Call poll()
      rc = poll(hfds, nhfds, -1);

      // Check to see if the poll call failed.
      if(rc < 0)
      {
         if(errno == EINTR) // an interrupt isn't considered a failure
         {
            if(cfg.debug == OTTO_TRUE)
            {
               lprintf(hlogp, INFO, "errno == EINTR\n");
            }
            continue;
         }
         else
         {
            if(cfg.debug == OTTO_TRUE)
            {
               lprintf(hlogp, INFO, "poll failed\n");
            }
         }
         break;
      }

      // Check to see if the timer expired.
      if(rc == 0)
      {
         continue;
      }
      else
      {
         // One or more descriptors are readable.
         // Loop through to find the descriptors that returned POLLIN
         current_size = nhfds;
         for(i=0; i<current_size; i++)
         {
            if(hfds[i].revents == 0)
               continue;

            // If revents is not POLLIN, it's an unexpected result,
            // log it but continue
            if(hfds[i].revents != POLLIN)
            {
               if(cfg.debug == OTTO_TRUE)
               {
                  lprintf(hlogp, INFO, "revents for connection %d %s%s%s", hfds[i].fd,
                          hfds[i].revents & POLLHUP  ? " POLLHUP"  : "",
                          hfds[i].revents & POLLERR  ? " POLLERR"  : "",
                          hfds[i].revents & POLLNVAL ? " POLLNVAL" : "");
               }
               if(hfds[i].revents & POLLHUP || hfds[i].revents & POLLERR)
               {
                  close(hfds[i].fd);
                  hfds[i].fd = -1;
                  compress_array = OTTO_TRUE;
               }
               continue;

            }

            // this descriptor has data
            if(i == 0)
            {
               // Accept all incoming connections that are
               // queued up on the listening socket before we
               // loop back and call poll again.
               do
               {
                  new_sd = accept(hfds[0].fd, NULL, NULL);
                  if(new_sd < 0)
                  {
                     if(errno != EWOULDBLOCK)
                     {
                        if(cfg.debug == OTTO_TRUE)
                        {
                           lprintf(hlogp, INFO, "accept() failed");
                        }
                     }
                     break;
                  }

                  // Add the new incoming connection to the pollfd structure
                  if(cfg.debug == OTTO_TRUE)
                  {
                     lprintf(hlogp, INFO, "New incoming connection - %d\n", new_sd);
                  }
                  hfds[nhfds].events = POLLIN;
                  hfds[nhfds].fd     = new_sd;
                  memset(&rbuf[nhfds], 0, sizeof(RECVBUF));
                  rbuf[nhfds].fd     = hfds[nhfds].fd;
                  nhfds++;
               } while (new_sd != -1);
            }
            else
            {
               // this is a client
               // try to process data or determine if the socket needs to be closed
               switch(handle_http(&rbuf[i]))
               {
                  case OTTO_CLOSE_CONN:
                     close(hfds[i].fd);
                     hfds[i].fd = -1;
                     compress_array = OTTO_TRUE;
                     break;
                  case OTTO_SUCCESS:
                     ; // do nothing yet
                     break;
               }
            }
         }

         // clean up the pollfds array
         if(compress_array == OTTO_TRUE)
         {
            compress_array = OTTO_FALSE;
            for(i=0, j=0; i<nhfds; i++, j++)
            {
               if(i != j)
               {
                  hfds[j].fd = hfds[i].fd;
                  memcpy(&rbuf[j], &rbuf[i], sizeof(RECVBUF));
               }
               if(hfds[j].fd == -1)
                  j--;
            }
            nhfds = j;
         }
      }

   } while (end_server == OTTO_FALSE); // End of serving running.

   return(retval);
}



int
handle_http(RECVBUF *recvbuf)
{
   int retval = OTTO_SUCCESS;
   char buf[BUFSIZE];
   char method[BUFSIZE];
   char uri[BUFSIZE];
   char version[BUFSIZE];
   char jobname[BUFSIZE];
   int type;
   int report_level;
   int rc;
   ottohtml_query q;
   char filename[BUFSIZE];                   // path derived from uri
   char *p;                                  // temporary pointer
   struct stat sbuf;                         // file status
   int fd;                                   // static content filedes


   // get the HTTP request line
   if((rc = ottoipc_recv_str(buf, BUFSIZE, recvbuf)) > 0)
   {
      // trim trailing \r\n
      p = &buf[rc-1];
      while(p != buf)
      {
         if(*p == '\r' || *p == '\n')
            *p = '\0';
         else
            break;

         p--;
      }

      sscanf(buf, "%s %s %s", method, uri, version);

      lprintf(hlogp, INFO, "Received '%s'\n", buf);

      // read (and ignore) the HTTP headers
      while((rc = ottoipc_recv_str(buf, BUFSIZE, recvbuf)) > 0)
      {
         if(strcmp(buf, "\r\n") == 0)
            break;
      }

      if(rc >= 0)
      {
         // only support the GET method
         if(strcasecmp(method, "GET"))
         {
            ottohtml_send_error(recvbuf->fd, method, "501", "Not Implemented", "ottohttpd does not implement this method");
            return(OTTO_SUCCESS);
         }
         else
         {
            // unescape the uri
            unescape_input(uri);

            // strip base url
            if(strncmp(uri, cfg.base_url, strlen(cfg.base_url)) == 0)
            {
               // check for slash after base url
               if(uri[strlen(cfg.base_url)] == '/')
                  strcpy(uri, &uri[strlen(cfg.base_url)+1]);
               else
                  strcpy(uri, &uri[strlen(cfg.base_url)]);
            }

            // fix missing index.html
            if(uri[0] == '\0')
               strcpy(uri, "index.html");

            // parse the uri
            ottohtml_parse_uri(&q, uri);

            // determine if the uri is an endpoint or a file
            // look for known endpoints
            type = OTTO_UNKNOWN;
            if(strcmp(q.endpoint, "detail") == 0)
            {
               // /job                = jd %
               // /job?name=string    = jd string
               type = OTTO_DETAIL;
            }
            if(strcmp(q.endpoint, "query") == 0)
            {
               // /jil                = jq %
               // /jil?name=string    = jq string
               type = OTTO_QUERY;
            }
            if(strcmp(q.endpoint, "summary") == 0)
            {
               // /status             = jr %
               // /status?name=string = jr string
               type = OTTO_SUMMARY;
            }
            if(strcmp(q.endpoint, "mspdi") == 0)
            {
               // /status             = jr %
               // /status?name=string = jr string
               type = OTTO_EXPORT_MSP;
            }
            if(strcmp(q.endpoint, "csv") == 0)
            {
               // /status             = jr %
               // /status?name=string = jr string
               type = OTTO_EXPORT_CSV;
            }
            if(strcmp(q.endpoint, "version") == 0)
            {
               // /status             = jr %
               // /status?name=string = jr string
               type = OTTO_VERSION;
            }

            switch(type)
            {
               case OTTO_DETAIL:
               case OTTO_QUERY:
               case OTTO_SUMMARY:
               case OTTO_EXPORT_MSP:
               case OTTO_EXPORT_CSV:
               case OTTO_VERSION:
                  // a known endpoint was found
                  // generate endpoint output
                  if(type != OTTO_VERSION)
                  {
                     strcpy(jobname, "%");
                     if(q.jobname[0] != '\0')
                        strcpy(jobname, q.jobname);

                     report_level = q.level;

                     copy_jobwork(&hctx);

                     hjoblist.nitems = 0;

                     build_joblist(&hjoblist, &hctx, jobname, root_job->head, 0, report_level, OTTO_TRUE);
                  }

                  switch(type)
                  {
                     case OTTO_DETAIL:      write_htmldtl    (recvbuf->fd, &hjoblist, &q); break;
                     case OTTO_QUERY:       write_htmljil    (recvbuf->fd, &hjoblist, &q); break;
                     case OTTO_SUMMARY:     write_htmlsum    (recvbuf->fd, &hjoblist, &q); break;
                     case OTTO_EXPORT_MSP:  write_htmlmspdi  (recvbuf->fd, &hjoblist, &q); break;
                     case OTTO_EXPORT_CSV:  write_htmlcsv    (recvbuf->fd, &hjoblist, &q); break;
                     case OTTO_VERSION:     write_htmlversion(recvbuf->fd);                break;
                  }
                  break;

               default:
                  // assume an unknown endpoint is a file
                  // only serve files from the $OTTOHTML directory

                  strcpy(filename, getenv("OTTOHTML"));
                  if(filename[strlen(filename)-1] != '/')
                     strcat(filename, "/");
                  strcat(filename, basename(uri));

                  // make sure the file exists
                  if(stat(filename, &sbuf) < 0)
                  {
                     ottohtml_send_error(recvbuf->fd, basename(uri), "404", "Not found", "ottohttpd couldn't find this file");
                  }
                  else
                  {
                     // Use mmap to return arbitrary-sized response body
                     fd = open(filename, O_RDONLY);
                     p  = mmap(0, sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

                     ottohtml_send(recvbuf->fd, "200", "OK", get_mime_type(filename), p, (int)sbuf.st_size);

                     munmap(p, sbuf.st_size);
                     close(fd);
                  }
                  break;
            }
         }
      }
   }

   if(rc <= 0)
   {
      retval = OTTO_CLOSE_CONN;
   }

   return(retval);
}



