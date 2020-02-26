#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


#include "ottolog.h"


// log variables
typedef struct _log_st
{
	char *logfile;
	FILE *logfp;
	int   loglevel;
	int   lsstarted;
	char  logfile_buffer[LOG_BUFLEN];
	char  console_buffer[LOG_BUFLEN];
	char  last_logged_day[30];
	int   lslevel;
} log_st;


log_st g_log = {0};


int vlprintf(int level, char *format, va_list ap);
int vlsprintf(int level, char *format, va_list ap);



int
init_log(char *logdir, char *logfile, int level, int truncate)
{
	int  retval = LOG_SUCCESS;
	int  i;
	char fname[PATH_MAX], *mode, *extension;

	if(truncate)
		mode = "w";
	else
		mode = "a";

	if(logdir == NULL || logdir[0] == '\0')
		logdir = ".";

	if(logfile != NULL && logfile[0] != '\0')
	{
		// remove redundant slashes
		for(i=(int)strlen(logdir)-1; i>0; i--)
			if(logdir[i] != '/')
				break;
			else
				logdir[i] = '\0';

		// look for an extension
		for(i=(int)strlen(logfile)-1; i>0; i--)
			if(logfile[i] == '.' || logfile[i] == '/')
				break;

		// if an extension is found leave it there otherwise add one
		if(logfile[i] == '.')
			extension = "";
		else
			extension = ".log";

		if(access(logdir, (R_OK | W_OK)) == 0)
		{
			sprintf(fname, "%s/%s%s", logdir, logfile, extension);
			if((g_log.logfp=fopen(fname, mode)) == NULL)
			{
				fprintf(stderr, "Open on '%s' failed!\n", fname);
				retval = LOG_FAIL;
			}
			else
			{
				if(level > NO_LOG)
					fprintf(g_log.logfp, "\n");
				if((g_log.logfile = strdup(fname)) == NULL)
				{
					fprintf(stderr, "strdup on logfile name failed!\n");
					retval = LOG_FAIL;
				}
			}
		}
	}
	else
	{
		fprintf(stderr, "logfile isn't defined.\n");
		retval = LOG_FAIL;
	}

	g_log.loglevel = level;

	return(retval);
}



int
lprintf(int level, char *format, ...)
{
	va_list ap;
	int     retval;

	if(level <= g_log.loglevel)
	{
		va_start(ap, format);
		retval = vlprintf(level, format, ap);
		va_end(ap);
	}

	return(retval);
}



int
vlprintf(int level, char *format, va_list ap)
{
	char   buffer[LOG_BUFLEN];
	int    length;
	time_t now;
	int    retval = 0;
	char   fulldate[30], day[30];

	now = time(0);
	strcpy(fulldate, ctime(&now));
	strcpy(day, fulldate);
	strcpy(&day[11], &fulldate[20]);
	if(strcmp(day, g_log.last_logged_day) != 0)
	{
		if(g_log.logfp != NULL)
		{
			fprintf(g_log.logfp, "\n               %s", day);
			strcpy(g_log.last_logged_day, day);
		}
	}

	sprintf(buffer, "%8.8s %s: ", &fulldate[11], levelstr(level));

	vsprintf(buffer + strlen(buffer), format, ap);

	length = strlen(buffer);
	if(buffer[length-1] != '\n')
	{
		buffer[length] = '\n';
		length++;
		buffer[length] = '\0';
	}

	/* Write message to the log file */
	if(g_log.logfp != NULL)
	{
		fprintf(g_log.logfp, "%s", buffer);
		fflush(g_log.logfp);
	}

	/* Write message to the console if level is not MINR or INFO */
	if(g_log.logfp == NULL || level < MINR)
		fprintf(stderr, "%s", buffer+15);

	return(retval);
}



int
lsprintf(int level, char *format, ...)
{
	va_list ap;
	int     retval;

	if(level <= g_log.loglevel)
	{
		va_start(ap, format);
		retval = vlsprintf(level, format, ap);
		va_end(ap);
	}

	return(retval);
}



int
vlsprintf(int level, char *format, va_list ap)
{
	int    length;
	time_t now;
	int    retval = 0;
	char   fulldate[30], day[30];
	char   buffer[LOG_BUFLEN];

	if(level != CAT  &&
		level != CATI &&
		level != END  &&
		level != ENDI)
	{
		if(g_log.lsstarted == LOG_TRUE)
			lprintf(INFO, "lsprintf restarted!\n");
		g_log.lsstarted = LOG_TRUE;
		g_log.lslevel = level;

		now = time(0);
		strcpy(fulldate, ctime(&now));
		strcpy(day, fulldate);
		strcpy(&day[11], &fulldate[20]);
		if(strcmp(day, g_log.last_logged_day) != 0)
		{
			fprintf(g_log.logfp, "\n               %s", day);
			strcpy(g_log.last_logged_day, day);
		}

		sprintf(g_log.logfile_buffer, "%8.8s %s: ", &fulldate[11], levelstr(level));
		g_log.console_buffer[0] = '\0';
	}

	if(g_log.lsstarted == LOG_TRUE)
	{
		if(level == CATI || level == ENDI)
			strcat(g_log.logfile_buffer, "               ");

		vsprintf(buffer, format, ap );

		strcat(g_log.logfile_buffer, buffer);
		strcat(g_log.console_buffer, buffer);

		if(level == END || level == ENDI)
		{
			length = strlen(g_log.logfile_buffer);
			if(g_log.logfile_buffer[length-1] != '\n')
			{
				g_log.logfile_buffer[length] = '\n';
				length++;
				g_log.logfile_buffer[length] = '\0';
			}

			length = strlen(g_log.console_buffer);
			if(g_log.console_buffer[length-1] != '\n')
			{
				g_log.console_buffer[length] = '\n';
				length++;
				g_log.console_buffer[length] = '\0';
			}

			/* Write message to the log file */
			if(g_log.logfp != NULL)
			{
				fprintf(g_log.logfp, "%s", g_log.logfile_buffer);
				fflush(g_log.logfp);
			}

			/* Write message to the console if level is not MINR or INFO */
			if(g_log.logfp == NULL || g_log.lslevel < MINR)
				fprintf(stderr, "%s", g_log.console_buffer);

			g_log.lsstarted = LOG_FALSE;
		}
	}

	return(retval);
}



void
lperror(int level, char *string)
{
	int add_colon;

	if(level <= g_log.loglevel)
	{
		add_colon = 1;
		if(strlen(string) == 1 && string[0] == ' ')
			add_colon = 0;

		lprintf(level, "%s%s%s\n", string, add_colon ? ": " : "",
				strerror(errno));
	}
}



char *
levelstr(int level)
{
	char *p;

	switch(level)
	{
		case DBG9: p = "DBG9"; break;
		case DBG8: p = "DBG8"; break;
		case DBG7: p = "DBG7"; break;
		case DBG6: p = "DBG6"; break;
		case DBG5: p = "DBG5"; break;
		case DBG4: p = "DBG4"; break;
		case DBG3: p = "DBG3"; break;
		case DBG2: p = "DBG2"; break;
		case DBG1: p = "DBG1"; break;
		case INFO: p = "INFO"; break;
		case MINR: p = "MINR"; break;
		case MAJR: p = "MAJR"; break;
		default:   p = "UNKN"; break;
	}

	return(p);
}



/*
vim: ts=3 sw=3 ai
*/
