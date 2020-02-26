#ifndef _LOG_H_
#define _LOG_H_

#define LOG_FALSE     0
#define LOG_TRUE      1

#define LOG_SUCCESS   0
#define LOG_FAIL     -1

#define LOG_BUFLEN  8192
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


int init_log(char *logdir, char *logfile, int level, int truncate);
int lprintf(int level, char *format, ...);
int lsprintf(int level, char *format, ...);
void lperror(int level, char *string);
char *levelstr(int level);


#endif
//
// vim: ts=3 sw=3 ai
//
