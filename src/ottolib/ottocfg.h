#ifndef _OTTOCFG_H_
#define _OTTOCFG_H_

#include <stdint.h>
#include <sys/types.h>

#define MAX_ENVVAR  1000

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
	uint16_t  server_port;

	// user identification variables
	int       euid;  // effective user id
	int       ruid;  // real user id

	// standard environment
	char     *env_ottocfg;
	char     *env_ottolog;
	char     *env_ottoenv;

	// general variables
	int       pause;
	int       debug;
	int       verbose;
	int       show_sofar;
	int       path_overridden;
	time_t    ottoenv_mtime;

	char     *envvar[MAX_ENVVAR];   // environment to be passed to child jobs
	char     *envvar_s[MAX_ENVVAR]; // static environment from $OTTOCFG
	char     *envvar_d[MAX_ENVVAR]; // dynamic environment from $OTTOENV
	int       n_envvar;
	int       n_envvar_s;
	int       n_envvar_d;
} ottocfg_st;

extern ottocfg_st cfg;

int  init_cfg(int argc, char **argv);
int  read_cfgfile(void);
void rebuild_environment(void);
void log_initialization(void);
void log_args(int argc, char **argv);
void log_cfg(void);

#endif
//
// vim: ts=3 sw=3 ai
//
