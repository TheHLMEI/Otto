#ifndef _OTTOIPC_H_
#define _OTTOIPC_H_

#include <netinet/in.h>
#include "ottodb.h"

#define OTTOIPC_MAX_HOST_LENGTH 255

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
	START_JOB,
	KILL_JOB,
	SEND_SIGNAL,
	RESET_JOB,
	CHANGE_STATUS,
	JOB_ON_AUTOHOLD,
	JOB_OFF_AUTOHOLD,
	JOB_ON_HOLD,
	JOB_OFF_HOLD,
	JOB_ON_NOEXEC,
	JOB_OFF_NOEXEC,
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
	BOX_NOT_FOUND,
	BOX_DELETED,
	JOB_ALREADY_EXISTS,
	JOB_DEPENDS_ON_MISSING_JOB,
	JOB_DEPENDS_ON_ITSELF,
	NO_SPACE_AVAILABLE,
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


#define COMMON_PDU_ATTRIBUTES uint8_t pdu_type; uint8_t opcode; char name[NAMLEN+1]; uint8_t option;

#pragma pack(1)
typedef struct _ottoipc_pdu_header_st
{
	int  version;
   int  payload_length;
   int  euid;
   int  ruid;
} ottoipc_pdu_header_st;

typedef struct _ottoipc_simple_pdu_st
{
	COMMON_PDU_ATTRIBUTES
} ottoipc_simple_pdu_st;

typedef struct _ottoipc_create_job_pdu_st
{
	COMMON_PDU_ATTRIBUTES
	char    type;
	char    box_name[NAMLEN+1];
	char    description[DSCLEN+1];
	char    command[CMDLEN+1];
	char    condition[CNDLEN+1];
	char    auto_hold;
	char    date_conditions;
	char    days_of_week;
	int64_t start_mins;
	int64_t start_times[24];
} ottoipc_create_job_pdu_st;

typedef struct _ottoipc_report_job_pdu_st
{
	COMMON_PDU_ATTRIBUTES
	int16_t job_id;
	int16_t level;
	int16_t parent;
	int16_t head;
	int16_t tail;
	int16_t prev;
	int16_t next;
	char    type;
	char    box_name[NAMLEN+1];
	char    description[DSCLEN+1];
	char    command[CMDLEN+1];
	char    condition[CNDLEN+1];
	char    expression[CNDLEN+1];
	char    expr_fail;
	char    auto_hold;
	char    date_conditions;
	char    days_of_week;
	int64_t start_mins;
	int64_t start_times[24];
	char    on_noexec;
	char    status;
	pid_t   pid;
	int     exit_status;
	time_t  start;
	time_t  finish;
	time_t  duration;
	char    flag;
} ottoipc_report_job_pdu_st;

typedef struct _ottoipc_update_job_pdu_st
{
	COMMON_PDU_ATTRIBUTES
	char    type;
	char    box_name[NAMLEN+1];
	char    description[DSCLEN+1];
	char    command[CMDLEN+1];
	char    condition[CNDLEN+1];
	char    auto_hold;
	char    date_conditions;
	char    days_of_week;
	int64_t start_mins;
	int64_t start_times[24];
} ottoipc_update_job_pdu_st;

typedef struct _ottoipc_simple_pdu_st ottoipc_delete_job_pdu_st;

#pragma pack()


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
int  ottoipc_dequeue_pdu(void **response);
void log_received_pdu(void *p);

char *stropcode(int i);
char *strresultcode(int i);
char *strstatus(int i);
char *strpdutype(int i);

#endif
// 
// vim: ts=3 sw=3 ai
// 
