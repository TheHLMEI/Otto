#ifndef _SIMPLELOG_H_
#define _SIMPLELOG_H_

#include <stdio.h>

#define SIMPLELOG_FALSE     0
#define SIMPLELOG_TRUE      1

#define SIMPLELOG_SUCCESS   0
#define SIMPLELOG_FAIL     -1

#define SIMPLELOG_BUFLEN  8192

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

// simplelog structure
typedef struct
{
	char *logfile;
	FILE *logfp;
	int   loglevel;
	int   lsstarted;
	char  logfile_buffer[SIMPLELOG_BUFLEN];
	char  console_buffer[SIMPLELOG_BUFLEN];
	char  last_logged_day[30];
	int   lslevel;
} SIMPLELOG;


SIMPLELOG *simplelog_init(char *logdir, char *logfile, int level, int truncate);
int   lprintf(SIMPLELOG *logp, int level, char *format, ...);
int   lsprintf(SIMPLELOG *logp, int level, char *format, ...);
void  lperror(SIMPLELOG *logp, int level, char *string);
char *levelstr(int level);


#endif
//
// vim: ts=3 sw=3 ai
//
