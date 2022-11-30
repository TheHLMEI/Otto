#ifndef _OTTO_H_
#define _OTTO_H_

#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>

#define OTTO_VERSION_STRING "v2.0.3"

//
// Defines
//


// general defines
#define OTTO_UNKNOWN   0

#define OTTO_FALSE     0
#define OTTO_TRUE      1

#define OTTO_SUCCESS   0
#define OTTO_FAIL     -1

#define OTTO_QUIET     0
#define OTTO_VERBOSE   1

#define OTTO_SERVER    0
#define OTTO_CLIENT    1

#define OTTO_SUN      (1L)
#define OTTO_MON   (1L<<1)
#define OTTO_TUE   (1L<<2)
#define OTTO_WED   (1L<<3)
#define OTTO_THU   (1L<<4)
#define OTTO_FRI   (1L<<5)
#define OTTO_SAT   (1L<<6)
#define OTTO_ALL   (OTTO_SUN | OTTO_MON | OTTO_TUE | OTTO_WED | OTTO_THU | OTTO_FRI | OTTO_SAT)

#define OTTO_BOX  'b'
#define OTTO_CMD  'c'

#define OTTO_USE_START_TIMES   1
#define OTTO_USE_START_MINUTES 2

#define OTTO_CSV 1
#define OTTO_JIL 2
#define OTTO_MSP 3

#define OTTO_DETAIL   1
#define OTTO_QUERY    2
#define OTTO_SUMMARY  3
#define OTTO_EXPORT   4
#define OTTO_VERSION  5


#define i64 int64_t
#define cde8int(a) ((i64)a[0]<<56|(i64)a[1]<<48|(i64)a[2]<<40|(i64)a[3]<<32|a[4]<<24|a[5]<<16|a[6]<<8|a[7])

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

#define HAS_TYPE                  (1L)
#define HAS_BOX_NAME        (1L <<  1) 
#define HAS_DESCRIPTION     (1L <<  2)
#define HAS_COMMAND         (1L <<  3)
#define HAS_CONDITION       (1L <<  4)
#define HAS_DATE_CONDITIONS (1L <<  5)
#define HAS_AUTO_HOLD       (1L <<  6)
#define HAS_LOOP            (1L <<  7)
#define HAS_ENVIRONMENT     (1L <<  8)
#define HAS_NEW_NAME        (1L <<  9)
#define HAS_START           (1L << 10)
#define HAS_FINISH          (1L << 11)

#define LOOP_NOT_RUNNING       0
#define LOOP_RUNNING           1
#define LOOP_BROKEN            2

// ottocfg defines
#define ENV_NAMES  \
   X(OTTO_JOBNAME) \
   X(HOME)         \
   X(USER)         \
   X(LOGNAME)      \
   X(PATH)

// enum of ENV_NAMES
// by macro expansion.
enum
{
#define X(value) value##VAR,
   ENV_NAMES
#undef X
   VAR_TOTAL
};


#define MAX_ENVVAR  1000

// ottojob defines
#define JOB_LAYOUT_VERSION     6

#define MAXLVL                99
#define MAXLOOP              100
#define NAMLEN                63   // job name, box name
#define DSCLEN               255   // job description
#define CMDLEN               255   // command line
#define CNDLEN              1023   // condition
#define TYPLEN                 1   // job type
#define FLGLEN                 1   // boolean flags
#define VARLEN                31   // loop variable name
#define FORLEN     (VARLEN + 10)   // loop expression "for VAR=xx,yy"
#define ENVLEN               255   // per-job environment variables

#define OTTO_CONDITION_BUFFER_EXCEEDED                   (1L)
#define OTTO_CONDITION_INVALID_KEYWORD                (1L<<1)
#define OTTO_CONDITION_MISSING_JOBNAME                (1L<<2)
#define OTTO_CONDITION_NEEDS_PARENS                   (1L<<3)
#define OTTO_CONDITION_ONE_OPERAND                    (1L<<4)
#define OTTO_CONDITION_UNBALANCED                     (1L<<5)
#define OTTO_INVALID_APPLICATION                      (1L<<6)
#define OTTO_INVALID_COMBINATION                      (1L<<7)
#define OTTO_INVALID_ENVNAME_CHARS                    (1L<<8)
#define OTTO_INVALID_ENVNAME_FIRSTCHAR                (1L<<9)
#define OTTO_INVALID_JOB_TYPE                        (1L<<10)
#define OTTO_INVALID_LOOPMAX_LEN                     (1L<<11)
#define OTTO_INVALID_LOOPMIN_LEN                     (1L<<12)
#define OTTO_INVALID_MAXNUM_CHARS                    (1L<<13)
#define OTTO_INVALID_MINNUM_CHARS                    (1L<<14)
#define OTTO_INVALID_NAME_CHARS                      (1L<<15)
#define OTTO_INVALID_NAME_LENGTH                     (1L<<16)
#define OTTO_INVALID_NUMBER_CHARS                    (1L<<17)
#define OTTO_INVALID_NUMBER_LENGTH                   (1L<<18)
#define OTTO_INVALID_OVERRIDE                        (1L<<19)
#define OTTO_INVALID_VALUE                           (1L<<20)
#define OTTO_LENGTH_EXCEEDED                         (1L<<21)
#define OTTO_MALFORMED_ENV_ASSIGNMENT                (1L<<22)
#define OTTO_MALFORMED_LOOP                          (1L<<23)
#define OTTO_MISSING_COMMAND                         (1L<<24)
#define OTTO_SAME_JOB_BOX_NAMES                      (1L<<25)
#define OTTO_SAME_NAME                               (1L<<26)

#define AS_ASCII               1
#define AS_HTML                2
#define AS_MSPDI               3
#define AS_CSV                 4

// ottodb defines
#define OTTODB_VERSION JOB_LAYOUT_VERSION
#define OTTODB_MAXJOBS 1000

// ottoipc defines
#define OTTO_CLOSE_CONN          -1
#define OTTOIPC_MAX_HOST_LENGTH 255

// ottocond defines
#define SELF_REFERENCE      (1)
#define MISS_REFERENCE   (1<<1)

// ottocrud defines
// ottolog defines
#define OTTOLOG_BUFLEN  8192

#define DBG9           9
#define DBG8           8
#define DBG7           7
#define DBG6           6
#define DBG5           5
#define DBG4           4
#define DBG3           3
#define DBG2           2
#define DBG1           1
#define INFO           0
#define MINR          -1
#define MAJR          -2
#define END           -3
#define ENDI          -4
#define CAT           -5
#define CATI          -6
#define NO_LOG      -100
#define ULTIMATE    -200

// format defines







//
// Enumerations
//
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
   JIL_ENVIRON,
   JIL_FINISH,
   JIL_INS_JOB,
   JIL_JOBTYPE,
   JIL_LOOP,
   JIL_NEWNAME,
   JIL_START,
   JIL_STRTMIN,
   JIL_STRTTIM,
   JIL_UPD_JOB,
   JIL_UNSUPPD
};

// ottodb enumerations
enum SORTS
{
   BY_UNKNOWN,
   BY_ACTIVE,
   BY_ID,
   BY_LINKED_LIST,
   BY_NAME,
   BY_PID,
   BY_DATE_COND
};

// ottoipc enumerations
enum OPCODES
{
	NOOP,

	// job definition operations
	CREATE_JOB,
	REPORT_JOB,
	UPDATE_JOB,
	DELETE_JOB,
	DELETE_BOX,
	CRUD_TOTAL,

	// scheduler control operations
	FORCE_START_JOB,
	START_JOB,
	KILL_JOB,
	MOVE_JOB_TOP,
	MOVE_JOB_UP,
	MOVE_JOB_DOWN,
	MOVE_JOB_BOTTOM,
	RESET_JOB,
	SEND_SIGNAL,
	CHANGE_STATUS,
	JOB_ON_AUTOHOLD,
	JOB_OFF_AUTOHOLD,
	JOB_ON_HOLD,
	JOB_OFF_HOLD,
	JOB_ON_NOEXEC,
	JOB_OFF_NOEXEC,
   BREAK_LOOP,
	SCHED_TOTAL,

	// daemon control operations
	PING,
	VERIFY_DB,
	DEBUG_ON,
	DEBUG_OFF,
	REFRESH,
	PAUSE_DAEMON,
	RESUME_DAEMON,
	STOP_DAEMON,
	OPCODE_TOTAL
};


enum STATUSES
{
	NO_STATUS=100,
	FAILURE,
	INACTIVE,
	RUNNING,
	SUCCESS,
	TERMINATED,
	STATUS_TOTAL
};


enum RESULTCODES
{
	ACK = 200,
	NACK,

	// job definition operations
	JOB_CREATED,
	JOB_REPORTED,
	JOB_UPDATED,
	JOB_NOT_FOUND,
	JOB_DELETED,
	JOB_ALREADY_EXISTS,
	JOB_DEPENDS_ON_MISSING_JOB,
	JOB_DEPENDS_ON_ITSELF,
	BOX_NOT_FOUND,
	BOX_DELETED,
	BOX_COMMAND,
	NO_SPACE_AVAILABLE,
	GRANDFATHER_PARADOX,
	NEW_NAME_ALREADY_EXISTS,
	RESULTCODE_TOTAL
};


enum PDUTYPE
{
	SIMPLE_PDU,
	CREATE_JOB_PDU,
	REPORT_JOB_PDU,
	UPDATE_JOB_PDU,
	DELETE_JOB_PDU,
	PDUTYPE_TOTAL
};

// ottocond enumerations
enum JOBSTATS
{
	STAT_IN=0,
	STAT_AC,
	STAT_RU,
	STAT_SU,
	STAT_FA,
	STAT_TE,
	STAT_OH,
	STAT_BR,
	STAT_MAX
};




// ottocrud enumerations
// ottolog enumerations
// ottoutil enumerations
// signals enumerations

// format enumerations
enum EXTDEFS
{
   COMMAND,
   DESCRIPTION,
   ENVIRONMENT,
   DAYS_OF_WEEK,
   START_MINUTES,
   START_TIMES,
   LOOP,
   AUTOHOLD,
   DATE_CONDITIONS,
   EXTDEF_TOTAL
};




//
// Typedefs
//

// ottocfg typedefs
typedef struct _ottocfg_st
{
	// Otto version
	char     *otto_version;

	// Otto DB layout version and number of jobs in the DB
	int16_t   ottodb_version;
	int16_t   ottodb_maxjobs;

	// program identification variables
	char     *progname;

	// network values
	char     *server_addr;
	uint16_t  ottosysd_port;
	uint16_t  httpd_port;

	// user identification variables
	int       use_getpw;
	uid_t     euid;     // effective user id
	uid_t     ruid;     // real user id
   char     *userp;    // pointer to environment variable value or "unknown"
   char     *lognamep; // pointer to environment variable value or "unknown"

	// standard environment
	char     *env_ottocfg;
	char     *env_ottolog;
	char     *env_ottoenv;
	char     *env_ottohtml;

	// general variables
   int       enable_httpd;
	int       pause;
	int       debug;
	int       verbose;
	int       show_sofar;
	int       path_overridden;
	time_t    ottoenv_mtime;
	char     *base_url;

	char     *envvar[MAX_ENVVAR];   // environment to be passed to child jobs
	char     *envvar_s[MAX_ENVVAR]; // static environment from $OTTOCFG
	char     *envvar_d[MAX_ENVVAR]; // dynamic environment from $OTTOENV
	int       n_envvar;
	int       n_envvar_s;
	int       n_envvar_d;
} ottocfg_st;

// ottohtml typedefs
typedef struct
{
   char endpoint[1024];
   char jobname[NAMLEN+1];
   int  level;
   char show_IN;
   char show_AC;
   char show_OH;
   char show_RU;
   char show_SU;
   char show_FA;
   char show_TE;
   char show_BR;
} ottohtml_query;

// ottojob typedefs
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
   char    environment[ENVLEN+1];

	char    expression[CNDLEN+1];   // current job state
	char    expr_fail;
	char    status;
	char    on_autohold;
	char    on_noexec;
   char    loopname[VARLEN+1];
   char    loopmin;
   char    loopmax;
   char    loopnum;
   char    loopsgn;
   char    loopstat;  
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
	int16_t attributes;
} JOB;

typedef struct _joblist
{
	int   nitems;
	JOB  *item;
} JOBLIST;
#pragma pack()

typedef struct _job_detail
{
   // linkage
	char    id[6];
	char    level[6];
	char    linkage[128];

   // job definition
	char    name[NAMLEN+1];
	char    type[8];
	char    box_name[NAMLEN+1];
	char    description[(6*DSCLEN)+1];  // outsized to accommodate html escapes
	char    command[(6*CMDLEN)+1];      // outsized to accommodate html escapes
	char    condition[(6*CNDLEN)+1];    // outsized to accommodate html escapes
	char    date_conditions[10];
	char    days_of_week[30];
	char    start_minutes[2048];
	char    start_times[2048];
	char    autohold[6];

   // current job state
	char    expression[(6*CNDLEN)+1];   // outsized to accommodate html escapes
	char    expression2[(6*CNDLEN)+1];  // outsized to accommodate html escapes
	char    expr_fail[6];
	char    status[10];
	char    on_autohold[6];
	char    on_noexec[6];
	char    pid[10];
	char    start[20];
	char    finish[20];
	char    duration[20];
	char    exit_status[10];
	char    environment[(6*ENVLEN)+1];  // outsized to accommodate html escapes
   char    loopname[VARLEN+1];
   char    loopmin[4];
   char    loopmax[4];
   char    loopnum[4];
   char    loopstat[128];  

} JOBTVAL;
#pragma pack()

// ottodb typedefs
typedef struct _dbctx
{
   int lastsort;
   JOB  *job;
} DBCTX;

// ottoipc typedefs
#define COMMON_PDU_ATTRIBUTES uint8_t pdu_type;              \
                              uint8_t opcode;                \
                              char    name[NAMLEN+1];        \
                              uint8_t option;
#define JOBLNK_PDU_ATTRIBUTES int16_t id;                    \
                              int16_t level;                 \
                              int16_t box;                   \
                              int16_t head;                  \
                              int16_t tail;                  \
                              int16_t prev;                  \
                              int16_t next;
#define JOBDEF_PDU_ATTRIBUTES char    type;                  \
                              int16_t attributes;            \
                              char    box_name[NAMLEN+1];    \
                              char    description[DSCLEN+1]; \
                              char    command[CMDLEN+1];     \
                              char    condition[CNDLEN+1];   \
                              char    date_conditions;       \
                              char    days_of_week;          \
                              int64_t start_minutes;         \
                              int64_t start_times[24];       \
                              char    autohold;              \
                              char    environment[ENVLEN+1]; \
                              char    loopname[VARLEN+1];    \
                              char    loopmin;               \
                              char    loopmax;               \
                              char    loopsgn;
#define JOBEXT_PDU_ATTRIBUTES char    new_name[NAMLEN+1];    \
                              time_t  start;                 \
                              time_t  finish;
#define JOBSTT_PDU_ATTRIBUTES char    expr_fail;             \
                              char    status;                \
                              char    on_autohold;           \
                              char    on_noexec;             \
                              pid_t   pid;                   \
                              time_t  start;                 \
                              time_t  finish;                \
                              time_t  duration;              \
                              int     exit_status;

#pragma pack(1)
typedef struct _ottoipc_pdu_header_st
{
	int   version;
   int   payload_length;
   uid_t euid;
   uid_t ruid;
} ottoipc_pdu_header_st;

typedef struct _ottoipc_simple_pdu_st
{
	COMMON_PDU_ATTRIBUTES
} ottoipc_simple_pdu_st;

typedef struct _ottoipc_create_job_pdu_st
{
	COMMON_PDU_ATTRIBUTES
	JOBDEF_PDU_ATTRIBUTES
} ottoipc_create_job_pdu_st;

typedef struct _ottoipc_report_job_pdu_st
{
	COMMON_PDU_ATTRIBUTES
	JOBLNK_PDU_ATTRIBUTES
	JOBDEF_PDU_ATTRIBUTES
	JOBSTT_PDU_ATTRIBUTES
} ottoipc_report_job_pdu_st;

typedef struct _ottoipc_update_job_pdu_st
{
	COMMON_PDU_ATTRIBUTES
	JOBDEF_PDU_ATTRIBUTES
	JOBEXT_PDU_ATTRIBUTES
} ottoipc_update_job_pdu_st;

typedef struct _ottoipc_simple_pdu_st ottoipc_delete_job_pdu_st;

typedef struct _recvbuf
{
   int           fd;
   unsigned char buffer[1024];
   int           readcount;
   int           currchar;
} RECVBUF;

#pragma pack()

// ottocond typedefs
typedef struct _ecnd_st
{
	char *s;
} ecnd_st;

// ottocrud typedefs
// ottolog typedefs
typedef struct
{
	char *logfile;
	FILE *logfp;
	int   loglevel;
	int   lsstarted;
	char  logfile_buffer[OTTOLOG_BUFLEN];
	char  console_buffer[OTTOLOG_BUFLEN];
	char  last_logged_day[30];
	int   lslevel;
} OTTOLOG;

// ottoutil typedefs
#pragma pack(1)
typedef struct _dynbuf // stdin read buffer
{
	char    *buffer;
	char    *s;
	int     bufferlen;
	int     eob;
	int     line;
} DYNBUF;

// signals typedefs

// format typedefs



//
// Globals
//

// ottocfg globals
extern ottocfg_st cfg;

// ottohtml globals
extern ottohtml_query q;

// ottojob globals
// ottodb globals
extern JOB  *job;
extern JOB  *root_job;
extern ino_t ottodb_inode;
extern int   ottodbfd;

// ottoipc globals
// ottocond globals
// ottocrud globals
// ottolog globals
// ottoutil globals
// signals globals

// format globals



//
// Prototypes
//

// ottocfg prototypes
int  init_cfg(int argc, char **argv);
int  read_cfgfile(void);
void rebuild_environment(void);
void log_initialization(void);
void log_args(int argc, char **argv);
void log_cfg(void);

// ottohtml prototypes
void ottohtml_parse_uri(ottohtml_query *q, char *uri);
void ottohtml_send_error(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void ottohtml_send(int fd, char *msgnum, char *status, char *content_type, char *content, int contentlen);
void ottohtml_send_attachment(int fd, char *filename, char *content, int contentlen);
char *get_mime_type(char *filename);
char *get_html_indent(int level);

// ottojob prototypes
int ottojob_copy_command(char *output, char *command, int outlen);
int ottojob_copy_condition(char *output, char *condition, int outlen);
int ottojob_copy_days_of_week(char *output, char *days_of_week);
int ottojob_copy_description(char *output, char *description, int outlen);
int ottojob_copy_environment(char *output, char *environment, int outlen);
int ottojob_copy_flag(char *output, char *input, int outlen);
int ottojob_copy_loop(char *loopvar, char *loopmin, char *loopmax, char *looopsgn,  char *input);
int ottojob_copy_name(char *output, char *name, int outlen);
int ottojob_copy_start_minutes(int64_t *output, char *start_minutes);
int ottojob_copy_start_times(int64_t *output, char *start_times);
int ottojob_copy_time(time_t *t, char *s);
int ottojob_copy_type(char *output, char *type, int outlen);

int ottojob_print_name_errors(int error_mask, char *action, char *name, char *kind);
int ottojob_print_type_errors(int error_mask, char *action, char *name, char *type);
int ottojob_print_box_name_errors(int error_mask, char *action, char *name, char *box_name);
int ottojob_print_command_errors(int error_mask, char *action, char *name, int outlen);
int ottojob_print_condition_errors(int error_mask, char *action, char *name, int outlen);
int ottojob_print_description_errors(int error_mask, char *action, char *name, int outlen);
int ottojob_print_environment_errors(int error_mask, char *action, char *name, int outlen);
int ottojob_print_auto_hold_errors(int error_mask, char *action, char *name, char *auto_hold);
int ottojob_print_date_conditions_errors(int error_mask, char *action, char *name, char *date_conditions);
int ottojob_print_days_of_week_errors(int error_mask, char *action, char *name);
int ottojob_print_start_mins_errors(int error_mask, char *action, char *name);
int ottojob_print_start_times_errors(int error_mask, char *action, char *name);
int ottojob_print_loop_errors(int error_mask, char *action, char *name);
int ottojob_print_new_name_errors(int error_mask, char *action, char *name, char *new_name);
int ottojob_print_start_errors(int error_mask, char *action, char *name);
int ottojob_print_finish_errors(int error_mask, char *action, char *name);
int ottojob_print_task_name_errors(int error_mask, char *action, char *name, int task1, int task2);

void ottojob_log_job_layout();
void build_joblist(JOBLIST *joblist, DBCTX *ctx, char *jobname, int id, int level_offset, int level_limit, int perform_check);
void build_deplist(JOBLIST *deplist, DBCTX *ctx, JOB *job);
int  ottojob_reduce_list(JOBLIST *joblist, char *name);
void ottojob_prepare_txt_values(JOBTVAL *tval, JOB *item, int format);

// ottodb prototypes
int   open_ottodb(int type);
void  copy_jobwork(DBCTX *ctx);
void  save_jobwork(DBCTX *ctx);
void  sort_jobwork(DBCTX *ctx,  int sort_mode);
int   find_jobname(DBCTX *ctx,  char *jobname);
void  clear_job(int job_index);
void  clear_jobwork(DBCTX *ctx, int job_index);

// ottoipc prototypes
int init_server_ipc(in_port_t port, int backlog);
int init_client_ipc(char *hostname, in_port_t port);

void ottoipc_initialize_send();
void ottoipc_enqueue_simple_pdu(ottoipc_simple_pdu_st *s);
void ottoipc_enqueue_create_job(ottoipc_create_job_pdu_st *s);
void ottoipc_enqueue_report_job(ottoipc_report_job_pdu_st *s);
void ottoipc_enqueue_update_job(ottoipc_update_job_pdu_st *s);
void ottoipc_enqueue_delete_job(ottoipc_delete_job_pdu_st *s);
int  ottoipc_pdu_size(void *p);
int  ottoipc_send_all(int socket);
int  ottoipc_recv_all(int socket);
int  ottoipc_recv_str(void *t, size_t tlen, RECVBUF *recvbuf);
int  ottoipc_dequeue_pdu(void **response);
void log_received_pdu(void *p);

char *stropcode(int i);
char *strresultcode(int i);
char *strstatus(int i);
char *strpdutype(int i);

// ottocond prototypes
int   compile_expression (char *output, DBCTX *ctx, char *condition, int outlen);
int   evaluate_expr (char *s);
int   evaluate_stat (ecnd_st *c);
int   evaluate_term (ecnd_st *c);
int   validate_dependencies(DBCTX *ctx, char *name, char *condition);

// ottocrud prototypes
int  create_job(ottoipc_create_job_pdu_st *pdu, DBCTX *ctx);
int  report_job(ottoipc_simple_pdu_st     *pdu, DBCTX *ctx);
int  update_job(ottoipc_update_job_pdu_st *pdu, DBCTX *ctx);
void delete_job(ottoipc_simple_pdu_st     *pdu, DBCTX *ctx);
void delete_box(ottoipc_simple_pdu_st     *pdu, DBCTX *ctx);

// ottolog prototypes
OTTOLOG *ottolog_init(char *logdir, char *logfile, int level, int truncate);
int   lprintf(OTTOLOG *logp, int level, char *format, ...);
int   lsprintf(OTTOLOG *logp, int level, char *format, ...);
void  lperror(OTTOLOG *logp, int level, char *string);
char *levelstr(int level);

// ottoutil prototypes
void  escape_html(char *t, int tlen);
void  escape_mspdi(char *t, int tlen);
void  mime_quote(char *t, int tlen);
char *copy_word(char *t, char *s);
void  copy_eol(char *t, char *s);
char *format_timestamp(time_t t);
void  mkcode(char *t, char *s);
void  mkampcode(char *t, char *s);
int   xmltoi(char *source);
void  xmlcpy(char *t, char *s);
int   xmlchr(char *s, int c);
int   xmlcmp(char *s1, char *s2);
int   read_stdin(DYNBUF *b, char *prompt);
void  advance_word(DYNBUF *b);
void  regress_word(DYNBUF *b);
int   strwcmp(char *pTameText, char *pWildText);
char *expand_path(char *s);
int   get_file_format(char *format);
int   hex_to_int(char hexChar);
void  unescape_input(char *input);
int   bprintf(DYNBUF *b, char *format, ...);
char *copy_envvar_assignment(int *rc, char *t, int tlen, char *s);
int   validate_envvar_assignment(char *s);

// strptime prototypes
char *otto_strptime(const char *buf, const char *fmt, struct tm *tm);

// signals prototypes
void  init_signals(void (* func)(int));
int   sig_set(int num, void (* func)(int));
int   sig_hold(int num);
int   sig_rlse(int num);
char *strsignal(int signo);

// format prototypes
int write_dtl(JOBLIST *joblist);
int write_pid(JOBLIST *joblist);
int write_sum(JOBLIST *joblist);
int write_cnd(JOBLIST *joblist, DBCTX *ctx);

int parse_csv(DYNBUF *b, JOBLIST *joblist);
int write_csv(JOBLIST *joblist);
int buffer_csv(DYNBUF *b, JOBLIST *joblist);

int parse_jil(DYNBUF *b, JOBLIST *joblist);
int write_jil(JOBLIST *joblist);
int jil_reserved_word(char *s);

int parse_mspdi(DYNBUF *b, JOBLIST *joblist);
int write_mspdi(JOBLIST *joblist, int autoschedule);
int buffer_mspdi(DYNBUF *b, JOBLIST *joblist, int autoschedule);

int write_htmldtl(int fd, JOBLIST *joblist, ottohtml_query *q);
int write_htmljil(int fd, JOBLIST *joblist, ottohtml_query *q);
int write_htmlmspdi(int fd, JOBLIST *joblist, ottohtml_query *q);
int write_htmlsum(int fd, JOBLIST *joblist, ottohtml_query *q);
int write_htmlversion(int fd) ;



// linenoise
#define LINENOISE_DEFAULT_HISTORY_MAX_LEN 100
#define LINENOISE_MAX_LINE 4096

typedef struct linenoiseCompletions
{
  size_t len;
  char **cvec;
} linenoiseCompletions;

typedef void(linenoiseCompletionCallback)(const char *, linenoiseCompletions *);
typedef char*(linenoiseHintsCallback)(const char *, int *color, int *bold);
typedef void(linenoiseFreeHintsCallback)(void *);

void  linenoiseSetCompletionCallback(linenoiseCompletionCallback *);
void  linenoiseSetHintsCallback(linenoiseHintsCallback *);
void  linenoiseSetFreeHintsCallback(linenoiseFreeHintsCallback *);
void  linenoiseAddCompletion(linenoiseCompletions *, const char *);
char *linenoise(const char *prompt);
void  linenoiseFree(void *ptr);
int   linenoiseHistoryAdd(const char *line);
int   linenoiseHistorySetMaxLen(int len);
int   linenoiseHistorySave(const char *filename);
int   linenoiseHistoryLoad(const char *filename);
void  linenoiseClearScreen(void);
void  linenoiseSetMultiLine(int ml);
void  linenoisePrintKeyCodes(void);

#endif
