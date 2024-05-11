#include <ctype.h>
#include <libgen.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "otto.h"

extern OTTOLOG *logp;

// an array of token strings
// by macro expansion.
char *strvars[VAR_TOTAL+1] =
{
#define X(value) #value"=",
   ENV_NAMES
#undef X
      "VAR_TOTAL"
};


#define COMPLAIN           OTTO_TRUE
#define DONT_COMPLAIN      OTTO_FALSE
#define KEEP_VARNAME       OTTO_TRUE
#define DONT_KEEP_VARNAME  OTTO_FALSE

ottocfg_st cfg = {OTTO_VERSION_STRING, OTTODB_VERSION, OTTODB_MAXJOBS, 0};

int  get_cfg_int(int *value, char *parm, char *val, int lowval, int highval, int defval);
int  get_cfg_int16(int16_t *value, char *parm, char *val, int lowval, int highval, int defval);
int  get_cfg_uint16(uint16_t *value, char *parm, char *val, int lowval, int highval, int defval);
int  get_cfg_bool(int *value, char *parm, char *val, int defval);
int  get_cfg_envvar(char *word);
int  get_envvar(char **envvar, char *envvarname, int complain, int keep_varname);
int  read_ottoenv(void);
int  get_dyn_envvar(char *word);



int
init_cfg(int argc, char **argv)
{
   int retval = OTTO_SUCCESS;
   char *dup;

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
   if((cfg.userp = getenv("USER")) == NULL)
      cfg.userp = "unknown";
   if((cfg.lognamep = getenv("LOGNAME")) == NULL)
      cfg.lognamep = "unknown";

   if(get_envvar(&cfg.envvar_s[HOMEVAR],    "HOME",    COMPLAIN, KEEP_VARNAME)      == OTTO_FAIL) retval = OTTO_FAIL;
   if(get_envvar(&cfg.envvar_s[USERVAR],    "USER",    COMPLAIN, KEEP_VARNAME)      == OTTO_FAIL) retval = OTTO_FAIL;
   if(get_envvar(&cfg.envvar_s[LOGNAMEVAR], "LOGNAME", COMPLAIN, KEEP_VARNAME)      == OTTO_FAIL) retval = OTTO_FAIL;
   if(get_envvar(&cfg.envvar_s[PATHVAR],    "PATH",    COMPLAIN, KEEP_VARNAME)      == OTTO_FAIL) retval = OTTO_FAIL;
   if(get_envvar(&cfg.env_ottocfg,          "OTTOCFG", COMPLAIN, DONT_KEEP_VARNAME) == OTTO_FAIL) retval = OTTO_FAIL;
   if(get_envvar(&cfg.env_ottolog,          "OTTOLOG", COMPLAIN, DONT_KEEP_VARNAME) == OTTO_FAIL) retval = OTTO_FAIL;

   get_envvar(&cfg.env_ottoenv,  "OTTOENV",  DONT_COMPLAIN, DONT_KEEP_VARNAME);
   get_envvar(&cfg.env_ottohtml, "OTTOHTML", DONT_COMPLAIN, DONT_KEEP_VARNAME);

   cfg.n_envvar_s = PATHVAR + 1;

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
            otto_sprintf(t, "%s=%s", envvarname, e);
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
      }
      retval = OTTO_FAIL;
   }

   return(retval);
}



int
read_cfgfile()
{
   int   retval    = OTTO_SUCCESS;
   FILE  *infile   = NULL;
   char  *instring = NULL;
   char  *e        = NULL;
   char  *s        = NULL;
   char  *t        = NULL;
   char  *word     = NULL;
   char  *word2    = NULL;
   char  *logname  = NULL;
   int    i        = 0;
   int    line     = 0;
   size_t islen;
   struct stat statbuf;
   struct passwd *pw;

   if((infile = fopen(cfg.env_ottocfg, "r")) != NULL)
   {
      if(fstat(fileno(infile), &statbuf) != 0)
      {
         lprintf(logp, MAJR, "Can't stat $OTTOCFG.");
         retval = OTTO_FAIL;
      }

      islen = statbuf.st_size;
      if((instring = malloc(islen)) == NULL)
      {
         lprintf(logp, MAJR, "Can't malloc instring.");
         retval = OTTO_FAIL;
      }
      if((word = malloc(islen)) == NULL)
      {
         lprintf(logp, MAJR, "Can't malloc word.");
         retval = OTTO_FAIL;
      }
      if((word2 = malloc(islen)) == NULL)
      {
         lprintf(logp, MAJR, "Can't malloc word2.");
         retval = OTTO_FAIL;
      }

      while(retval == OTTO_SUCCESS && fgets(instring, islen, infile) != NULL)
      {
         line++;

         // trim_comment(instring);

         s = instring;
         while(isspace(*s) && *s != '\0')
            s++;

         s = copy_word(word, s);
         if(word[0] != '\0')
         {
            e = s;
            s = copy_word(word2, s);
            if(word2[0] == '\0')
               continue;

            if(strcmp(word, "server_addr") == 0)
            {
               if(cfg.server_addr != NULL)
                  free(cfg.server_addr);
               if((cfg.server_addr = strdup(word2)) == NULL)
               {
                  lprintf(logp, MAJR, "Can't malloc server_addr.");
                  retval = OTTO_FAIL;
               }
               continue;
            }

            if(strcmp(word, "ottosysd_port") == 0)
            {
               if(get_cfg_uint16(&cfg.ottosysd_port, "ottosysd_port", word2, 1, 65535, 0) == OTTO_FAIL)
               {
                  lprintf(logp, MAJR, "Can't parse ottosysd_port.");
                  retval = OTTO_FAIL;
               }
               continue;
            }

            if(strcmp(word, "httpd_port") == 0)
            {
               if(get_cfg_uint16(&cfg.httpd_port, "httpd_port", word2, 1, 65535, 0) == OTTO_FAIL)
               {
                  lprintf(logp, MAJR, "Can't parse httpd_port.");
                  retval = OTTO_FAIL;
               }
               continue;
            }

            if(strcmp(word, "ottodb_maxjobs") == 0)
            {
               if(get_cfg_int16(&cfg.ottodb_maxjobs, "ottodb_maxjobs", word2, 1, 32766, OTTODB_MAXJOBS) == OTTO_FAIL)
               {
                  lprintf(logp, MAJR, "Can't parse maxjobs.");
                  retval = OTTO_FAIL;
               }
               continue;
            }

            if(strcmp(word, "verbose") == 0)
            {
               if(get_cfg_bool(&cfg.verbose, "verbose", word2, OTTO_FALSE) == OTTO_FAIL)
               {
                  lprintf(logp, MAJR, "Can't parse verbose.");
                  retval = OTTO_FAIL;
               }
               continue;
            }

            if(strcmp(word, "show_sofar") == 0)
            {
               if(get_cfg_bool(&cfg.show_sofar, "show_sofar", word2, OTTO_FALSE) == OTTO_FAIL)
               {
                  lprintf(logp, MAJR, "Can't parse show_sofar.");
                  retval = OTTO_FAIL;
               }
               continue;
            }

            if(strcmp(word, "use_getpw") == 0)
            {
               if(get_cfg_bool(&cfg.use_getpw, "use_getpw", word2, OTTO_FALSE) == OTTO_FAIL)
               {
                  lprintf(logp, MAJR, "Can't parse use_getpw.");
                  retval = OTTO_FAIL;
               }
               continue;
            }

            if(strcmp(word, "envvar") == 0)
            {
               copy_eol(word2, e);
               if(get_cfg_envvar(word2) == OTTO_FAIL)
               {
                  lprintf(logp, MAJR, "Can't parse envvar.");
                  retval = OTTO_FAIL;
               }
               continue;
            }

            if(strcmp(word, "debug") == 0)
            {
               if(get_cfg_bool(&cfg.debug, "debug", word2, OTTO_FALSE) == OTTO_FAIL)
               {
                  lprintf(logp, MAJR, "Can't parse debug.");
                  retval = OTTO_FAIL;
               }
               continue;
            }

            if(strcmp(word, "enable_httpd") == 0)
            {
               if(get_cfg_bool(&cfg.enable_httpd, "enable_httpd", word2, OTTO_FALSE) == OTTO_FAIL)
               {
                  lprintf(logp, MAJR, "Can't parse enable_httpd.");
                  retval = OTTO_FAIL;
               }
               continue;
            }

            if(strcmp(word, "base_url") == 0)
            {
               if(cfg.base_url != NULL)
                  free(cfg.base_url);
               if((cfg.base_url = strdup(word2)) == NULL)
               {
                  lprintf(logp, MAJR, "Can't malloc base_url.");
                  retval = OTTO_FAIL;
               }
               continue;
            }

         }
      }
      fclose(infile);

      if(cfg.ottosysd_port == 0)
      {
         lprintf(logp, MAJR, "Invalid or missing value for ottosysd_port in $OTTOCFG.");
         retval = OTTO_FAIL;
      }
   }
   else
   {
      lprintf(logp, MAJR, "Couldn't open $OTTOCFG for input.");
      retval = OTTO_FAIL;
   }

   if(instring != NULL)
      free(instring);

   if(word != NULL)
      free(word);

   if(word2 != NULL)
      free(word2);

   if(retval == OTTO_SUCCESS)
   {
      if(cfg.use_getpw == OTTO_TRUE)
      {
         // find out who is running this command
         pw = getpwuid(getuid());
         cfg.euid = pw->pw_uid;

         pw = getpwnam(getlogin());
         cfg.ruid = pw->pw_uid;
      }
      else
      {
         // ruid and euid still provide a (small - 8 bytes) buffer to hold a name
         if((logname = getenv("LOGNAME")) != NULL)
         {
            // find the last backslash to remove any domain name prefix
            s = logname;

            // advance to the end of the string
            while(*s != '\0')
               s++;

            // now back up to the backslash
            while(s != logname && *s != '\\')
               s--;

            if(*s == '\\')
               s++;

            // fill euid first
            t = (char *)&cfg.euid;
            for(i=0; i<sizeof(cfg.euid); i++)
            {
               if(s[i] == '\0')
                  break;
               else
                  t[i] = s[i];
            }

            if(i == sizeof(cfg.euid))
            {
               // still characters left so fill ruid now
               s += sizeof(cfg.euid);
               t = (char *)&cfg.ruid;
               for(i=0; i<sizeof(cfg.ruid); i++)
               {
                  if(s[i] == '\0')
                     break;
                  else
                     t[i] = s[i];
               }
            }

         }
      }
   }

   return(retval);
}



int
get_cfg_int(int *value, char *parm, char *val, int lowval, int highval, int defval)
{
   int retval = OTTO_SUCCESS;
   int tempint;

   *value = defval;

   sscanf(val, "%d", &tempint);
   if(tempint >= lowval && tempint <= highval)
   {
      *value = tempint;
   }
   else
   {
      lprintf(logp, MAJR, "Invalid value for %s in $OTTOCFG (%d). Defaulting to %d.", parm, tempint, defval);
   }

   return(retval);
}



int
get_cfg_int16(int16_t *value, char *parm, char *val, int lowval, int highval, int defval)
{
   int retval = OTTO_SUCCESS;
   int tempint;

   *value = defval;

   sscanf(val, "%d", &tempint);
   if(tempint >= lowval && tempint <= highval)
   {
      *value = tempint;
   }
   else
   {
      lprintf(logp, MAJR, "Invalid value for %s in $OTTOCFG (%d). Defaulting to %d.", parm, tempint, defval);
   }

   return(retval);
}



int
get_cfg_uint16(uint16_t *value, char *parm, char *val, int lowval, int highval, int defval)
{
   int retval = OTTO_SUCCESS;
   int tempint;

   *value = defval;

   sscanf(val, "%d", &tempint);
   if(tempint >= lowval && tempint <= highval)
   {
      *value = tempint;
   }
   else
   {
      lprintf(logp, MAJR, "Invalid value for %s in $OTTOCFG (%d). Defaulting to %d.", parm, tempint, defval);
   }

   return(retval);
}



int
get_cfg_bool(int *value, char *parm, char *val, int defval)
{
   int retval = OTTO_SUCCESS;

   switch(tolower(val[0]))
   {
      case 'y': case 't': case '1':
         *value = OTTO_TRUE;
         break;
      case 'f': case 'n': case '0':
         *value = OTTO_FALSE;
         break;
      default:
         switch(defval)
         {
            case OTTO_TRUE:
               lprintf(logp, MAJR, "Invalid value for %s in $OTTOCFG (%s). Defaulting to 'true'.", parm, val);
               *value = OTTO_TRUE;
               break;
            case OTTO_FALSE:
               lprintf(logp, MAJR, "Invalid value for %s in $OTTOCFG (%s). Defaulting to 'false'.", parm, val);
               *value = OTTO_FALSE;
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
      retval = OTTO_FALSE;
   }

   if(retval == OTTO_SUCCESS)
   {
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
   }

   if(retval != OTTO_SUCCESS)
   {
      lprintf(logp, MAJR, "Malformed envvar '%s'. No equal found\n", word);
      retval = OTTO_FALSE;
   }

   if(retval == OTTO_SUCCESS)
   {
      // validate name character set
      for(i=0; name[i] != '\0'; i++)
      {
         switch(i)
         {
            case 0:
               if(name[i] != '_' && !isalpha(name[i]))
               {
                  lprintf(logp, MAJR, "Malformed envvar '%s'. Bad initial character\n", name);
                  retval = OTTO_FALSE;
               }
               break;
            default:
               if(name[i] != '_' && !isalnum(name[i]))
               {
                  lprintf(logp, MAJR, "Malformed envvar '%s'. Bad character\n", name);
                  retval = OTTO_FALSE;
               }
               break;
         }
      }
   }

   if(retval == OTTO_SUCCESS)
   {
      // trap attempts to override derived environment variables
      strcat(name, "=");
      for(i=OTTO_JOBNAMEVAR; i<PATHVAR; i++)
      {
         if(strcmp(name, strvars[i]) == 0)
         {
            lprintf(logp, MAJR, "Denying override of %*.*s in config file.\n", strlen(name)-1, strlen(name)-1, name);
            retval = OTTO_FALSE;
         }
      }
   }

   if(retval == OTTO_SUCCESS)
   {
      // allow any other environment variables (including PATH) to be overridden
      // override any envvar already set up
      for(i=PATHVAR+1; i<cfg.n_envvar_s; i++)
      {
         if(strncmp(cfg.envvar_s[i], name, strlen(name)) == 0)
         {
            free(cfg.envvar_s[i]);
            if((cfg.envvar_s[i] = strdup(word)) == NULL)
            {
               lprintf(logp, MAJR, "Error overriding %*.*s.\n", strlen(name)-1, strlen(name)-1, name);
               retval = OTTO_FALSE;
            }
            else
            {
               if(strcmp(name, "PATH=") == 0)
                  cfg.path_overridden = OTTO_TRUE;
            }
         }
      }
   }

   if(retval == OTTO_SUCCESS)
   {
      // if the envvar wasn't found add it
      if(i == cfg.n_envvar_s)
      {
         if(cfg.n_envvar_s < MAX_ENVVAR)
         {
            cfg.envvar_s[cfg.n_envvar_s] = strdup(word);
            if(cfg.envvar_s[cfg.n_envvar_s] == NULL)
            {
               lprintf(logp, MAJR, "Error mallocing envvar %d.\n", cfg.n_envvar_s);
               retval = OTTO_FALSE;
            }
            else
            {
               cfg.n_envvar_s++;
            }
         }
         else
         {
            lprintf(logp, MAJR, "Error exceeding number of allowed static envvars %d.\n", cfg.n_envvar_s);
            retval = OTTO_FALSE;
         }
      }
   }

   if(name != NULL)
      free(name);

   return(retval);
}



void
rebuild_environment()
{
   struct stat statbuf;
   char name[PATH_MAX];
   int i, j;
   int environment_changed = OTTO_FALSE;

   // check if the dynamic environment changed
   if(cfg.env_ottoenv != NULL && stat(cfg.env_ottoenv, &statbuf) != -1)
   {
      if(cfg.ottoenv_mtime < statbuf.st_mtime)
      {
         read_ottoenv();
         cfg.ottoenv_mtime = statbuf.st_mtime;
         environment_changed = OTTO_TRUE;

         if(cfg.n_envvar_d > 0)
         {
            lsprintf(logp, INFO, "Dynamic Environment:\n");
            for(i=0; i<cfg.n_envvar_d; i++)
               lsprintf(logp, CATI, "envvar:      %s\n", cfg.envvar_d[i]);
            lsprintf(logp, END, "");
         }
      }
   }

   // only rebuild it if it's the first time through or the dynamic environment changed
   if(cfg.n_envvar == 0 || environment_changed == OTTO_TRUE)
   {
      // clear out current environment
      for(i=0; i<MAX_ENVVAR; i++)
         cfg.envvar[i] = NULL;

      // add static environment
      for(i=0; i<cfg.n_envvar_s; i++)
         cfg.envvar[i] = cfg.envvar_s[i];
      cfg.n_envvar = cfg.n_envvar_s;

      // add dynamic environment
      // allow any other environment variables to be overridden
      // override any envvar already set up
      for(i=0; i<cfg.n_envvar_d; i++)
      {
         // get the enviroonment variable name and the equal sign
         for(j=0; j<strlen(cfg.envvar_d[i]); j++)
         {
            name[j] = cfg.envvar_d[i][j];
            if(name[j] == '=')
            {
               name[++j] = '\0';
               break;
            }
         }

         // allow override of anything but the derived values (including PATH)
         for(j=PATHVAR; j<cfg.n_envvar; j++)
         {
            // replace the value if it's found
            if(strncmp(cfg.envvar[j], name, strlen(name)) == 0)
            {
               cfg.envvar[j] = cfg.envvar_d[i];
               break;
            }
         }
         // add it
         if(j == cfg.n_envvar)
         {
            cfg.envvar[cfg.n_envvar] = cfg.envvar_d[i];
            cfg.n_envvar++;
         }
      }
   }
}



int
read_ottoenv()
{
   int   retval = OTTO_SUCCESS;
   char  *instring=NULL, *s, *e;
   char  *word=NULL, *word2=NULL;
   FILE  *infile = NULL;
   off_t islen;
   int   i;
   struct stat statbuf;

   for(i=0; i<cfg.n_envvar_d; i++)
      if(cfg.envvar_d[i] != NULL)
         free(cfg.envvar_d[i]);
   cfg.n_envvar_d = 0;

   if(cfg.env_ottoenv != NULL)
   {
      if((infile = fopen(cfg.env_ottoenv, "r")) != NULL)
      {
         if(fstat(fileno(infile), &statbuf) != 0)
         {
            lprintf(logp, MAJR, "Can't stat $OTTOENV.");
            retval = OTTO_FAIL;
         }

         if(retval == OTTO_SUCCESS)
         {
            islen = statbuf.st_size;
            if((instring = malloc(islen)) == NULL)
            {
               lprintf(logp, MAJR, "Can't malloc instring.");
               retval = OTTO_FAIL;
            }
            if((word = malloc(islen)) == NULL)
            {
               lprintf(logp, MAJR, "Can't malloc word.");
               retval = OTTO_FAIL;
            }
            if((word2 = malloc(islen)) == NULL)
            {
               lprintf(logp, MAJR, "Can't malloc word2.");
               retval = OTTO_FAIL;
            }
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
                     get_dyn_envvar(word2);
                     continue;
                  }
               }
            }
         }
      }
      fclose(infile);
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
get_dyn_envvar(char *word)
{
   int retval = OTTO_SUCCESS;
   char *name = NULL;
   int i;

   if((name = strdup(word)) == NULL)
   {
      fprintf(stderr, "name strdup failed.\n");
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS)
   {
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
   }

   if(retval != OTTO_SUCCESS)
   {
      lprintf(logp, MAJR, "Malformed envvar '%s'. No equal found\n", word);
      retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS)
   {
      // validate name character set
      for(i=0; name[i] != '\0'; i++)
      {
         switch(i)
         {
            case 0:
               if(name[i] != '_' && !isalpha(name[i]))
               {
                  lprintf(logp, MAJR, "Malformed envvar '%s'. Bad initial character\n", name);
                  retval = OTTO_FAIL;
               }
               break;
            default:
               if(name[i] != '_' && !isalnum(name[i]))
               {
                  lprintf(logp, MAJR, "Malformed envvar '%s'. Bad character\n", name);
                  retval = OTTO_FAIL;
               }
               break;
         }
      }
   }

   if(retval == OTTO_SUCCESS)
   {
      // trap attempts to override derived environment variables
      strcat(name, "=");
      for(i=OTTO_JOBNAMEVAR; i<VAR_TOTAL; i++)
      {
         if(strcmp(name, strvars[i]) == 0)
         {
            lprintf(logp, MAJR, "Denying override of %*.*s in config file.\n", strlen(name)-1, strlen(name)-1, name);
            retval = OTTO_FAIL;
            break;
         }
      }
   }

   if(retval == OTTO_SUCCESS)
   {
      // allow any other environment variables to be overridden
      // override any envvar already set up
      for(i=0; i<cfg.n_envvar_d; i++)
      {
         if(strncmp(cfg.envvar_d[i], name, strlen(name)) == 0)
         {
            free(cfg.envvar_d[i]);
            if((cfg.envvar_d[i] = strdup(word)) == NULL)
            {
               lprintf(logp, MAJR, "Error overriding %*.*s.\n", strlen(name)-1, strlen(name)-1, name);
               retval = OTTO_FAIL;
               break;
            }
         }
      }
   }

   if(retval == OTTO_SUCCESS)
   {
      // if the envvar wasn't found add it
      if(i == cfg.n_envvar_d)
      {
         if(cfg.n_envvar_d < MAX_ENVVAR)
         {
            cfg.envvar_d[cfg.n_envvar_d] = strdup(word);
            if(cfg.envvar_d[cfg.n_envvar_d] == NULL)
            {
               lprintf(logp, MAJR, "Error mallocing envvar %d.\n", cfg.n_envvar_d);
               retval = OTTO_FAIL;
            }
            else
            {
               cfg.n_envvar_d++;
            }
         }
         else
         {
            lprintf(logp, MAJR, "Error exceeding number of allowed dynamic envvars %d.\n", cfg.n_envvar_d);
            retval = OTTO_FAIL;
         }
      }
   }

   if(name != NULL)
      free(name);

   return(retval);
}



void
log_initialization()
{
   lsprintf(logp, INFO, "version: %5s\n", cfg.otto_version);
   lsprintf(logp, CATI, "initialized by: %s (%s)", cfg.userp, cfg.lognamep);
   lsprintf(logp, END, "\n");
}



void
log_args(int argc, char **argv)
{
   int i;

   lsprintf(logp, INFO, "Command line:\n");
   lsprintf(logp, CATI, "");
   for(i=0; i<argc; i++)
      lsprintf(logp, CAT, "%s ", argv[i]);
   lsprintf(logp, END, "\n");
}



void
log_cfg()
{
   int i;

   lsprintf(logp, INFO, "Environment:\n");
   lsprintf(logp, CATI, "OTTOCFG=%s\n", cfg.env_ottocfg);
   lsprintf(logp, CATI, "OTTOENV=%s\n", cfg.env_ottoenv == NULL ? "null" : cfg.env_ottoenv);
   lsprintf(logp, CATI, "OTTOHTML=%s\n",cfg.env_ottohtml == NULL ? "null" : cfg.env_ottohtml);
   lsprintf(logp, CATI, "OTTOLOG=%s\n", cfg.env_ottolog);
   lsprintf(logp, CATI, "%s\n",         cfg.envvar_s[HOMEVAR]);
   lsprintf(logp, CATI, "%s\n",         cfg.envvar_s[USERVAR]);
   lsprintf(logp, CATI, "%s\n",         cfg.envvar_s[LOGNAMEVAR]);
   if(cfg.path_overridden == OTTO_FALSE)
      lsprintf(logp, CATI, "%s\n", cfg.envvar_s[PATHVAR]);
   lsprintf(logp, END, "");

   lsprintf(logp, INFO, "Configuration:\n");
   lsprintf(logp, CATI, "server_addr:        %s\n", cfg.server_addr    == NULL      ? "null" : cfg.server_addr);
   lsprintf(logp, CATI, "ottosysd_port:      %d\n", cfg.ottosysd_port);
   lsprintf(logp, CATI, "httpd_port:         %d\n", cfg.httpd_port);
   lsprintf(logp, CATI, "ottodb_version:     %d\n", cfg.ottodb_version);
   lsprintf(logp, CATI, "ottodb_maxjobs:     %d\n", cfg.ottodb_maxjobs);
   lsprintf(logp, CATI, "pause:              %s\n", cfg.pause          == OTTO_TRUE ? "true" : "false");
   lsprintf(logp, CATI, "verbose:            %s\n", cfg.verbose        == OTTO_TRUE ? "true" : "false");
   lsprintf(logp, CATI, "show_sofar:         %s\n", cfg.show_sofar     == OTTO_TRUE ? "true" : "false");
   lsprintf(logp, CATI, "debug:              %s\n", cfg.debug          == OTTO_TRUE ? "true" : "false");

   // add envvars from config file
   if(cfg.path_overridden == OTTO_TRUE)
      lsprintf(logp, CATI, "envvar:             %s\n", cfg.envvar_s[PATHVAR]);
   for(i=PATHVAR+1; i<cfg.n_envvar_s; i++)
      lsprintf(logp, CATI, "envvar:             %s\n", cfg.envvar_s[i]);
   lsprintf(logp, END, "");
}



