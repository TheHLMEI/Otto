#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "otto.h"


int vlprintf(OTTOLOG *logp, int level, char *format, va_list ap);
int vlsprintf(OTTOLOG *logp, int level, char *format, va_list ap);



OTTOLOG *
ottolog_init(char *logdir, char *logfile, int level, int truncate)
{
	OTTOLOG *retval = NULL;
	int  i;
	char fname[PATH_MAX], *mode, *extension;

	if(logfile != NULL && logfile[0] != '\0')
	{
		if((retval = (OTTOLOG *)malloc(sizeof(OTTOLOG))) == NULL)
		{
			// Failure of memory allocation is fatal.
			fprintf(stderr, "In ottolog_init(), malloc() failed for log file '%s'. [%d]\n", logfile, errno);
			perror("Error");
		}
		else
		{
			retval->loglevel = level;

			if(logdir == NULL || logdir[0] == '\0')
				logdir = ".";

			// remove trailing slash(es) on the logdir
			for(i=(int)strlen(logdir)-1; i>0; i--)
				if(logdir[i] != '/')
					break;
				else
					logdir[i] = '\0';

			// look for an extension on the logfile
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

				if(truncate)
					mode = "w";
				else
					mode = "a";
				if((retval->logfp=fopen(fname, mode)) == NULL)
				{
					fprintf(stderr, "Open on '%s' failed!\n", fname);
					free(retval);
					retval = NULL;
				}
				else
				{
					if(level > NO_LOG)
						fprintf(retval->logfp, "\n");
					if((retval->logfile = strdup(fname)) == NULL)
					{
						fprintf(stderr, "strdup on logfile name (%s) failed!\n", fname);
						free(retval);
						retval = NULL;
					}
				}
			}
			else
			{
				fprintf(stderr, "access on logdir (%s) failed!\n", logdir);
				free(retval);
				retval = NULL;
			}
		}
	}
	else
	{
		fprintf(stderr, "logfile isn't defined.\n");
		retval = NULL;
	}

	return(retval);
}



int
lprintf(OTTOLOG *logp, int level, char *format, ...)
{
	va_list ap;
	int     retval;

	if(level <= logp->loglevel)
	{
		va_start(ap, format);
		retval = vlprintf(logp, level, format, ap);
		va_end(ap);
	}

	return(retval);
}



int
vlprintf(OTTOLOG *logp, int level, char *format, va_list ap)
{
	char   buffer[OTTOLOG_BUFLEN];
	int    length;
	time_t now;
	int    retval = 0;
	char   fulldate[30], day[30];

	now = time(0);
	strcpy(fulldate, ctime(&now));
	strcpy(day, fulldate);
	strcpy(&day[11], &fulldate[20]);
	if(strcmp(day, logp->last_logged_day) != 0)
	{
		if(logp->logfp != NULL)
		{
			fprintf(logp->logfp, "\n               %s", day);
			strcpy(logp->last_logged_day, day);
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
	if(logp->logfp != NULL)
	{
		fprintf(logp->logfp, "%s", buffer);
		fflush(logp->logfp);
	}

	/* Write message to the console if level is not MINR or INFO */
	if(logp->logfp == NULL || level < MINR)
		fprintf(stderr, "%s", buffer+15);

	return(retval);
}



int
lsprintf(OTTOLOG *logp, int level, char *format, ...)
{
	va_list ap;
	int     retval;

	if(level <= logp->loglevel)
	{
		va_start(ap, format);
		retval = vlsprintf(logp, level, format, ap);
		va_end(ap);
	}

	return(retval);
}



int
vlsprintf(OTTOLOG *logp, int level, char *format, va_list ap)
{
	int    length;
	time_t now;
	int    retval = 0;
	char   fulldate[30], day[30];
	char   buffer[OTTOLOG_BUFLEN];

	if(level != CAT  &&
		level != CATI &&
		level != END  &&
		level != ENDI)
	{
		if(logp->lsstarted == OTTO_TRUE)
			lprintf(logp, INFO, "lsprintf restarted!\n");
		logp->lsstarted = OTTO_TRUE;
		logp->lslevel = level;

		now = time(0);
		strcpy(fulldate, ctime(&now));
		strcpy(day, fulldate);
		strcpy(&day[11], &fulldate[20]);
		if(strcmp(day, logp->last_logged_day) != 0)
		{
			fprintf(logp->logfp, "\n               %s", day);
			strcpy(logp->last_logged_day, day);
		}

		sprintf(logp->logfile_buffer, "%8.8s %s: ", &fulldate[11], levelstr(level));
		logp->console_buffer[0] = '\0';
	}

	if(logp->lsstarted == OTTO_TRUE)
	{
		if(level == CATI || level == ENDI)
			strcat(logp->logfile_buffer, "               ");

		vsprintf(buffer, format, ap );

		strcat(logp->logfile_buffer, buffer);
		strcat(logp->console_buffer, buffer);

		if(level == END || level == ENDI)
		{
			length = strlen(logp->logfile_buffer);
			if(logp->logfile_buffer[length-1] != '\n')
			{
				logp->logfile_buffer[length] = '\n';
				length++;
				logp->logfile_buffer[length] = '\0';
			}

			length = strlen(logp->console_buffer);
			if(logp->console_buffer[length-1] != '\n')
			{
				logp->console_buffer[length] = '\n';
				length++;
				logp->console_buffer[length] = '\0';
			}

			/* Write message to the log file */
			if(logp->logfp != NULL)
			{
				fprintf(logp->logfp, "%s", logp->logfile_buffer);
				fflush(logp->logfp);
			}

			/* Write message to the console if level is not MINR or INFO */
			if(logp->logfp == NULL || logp->lslevel < MINR)
				fprintf(stderr, "%s", logp->console_buffer);

			logp->lsstarted = OTTO_FALSE;
		}
	}

	return(retval);
}



void
lperror(OTTOLOG *logp, int level, char *string)
{
	int add_colon;

	if(level <= logp->loglevel)
	{
		add_colon = 1;
		if(strlen(string) == 1 && string[0] == ' ')
			add_colon = 0;

		lprintf(logp, level, "%s%s%s\n", string, add_colon ? ": " : "",
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



//
// vim: ts=3 sw=3 ai
//
