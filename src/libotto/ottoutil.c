#include <ctype.h>
#include <limits.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


#include "otto.h"
#include "linenoise.h"

extern OTTOLOG *logp;

#define cdeAMP_____     ((i64)'A'<<56 | (i64)'M'<<48 | (i64)'P'<<40 | (i64)';'<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeAPOS____     ((i64)'A'<<56 | (i64)'P'<<48 | (i64)'O'<<40 | (i64)'S'<<32 | ';'<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeGT______     ((i64)'G'<<56 | (i64)'T'<<48 | (i64)';'<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeLT______     ((i64)'L'<<56 | (i64)'T'<<48 | (i64)';'<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeQUOT____     ((i64)'Q'<<56 | (i64)'U'<<48 | (i64)'O'<<40 | (i64)'T'<<32 | ';'<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeNBSP____     ((i64)'N'<<56 | (i64)'B'<<48 | (i64)'S'<<40 | (i64)'P'<<32 | ';'<<24 | ' '<<16 | ' '<<8 | ' ')

void
escape_html(char *t, int tlen)
{
   char tmpbuf[16384];
   char *s;

   strcpy(tmpbuf, t);
   s = tmpbuf;

   while(*s != '\0')
   {
      switch(*s)
      {
         case '<':  strcpy(t, "&lt;");   t+=4; break;
         case '>':  strcpy(t, "&gt;");   t+=4; break;
         case '&':  strcpy(t, "&amp;");  t+=5; break;
         case '"':  strcpy(t, "&quot;"); t+=6; break;
         case '\'': strcpy(t, "&apos;"); t+=6; break;
         case ' ':  strcpy(t, "&nbsp;"); t+=6; break;
         default:   *t++ = *s;                 break;
      }

      s++;
   }
   *t = '\0';
}



void
escape_mspdi(char *t, int tlen)
{
   char tmpbuf[16384];
   char *s;

   strcpy(tmpbuf, t);
   s = tmpbuf;

   while(*s != '\0')
   {
      switch(*s)
      {
         case '<':  strcpy(t, "&lt;");   t+=4; break;
         case '>':  strcpy(t, "&gt;");   t+=4; break;
         case '&':  strcpy(t, "&amp;");  t+=5; break;
         case '"':  strcpy(t, "&quot;"); t+=6; break;
         case '\'': strcpy(t, "&apos;"); t+=6; break;
         default:   *t++ = *s;                 break;
      }

      s++;
   }
   *t = '\0';
}



void
mime_quote(char *t, int tlen)
{
   char tmpbuf[16384];
   char *s;

   strcpy(tmpbuf, t);
   s = tmpbuf;

   // load in the first quote
   *t++ = '"';

   while(*s != '\0')
   {
      if(*s == '"')
      {
         *t++ = '"';
      }
      *t++ = *s++;
   }

   // load in fina quote
   *t++ = '"';

   *t = '\0';
}



char *
copy_word(char *t, char *s)
{
   // copy non-space
   while(!isspace(*s) && *s != '\0')
      *t++ = *s++;
   *t = '\0';

   // consume space
   while(isspace(*s) && *s != '\0')
      s++;

   return(s);
}



void
copy_eol(char *t, char *s)
{
   // copy to end of line
   while(*s != '\n' && *s != '\0')
      *t++ = *s++;
   *t = '\0';
}



void
mkcode(char *t, char *s)
{
   int i;

   // copy characters up to limit of 8, '>', or whitespace
   for(i=0; i<8; i++)
   {
      if(s[i] == '>' || isspace(s[i]) || s[i] == '\0')
         break;
      t[i] = toupper(s[i]);
   }

   // space pad remaining 8 characters
   for(; i<8; i++)
   {
      t[i] = ' ';
   }
}



void
mkampcode(char *t, char *s)
{
   int i;

   // copy characters up to limit of 8, up to and including ';', or whitespace
   for(i=0; i<8; i++)
   {
      if(isspace(s[i]))
         break;
      t[i] = toupper(s[i]);
      if(s[i] == ';')
      {
         i++;
         break;
      }
   }

   // space pad remaining 8 characters
   for(; i<8; i++)
   {
      t[i] = ' ';
   }
}



int
xmltoi(char *source)
{
   int     i;
   int     found_digit;
   int32_t numeric_value;

   found_digit   = OTTO_FALSE;
   numeric_value = 0;
   for(i=0; source[i] != '<' && source[i] != '\0'; i++)
   {
      if(source[i] == ' ')
      {
         if(found_digit == OTTO_TRUE)
            break;
         else
            continue;
      }
      if(source[i] >= '0' && source[i] <= '9')
      {
         found_digit = OTTO_TRUE;
         numeric_value = (numeric_value * 10) + (source[i] - '0');
      }
      else /* this array position contains a character not a digit or space */
         break; /* get out of the loop, a non-numeric is an error */
   }

   return(numeric_value);
}



void
xmlcpy(char *t, char *s)
{
   char code[9];
   char *tstart = t;

   if(t != NULL)
   {
      if(s != NULL)
      {
         while(isspace(*s))
            s++;

         while(*s != '\0' && *s != '<')
         {
            if(*s == '&')
            {
               s++;
               mkampcode(code, s);
               switch(cde8int(code))
               {
                  case cdeLT______: *t++ = '<';  s += 2; break;
                  case cdeGT______: *t++ = '>';  s += 2; break;
                  case cdeAMP_____: *t++ = '&';  s += 3; break;
                  case cdeAPOS____: *t++ = '\''; s += 4; break;
                  case cdeQUOT____: *t++ = '"';  s += 4; break;
                  case cdeNBSP____: *t++ = ' ';  s += 4; break;
                  default:          *t++ = '&';          break;
               }
            }
            else
            {
               *t++ = *s;
            }

            s++;
         }

         while(t > tstart && isspace(*(t-1)))
            t--;

      }

      *t = '\0';
   }
}



int
xmlchr(char *s, int c)
{
   int retval=OTTO_FALSE;
   char stxt[4096];

   xmlcpy(stxt, s);

   if(strchr(stxt, c) != NULL)
      retval = OTTO_TRUE;

   return(retval);
}



int
xmlcmp(char *s1, char *s2)
{
   char s1txt[4096], s2txt[4096];

   xmlcpy(s1txt, s1);
   xmlcpy(s2txt, s2);

   return(strcmp(s1txt, s2txt));
}



int
read_stdin(DYNBUF *b, char *client)
{
   int retval = OTTO_SUCCESS;
   int nread;
   char *line;
   char buffer[8192];
   char prompt[PATH_MAX];
   char histfname[PATH_MAX];
   char *histfile = NULL;


   if(b != NULL)
   {
      memset(b, 0, sizeof(DYNBUF));

      // start the buffer with a newline character since JIL parsing does a
      // one character look back on keywords
      b->bufferlen = 2;  // space for newline and null termnator
      b->buffer = realloc(b->buffer, b->bufferlen);
      if(b->buffer == NULL)
      {
         lperror(logp, MAJR, "Failed to reallocate buf");
         retval = OTTO_FAIL;
      }
      else
      {
         b->buffer[b->eob] = '\n';  // add newline
         b->eob++;
         b->buffer[b->eob] = '\0';  // null terminate
      }

      if(isatty(STDIN_FILENO))
      {
         otto_sprintf(prompt, "%s> ", client);
         otto_sprintf(histfname, "$HOME/.%s_history", client);
         histfile = expand_path(histfname);

         linenoiseHistoryLoad(histfile); // Load the history

         while(retval == OTTO_SUCCESS && (line = linenoise(prompt)) != NULL)
         {
            if(strcmp(line, "q")    == 0 ||
               strcmp(line, "quit") == 0 ||
               strcmp(line, "exit") == 0)
            {
               break;
            }

            linenoiseHistoryAdd(line);      // Add to the history
            linenoiseHistorySave(histfile); // Save the history to disk

            nread = strlen(line);
            b->bufferlen += nread + 2;  // add space for newline and null termnator
            b->buffer = realloc(b->buffer, b->bufferlen);
            if(b->buffer == NULL)
            {
               lperror(logp, MAJR, "Failed to reallocate buf");
               retval = OTTO_FAIL;
            }
            else
            {
               memcpy(&b->buffer[b->eob], line, nread);
               b->eob += nread;
               b->buffer[b->eob] = '\n';  // add newline
               b->eob++;
               b->buffer[b->eob] = '\0';  // null terminate
            }

            linenoiseFree(line);
         }

         printf("\n");
         fflush(stdout);
      }
      else
      {
         while(retval == OTTO_SUCCESS && fgets(buffer, 8192, stdin) != NULL)
         {
            nread = strlen(buffer);
            b->bufferlen += nread + 1;
            b->buffer = realloc(b->buffer, b->bufferlen);
            if(b->buffer == NULL)
            {
               lperror(logp, MAJR, "Failed to reallocate buf");
               retval = OTTO_FAIL;
            }
            memcpy(&b->buffer[b->eob], buffer, nread);
            b->eob += nread;
            b->buffer[b->eob] = '\0';
         }
      }

      b->s = b->buffer;

      if(b->bufferlen == 0)
         retval = OTTO_FAIL;
   }
   else
   {
      lperror(logp, MAJR, "buffer is null");
      retval = OTTO_FAIL;
   }

   return(retval);
}



void
advance_word(DYNBUF *b)
{
   // treat double quoted strings as whole words
   if(*(b->s) == '"')
   {
      b->s++;
      while(*(b->s) != '\0')
      {
         // handle escaped quotes
         if(*(b->s) == '"' && *(b->s-1) != '\\')
         {
            b->s++;
            break;
         }
         b->s++;
      }
   }
   else
   {
      // consume non-space
      while(!isspace(*(b->s)) && *(b->s) != '\0')
         b->s++;
   }

   // consume space
   while(isspace(*(b->s)) && *(b->s) != '\0')
   {
      if(*(b->s) == '\n')
         b->line++;
      b->s++;
   }
}



void
regress_word(DYNBUF *b)
{
   if(b->s != b->buffer)
   {
      b->s--;
      if(*(b->s) == '\n')
         b->line--;
   }
}



/*------------------------------ borrowed functions ------------------------------*/
//This function compares text strings, one of which can have wildcards ('*').
//Shamelessly lifted (and modified) from
// http://www.drdobbs.com/architecture-and-design/matching-wildcards-an-algorithm/210200888
//
// char *pTameText      - A string without wildcards
// char *pWildText      - A (potentially) corresponding string with wildcards

int
strwcmp(char *pTameText, char *pWildText)
{
   int  bMatch = OTTO_TRUE;
   char * pAfterLastWild = NULL; // The location after the last '%', if we.ve encountered one
   char * pAfterLastTame = NULL; // The location in the tame string, from which we started after last wildcard
   char t, w;

   // Walk the text strings one character at a time.
   while (1)
   {
      t = *pTameText;
      w = *pWildText;

      // How do you match a unique text string?
      if (!t || t == '\0')
      {
         // Easy: unique up on it!
         if (!w || w == '\0')
         {
            break;                                 // "x" matches "x"
         }
         else if (w == '%')
         {
            pWildText++;
            continue;                              // "x*" matches "x" or "xy"
         }
         else if (pAfterLastTame)
         {
            if (!(*pAfterLastTame) || *pAfterLastTame == '\0')
            {
               bMatch = OTTO_FALSE;
               break;
            }
            pTameText = pAfterLastTame++;
            pWildText = pAfterLastWild;
            continue;
         }

         bMatch = OTTO_FALSE;
         break;                                    // "x" doesn't match "xy"
      }
      else
      {
         // How do you match a tame text string?
         if (t != w)
         {
            // The tame way: unique up on it!
            if (w == '%')
            {
               pAfterLastWild = ++pWildText;
               pAfterLastTame = pTameText;
               w = *pWildText;

               if (!w || w == '\0')
               {
                  break;                           // "*" matches "x"
               }
               continue;                           // "*y" matches "xy"
            }
            else if (pAfterLastWild)
            {
               if (pAfterLastWild != pWildText)
               {
                  pWildText = pAfterLastWild;
                  w = *pWildText;

                  if (t == w)
                  {
                     pWildText++;
                  }
               }
               pTameText++;
               continue;                           // "*sip*" matches "mississippi"
            }
            else
            {
               bMatch = OTTO_FALSE;
               break;                              // "x" doesn't match "y"
            }
         }
      }

      pTameText++;
      pWildText++;
   }

   return(bMatch);
}



char *
expand_path(char *s)
{
   char *retval = NULL;
   char command[PATH_MAX+10];
   char tmpstr[PATH_MAX];
   int  i;
   FILE *p;

   strcpy(command, "echo ");
   strcpy(&command[5], s);

   if((p = popen(command, "r")) != NULL)
   {
      if(fgets(tmpstr, PATH_MAX, p) != NULL)
      {
         // trim trailing newline
         for(i=(int)strlen(tmpstr)-1; i>0; i--)
         {
            if(tmpstr[i] != '\0' && tmpstr[i] != '\n')
               break;
            if(tmpstr[i] == '\n')
               tmpstr[i] = '\0';
         }

         // printf("'%s' resolves to '%s'\n", s, tmpstr);
         retval = strdup(tmpstr);
      }
      pclose(p);
   }

   return(retval);
}



int
get_file_format(char *format)
{
   int retval = OTTO_UNKNOWN;

   if(strcasecmp(format, "CSV") == 0)
      retval = OTTO_CSV;

   if(strcasecmp(format, "JIL") == 0)
      retval = OTTO_JIL;

   if(strcasecmp(format, "MSP") == 0)
      retval = OTTO_MSP;

   return(retval);
}



// convert a hexadecimal digit from 0 to f to its decimal counterpart
int
hex_to_int(char hexChar)
{
   if (hexChar >= '0' && hexChar <= '9')
   {
      return hexChar - '0';
   }
   else if (hexChar >= 'a' && hexChar <= 'f')
   {
      return hexChar - 'a' + 10;
   }
   else if (hexChar >= 'A' && hexChar <= 'F')
   {
      return hexChar - 'A' + 10;
   }

   return -1;
}



// Unescapes url encoding: e.g. %20 becomes a space
// Changes are made in place
void
unescape_input(char *input)
{
   size_t length = strlen(input);
   size_t i, j;

   for (i = 0; i < length; ++i)
   {
      if (input[i] == '%' && i < length - 2)
      {
         int high = hex_to_int(input[i + 1]) * 16;
         int low = hex_to_int(input[i + 2]);

         // if either high or low is -1, it means our conversion failed
         // (i.e. one of the digits was not hexadecimal
         if (high >= 0 && low >= 0)
         {
            input[i] = (char) (high + low);

            // pull characters to fill the 'empty' space
            for(j = i + 3; j < length; ++j)
            {
               input[j - 2] = input[j];
            }

            // two characters were removed from the string, so we shorten the length
            input[length - 2] = '\0';
            input[length - 1] = '\0';
            length -= 2;
         }
      }
   }
}



// TODO rewrite this to use snprintf to determine length needed before printing
//      inefficient but not as unstable as using a fixed buffer
int
bprintf(DYNBUF *b, char *format, ...)
{
   int     retval = OTTO_SUCCESS;
   va_list ap;
   int     nwritten=0;
   char    buffer[8192];

   if(b != NULL)
   {
      va_start(ap, format);
      nwritten = vsprintf(buffer, format, ap);
      va_end(ap);

      b->bufferlen += nwritten + 1;
      b->buffer = realloc(b->buffer, b->bufferlen);
      if(b->buffer == NULL)
      {
         retval = OTTO_FAIL;
      }
      else
      {
         memcpy(&b->buffer[b->eob], buffer, nwritten);
         b->eob += nwritten;
         b->buffer[b->eob] = '\0';
      }
   }

   return(retval);
}



char *
copy_envvar_assignment(int *rc, char *t, int tlen, char *s)
{
   *rc = 0;

   // consume leading space
   while(*s != '\n' && *s != '\0' && (isspace(*s) || *s == ';'))
      s++;

   // copy characters up to newline or null terminator
   // or an unguarded semicolon or space
   while(tlen > 1 && *s != ';' && *s != '\n' && *s != '\0' && !isspace(*s))
   {
      // copy quoted strings entirely
      if(*s == '"')
      {
         *t++ = *s++;
         tlen--;
         while(tlen > 1 && *s != '\n' && *s != '\0')
         {
            *t++ = *s++;
            tlen--;
            // handle escaped quotes
            if(*(s-1) == '"' && *(s-2) != '\\')
            {
               break;
            }
         }
      }
      else
      {
         *t++ = *s++;
         tlen--;
      }
   }

   // null terminate
   *t = '\0';
   tlen--;

   // check if target length is overflowed
   if(tlen == 0 && *s != '\n' && *s != '\0' && !isspace(*s))
      *rc = OTTO_LENGTH_EXCEEDED;

   return(s);
}


int
validate_envvar_assignment(char *instring)
{
   int   rc = OTTO_SUCCESS;
   int   i;
   char *equal;
   char *s;
   char *strvars[VAR_TOTAL+1] =
   {
#define X(value) #value"=",
      ENV_NAMES
#undef X
         "VAR_TOTAL"
   };


   // find equal sign
   for(equal=instring; *equal != '\0'; equal++)
   {
      if(*equal == '=')
      {
         break;
      }
   }
   if(equal == instring || *equal == '\0' || *(equal+1) == '\0')
   {
      rc |= OTTO_MALFORMED_ENV_ASSIGNMENT;
   }

   if(rc == OTTO_SUCCESS)
   {
      s = instring;
      // validate name character set
      for(s=instring; rc==OTTO_SUCCESS && s<equal; s++)
      {
         if(s == instring)
         {
            if(*s != '_' && !isalpha(*s))
            {
               rc |= OTTO_INVALID_ENVNAME_FIRSTCHAR;
            }
         }
         else
         {
            if(*s != '_' && !isalnum(*s))
            {
               rc |= OTTO_INVALID_ENVNAME_CHARS;
            }
         }
      }
   }

   if(rc == OTTO_SUCCESS)
   {
      // trap attempts to override derived environment variables
      for(i=OTTO_JOBNAMEVAR; i<PATHVAR; i++)
      {
         if(strncmp(instring, strvars[i], strlen(strvars[i])) == 0)
         {
            rc |= OTTO_INVALID_OVERRIDE;
         }
      }
   }

   return(rc);
}


int
otto_sprintf(char *buf, char *format, ...)
{
   va_list ap;
   int     retval;

   if(buf != NULL)
   {
      va_start(ap, format);
      retval = vsprintf(buf, format, ap);
      va_end(ap);
   }

   return(retval);
}




