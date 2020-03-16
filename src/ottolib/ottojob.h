#ifndef _OTTOJOB_H_
#define _OTTOJOB_H_

#include <sys/types.h>

#define JOB_LAYOUT_VERSION 4

#define MAXLVL                99
#define NAMLEN                64
#define DSCLEN              1024
#define CMDLEN               512
#define CNDLEN              1024
#define TIMLEN               255
#define TYPLEN                 1
#define FLGLEN                 1

#pragma pack(1)
typedef struct _job
{
	int16_t id;                     // linkage
	int16_t level;
	int16_t box;
	int16_t head;
	int16_t tail;
	int16_t prev;
	int16_t next;

	char    name[NAMLEN+1];         // job definition
	char    type;
	char    box_name[NAMLEN+1];
	char    description[DSCLEN+1];
	char    command[CMDLEN+1];
	char    condition[CNDLEN+1];
	char    date_conditions;
	char    days_of_week;
	int64_t start_minutes;
	int64_t start_times[24];
	char    autohold;

	char    expression[CNDLEN+1];   // current job state
	char    expr_fail;
	char    status;
	char    on_autohold;
	char    on_noexec;
	pid_t   pid;
	time_t  start;
	time_t  finish;
	time_t  duration;
	int     exit_status;

	union                           // supplemental info
	{
		char opcode;
		char printed;
		char gpflag;
	};
	char    attributes;
} JOB;

typedef struct _joblist
{
	int   nitems;
	JOB  *item;
} JOBLIST;
#pragma pack()


#define OTTO_INVALID_VALUE          (1L)
#define OTTO_INVALID_NAME_CHARS     (1L<<1)
#define OTTO_INVALID_NAME_LENGTH    (1L<<2)

int ottojob_copy_command(char *output, char *command, int outlen);
int ottojob_copy_condition(char *output, char *condition, int outlen);
int ottojob_copy_days_of_week(char *output, char *days_of_week);
int ottojob_copy_description(char *output, char *description, int outlen);
int ottojob_copy_flag(char *output, char *input, int outlen);
int ottojob_copy_name(char *output, char *name, int outlen);
int ottojob_copy_start_minutes(int64_t *output, char *start_minutes);
int ottojob_copy_start_times(int64_t *output, char *start_times);
int ottojob_copy_type(char *output, char *type, int outlen);

void ottojob_log_job_layout();
void build_joblist(JOBLIST *joblist, char *jobname, int id, int level_offset, int level_limit, int perform_check);
int  ottojob_reduce_list(JOBLIST *joblist, char *name);


#endif
//
// vim: ts=3 sw=3 ai
//
