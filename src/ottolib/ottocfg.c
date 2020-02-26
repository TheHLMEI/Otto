#include <ctype.h>
#include <libgen.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


#include "ottobits.h"
#include "ottocfg.h"
#include "ottodb.h"
#include "ottoipc.h"
#include "ottojob.h"
#include "ottolog.h"
#include "ottoutil.h"

#define COMPLAIN           OTTO_TRUE
#define DONT_COMPLAIN      OTTO_FALSE
#define KEEP_VARNAME       OTTO_TRUE
#define DONT_KEEP_VARNAME  OTTO_FALSE

ottocfg_st cfg = {OTTO_VERSION, OTTODB_VERSION, OTTODB_MAXJOBS, 0};

int  get_cfg_int(char *parm, char *val, int lowval, int highval, int defval);
int  get_cfg_bool(char *parm, char *val, int defval);
int  get_cfg_envvar(char *word);
int  get_envvar(char **envvar, char *envvarname, int complain, int keep_varname);
int  get_ottoenv(char *word);



int
init_cfg(int argc, char **argv)
{
	int retval = OTTO_SUCCESS;
	char *dup;
	struct passwd *pw;

	if((dup = strdup(argv[0])) == NULL)
	{
		fprintf(stderr, "argv[0] strdup failed.\n");
		retval = OTTO_FAIL;
	}

	if(retval == OTTO_SUCCESS)
	{
		cfg.progname = strdup(basename(dup));

		if(cfg.progname == NULL)
		{
			fprintf(stderr, "argv[0] strdup failed.\n");
			retval = OTTO_FAIL;
		}

		free(dup);
	}

	// find out who is running this command
	pw = getpwnam(getlogin());
	cfg.ruid = pw->pw_uid;

	pw = getpwuid(getuid());
	cfg.euid = pw->pw_uid;

	if(get_envvar(&cfg.env_home,    "HOME",    COMPLAIN, KEEP_VARNAME)      == OTTO_FAIL) retval = OTTO_FAIL;
	if(get_envvar(&cfg.env_user,    "USER",    COMPLAIN, KEEP_VARNAME)      == OTTO_FAIL) retval = OTTO_FAIL;
	if(get_envvar(&cfg.env_path,    "PATH",    COMPLAIN, KEEP_VARNAME)      == OTTO_FAIL) retval = OTTO_FAIL;
	if(get_envvar(&cfg.env_logname, "LOGNAME", COMPLAIN, KEEP_VARNAME)      == OTTO_FAIL) retval = OTTO_FAIL;
	if(get_envvar(&cfg.env_ottocfg, "OTTOCFG", COMPLAIN, DONT_KEEP_VARNAME) == OTTO_FAIL) retval = OTTO_FAIL;
	if(get_envvar(&cfg.env_ottolog, "OTTOLOG", COMPLAIN, DONT_KEEP_VARNAME) == OTTO_FAIL) retval = OTTO_FAIL;

	get_envvar(&cfg.env_ottoenv, "OTTOENV", DONT_COMPLAIN, DONT_KEEP_VARNAME);

	return(retval);
}



int
get_envvar(char **envvar, char *envvarname, int complain, int keep_varname)
{
	int retval = OTTO_SUCCESS;
	char *e, *t = NULL;
	int elen = strlen(envvarname)+2;

	*envvar = NULL;

	if((e = getenv(envvarname)) != NULL)
	{
		if((t = malloc(strlen(e)+elen)) != NULL)
		{
			if(keep_varname == OTTO_TRUE)
				sprintf(t, "%s=%s", envvarname, e);
			else
				strcpy(t, e);
			*envvar = t;
		}
		else
		{
			printf("Error mallocing %s.\n", envvarname);
			retval = OTTO_FAIL;
		}
	}
	else
	{
		if(complain == OTTO_TRUE)
		{
			printf("$%s isn't defined.\n", envvarname);
			retval = OTTO_FAIL;
		}
	}

	return(retval);
}



int
read_cfgfile()
{
	int   retval = OTTO_SUCCESS;
	char  *instring, *s, *e;
	char  *word, *word2;
	FILE  *infile = NULL;
	off_t islen;
	struct stat buf;

	if((infile = fopen(cfg.env_ottocfg, "r")) != NULL)
	{
		if(fstat(fileno(infile), &buf) != 0)
		{
			lprintf(MAJR, "Can't stat $OTTOCFG.");
			retval = OTTO_FAIL;
		}

		islen = buf.st_size;
		if((instring = malloc(islen)) == NULL)
		{
			lprintf(MAJR, "Can't malloc instring.");
			retval = OTTO_FAIL;
		}
		if((word = malloc(islen)) == NULL)
		{
			lprintf(MAJR, "Can't malloc word.");
			retval = OTTO_FAIL;
		}
		if((word2 = malloc(islen)) == NULL)
		{
			lprintf(MAJR, "Can't malloc word2.");
			retval = OTTO_FAIL;
		}

		if(retval == OTTO_SUCCESS)
		{
			while(fgets(instring, islen, infile) != NULL)
			{
				s = instring;
				while(isspace(*s) && *s != '\0')
					s++;

				s = copy_word(word, s);
				if(word[0] != '#')
				{
					e = s;
					s = copy_word(word2, s);
					if(word2[0] == '\0')
						continue;

					if(strcmp(word, "server_addr") == 0)
					{
						if(cfg.server_addr != NULL)
							free(cfg.server_addr);
						cfg.server_addr = strdup(word2);
						continue;
					}

					if(strcmp(word, "server_port") == 0)
					{
						cfg.server_port = get_cfg_int("server_port", word2, 1, 65535, 0);
						continue;
					}

					if(strcmp(word, "ottodb_maxjobs") == 0)
					{
						cfg.ottodb_maxjobs = get_cfg_int("ottodb_maxjobs", word2, 1, 32766, OTTODB_MAXJOBS);
						continue;
					}

					if(strcmp(word, "verbose") == 0)
					{
						cfg.verbose = get_cfg_bool("verbose", word2, OTTO_FALSE);
						continue;
					}

					if(strcmp(word, "show_sofar") == 0)
					{
						cfg.show_sofar = get_cfg_bool("show_sofar", word2, OTTO_FALSE);
						continue;
					}

					if(strcmp(word, "envvar") == 0)
					{
						copy_eol(word2, e);
						get_cfg_envvar(word2);
						continue;
					}

					if(strcmp(word, "debug") == 0)
					{
						cfg.debug = get_cfg_bool("debug", word2, OTTO_FALSE);
						continue;
					}

				}
			}
		}
		fclose(infile);
	}
	else
	{
		lprintf(MAJR, "Couldn't open $OTTOCFG for input.");
		retval = OTTO_FAIL;
	}

	if(cfg.server_port == 0)
	{
		lprintf(MAJR, "Invalid or missing value for port in $OTTOCFG.");
		retval = OTTO_FAIL;
	}

	if(instring != NULL)
		free(instring);

	if(word != NULL)
		free(word);

	if(word2 != NULL)
		free(word2);

	return(retval);
}



int
get_cfg_int(char *parm, char *val, int lowval, int highval, int defval)
{
	int retval = defval;
	int tempint;

	sscanf(val, "%d", &tempint);
	if(tempint >= lowval && tempint <= highval)
	{
		retval = tempint;
	}
	else
	{
		lprintf(MAJR, "Invalid value for %s in $OTTOCFG (%d). Defaulting to %d.", parm, tempint, defval);
	}

	return(retval);
}



int
get_cfg_bool(char *parm, char *val, int defval)
{
	int retval = OTTO_TRUE;

	switch(tolower(val[0]))
	{
		case 'y': case 't': case '1':
			retval = OTTO_TRUE;
			break;
		case 'f': case 'n': case '0':
			retval = OTTO_FALSE;
			break;
		default:
			switch(defval)
			{
				case OTTO_TRUE:
					lprintf(MAJR, "Invalid value for %s in $OTTOCFG (%s). Defaulting to 'true'.", parm, val);
					retval = OTTO_TRUE;
					break;
				case OTTO_FALSE:
					lprintf(MAJR, "Invalid value for %s in $OTTOCFG (%s). Defaulting to 'false'.", parm, val);
					retval = OTTO_FALSE;
					break;
			}
			break;
	}

	return(retval);
}



int
get_cfg_envvar(char *word)
{
	int retval = OTTO_SUCCESS;
	char *name;
	int i;

	cfg.path_overridden = OTTO_FALSE;

	if((name = strdup(word)) == NULL)
	{
		fprintf(stderr, "name strdup failed.\n");
		exit(-1);
	}

	// find equal sign
	retval = OTTO_FAIL;
	for(i=0; name[i] != '\0'; i++)
	{
		if(name[i] == '=')
		{
			name[i] = '\0';
			retval = OTTO_SUCCESS;
			break;
		}
	}

	if(retval == OTTO_FAIL)
	{
		lprintf(MAJR, "Malformed envvar '%s'. No equal found\n", word);
		return(retval);
	}

	// validate name character set
	for(i=0; name[i] != '\0'; i++)
	{
		switch(i)
		{
			case 0:
				if(name[i] != '_' && !isalpha(name[i]))
				{
					lprintf(MAJR, "Malformed envvar '%s'. Bad initial character\n", name);
					return(OTTO_FAIL);
				}
				break;
			default:
				if(name[i] != '_' && !isalnum(name[i]))
				{
					lprintf(MAJR, "Malformed envvar '%s'. Bad character\n", name);
					return(OTTO_FAIL);
				}
				break;
		}
	}

	// trap attempts to override derived environment variables
	strcat(name, "=");
	if(strcmp(name, "HOME=") == 0)
	{
		lprintf(MAJR, "Denying override of $HOME in config file\n");
		return(OTTO_FAIL);
	}

	if(strcmp(name, "USER=") == 0)
	{
		lprintf(MAJR, "Denying override of $USER in config file\n");
		return(OTTO_FAIL);
	}

	if(strcmp(name, "LOGNAME=") == 0)
	{
		lprintf(MAJR, "Denying override of $LOGNAME in config file\n");
		return(OTTO_FAIL);
	}

	if(strcmp(name, "AUTO_JOB_NAME=") == 0)
	{
		lprintf(MAJR, "Denying override of $AUTO_JOB_NAME in config file\n");
		return(OTTO_FAIL);
	}

	// allow PATH to be overridden
	if(strcmp(name, "PATH=") == 0)
	{
		if(cfg.env_path != NULL)
		{
			free(cfg.env_path);
			cfg.env_path = NULL;
		}
		if((cfg.env_path = strdup(word)) == NULL)
		{
			lprintf(MAJR, "Error overriding env_path.\n");
			return(OTTO_FAIL);
		}
		else
		{
			cfg.path_overridden = OTTO_TRUE;
		}
	}
	else
	{
		// override any envvar already set up
		for(i=0; i<cfg.n_envvar; i++)
		{
			if(strncmp(cfg.envvar[i], name, strlen(name)) == 0)
			{
				free(cfg.envvar[i]);
				if((cfg.envvar[i] = strdup(word)) == NULL)
				{
					lprintf(MAJR, "Error overriding %*.*s.\n", strlen(name)-1, strlen(name)-1, name);
					return(OTTO_FAIL);
				}
			}
		}

		// if the envvar wasn't found add it
		if(i == cfg.n_envvar)
		{
			if(cfg.n_envvar < MAX_ENVVAR)
			{
				cfg.envvar[cfg.n_envvar] = strdup(word);
				if(cfg.envvar[cfg.n_envvar] == NULL)
				{
					lprintf(MAJR, "Error mallocing envvar %d.\n", cfg.n_envvar);
					return(OTTO_FAIL);
				}
				else
				{
					cfg.n_envvar++;
				}
			}
			else
			{
				lprintf(MAJR, "Error exceeding number of allowed envvars %d.\n", cfg.n_envvar);
				return(OTTO_FAIL);
			}
		}
	}

	free(name);

	return(retval);
}



#ifdef NOT_NOW
int
read_ottoenv()
{
	int   retval = OTTO_SUCCESS;
	char  *instring, *s, *e;
	char  *word, *word2;
	FILE  *infile = NULL;
	off_t islen;
	int   tempint;
	struct stat buf;

	if((infile = fopen(env_ottoenv, "r")) != NULL)
	{
		if(fstat(fileno(infile), &buf) != 0)
		{
			lprintf(MAJR, "Can't stat $OTTOCFG.");
			retval = OTTO_FAIL;
		}

		islen = buf.st_size;
		if((instring = malloc(islen)) == NULL)
		{
			lprintf(MAJR, "Can't malloc instring.");
			retval = OTTO_FAIL;
		}
		if((word = malloc(islen)) == NULL)
		{
			lprintf(MAJR, "Can't malloc word.");
			retval = OTTO_FAIL;
		}
		if((word2 = malloc(islen)) == NULL)
		{
			lprintf(MAJR, "Can't malloc word2.");
			retval = OTTO_FAIL;
		}

		if(retval == OTTO_SUCCESS)
		{
			while(fgets(instring, islen, infile) != NULL)
			{
				s = instring;
				while(isspace(*s) && *s != '\0')
					s++;

				s = copy_word(word, s);
				if(word[0] != '#')
				{
					e = s;
					s = copy_word(word2, s);
					if(word2[0] == '\0')
						continue;

					if(strcmp(word, "envvar") == 0)
					{
						copy_eol(word2, e);
						get_ottoenv_envvar(word2);
						continue;
					}
				}
			}
		}
		fclose(infile);
	}
	else
	{
		lprintf(MAJR, "Couldn't open $OTTOENV for input.");
		retval = OTTO_FAIL;
	}

	if(instring != NULL)
		free(instring);

	if(word != NULL)
		free(instring);

	if(word2 != NULL)
		free(instring);

	return(retval);
}



int
get_ottoenv_envvar(char *word)
{
	int retval = OTTO_SUCCESS;
	char *name;
	int i;

	if((name = strdup(word)) == NULL)
	{
		fprintf(stderr, "name strdup failed.\n");
		exit(-1);
	}

	// find equal sign
	retval = OTTO_FAIL;
	for(i=0; name[i] != '\0'; i++)
	{
		if(name[i] == '=')
		{
			name[i] = '\0';
			retval = OTTO_SUCCESS;
			break;
		}
	}

	if(retval == OTTO_FAIL)
	{
		lprintf(MAJR, "Malformed envvar '%s'. No equal found\n", word);
		return(retval);
	}

	// validate name character set
	for(i=0; name[i] != '\0'; i++)
	{
		switch(i)
		{
			case 0:
				if(name[i] != '_' && !isalpha(name[i]))
				{
					lprintf(MAJR, "Malformed envvar '%s'. Bad initial character\n", name);
					return(OTTO_FAIL);
				}
				break;
			default:
				if(name[i] != '_' && !isalnum(name[i]))
				{
					lprintf(MAJR, "Malformed envvar '%s'. Bad character\n", name);
					return(OTTO_FAIL);
				}
				break;
		}
	}

	// trap attempts to override derived environment variables
	strcat(name, "=");
	for(i=0; i<n_envvar_r; i++)
	{
		if(strncmp(envvar_s[i], name, strlen(name)) == 0)
		{
			lprintf(MAJR, "Denying override of $*.*s in $OTTOENV file\n", nlen, nlen, name);
			return(OTTO_FAIL);
		}
	}

	if(strcmp(name, "HOME=") == 0)
	{
		return(OTTO_FAIL);
	}

	if(strcmp(name, "USER=") == 0)
	{
		lprintf(MAJR, "Denying override of $USER in config file\n");
		return(OTTO_FAIL);
	}

	if(strcmp(name, "LOGNAME=") == 0)
	{
		lprintf(MAJR, "Denying override of $LOGNAME in config file\n");
		return(OTTO_FAIL);
	}

	if(strcmp(name, "AUTO_JOB_NAME=") == 0)
	{
		lprintf(MAJR, "Denying override of $AUTO_JOB_NAME in config file\n");
		return(OTTO_FAIL);
	}

	if(strcmp(name, "PATH=") == 0)
	{
		lprintf(MAJR, "Denying override of $PATH in config file\n");
		return(OTTO_FAIL);
	}
	else
	{
		// override any envvar already set up
		for(i=0; i<n_envvar; i++)
		{
			if(strncmp(envvar_s[i], name, strlen(name)) == 0)
			{
				free(envvar[i]);
				if((envvar[i] = strdup(word)) == NULL)
				{
					lprintf(MAJR, "Error overriding %*.*s.\n", strlen(name)-1, strlen(name)-1, name);
					return(OTTO_FAIL);
				}
			}
		}

		// if the envvar wasn't found add it
		if(i == n_envvar)
		{
			if(n_envvar < MAX_ENVVAR)
			{
				envvar[n_envvar] = strdup(word);
				if(envvar[n_envvar] == NULL)
				{
					lprintf(MAJR, "Error mallocing envvar %d.\n", n_envvar);
					return(OTTO_FAIL);
				}
				else
				{
					n_envvar++;
				}
			}
			else
			{
				lprintf(MAJR, "Error exceeding number of allowed envvars %d.\n", n_envvar);
				return(OTTO_FAIL);
			}
		}
	}

	free(name);

	return(retval);
}
#endif



void
log_initialization()
{
	struct passwd *epw = getpwuid(cfg.euid);
	struct passwd *rpw = getpwuid(cfg.ruid);

	lsprintf(INFO, "version: %5s\n", cfg.otto_version);
	lsprintf(CATI, "initialized by: %s (%s)", epw->pw_name, rpw->pw_name);
	lsprintf(END, "\n");
}



void
log_args(int argc, char **argv)
{
	int i;

	lsprintf(INFO, "Command line:\n");
	lsprintf(CATI, "");
	for(i=0; i<argc; i++)
		lsprintf(CAT, "%s ", argv[i]);
	lsprintf(END, "\n");
}



void
log_cfg()
{
	int i;

	lsprintf(INFO, "Environment:\n");
	lsprintf(CATI, "OTTOCFG=%s\n", cfg.env_ottocfg);
	lsprintf(CATI, "OTTOLOG=%s\n", cfg.env_ottolog);
	lsprintf(CATI, "OTTOENV=%s\n", cfg.env_ottoenv == NULL ? "null" : cfg.env_ottoenv);
	lsprintf(CATI, "%s\n",         cfg.env_home);
	lsprintf(CATI, "%s\n",         cfg.env_logname);
	lsprintf(CATI, "%s\n",         cfg.env_user);
	if(cfg.path_overridden == OTTO_FALSE)
		lsprintf(CATI, "%s\n", cfg.env_path);
	lsprintf(END, "");

	lsprintf(INFO, "Configuration:\n");
	lsprintf(CATI, "server_addr:        %5s\n", cfg.server_addr);
	lsprintf(CATI, "server_port:        %5d\n", cfg.server_port);
	lsprintf(CATI, "ottodb_version:     %5d\n", cfg.ottodb_version);
	lsprintf(CATI, "ottodb_maxjobs:     %5d\n", cfg.ottodb_maxjobs);
	lsprintf(CATI, "pause:              %5s\n", cfg.pause      == OTTO_TRUE ? "true" : "false");
	lsprintf(CATI, "verbose:            %5s\n", cfg.verbose    == OTTO_TRUE ? "true" : "false");
	lsprintf(CATI, "show_sofar:         %5s\n", cfg.show_sofar == OTTO_TRUE ? "true" : "false");
	lsprintf(CATI, "debug:              %5s\n", cfg.debug      == OTTO_TRUE ? "true" : "false");

	// add envvars from config file
	for(i=0; i<cfg.n_envvar; i++)
		lsprintf(CATI, "envvar:      %s\n", cfg.envvar[i]);
	if(cfg.path_overridden == OTTO_TRUE)
		lsprintf(CATI, "envvar:      %s\n", cfg.env_path);
	lsprintf(END, "");
}



//
// vim: ts=3 sw=3 ai
//
