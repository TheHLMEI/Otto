#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "ottobits.h"
#include "ottocfg.h"
#include "ottocond.h"
#include "ottoipc.h"
#include "simplelog.h"

extern SIMPLELOG *logp;


size_t ipc_bufferlen = 0;

char   *send_buffer  = NULL;
char   *send_header  = NULL;
char   *send_pdus    = NULL;

char   *recv_buffer  = NULL;
char   *recv_header  = NULL;
char   *recv_pdus    = NULL;
char   *dequeued_pdu = NULL;


#define MAX(a,b) (((a)>(b))?(a):(b))

int MAX_PDU_LENGTH=MAX(sizeof(ottoipc_simple_pdu_st),
						 MAX(sizeof(ottoipc_create_job_pdu_st),
						 MAX(sizeof(ottoipc_report_job_pdu_st), sizeof(ottoipc_update_job_pdu_st))
						 ));
#undef MAX


int allocate_ipc_buffers(void);
int ottoipc_recv_some(int socket, char *buffer, size_t ipc_bufferlen);

int
init_server_ipc(in_port_t port, int backlog)
{
	int    retval = -1;
	int    on = 1;
	struct sockaddr_in6   addr;

	// Create an AF_INET6 stream socket to receive connections on
	retval = socket(AF_INET6, SOCK_STREAM, 0);
	if(retval < 0)
	{
		perror("socket() failed");
	}

	// Allow socket descriptor to be reuseable
	if(retval >= 0 &&
		setsockopt(retval, SOL_SOCKET,  SO_REUSEADDR, (char *)&on, sizeof(on)) < 0)
	{
		perror("setsockopt() failed");
		close(retval);
		retval = -1;
	}

	// Set socket to be nonblocking. All of the sockets for
	// the incoming connections will also be nonblocking since
	// they will inherit that state from the listening socket.
	if(retval >= 0 &&
		ioctl(retval, FIONBIO, (char *)&on) < 0)
	{
		perror("ioctl() failed");
		close(retval);
		retval = -1;
	}

	// Bind the socket
	if(retval >= 0)
	{
		memset(&addr, 0, sizeof(addr));
		addr.sin6_family = AF_INET6;
		addr.sin6_addr   = in6addr_any;
		addr.sin6_port   = htons(port);
		if(bind(retval, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		{
			perror("bind() failed");
			close(retval);
			retval = -1;
		}
	}


	// Set the listen back log
	if(retval >= 0 &&
		listen(retval, backlog) < 0)
	{
		perror("listen() failed");
		close(retval);
		retval = -1;
	}


	// allocate IPC buffers
	if(retval >= 0 &&
		allocate_ipc_buffers() == OTTO_FAIL)
	{
		close(retval);
		retval = -1;
	}

	return(retval);
}



int
init_client_ipc(char *hostname, in_port_t portnumber)
{
	int    retval = -1;
	int    rc;
	char   host[OTTOIPC_MAX_HOST_LENGTH];
	char   port[6];
	struct in6_addr addr;
	struct addrinfo hints, *res=NULL;

	// If a hostname was passed in, copy it to host,
	// otherwise copy "localhost"
	if(hostname != NULL    &&
		hostname[0] != '\0')
		strcpy(host, hostname);
	else
		strcpy(host, "localhost");

	// copy in port number
	sprintf(port, "%u", portnumber);

	memset(&hints, 0x00, sizeof(hints));
	hints.ai_flags    = AI_NUMERICSERV;
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	// Check if a numeric address was passed in for the host using
	// inet_pton() to convert the text form of the address to binary
	// form.
	if(inet_pton(AF_INET, host, &addr) == 1)
	{
		// valid IPv4 text address
		hints.ai_family = AF_INET;
		hints.ai_flags |= AI_NUMERICHOST;
	}
	else
	{
		if(inet_pton(AF_INET6, host, &addr) == 1)
		{
			// valid IPv6 text address
			hints.ai_family = AF_INET6;
			hints.ai_flags |= AI_NUMERICHOST;
		}
	}

	// Get the address information for the server using getaddrinfo().
	if((rc = getaddrinfo(host, port, &hints, &res)) != 0)
	{
		printf("Host not found --> %s\n", gai_strerror(rc));
		if (rc == EAI_SYSTEM)
			perror("getaddrinfo() failed");
	}
	else
	{
		if((retval = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
		{
			perror("socket() failed");
		}

		// Use the connect() function to establish a connection to the
		// server.
		if(retval >= 0 &&
			connect(retval, res->ai_addr, res->ai_addrlen) < 0)
		{
			// Note: the res is a linked list of addresses found for server.
			// If the connect() fails to the first one, subsequent addresses
			// (if any) in the list can be tried if required.
			perror("connect() failed");
			close(retval);
			retval = -1;
		}
	}

	// allocate IPC buffers
	if(retval >= 0 &&
		allocate_ipc_buffers() == OTTO_FAIL)
	{
		close(retval);
		retval = -1;
	}

	// Free any results returned from getaddrinfo
	if(res != NULL)
		freeaddrinfo(res);

	return(retval);
}



int
allocate_ipc_buffers()
{
	int retval = OTTO_SUCCESS;

	ipc_bufferlen = (2*MAX_PDU_LENGTH*cfg.ottodb_maxjobs) + sizeof(ottoipc_pdu_header_st);

	if(cfg.debug == OTTO_TRUE)
		lprintf(logp, INFO, "IPC buffer length = %lu\n", ipc_bufferlen);

	// allocate send buffer
	if(retval == OTTO_SUCCESS &&
		(send_buffer = malloc(ipc_bufferlen)) == NULL)
	{
		perror("sendbuffer malloc() failed");
		retval = OTTO_FAIL;
	}
	else
	{
		send_header = send_buffer;
		send_pdus   = send_buffer + sizeof(ottoipc_pdu_header_st);
	}

	// allocate recv buffer
	if(retval == OTTO_SUCCESS &&
		(recv_buffer = malloc(ipc_bufferlen)) == NULL)
	{
		perror("recvbuffer malloc() failed");
		free(send_buffer);
		retval = OTTO_FAIL;
	}
	else
	{
		recv_header = recv_buffer;
		recv_pdus   = recv_buffer + sizeof(ottoipc_pdu_header_st);
	}

	return(retval);
}



void
ottoipc_initialize_send()
{
	ottoipc_pdu_header_st *h = (ottoipc_pdu_header_st *)send_header;

	memset(h, 0, sizeof(ottoipc_pdu_header_st));

	h->version        = 1;
	h->euid           = cfg.euid;
	h->ruid           = cfg.ruid;
	h->payload_length = 0;
}


void
ottoipc_enqueue_simple_pdu(ottoipc_simple_pdu_st *s)
{
	ottoipc_pdu_header_st *h       = (ottoipc_pdu_header_st *)send_header;
	ottoipc_simple_pdu_st *t = (ottoipc_simple_pdu_st *)&send_pdus[h->payload_length];

	memcpy(t, s, sizeof(ottoipc_simple_pdu_st));
	t->pdu_type = SIMPLE_PDU;
	
	h->payload_length += sizeof(ottoipc_simple_pdu_st);
}



void
ottoipc_enqueue_create_job(ottoipc_create_job_pdu_st *s)
{
	ottoipc_pdu_header_st *h       = (ottoipc_pdu_header_st *)send_header;
	ottoipc_create_job_pdu_st *t = (ottoipc_create_job_pdu_st *)&send_pdus[h->payload_length];

	memcpy(t, s, sizeof(ottoipc_create_job_pdu_st));
	t->pdu_type = CREATE_JOB_PDU;
	
	h->payload_length += sizeof(ottoipc_create_job_pdu_st);
}



void
ottoipc_enqueue_report_job(ottoipc_report_job_pdu_st *s)
{
	ottoipc_pdu_header_st *h       = (ottoipc_pdu_header_st *)send_header;
	ottoipc_report_job_pdu_st *t = (ottoipc_report_job_pdu_st *)&send_pdus[h->payload_length];

	memcpy(t, s, sizeof(ottoipc_report_job_pdu_st));
	t->pdu_type = REPORT_JOB_PDU;
	
	h->payload_length += sizeof(ottoipc_report_job_pdu_st);
}



void
ottoipc_enqueue_update_job(ottoipc_update_job_pdu_st *s)
{
	ottoipc_pdu_header_st *h       = (ottoipc_pdu_header_st *)send_header;
	ottoipc_update_job_pdu_st *t = (ottoipc_update_job_pdu_st *)&send_pdus[h->payload_length];

	memcpy(t, s, sizeof(ottoipc_update_job_pdu_st));
	t->pdu_type = UPDATE_JOB_PDU;
	
	h->payload_length += sizeof(ottoipc_update_job_pdu_st);
}



void
ottoipc_enqueue_delete_job(ottoipc_delete_job_pdu_st *s)
{
	ottoipc_pdu_header_st *h       = (ottoipc_pdu_header_st *)send_header;
	ottoipc_delete_job_pdu_st *t = (ottoipc_delete_job_pdu_st *)&send_pdus[h->payload_length];

	memcpy(t, s, sizeof(ottoipc_delete_job_pdu_st));
	t->pdu_type = DELETE_JOB_PDU;
	
	h->payload_length += sizeof(ottoipc_delete_job_pdu_st);
}



int
ottoipc_pdu_size(void *p)
{
	int retval = 0;
	ottoipc_simple_pdu_st *pdu = (ottoipc_simple_pdu_st *)p;

	switch(pdu->pdu_type)
	{
		case SIMPLE_PDU:     retval = sizeof(ottoipc_simple_pdu_st);     break;
		case CREATE_JOB_PDU: retval = sizeof(ottoipc_create_job_pdu_st); break;
		case REPORT_JOB_PDU: retval = sizeof(ottoipc_report_job_pdu_st); break;
		case UPDATE_JOB_PDU: retval = sizeof(ottoipc_update_job_pdu_st); break;
		case DELETE_JOB_PDU: retval = sizeof(ottoipc_delete_job_pdu_st); break;
	}

	return(retval);
}



int
ottoipc_send_all(int socket)
{
	int retval = OTTO_SUCCESS;
	ottoipc_pdu_header_st *h = (ottoipc_pdu_header_st *)send_header;
	size_t length = sizeof(ottoipc_pdu_header_st) + h->payload_length;
	char *p = send_header;
	int i;

	while(length > 0)
	{
		i = send(socket, p, length, 0);

		if(i < 1)
		{
			retval =  OTTO_FAIL;
			break;
		}

		p += i;
		length -= i;
	}

	return(retval);
}



int
ottoipc_recv_all(int socket)
{
	int retval = OTTO_SUCCESS;
	ottoipc_pdu_header_st *h = (ottoipc_pdu_header_st *)recv_header;

	// first recv the message header
	if((retval = ottoipc_recv_some(socket, recv_buffer, sizeof(ottoipc_pdu_header_st))) == OTTO_SUCCESS)
	{
		retval = ottoipc_recv_some(socket, &recv_buffer[sizeof(ottoipc_pdu_header_st)], h->payload_length);
	}

	dequeued_pdu = recv_pdus;

	return(retval);
}



int
ottoipc_recv_some(int socket, char *buffer, size_t ipc_bufferlen)
{
	int retval = OTTO_SUCCESS;
	int exit_loop = OTTO_FALSE;
	int n = 0;
	int total = 0;

	// get requested length
	while(exit_loop == OTTO_FALSE)
	{
		n = recv(socket, &buffer[total], (ipc_bufferlen - total), 0);
		switch(n)
		{

			case -1: // interrupted log it and exit loop
				switch(errno)
				{
					case EAGAIN:       lprintf(logp, MAJR, "recv returned EAGAIN\n");       break;
					case EBADF:        lprintf(logp, MAJR, "recv returned EBADF\n");        break;
					case ECONNREFUSED: lprintf(logp, MAJR, "recv returned ECONNREFUSED\n"); break;
					case ECONNRESET:   lprintf(logp, MAJR, "recv returned ECONNRESET\n");   break;
					case EFAULT:       lprintf(logp, MAJR, "recv returned EFAULT\n");       break;
					case EINTR:        lprintf(logp, MAJR, "recv returned EINTR\n");        break;
					case EINVAL:       lprintf(logp, MAJR, "recv returned EINVAL\n");       break;
					case ENOMEM:       lprintf(logp, MAJR, "recv returned ENOMEN\n");       break;
					case ENOTCONN:     lprintf(logp, MAJR, "recv returned ENOTCONN\n");     break;
					case ENOTSOCK:     lprintf(logp, MAJR, "recv returned ENOTSOCK\n");     break;
					default:           lprintf(logp, MAJR, "recv returned (%d)\n", errno);  break;
				}
				exit_loop = OTTO_TRUE;
				retval    = OTTO_FAIL;
				break;
			case 0:  // other end hung up, okay to exit
				exit_loop = OTTO_TRUE;
				retval    = OTTO_FAIL;
				break;
			default:
				total += n;
				if(total == ipc_bufferlen)
					exit_loop = OTTO_TRUE;
				break;
		}
	}

	return(retval);
}



int
ottoipc_dequeue_pdu(void **response)
{
	int retval = OTTO_SUCCESS;
	ottoipc_pdu_header_st *h = (ottoipc_pdu_header_st *)recv_header;

	if(dequeued_pdu < (recv_pdus + h->payload_length))
	{
		*response = (void *)dequeued_pdu;
		dequeued_pdu = (dequeued_pdu + ottoipc_pdu_size(dequeued_pdu));
	}
	else
	{
		*response = NULL;
		retval = OTTO_FAIL;
	}

return(retval);
}




void
log_received_pdu(void *p)
{
	ottoipc_pdu_header_st *header = (ottoipc_pdu_header_st *)recv_header;
	ottoipc_simple_pdu_st *pdu    = (ottoipc_simple_pdu_st *)p;
	struct passwd *epw;
	struct passwd *rpw;
	char ename[512], rname[512];

	strcpy(ename, "unknown");
	strcpy(rname, "unknown");

	if((epw = getpwuid(header->euid)) != NULL)
		strcpy(ename, epw->pw_name);

	if((rpw = getpwuid(header->ruid)) != NULL)
		strcpy(rname, rpw->pw_name);

	lsprintf(logp, INFO, "Received PDU:\n");
	lsprintf(logp, CATI, "  user   = %s (%s)\n", ename, rname);
	lsprintf(logp, CATI, "  type   = %s\n", strpdutype(pdu->pdu_type));
	lsprintf(logp, CATI, "  opcode = %s\n", stropcode(pdu->opcode));
	lsprintf(logp, CATI, "  name   = %s\n", pdu->name);
	if(pdu->option < NO_STATUS)
		lsprintf(logp, CATI, "  option = %s\n", strsignal(pdu->option));
	else if(pdu->option < ACK)
	{
		if(pdu->option > NO_STATUS && pdu->option < STATUS_TOTAL)
			lsprintf(logp, CATI, "  option = %s\n", strstatus(pdu->option));
		else
			lsprintf(logp, CATI, "  option = Undefined status code\n");
	}
	else
	{
		if(pdu->option >= ACK && pdu->option < RESULTCODE_TOTAL)
			lsprintf(logp, CATI, "  option = %s\n", strresultcode(pdu->option));
		else
			lsprintf(logp, CATI, "  option = Undefined result code\n");
	}
	lsprintf(logp, END, "");
}



char *
stropcode(int i)
{
	char *retval = NULL;
	static char msg[32];

	switch(i)
	{
		case NOOP:             retval = "NOOP";             break;

		// job definition operations
		case CREATE_JOB:       retval = "CREATE_JOB";       break;
		case REPORT_JOB:       retval = "REPORT_JOB";       break;
		case UPDATE_JOB:       retval = "UPDATE_JOB";       break;
		case DELETE_JOB:       retval = "DELETE_JOB";       break;
		case DELETE_BOX:       retval = "DELETE_BOX";       break;
		case CRUD_TOTAL:       retval = "CRUD_TOTAL";       break;

		// scheduler control operations
		case START_JOB:        retval = "START_JOB";        break;
		case KILL_JOB:         retval = "KILL_JOB";         break;
		case MOVE_JOB_TOP:     retval = "MOVE_JOB_TOP";     break;
		case MOVE_JOB_UP:      retval = "MOVE_JOB_UP";      break;
		case MOVE_JOB_DOWN:    retval = "MOVE_JOB_DOWN";    break;
		case MOVE_JOB_BOTTOM:  retval = "MOVE_JOB_BOTTOM";  break;
		case RESET_JOB:        retval = "RESET_JOB";        break;
		case SEND_SIGNAL:      retval = "SEND_SIGNAL";      break;
		case CHANGE_STATUS:    retval = "CHANGE_STATUS";    break;
		case JOB_ON_AUTOHOLD:  retval = "JOB_ON_AUTOHOLD";  break;
		case JOB_OFF_AUTOHOLD: retval = "JOB_OFF_AUTOHOLD"; break;
		case JOB_ON_HOLD:      retval = "JOB_ON_HOLD";      break;
		case JOB_OFF_HOLD:     retval = "JOB_OFF_HOLD";     break;
		case JOB_ON_NOEXEC:    retval = "JOB_ON_NOEXEC";    break;
		case JOB_OFF_NOEXEC:   retval = "JOB_OFF_NOEXEC";   break;
		case SCHED_TOTAL:      retval = "SCHED_TOTAL";      break;

		 // daemon control operations
		case PING:             retval = "PING";             break;
		case VERIFY_DB:        retval = "VERIFY_DB";        break;
		case DEBUG_ON:         retval = "DEBUG_ON";         break;
		case DEBUG_OFF:        retval = "DEBUG_OFF";        break;
		case REFRESH:          retval = "REFRESH";          break;
		case PAUSE_DAEMON:     retval = "PAUSE_DAEMON";     break;
		case RESUME_DAEMON:    retval = "RESUME_DAEMON";    break;
		case STOP_DAEMON:      retval = "STOP_DAEMON";      break;
		case OPCODE_TOTAL:     retval = "OPCODE_TOTAL";     break;

		default: sprintf(msg, "Opcode %d", i); retval = msg; break;
	}

	return retval;
}



char *
strresultcode(int i)
{
	char *retval = NULL;
	static char msg[32];

	switch(i)
	{
		case ACK:                        retval = "ACK";                        break;
		case NACK:                       retval = "NACK";                       break;

		// job definition operations
		case JOB_CREATED:                retval = "JOB_CREATED";                break;
		case JOB_REPORTED:               retval = "JOB_REPORTED";               break;
		case JOB_UPDATED:                retval = "JOB_UPDATED";                break;
		case JOB_NOT_FOUND:              retval = "JOB_NOT_FOUND";              break;
		case JOB_DELETED:                retval = "JOB_DELETED";                break;
		case BOX_NOT_FOUND:              retval = "BOX_NOT_FOUND";              break;
		case BOX_DELETED:                retval = "BOX_DELETED";                break;
		case JOB_ALREADY_EXISTS:         retval = "JOB_ALREADY_EXISTS";         break;
		case JOB_DEPENDS_ON_MISSING_JOB: retval = "JOB_DEPENDS_ON_MISSING_JOB"; break;
		case JOB_DEPENDS_ON_ITSELF:      retval = "JOB_DEPENDS_ON_ITSELF";      break;
		case NO_SPACE_AVAILABLE:         retval = "NO_SPACE_AVAILABLE";         break;
		case RESULTCODE_TOTAL:           retval = "RESULTCODE_TOTAL";           break;

		default: sprintf(msg, "Resultcode %d", i); retval = msg; break;
	}

	return retval;
}



char *
strstatus(int i)
{
	char *retval = NULL;
	static char msg[32];

	switch(i)
	{
		case NO_STATUS:    retval = "NO STATUS";    break;
		case FAILURE:      retval = "FAILURE";      break;
		case INACTIVE:     retval = "INACTIVE";     break;
		case RUNNING:      retval = "RUNNING";      break;
		case SUCCESS:      retval = "SUCCESS";      break;
		case TERMINATED:   retval = "TERMINATED";   break;
		case STATUS_TOTAL: retval = "STATUS_TOTAL"; break;

		default: sprintf(msg, "Status %d", i); retval = msg; break;
	}

	return retval;
}



char *
strpdutype(int i)
{
	char *retval = NULL;
	static char msg[32];

	switch(i)
	{
		case SIMPLE_PDU:     retval = "SIMPLE_PDU";     break;
		case CREATE_JOB_PDU: retval = "CREATE_JOB_PDU"; break;
		case REPORT_JOB_PDU: retval = "REPORT_JOB_PDU"; break;
		case UPDATE_JOB_PDU: retval = "UPDATE_JOB_PDU"; break;
		case DELETE_JOB_PDU: retval = "DELETE_JOB_PDU"; break;
		case PDUTYPE_TOTAL:  retval = "PDUTYPE_TOTAL";  break;

		default: sprintf(msg, "PDUtype %d", i); retval = msg; break;
	}

	return retval;
}



// 
// vim: ts=3 sw=3 ai
// 
