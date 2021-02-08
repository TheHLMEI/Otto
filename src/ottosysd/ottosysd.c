#include <ctype.h>
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "ottobits.h"
#include "ottocfg.h"
#include "ottocond.h"
#include "ottocrud.h"
#include "ottodb.h"
#include "ottoipc.h"
#include "signals.h"
#include "simplelog.h"

SIMPLELOG *logp = NULL;

#define OTTO_CLOSE_CONN -100

// internal representation of minutes during the week to check for a timed job
// updates each time dependencies are compiled
int64_t    check_times[7][24];
int        last_check_day=-1, last_check_hour=-1, last_check_min=-1;

int server_socket = -1;


/*---------------------------- program control functions --------------------------*/
void  ottosysd_exit(int signum);
int   ottosysd_getargs (int argc, char **argv);
void  ottosysd_usage(void);
int   ottosysd (void);
int   write_pidfile ();
void  delete_pidfile ();
/*------------------------------ event based functions ----------------------------*/
int   handle_ipc(int fd);
void  handle_crud_pdu(ottoipc_simple_pdu_st *pdu);
void  handle_scheduler_pdu(ottoipc_simple_pdu_st *pdu);
void  handle_daemon_pdu(ottoipc_simple_pdu_st *pdu);
void  check_dependencies ();
void  compile_dependencies ();
/*------------------------------ time based functions -----------------------------*/
void  handle_timeout();
int   get_msec_to_next_minute(void);
void  fire_this_minute(int this_wday, int this_hour, int this_min);
/*------------------------------ job utility functions ----------------------------*/
void  activate_box         (int id);
void  activate_box_chain   (int id);
void  run_box              (int id);
int   run_job              (int id);
void  finish_box           (int box_id, time_t finish);
void  finish_job           (int sigNum);
void  start_job            (int id);
void  kill_job             (int id, int signal);
void  move_job             (int id, int action);
void  reset_job            (int id);
void  reset_job_chain      (int id);
int   check_reset          (int id);
int   check_reset_chain    (int id);
void  set_job_status       (int id, int status);
void  set_job_autohold     (int id, int action);
void  set_job_hold         (int id, int action);
void  set_job_noexec       (int id, int action);
void  set_job_noexec_chain (int id, int action);
void  set_levels           (void);
void  set_levels_chain     (int id, int level);



int
main(int argc, char *argv[])
{
	int retval = init_cfg(argc, argv);

	if(retval == OTTO_SUCCESS)
		retval = write_pidfile();

	if(retval == OTTO_SUCCESS)
		init_signals(ottosysd_exit);

	if(retval == OTTO_SUCCESS)
		if((logp = simplelog_init(cfg.env_ottolog, cfg.progname, INFO, 0)) == NULL)
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

	if(retval == OTTO_SUCCESS)
	{
		if((server_socket = init_server_ipc(cfg.server_port, 32)) == -1)
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
	struct pollfd fds[200];
	int    nfds = 1, current_size = 0, i, j;

	// set signal handlers
	sig_set(SIGCLD,  finish_job);
	sig_set(SIGINT,  ottosysd_exit);

	// clean up before start
	compile_dependencies();
	check_dependencies();

	// Initialize the pollfd structure
	memset(fds, 0 , sizeof(fds));

	// Set up the initial listening socket
	fds[0].fd = server_socket;
	fds[0].events = POLLIN;

	// Loop waiting for incoming connects, incoming data
	// on any connected sockets, or a timeout
	do
	{
		// get the number of microseconds to the next whole minute
		timeout = get_msec_to_next_minute();

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
get_msec_to_next_minute()
{
	int retval = 0;
	struct timeval now;
	time_t next_minute_in_seconds;

	next_minute_in_seconds = (((time(0) / 60) + 1) * 60) - 1;

	gettimeofday(&now, 0);

	retval  = (next_minute_in_seconds - now.tv_sec) * 1000;
	retval += (1000 - now.tv_usec/1000);

	if(cfg.debug == OTTO_TRUE)
		lprintf(logp, INFO, "msec to next minute %d\n", retval);
	
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
		lprintf(logp, INFO, "last_check_time = %d-%2d:%2d  now = %d-%2d:%2d\n",
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
			lprintf(logp, INFO, "checking time %d-%2d:%2d\n", last_check_day, last_check_hour, last_check_min);
		}

		// only do work if this minute on this day has a job to fire off
		if(check_times[last_check_day][last_check_hour] & (1L << last_check_min))
		{
			if(cfg.debug == OTTO_TRUE)
			{
				lprintf(logp, INFO, "something is scheduled for %d-%2d:%2d\n", last_check_day, last_check_hour, last_check_min);
			}

			// only copy and sort on the first iteration
			if(jobwork_copied == OTTO_FALSE)
			{
				if(cfg.debug == OTTO_TRUE)
				{
					lprintf(logp, INFO, "copying jobwork\n");
				}

				copy_jobwork();
				sort_jobwork(BY_DATE_COND);
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

	// only do work if this hour on this minute has a job to fire off
	if(check_times[this_wday][this_hour] & (1L << this_min))
	{
		for(i=0; i<cfg.ottodb_maxjobs; i++)
		{
			if(cfg.debug == OTTO_TRUE)
			{
				lprintf(logp, INFO, "name %s date_conditions %d\n", jobwork[i].name, jobwork[i].date_conditions);
			}

			if(jobwork[i].name[0] == '\0' || jobwork[i].date_conditions == 0)
				break;

			id = jobwork[i].id;

			if(job[id].days_of_week & (1L << this_wday))
			{

				switch(job[id].status)
				{
					case STAT_IN:
					case STAT_SU:
					case STAT_FA:
					case STAT_TE:
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
											activate_box(id);
											job[id].start    = time(0);
											job[id].finish   = 0;
											job[id].duration = 0;
											job[id].status   = STAT_RU;
											do_check             = OTTO_TRUE;
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
	int retval = OTTO_SUCCESS;
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
					handle_crud_pdu(pdu);
				}
				else if(pdu->opcode < SCHED_TOTAL)
				{
					handle_scheduler_pdu(pdu);
				}
				else
				{
					handle_daemon_pdu(pdu);
				}
				compile_dependencies();
				check_dependencies();
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



void
handle_crud_pdu(ottoipc_simple_pdu_st *pdu)
{
	switch(pdu->opcode)
	{
		case CREATE_JOB: create_job((ottoipc_create_job_pdu_st *)pdu); break;
		case REPORT_JOB: report_job(pdu); break;
		case UPDATE_JOB: update_job((ottoipc_update_job_pdu_st *)pdu); break;
		case DELETE_JOB: delete_job(pdu); break;
		case DELETE_BOX: delete_box(pdu); break;
		default:
							  pdu->option = NOOP;
							  ottoipc_initialize_send();
							  ottoipc_enqueue_simple_pdu(pdu);
							  break;
	}

	if(pdu->opcode != REPORT_JOB)
		set_levels();
}



void
handle_scheduler_pdu(ottoipc_simple_pdu_st *pdu)
{
	int id, jobwork_id, option;

	copy_jobwork();
	sort_jobwork(BY_NAME);

	jobwork_id = find_jobname(pdu->name);
	if(jobwork_id >= 0 && jobwork_id < cfg.ottodb_maxjobs)
	{
		id = jobwork[jobwork_id].id;

		// update pdu response in place then enqueue it to send
		option = pdu->option;
		pdu->option = ACK;
		switch(pdu->opcode)
		{
			case START_JOB:        start_job       (id);              break;
			case KILL_JOB:         kill_job        (id, SIGKILL);     break;
			case MOVE_JOB_TOP:     move_job        (id, pdu->opcode); break;
			case MOVE_JOB_UP:      move_job        (id, pdu->opcode); break;
			case MOVE_JOB_DOWN:    move_job        (id, pdu->opcode); break;
			case MOVE_JOB_BOTTOM:  move_job        (id, pdu->opcode); break;
			case RESET_JOB:        reset_job       (id);              break;
			case SEND_SIGNAL:      kill_job        (id, option);      break;
			case CHANGE_STATUS:    set_job_status  (id, option);      break;
			case JOB_ON_AUTOHOLD:  set_job_autohold(id, pdu->opcode); break;
			case JOB_OFF_AUTOHOLD: set_job_autohold(id, pdu->opcode); break;
			case JOB_ON_HOLD:      set_job_hold    (id, pdu->opcode); break;
			case JOB_OFF_HOLD:     set_job_hold    (id, pdu->opcode); break;
			case JOB_ON_NOEXEC:    set_job_noexec  (id, pdu->opcode); break;
			case JOB_OFF_NOEXEC:   set_job_noexec  (id, pdu->opcode); break;
			default:               pdu->option = NOOP;                break;
		}
	}
	else
	{
		// update pdu response in place then enqueue it to send
		pdu->option = JOB_NOT_FOUND;
	}

	ottoipc_initialize_send();
	ottoipc_enqueue_simple_pdu(pdu);
}



void
handle_daemon_pdu(ottoipc_simple_pdu_st *pdu)
{
	pdu->option = ACK;
	switch(pdu->opcode)
	{
		case PING:
			sprintf(pdu->name, "ottosysd %s, %s, Debug %s, DB Layout v%d",
					cfg.otto_version, cfg.pause ? "Paused" : "Running", cfg.debug ? "On" : "Off", cfg.ottodb_version);
			break;

		case VERIFY_DB:     sprintf(pdu->name, "%ld", ottodb_inode); break;
		case DEBUG_ON:      cfg.debug = OTTO_TRUE;                   break;
		case DEBUG_OFF:     cfg.debug = OTTO_FALSE;                  break;
		case REFRESH:       compile_dependencies();                  break;
		case PAUSE_DAEMON:  cfg.pause = OTTO_TRUE;                   break;
		case RESUME_DAEMON: cfg.pause = OTTO_FALSE;                  break;
		case STOP_DAEMON:   delete_pidfile(); exit(0);               break;
		default:            pdu->option = NOOP;                      break;
	}

	ottoipc_initialize_send();
	ottoipc_enqueue_simple_pdu(pdu);
}



void
check_dependencies()
{
	int box, i, id, recheck, expr_ret;

	recheck = OTTO_TRUE;
	while(recheck == OTTO_TRUE)
	{
		recheck = OTTO_FALSE;

		copy_jobwork();
		sort_jobwork(BY_ACTIVE);

		// jobwork is sorted by status (AC first), type (b then c)
		// linear search down the list of active jobs checking dependencies
		for(i=0; i<cfg.ottodb_maxjobs; i++)
		{
			if(jobwork[i].name[0] == '\0' || jobwork[i].status != STAT_AC)
				break;

			// make things easier to type
			id  = jobwork[i].id;
			box = job[id].box;

			// only check jobs with no parent box or whose parent box is RUNNING
			if(box == -1 || job[box].status == STAT_RU)
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
								job[id].start    = time(0);
								job[id].finish   = 0;
								job[id].duration = 0;
								job[id].status   = STAT_RU;
								break;
							case OTTO_CMD:
								run_job(id);
								break;
						}
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

	copy_jobwork();
	sort_jobwork(BY_NAME);

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
		if(jobwork[i].name[0] == '\0')
			break;

		my_id = jobwork[i].id;
		memset(job[my_id].expression, 0, CNDLEN+1);

		if(jobwork[i].condition[0] != '\0')
		{
			job[my_id].expr_fail = OTTO_FALSE;
			if(compile_expression(job[my_id].expression, jobwork[i].condition, CNDLEN) == OTTO_FAIL)
			{
				job[my_id].expr_fail = OTTO_TRUE;
			}
		}

		// add times to global array
		if(jobwork[i].date_conditions != 0)
		{
			// loop over days
			for(d=0; d<7; d++)
			{
				if(jobwork[i].days_of_week & (1L << d))
				{
					// bitwise OR the hours into the global array for this day
					for(h=0; h<24; h++)
					{
						if(jobwork[i].start_times[h] != 0)
						{
							check_times[d][h] |= jobwork[i].start_times[h];
							if(cfg.debug == OTTO_TRUE)
							{
								lprintf(logp, INFO, "Adding %d-%2d to check_times for %s\n", d, h, jobwork[i].name);
								minutes[0] = '\0';
								for(dmin=0; dmin<60; dmin++)
								{
									if(check_times[d][h] & (1L << dmin))
									{
										sprintf(minute, " %d", dmin);
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
run_box(int id)
{
	// first activate the box and its contents
	activate_box(id);
	
	// now set the requested box into the run state
	// check_dependencies will take care of the rest

	// mark box as RUNNING regardless of previous state
	job[id].status   = STAT_RU;
	job[id].start    = time(0);
}



void
activate_box(int id)
{
	// only activate the box if it's not ON_HOLD
	if(job[id].status != STAT_OH)
	{
		// mark box as ACTIVATED
		job[id].status   = STAT_AC;
		job[id].start    = 0;
		job[id].finish   = 0;
		job[id].duration = 0;

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
					job[id].status   = STAT_AC;
					job[id].start    = 0;
					job[id].finish   = 0;
					job[id].duration = 0;

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



void
finish_box(int box_id, time_t finish)
{
	int fail_found=OTTO_FALSE, id, jobs_finished=OTTO_TRUE;

	id = job[box_id].head;

	while(id != -1)
	{
		if(job[id].status != STAT_SU &&
			job[id].status != STAT_FA &&
			job[id].status != STAT_TE)
		{
			jobs_finished = OTTO_FALSE;
		}

		if(job[id].status == STAT_FA ||
			job[id].status == STAT_TE)
		{
			fail_found = OTTO_TRUE;
		}

		id = job[id].next;
	}

	if(jobs_finished == OTTO_TRUE)
	{
		if(fail_found == OTTO_TRUE)
			job[box_id].status = STAT_FA;
		else
			job[box_id].status = STAT_SU;

		job[box_id].finish = finish;
		if(job[box_id].start != 0)
			job[box_id].duration = finish - job[box_id].start;

		if(job[box_id].box != -1)
			finish_box(job[box_id].box, finish);
	}
}



int
run_job(int id)
{
	pid_t pid;
	char auto_job_name[NAMLEN+15];
	
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

		return(0);
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
			sprintf(auto_job_name, "AUTO_JOB_NAME=%s", job[id].name);
			cfg.envvar[0] = auto_job_name;

			// ensure file descriptors are closed on exec
			fcntl(0,        F_SETFD, FD_CLOEXEC);  // stdin
			fcntl(1,        F_SETFD, FD_CLOEXEC);  // stdout
			fcntl(2,        F_SETFD, FD_CLOEXEC);  // stderr
			fcntl(ottodbfd, F_SETFD, FD_CLOEXEC);  // otto database
			// fcntl(sock,     F_SETFD, FD_CLOEXEC);  // server socket
			// if(csock != -1)
			//	fcntl(csock, F_SETFD, FD_CLOEXEC);  // client socket

			// overlay a shell to execute the command
			execle("/bin/sh", "/bin/sh", "-c", job[id].command, NULL, cfg.envvar);

			lprintf(logp, MAJR, "execle failed for %s.\n", job[id].name);
		}
	}
		
	return(pid);
}



void
finish_job(int sigNum)
{
	int i, id;
	int status;
	pid_t pid;

	sig_hold(SIGCLD);

	copy_jobwork();
	sort_jobwork(BY_PID);

	while((pid = waitpid(-1, &status, WNOHANG)) > 0)
	{
		for(i=0; i<cfg.ottodb_maxjobs; i++)
		{
			if(jobwork[i].name[0] == '\0')
				break;

			if(jobwork[i].pid == pid)
			{
				id = jobwork[i].id;
				if(WIFEXITED(status))
				{
					job[id].exit_status = WEXITSTATUS(status);
				}
				else if(WIFSIGNALED(status))
				{
					job[id].exit_status = WTERMSIG(status);
				}
				lprintf(logp, INFO, "%s ended with return code %d\n", job[id].name, status);
				job[id].pid = 0;
				if(job[id].exit_status == 0)
					job[id].status = STAT_SU;
				else
					job[id].status = STAT_FA;
				job[id].finish = time(0);
				job[id].duration = job[id].finish - job[id].start;

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
reset_job(int id)
{
	if(check_reset(id) == OTTO_TRUE)
	{
		job[id].on_autohold = job[id].autohold;
		job[id].on_noexec   = OTTO_FALSE;
		job[id].status      = STAT_IN;
		job[id].pid         = 0;
		job[id].exit_status = 0;
		job[id].start       = 0;
		job[id].finish      = 0;
		job[id].duration    = 0;

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
		job[id].on_noexec   = OTTO_FALSE;
		job[id].status      = STAT_IN;
		job[id].pid         = 0;
		job[id].exit_status = 0;
		job[id].start       = 0;
		job[id].finish      = 0;
		job[id].duration    = 0;

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
start_job(int id)
{
	switch(job[id].type)
	{
		case OTTO_BOX: run_box(id); break;
		case OTTO_CMD: run_job(id); break;
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
move_job(int id, int direction)
{
	int steps  = cfg.ottodb_maxjobs; // assume the job will move the entire length of the DB
	int new_prev, new_next;

	switch(direction)
	{
		case MOVE_JOB_UP:
			steps = 1;    // limit steps to one and fall through
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
			steps = 1;    // limit steps to one and fall through
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
	copy_jobwork();

	set_levels_chain(root_job->head, 0);

	save_jobwork();
}



void
set_levels_chain(int id, int level)
{
	while(id != -1)
	{
		jobwork[id].level = level;
		if(jobwork[id].type == OTTO_BOX)
			set_levels_chain(jobwork[id].head, level+1);

		id = jobwork[id].next;
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



//
// vim: ts=3 sw=3 ai
//
