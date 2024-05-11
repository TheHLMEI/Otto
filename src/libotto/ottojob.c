/*------------------ job structure copy and validation functions ------------------*/
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "otto.h"

extern OTTOLOG *logp;


#define OTTO_TERM        0
#define OTTO_CONJUNCTION 1

#define VCNDLEN             4096


typedef struct _vcnd_st
{
   char *s;
   char *t;
   char  input[VCNDLEN];
   char  output[VCNDLEN];
} vcnd_st;


char jobname_char[256] = {
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
// !  "  #  $  %  &  '  (  )  *  +  ,  -  .  /  0  1  2  3  4  5  6  7  8  9  :  ;  <  =  >
   0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
// @  A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P  Q  R  S  T  U  V  W  X  Y  Z  [  \  ]  ^  _
   0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1,
// `  a  b  c  d  e  f  g  h  i  j  k  l  m  n  o  p  q  r  s  t  u  v  w  x  y  z  {  |  }  ~  .
   0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};


char envname_char[256] = {
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
// !  "  #  $  %  &  '  (  )  *  +  ,  -  .  /  0  1  2  3  4  5  6  7  8  9  :  ;  <  =  >
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
// @  A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P  Q  R  S  T  U  V  W  X  Y  Z  [  \  ]  ^  _
   0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1,
// `  a  b  c  d  e  f  g  h  i  j  k  l  m  n  o  p  q  r  s  t  u  v  w  x  y  z  {  |  }  ~  .
   0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};



int  ottojob_validate_condition(vcnd_st *c);
int  is_status(char *word);
void ottojob_copy_condition_word(char *t, char *s, int tlen);



int
ottojob_copy_command(char *output, char *command, int outlen)
{
   int retval = OTTO_SUCCESS;
   char *p, *s, *t;
   int l;

   s = command;
   t = output;
   l = outlen;
   p = t;

   if(t != NULL && l >= 1)
      *t = '\0';

   if(s != NULL && t != NULL && l >= 1)
   {
      while(*s != '\r' && *s != '\n' && *s != '\0')
      {
         if(l > 0)
         {
            *t++ = *s++;
            if(!isspace(*(t-1)))
               p = t;
            l--;
         }
         else
         {
            return(OTTO_LENGTH_EXCEEDED);
         }
      }
      *p = '\0';
   }

   return(retval);
}



int
ottojob_copy_condition(char *output, char *condition, int outlen)
{
   int retval = OTTO_SUCCESS;
   int paren_count = 0, len = 0;
   char *p, *s, *t;
   vcnd_st c;

   if(output != NULL && outlen >= 1)
      *output = '\0';

   if(condition != NULL && output != NULL && outlen >= 1)
   {
      s = condition;
      t = c.input;
      p = t;

      while(*s != '\r' && *s != '\n' && *s != '\0')
      {
         if(isspace(*s) && (t == c.input || isspace(*(t-1))))
         {
            s++; // eat it
         }
         else
         {
            if(*s == '(') paren_count++;
            if(*s == ')') paren_count--;
            *t++ = *s++;
            if(!isspace(*(t-1)))
               p = t;
         }
         if(t - c.input == VCNDLEN)
         {
            return(OTTO_CONDITION_BUFFER_EXCEEDED);
         }
      }
      *p = '\0';

      if(paren_count != 0)
      {
         return(OTTO_CONDITION_UNBALANCED);
      }

      c.s = c.input;
      c.t = c.output;

      retval = ottojob_validate_condition(&c);

      output[0] = '\0';
      if(retval == OTTO_SUCCESS)
      {
         len = strlen(c.output);
         if(len < outlen)
         {
            strcpy(output, c.output);
         }
         else
         {
            return(OTTO_LENGTH_EXCEEDED);
         }
      }
   }

   return(retval);
}



int
ottojob_copy_days_of_week(char *output, char *days_of_week)
{
   int retval = OTTO_SUCCESS;
   char *s, *t;
   char code[10], word[8128];
   int exit_loop = OTTO_FALSE;
   int comma_found = OTTO_FALSE;

   s = days_of_week;
   *output = 0;

   if(s != NULL && output != NULL)
   {
      // consume leading spaces
      while(*s != '\0' && isspace(*s))
         s++;

      // read a word at a time separated by comma or whitespace
      while(*s != '\0' && exit_loop == OTTO_FALSE)
      {
         // transfer a word
         t = word;
         while(*s != '\0' && *s != ',' && !isspace(*s))
            *t++ = *s++;
         *t = '\0';

         // evaluate the word
         mkcode(code, word);
         switch(cde8int(code))
         {
            case cdeSU______:  *output |= OTTO_SUN;  break;
            case cdeMO______:  *output |= OTTO_MON;  break;
            case cdeTU______:  *output |= OTTO_TUE;  break;
            case cdeWE______:  *output |= OTTO_WED;  break;
            case cdeTH______:  *output |= OTTO_THU;  break;
            case cdeFR______:  *output |= OTTO_FRI;  break;
            case cdeSA______:  *output |= OTTO_SAT;  break;
            case cdeALL_____:  *output |= OTTO_ALL;  break;
            default:
                               retval = OTTO_INVALID_VALUE;
                               break;
         }

         // consume whitespace (including commas)
         comma_found = OTTO_FALSE;
         while(*s != '\0' && (*s == ',' || isspace(*s)))
         {
            if(*s == ',')
               comma_found = OTTO_TRUE;
            s++;
         }

         // check to see if there's another word on the line
         if(comma_found == OTTO_FALSE)
         {
            exit_loop = OTTO_TRUE;
         }
      }
   }

   return(retval);
}



int
ottojob_copy_description(char *output, char *description, int outlen)
{
   int retval = OTTO_SUCCESS;
   char *s, *t;
   int l;

   s = description;
   t = output;
   l = outlen;

   if(t != NULL && l >= 1)
      *t = '\0';

   if(s != NULL && t != NULL && l >= 1)
   {
      // consume first quote
      if(*s == '"')
         s++;

      // now get up to the next quote
      while(*s != '"' && *s != '\0')
      {
         if(l > 0)
         {
            if(*s == '\\' && *(s+1) == '"')
               s++;

            // only transfer printable characters
            if(isprint(*s))
            {
               *t++ = *s++;
               l--;
            }
         }
         else
         {
            return(OTTO_LENGTH_EXCEEDED);
         }
      }

      *t = '\0';
   }

   return(retval);
}



int
ottojob_copy_environment(char *output, char *environment, int outlen)
{
   int retval = OTTO_SUCCESS;
   char *s, *t;
   int l;
   char tmpenv[ENVLEN+1];

   s = environment;
   t = output;
   l = outlen;

   if(t != NULL && l >= 1)
      *t = '\0';

   if(s != NULL && t != NULL && l >= 1)
   {
      while(retval == OTTO_SUCCESS && *s != '\n' && *s != '\0')
      {
         // copy and environment variable assignment from the source string
         s = copy_envvar_assignment(&retval, tmpenv, sizeof(tmpenv), s);

         // validate the environment variable assignment
         if(retval == OTTO_SUCCESS && tmpenv[0] != '\0')
         {
            retval |= validate_envvar_assignment(tmpenv);
         }

         // tack it onto the end of the string
         if(retval == OTTO_SUCCESS)
         {
            if(strlen(output) + strlen(tmpenv) > (outlen-2))
            {
               return(OTTO_LENGTH_EXCEEDED);
            }
            else
            {
               strcat(output, tmpenv);
               strcat(output, ";");
            }
         }
      }
   }

   return(retval);
}



int
ottojob_copy_flag(char *output, char *input, int outlen)
{
   int retval = OTTO_SUCCESS;
   char *s, *t;

   s = input;
   t = output;

   if(t != NULL && outlen >= 1)
      *t = '\0';

   if(s != NULL && t != NULL && outlen >= 1)
   {
      switch(tolower(*s))
      {
         case 't':
         case 'y':
         case '1':
            *t = OTTO_TRUE;
            break;
         case 'f':
         case 'n':
         case '0':
            *t = OTTO_FALSE;
            break;
         default:
            retval = OTTO_INVALID_VALUE;
      }
   }

   return(retval);
}



int
ottojob_copy_loop(char *loopname, char *loopmin, char *loopmax, char *loopsgn, char *input)
{
   int retval = OTTO_SUCCESS;
   char *s;
   char *word[6];
   int i, len[6];
   int varidx, minidx, maxidx;

   s = input;

   if(s != NULL && loopname != NULL && loopmin != NULL && loopmax != NULL)
   {
      // the loop line is of the form
      // FOR VARIABLE=min,max
      // where the FOR is optional and there may be spaces between words
      // look for 6 "words" ('for' variablename '=' min ',' max')
      for(i=0; i<6; i++)
      {
         // consume whitespace
         while(*s != '\0' && *s != '\r' && *s != '\n' && isspace(*s))
            s++;

         // mark the word start
         word[i] = s;

         if(word[i][0] != '=' && word[i][0] != ',')
         {
            // advance to the word delimiter
            while(*s != '\0' && *s != '\r' && *s != '\n' && *s != '=' && *s != ',' && !isspace(*s))
               s++;
         }
         else
         {
            if(*s != '\0' && *s != '\r' && *s != '\r')
               s++;
         }

         // compute the length of the word
         len[i] = (int)(s - word[i]);
      }

      // the equal sign should be the second or third word depending on if the word FOR was used
      // if it's 3 then the first work MUST be FOR (case insensitive)
      if(word[1][0] == '=' ||
         (word[2][0] == '=' && len[0] == 3 && strncasecmp(word[0], "for", 3) == 0))
      {
         if(word[1][0] == '=')
         {
            varidx = 0;
            minidx = 2;
            maxidx = 4;
         }
         else
         {
            varidx = 1;
            minidx = 3;
            maxidx = 5;
         }

         // copy values
         if(len[varidx] >= 1 && len[varidx] <= VARLEN)
         {
            if(isdigit(word[varidx][0]))
            {
               retval |= OTTO_INVALID_ENVNAME_FIRSTCHAR;
            }

            for(i=0; i<len[varidx]; i++)
            {
               if(envname_char[word[varidx][i]] == 1)
               {
                   loopname[i] = word[varidx][i];
               }
               else
               {
                  retval |= OTTO_INVALID_ENVNAME_CHARS;
               }
            }
            loopname[i] = '\0';
         }
         else
         {
            retval |= OTTO_INVALID_NAME_LENGTH;
         }

         if(len[minidx] >= 1 && len[minidx] <= 2)
         {
            *loopmin = 0;
            for(i=0; i<len[minidx]; i++)
            {
               if(isdigit(word[minidx][i]))
               {
                   *loopmin = (*loopmin * 10) + (word[minidx][i] - '0');
               }
               else
               {
                  retval |= OTTO_INVALID_MINNUM_CHARS;
               }
            }
         }
         else
         {
            retval |= OTTO_INVALID_LOOPMIN_LEN;
         }

         if(len[maxidx] >= 1 && len[maxidx] <= 2)
         {
            *loopmax = 0;
            for(i=0; i<len[maxidx]; i++)
            {
               if(isdigit(word[maxidx][i]))
               {
                   *loopmax = (*loopmax * 10) + (word[maxidx][i] - '0');
               }
               else
               {
                  retval |= OTTO_INVALID_MAXNUM_CHARS;
               }
            }
         }
         else
         {
            retval |= OTTO_INVALID_LOOPMAX_LEN;
         }

      }
      else
      {
         retval |= OTTO_MALFORMED_LOOP;
      }
   }

   // swap signs and store as negative values to make loop comparisons easier
   // (always count up i.e. loopmin <= loopnum <= loopmax and loopnum++)
   *loopsgn  = 1;
   if(*loopmin > *loopmax)
   {
      *loopmin *= -1;
      *loopmax *= -1;
      *loopsgn  = -1;
   }

   if(cfg.debug == OTTO_TRUE)
   {
      lprintf(logp, INFO, "loop: FOR %s=%d,%d\n", loopname, *loopmin, *loopmax);
   }

   return(retval);
}



int
ottojob_copy_name(char *output, char *name, int outlen)
{
   int retval = OTTO_SUCCESS;
   int namelen = 0;
   char *s, *t;

   s = name;
   t = output;

   if(t != NULL && outlen >= 1)
      *t = '\0';

   if(s != NULL && t != NULL && outlen >= 1)
   {
      while(*s != '\0' && *s != ')' && !isspace(*s))
      {
         if(jobname_char[*s] == 1)
         {
            *t++ = *s++;
            namelen++;
         }
         else
         {
            retval |= OTTO_INVALID_NAME_CHARS;
         }
      }
      *t = '\0';

      if(namelen > NAMLEN)
      {
         retval |= OTTO_INVALID_NAME_LENGTH;
      }
   }

   return(retval);
}



int
ottojob_copy_start_minutes(int64_t *output, char *start_minutes)
{
   int retval = OTTO_SUCCESS;
   char *s, *t;
   char word[8128];
   int exit_loop = OTTO_FALSE;
   int comma_found = OTTO_FALSE;
   int min = 0, time_is_valid = OTTO_FALSE;

   s = start_minutes;
   *output = 0;

   if(s != NULL && output != NULL)
   {
      // consume leading spaces
      while(*s != '\0' && isspace(*s))
         s++;

      // read a word at a time separated by comma or whitespace
      while(*s != '\0' && exit_loop == OTTO_FALSE)
      {
         // transfer a word
         t = word;
         while(*s != '\0' && *s != ',' && !isspace(*s))
            *t++ = *s++;
         *t = '\0';

         // evaluate the word
         time_is_valid = OTTO_FALSE;
         switch(strlen(word))
         {
            case 1:
               if(isdigit(word[0]))
               {
                  min  = word[0] - '0';
                  time_is_valid = OTTO_TRUE;
               }
               break;
            case 2:
               if(isdigit(word[0]) &&
                  isdigit(word[1]))
               {
                  min  = ((word[0] - '0') * 10) + (word[1] - '0');
                  if(min < 60)
                     time_is_valid = OTTO_TRUE;
               }
               break;
         }

         if(time_is_valid == OTTO_TRUE)
         {
            *output |= (1L << min);
         }
         else
         {
            retval = OTTO_INVALID_VALUE;
         }

         // consume whitespace (including commas)
         comma_found = OTTO_FALSE;
         while(*s != '\0' && (*s == ',' || isspace(*s)))
         {
            if(*s == ',')
               comma_found = OTTO_TRUE;
            s++;
         }

         // check to see if there's another word on the line
         if(comma_found == OTTO_FALSE)
         {
            exit_loop = OTTO_TRUE;
         }
      }
   }

   return(retval);
}



int
ottojob_copy_start_times(int64_t *output, char *start_times)
{
   int retval = OTTO_SUCCESS;
   char *s, *t;
   char word[8128];
   int exit_loop = OTTO_FALSE;
   int quote_found = OTTO_FALSE;
   int comma_found = OTTO_FALSE;
   int i, hour = 0, min = 0, time_is_valid = OTTO_FALSE;

   s = start_times;
   for(i=0; i<24; i++)
      output[i] = 0;

   if(s != NULL && output != NULL)
   {
      // consume leading spaces
      while(*s != '\0' && isspace(*s))
         s++;

      // consume leading quote if there is one
      if(*s == '"')
      {
         s++;
         quote_found = OTTO_TRUE;
      }

      // read a word at a time separated by comma or whitespace
      while(*s != '\0' && exit_loop == OTTO_FALSE)
      {
         // transfer a word
         t = word;
         while(*s != '\0' && *s != ',' && *s != '"' && !isspace(*s))
         {
            if(quote_found == OTTO_FALSE && *s == ':' && *(t-1) == '\\')
               *(t-1) = *s++;
            else
               *t++ = *s++;
         }
         *t = '\0';

         // evaluate the word
         time_is_valid = OTTO_FALSE;
         switch(strlen(word))
         {
            case 4:
               if(isdigit(word[0]) &&
                  word[1] == ':'   &&
                  isdigit(word[2]) &&
                  isdigit(word[3]))
               {
                  hour = word[0] - '0';
                  min  = ((word[2] - '0') * 10) + (word[3] - '0');
                  if(min < 60)
                     time_is_valid = OTTO_TRUE;
               }
               break;
            case 5:
               if(isdigit(word[0]) &&
                  isdigit(word[1]) &&
                  word[2] == ':'   &&
                  isdigit(word[3]) &&
                  isdigit(word[4]))
               {
                  hour = ((word[0] - '0') * 10) + (word[1] - '0');
                  min  = ((word[3] - '0') * 10) + (word[4] - '0');
                  if(hour < 24 && min < 60)
                     time_is_valid = OTTO_TRUE;
               }
               break;
         }

         if(time_is_valid == OTTO_TRUE)
         {
            output[hour] |= (1L << min);
         }
         else
         {
            retval = OTTO_INVALID_VALUE;
         }

         // consume whitespace (including commas)
         comma_found = OTTO_FALSE;
         while(*s != '\0' && *s != '"' && (*s == ',' || isspace(*s)))
         {
            if(*s == ',')
               comma_found = OTTO_TRUE;
            s++;
         }

         // check to see if there's another word on the line
         if(comma_found == OTTO_FALSE || *s == '"')
         {
            exit_loop = OTTO_TRUE;
         }
      }
   }

   return(retval);
}



int
ottojob_copy_time(time_t *t, char *s)
{
   int retval = OTTO_SUCCESS;
   struct tm tm;

   if(t != NULL)
      *t = 0;

   if(s == NULL || t == NULL)
   {
      retval = OTTO_FAIL;
   }
   else
   {
      // string should arrive quoted so advance beyond quote
      s++;
      if(otto_strptime(s, "%m/%d/%Y %H:%M:%S", &tm) != NULL)  // dates and times in the form of '05/08/2022 16:28:13'
      {
         tm.tm_isdst = -1;  // force a TZ lookup instead of having to know the the DST status for the server
         *t = mktime(&tm);
      }
      else
      {
         retval = OTTO_INVALID_VALUE;
      }
   }

   return(retval);
}



int
ottojob_copy_type(char *output, char *type, int outlen)
{
   int retval = OTTO_SUCCESS;
   char *s, *t;

   if(output == NULL || outlen < 1)
   {
      retval = OTTO_FAIL;
   }
   else
   {
      s = type;
      t = output;

      // default to OTTO_CMD
      *t = OTTO_CMD;

      if(s != NULL && t != NULL && outlen >= 1)
      {
         *t = tolower(*s);

         if(*t != OTTO_BOX && *t != OTTO_CMD)
         {
            retval = OTTO_INVALID_VALUE;
         }
      }
   }

   return(retval);
}



int
ottojob_validate_condition(vcnd_st *c)
{
   int  retval     = OTTO_SUCCESS;
   int  and_count  = 0;
   int  or_count   = 0;
   int  open_paren = OTTO_FALSE;
   int  namelen    = 0;
   int  last       = OTTO_CONJUNCTION;
   char tmpstr[VCNDLEN];

   if(*(c->s) == '(')
   {
      *(c->t)++ = '(';
      c->s++;
      open_paren = OTTO_TRUE;
   }

   while(*(c->s) != '\0' && retval == OTTO_SUCCESS)
   {
      if(!isspace(*(c->s)))
      {
         switch(toupper(*(c->s)))
         {
            case '(':
               retval = ottojob_validate_condition(c);
               last = OTTO_TERM;
               break;

            case ')':
               *(c->t)++ = ')';
               if(open_paren == OTTO_FALSE)
               {
                  retval = OTTO_CONDITION_UNBALANCED;
               }
               break;

            case '&':
               if(last == OTTO_TERM)
               {
                  *(c->t)++ = ' ';
                  *(c->t)++ = '&';
                  *(c->t)++ = ' ';
                  and_count++;
                  last = OTTO_CONJUNCTION;
               }
               else
               {
                  retval |= OTTO_CONDITION_ONE_OPERAND;
               }
               break;

            case '|':
               if(last == OTTO_TERM)
               {
                  *(c->t)++ = ' ';
                  *(c->t)++ = '|';
                  *(c->t)++ = ' ';
                  or_count++;
                  last = OTTO_CONJUNCTION;
               }
               else
               {
                  retval |= OTTO_CONDITION_ONE_OPERAND;
               }
               break;

            case 'A':
               // possible AND or and
               if(strncasecmp(c->s, "and", 3) == 0 && (isspace(*(c->s+3)) || *(c->s+3) == ')'))
               {
                  if(last == OTTO_TERM)
                  {
                     *(c->t)++ = ' ';
                     *(c->t)++ = '&';
                     *(c->t)++ = ' ';
                     and_count++;
                     c->s+=2;
                     last = OTTO_CONJUNCTION;
                  }
                  else
                  {
                     retval |= OTTO_CONDITION_ONE_OPERAND;
                  }
               }
               break;

            case 'O':
               // possible OR or or
               if(strncasecmp(c->s, "or", 2) == 0 && (isspace(*(c->s+2)) || *(c->s+2) == ')'))
               {
                  if(last == OTTO_TERM)
                  {
                     *(c->t)++ = ' ';
                     *(c->t)++ = '|';
                     *(c->t)++ = ' ';
                     or_count++;
                     c->s++;
                     last = OTTO_CONJUNCTION;
                  }
                  else
                  {
                     retval |= OTTO_CONDITION_ONE_OPERAND;
                  }
               }
               break;

            case 'D': case 'F': case 'N': case 'R': case 'S': case 'T':
               // possible status
               switch(is_status(c->s))
               {
                  case OTTO_TRUE:
                     if(last == OTTO_CONJUNCTION)
                     {
                        *(c->t)++ = tolower(*(c->s));
                        while(*(c->s) != '\0' && *(c->s) != '(')
                           c->s++;
                        if(*(c->s) == '(')
                        {
                           *(c->t++) = '(';
                           c->s++;

                           while(*(c->s) != '\0' && isspace(*(c->s)))
                              c->s++;

                           if(*(c->s) == ')')
                           {
                              retval |= OTTO_CONDITION_MISSING_JOBNAME;
                           }
                           else
                           {
                              namelen = 0;
                              while(*(c->s) != '\0' && *(c->s) != ')' && !isspace(*(c->s)))
                              {
                                 if(jobname_char[*(c->s)] == 1)
                                 {
                                    *(c->t)++ = *(c->s);
                                    c->s++;
                                    namelen++;
                                 }
                                 else
                                 {
                                    retval |= OTTO_INVALID_NAME_CHARS;
                                    break;
                                 }
                              }

                              if(namelen > NAMLEN)
                              {
                                 retval |= OTTO_INVALID_NAME_LENGTH;
                              }

                              while(*(c->s) != '\0' && isspace(*(c->s)))
                                 c->s++;

                              if(*(c->s) == ')')
                              {
                                 *(c->t)++ = ')';
                              }
                              else
                              {
                                 retval |= OTTO_INVALID_NAME_CHARS;
                              }
                           }
                        }
                        last = OTTO_TERM;
                     }
                     else
                     {
                        retval |= OTTO_CONDITION_ONE_OPERAND;
                     }
                     break;
                  default:
                     ottojob_copy_condition_word(tmpstr, c->s, sizeof(tmpstr));
                     retval |= OTTO_CONDITION_INVALID_KEYWORD;
                     break;
               }
               break;

            default:
               ottojob_copy_condition_word(tmpstr, c->s, sizeof(tmpstr));
               retval |= OTTO_CONDITION_INVALID_KEYWORD;
               break;
         }
      }

      c->s++;
   }
   *(c->t) = '\0';

   if(and_count > 0 && or_count > 0)
   {
      retval |= OTTO_CONDITION_NEEDS_PARENS;
   }

   if(last == OTTO_CONJUNCTION && strlen(c->output) != 0)
   {
      retval |= OTTO_CONDITION_ONE_OPERAND;
   }

   return(retval);
}



int
is_status(char *word)
{
   int retval = OTTO_FALSE;

   if(isspace(word[1]) || word[1] == '(')
   {
      retval = OTTO_TRUE;
   }
   else
   {
      switch(toupper(word[0]))
      {
         case 'D': if(strncasecmp(word, "done",        4) == 0 && (isspace(word[4])  || word[4]  == '(')) retval = OTTO_TRUE; break;
         case 'F': if(strncasecmp(word, "failure",     7) == 0 && (isspace(word[7])  || word[7]  == '(')) retval = OTTO_TRUE; break;
         case 'N': if(strncasecmp(word, "notrunning", 10) == 0 && (isspace(word[10]) || word[10] == '(')) retval = OTTO_TRUE; break;
         case 'R': if(strncasecmp(word, "running",     7) == 0 && (isspace(word[7])  || word[7]  == '(')) retval = OTTO_TRUE; break;
         case 'S': if(strncasecmp(word, "success",     7) == 0 && (isspace(word[7])  || word[7]  == '(')) retval = OTTO_TRUE; break;
         case 'T': if(strncasecmp(word, "terminated", 10) == 0 && (isspace(word[10]) || word[10] == '(')) retval = OTTO_TRUE; break;
      }
   }

   return(retval);
}



void
ottojob_copy_condition_word(char *t, char *s, int tlen)
{
   int exit_loop = OTTO_FALSE;

   if(t != NULL && tlen >= 1)
      *t = '\0';

   if(s != NULL && t != NULL && tlen >= 1)
   {
      while(exit_loop == OTTO_FALSE && *s != '\0' && !isspace(*s))
      {
         switch(*s)
         {
            case '(':
            case ')':
            case '&':
            case '|':
               exit_loop = OTTO_TRUE;
               break;
            default:
               if(tlen > 0)
               {
                  *t++ = *s;
                  tlen--;
               }
               else
               {
                  exit_loop = OTTO_TRUE;
               }
               break;
         }

         s++;
      }
      *t = '\0';
   }
}



void
ottojob_log_job_layout()
{
   JOB j;

   lsprintf(logp, INFO, "Job structure layout:\n");
   lsprintf(logp, CATI, "id              at %4d for %4d type uinteger notnull\n", offsetof(JOB, id),              sizeof(j.id));
   lsprintf(logp, CATI, "level           at %4d for %4d type uinteger notnull\n", offsetof(JOB, level),           sizeof(j.level));
   lsprintf(logp, CATI, "box             at %4d for %4d type uinteger notnull\n", offsetof(JOB, box),             sizeof(j.box));
   lsprintf(logp, CATI, "head            at %4d for %4d type uinteger notnull\n", offsetof(JOB, head),            sizeof(j.head));
   lsprintf(logp, CATI, "tail            at %4d for %4d type uinteger notnull\n", offsetof(JOB, tail),            sizeof(j.tail));
   lsprintf(logp, CATI, "prev            at %4d for %4d type uinteger notnull\n", offsetof(JOB, prev),            sizeof(j.prev));
   lsprintf(logp, CATI, "next            at %4d for %4d type uinteger notnull\n", offsetof(JOB, next),            sizeof(j.next));

   lsprintf(logp, CATI, "name            at %4d for %4d type char     notnull\n", offsetof(JOB, name),            sizeof(j.name));
   lsprintf(logp, CATI, "type            at %4d for %4d type char     notnull\n", offsetof(JOB, type),            sizeof(j.type));
   lsprintf(logp, CATI, "box_name        at %4d for %4d type char     notnull\n", offsetof(JOB, box_name),        sizeof(j.box_name));
   lsprintf(logp, CATI, "description     at %4d for %4d type char     notnull\n", offsetof(JOB, description),     sizeof(j.description));
   lsprintf(logp, CATI, "command         at %4d for %4d type char     notnull\n", offsetof(JOB, command),         sizeof(j.command));
   lsprintf(logp, CATI, "condition       at %4d for %4d type char     notnull\n", offsetof(JOB, condition),       sizeof(j.condition));
   lsprintf(logp, CATI, "date_conditions at %4d for %4d type integer  notnull\n", offsetof(JOB, date_conditions), sizeof(j.date_conditions));
   lsprintf(logp, CATI, "days_of_week    at %4d for %4d type char     notnull\n", offsetof(JOB, days_of_week),    sizeof(j.days_of_week));
   lsprintf(logp, CATI, "start_minutes   at %4d for %4d type char     notnull\n", offsetof(JOB, start_minutes),   sizeof(j.start_minutes));
   lsprintf(logp, CATI, "start_times     at %4d for %4d type char     notnull\n", offsetof(JOB, start_times),     sizeof(j.start_times));
   lsprintf(logp, CATI, "autohold        at %4d for %4d type integer  notnull\n", offsetof(JOB, autohold),        sizeof(j.autohold));
   lsprintf(logp, CATI, "autonoexec      at %4d for %4d type integer  notnull\n", offsetof(JOB, autonoexec),      sizeof(j.autonoexec));
   lsprintf(logp, CATI, "environment     at %4d for %4d type char     notnull\n", offsetof(JOB, environment),     sizeof(j.environment));
   lsprintf(logp, CATI, "loopname        at %4d for %4d type char     notnull\n", offsetof(JOB, loopname),        sizeof(j.loopname));
   lsprintf(logp, CATI, "loopmin         at %4d for %4d type integer  notnull\n", offsetof(JOB, loopmin),         sizeof(j.loopmin));
   lsprintf(logp, CATI, "loopmax         at %4d for %4d type integer  notnull\n", offsetof(JOB, loopmax),         sizeof(j.loopmax));
   lsprintf(logp, CATI, "loopsgn         at %4d for %4d type integer  notnull\n", offsetof(JOB, loopsgn),         sizeof(j.loopsgn));

   lsprintf(logp, CATI, "expression      at %4d for %4d type char     notnull\n", offsetof(JOB, expression),      sizeof(j.expression));
   lsprintf(logp, CATI, "expr_fail       at %4d for %4d type integer  notnull\n", offsetof(JOB, expr_fail),       sizeof(j.expr_fail));
   lsprintf(logp, CATI, "status          at %4d for %4d type integer  notnull\n", offsetof(JOB, status),          sizeof(j.status));
   lsprintf(logp, CATI, "on_autohold     at %4d for %4d type integer  notnull\n", offsetof(JOB, on_autohold),     sizeof(j.on_autohold));
   lsprintf(logp, CATI, "on_noexec       at %4d for %4d type integer  notnull\n", offsetof(JOB, on_noexec),       sizeof(j.on_noexec));
   lsprintf(logp, CATI, "loopnum         at %4d for %4d type integer  notnull\n", offsetof(JOB, loopnum),         sizeof(j.loopnum));
   lsprintf(logp, CATI, "loopstat        at %4d for %4d type integer  notnull\n", offsetof(JOB, loopstat),        sizeof(j.loopstat));
   lsprintf(logp, CATI, "pid             at %4d for %4d type integer  notnull\n", offsetof(JOB, pid),             sizeof(j.pid));
   lsprintf(logp, CATI, "start           at %4d for %4d type integer  notnull\n", offsetof(JOB, start),           sizeof(j.start));
   lsprintf(logp, CATI, "finish          at %4d for %4d type integer  notnull\n", offsetof(JOB, finish),          sizeof(j.finish));
   lsprintf(logp, CATI, "duration        at %4d for %4d type integer  notnull\n", offsetof(JOB, duration),        sizeof(j.duration));
   lsprintf(logp, CATI, "exit_status     at %4d for %4d type integer  notnull\n", offsetof(JOB, exit_status),     sizeof(j.exit_status));

   lsprintf(logp, CATI, "gpflag          at %4d for %4d type char     notnull\n", offsetof(JOB, gpflag),          sizeof(j.gpflag));
   lsprintf(logp, CATI, "attributes      at %4d for %4d type char     notnull\n", offsetof(JOB, attributes),      sizeof(j.attributes));
   lsprintf(logp, END, "");
}



void
build_joblist(JOBLIST *joblist, DBCTX *ctx, char *jobname, int id, int level_offset, int level_limit, int perform_check)
{
   int check;

   if(joblist != NULL && joblist->item != NULL)
   {
      if(perform_check == OTTO_TRUE || (id > -1 && ctx->job[id].level - level_offset <= level_limit))
      {
         while(id != -1)
         {
            check = perform_check;

            // if there is no need to check this job's name or
            // there is a need to check and it matches the supplied jobname
            if((check == OTTO_FALSE) ||
               (check == OTTO_TRUE && strwcmp(ctx->job[id].name, jobname) == OTTO_TRUE))
            {
               // if this job's parent wasn't printed start the level limiting
               // with this job's level
               if(ctx->job[ctx->job[id].box].printed != OTTO_TRUE)
                  level_offset = ctx->job[id].level;

               // mark this job as having been printed
               ctx->job[id].printed = OTTO_TRUE;

               // copy the job's values into the job list
               memcpy(&joblist->item[joblist->nitems], &ctx->job[id], sizeof(JOB));
               joblist->item[joblist->nitems].opcode = CREATE_JOB;
               joblist->nitems++;
            }

            // recurse if this job has children
            if(ctx->job[id].type == OTTO_BOX)
            {
               if(ctx->job[id].printed == OTTO_TRUE)
               {
                  // don't bother with the name check if this box is being printed
                  check = OTTO_FALSE;
               }
               build_joblist(joblist, ctx, jobname, ctx->job[id].head, level_offset, level_limit, check);
            }

            id = ctx->job[id].next;
         }
      }
   }
}



void
build_deplist(JOBLIST *deplist, DBCTX *ctx, JOB *job)
{
   int id, jobid, exit_loop;
   int16_t index;
   char   *x, *s, opcode;


   if(deplist != NULL && deplist->item != NULL && ctx != NULL && job != NULL)
   {
      jobid = job->id;

      for(id=0; id<cfg.ottodb_maxjobs; id++)
      {
         // look for the jobid in this job's condition statement
         if(ctx->job[id].condition[0] != '\0')
         {
            s         = ctx->job[id].expression;
            exit_loop = OTTO_FALSE;
            while(exit_loop == OTTO_FALSE && *s != '\0')
            {
               switch(*s)
               {
                  case '&':
                  case '|':
                  case '(':
                  case ')':
                  default:
                     // do nothing
                     ;
                     break;

                  case 'd':
                  case 'f':
                  case 'n':
                  case 'r':
                  case 's':
                  case 't':
                     opcode = *s++;
                     x = (char *)&index;
                     *x++ = *s++;
                     *x   = *s;
                     if(index == jobid)
                     {
                        // copy the job into the deplist
                        memcpy(&deplist->item[deplist->nitems], &ctx->job[id], sizeof(JOB));
                        deplist->nitems++;


                        // set the opcode field to the type of dependency
                        deplist->item[deplist->nitems].opcode = opcode;

                        // set flag to exit loop
                        exit_loop = OTTO_TRUE;
                     }
                     break;
               }

               s++;
            }
         }
      }
   }
}



int
ottojob_reduce_list(JOBLIST *joblist, char *name)
{
   int retval = OTTO_SUCCESS;
   JOBLIST tmplist;
   int i, n;

   if(joblist == NULL || joblist->nitems <= 0)
      retval = OTTO_FAIL;

   tmplist.nitems = 0;
   if(retval == OTTO_SUCCESS)
   {
      if((tmplist.item = (JOB *)calloc(joblist->nitems, sizeof(JOB))) == NULL)
         retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS)
   {
      // find the jobname specified on the command line
      for(i=0; i<=joblist->nitems; i++)
      {
         if(strcmp(joblist->item[i].name, name) == 0)
            break;
      }
      if(i == joblist->nitems)
         retval = OTTO_FAIL;
   }

   if(retval == OTTO_SUCCESS)
   {
      // add the item to the tmplist
      memcpy(&tmplist.item[tmplist.nitems++], &joblist->item[i], sizeof(JOB));

      // loop through remaining jobs adding them if appropriate
      for(n=i+1; n<=joblist->nitems; n++)
      {
         if(joblist->item[n].level <= joblist->item[i].level)
            break;
         memcpy(&tmplist.item[tmplist.nitems++], &joblist->item[n], sizeof(JOB));
      }
   }

   // copy the tmplist back to the joblist
   if(tmplist.item != NULL)
   {
      memcpy(joblist->item, tmplist.item, sizeof(JOB)*joblist->nitems);
      free(tmplist.item);
   }
   else
   {
      memset(joblist->item, 0, sizeof(JOB)*joblist->nitems);
   }
   joblist->nitems = tmplist.nitems;

   return(retval);
}



void
ottojob_prepare_txt_values(JOBTVAL *tval, JOB *item, int format)
{
   time_t     tmp_time_t, now, duration;
   struct tm *tm_time;
   int        i, j, one_printed;
   int16_t    index;
   char      *x, *s, *t;
   ecnd_st    c;
   int        value, tlen;
   char       *dstr="", rstr[3], *sstr = NULL;

   memset(tval, 0, sizeof(JOBTVAL));

   now = time(0);

   otto_sprintf(tval->id,              "%d", item->id);
   otto_sprintf(tval->level,           "%d", item->level);
   otto_sprintf(tval->linkage,         "box %d, head %d, tail %d, prev %d, next %d", item->box, item->head, item->tail, item->prev, item->next);
   otto_sprintf(tval->name,            "%s", item->name);
   otto_sprintf(tval->type,            "%c", item->type);
   otto_sprintf(tval->box_name,        "%s", item->box_name);
   
   s = item->description;
   t = tval->description;
   tlen = 0;
   while(*s != '\0' && tlen <= 256)
   {
      if(*s == '"')
      {
         *t++ = '\\';
         tlen++;
      }
      *t++ = *s++;
      *t = '\0';
      tlen++;
   }
   if(format == AS_MSPDI && tlen > 256)
   {
      tval->description[252] = '.';
      tval->description[253] = '.';
      tval->description[254] = '.';
      tval->description[255] = '\0';
   }


   s = item->environment;
   t = tval->environment;
   tlen = 0;
   while(*s != '\0' && tlen <= 256)
   {
      *t++ = *s++;
      *t = '\0';
      tlen++;
   }
   if(format == AS_MSPDI && tlen > 256)
   {
      tval->environment[252] = '.';
      tval->environment[253] = '.';
      tval->environment[254] = '.';
      tval->environment[255] = '\0';
   }



   otto_sprintf(tval->command,         "%s", item->command);
   otto_sprintf(tval->condition,       "%s", item->condition);
   otto_sprintf(tval->date_conditions, "%s", item->date_conditions == 0 ? "0" : "1");
   if(item->days_of_week == OTTO_ALL)
   {
      strcpy(tval->days_of_week, "all");
   }
   else
   {
      one_printed = OTTO_FALSE;
      for(i=0; i<7; i++)
      {
         if(item->days_of_week & (1L << i))
         {
            if(one_printed == OTTO_TRUE)
               strcat(tval->days_of_week, ",");
            switch((1L << i))
            {
               case OTTO_SUN: strcat(tval->days_of_week, "su"); break;
               case OTTO_MON: strcat(tval->days_of_week, "mo"); break;
               case OTTO_TUE: strcat(tval->days_of_week, "tu"); break;
               case OTTO_WED: strcat(tval->days_of_week, "we"); break;
               case OTTO_THU: strcat(tval->days_of_week, "th"); break;
               case OTTO_FRI: strcat(tval->days_of_week, "fr"); break;
               case OTTO_SAT: strcat(tval->days_of_week, "sa"); break;
            }
            one_printed = OTTO_TRUE;
         }
      }
   }

   switch(item->date_conditions)
   {
      case OTTO_USE_START_MINUTES:
         one_printed = OTTO_FALSE;
         for(i=0; i<60; i++)
         {
            if(item->start_minutes & (1L << i))
            {
               if(one_printed == OTTO_TRUE)
                  strcat(tval->start_minutes, ",");
               otto_sprintf(&tval->start_minutes[strlen(tval->start_minutes)], "%d", i);
               one_printed = OTTO_TRUE;
            }
         }
         break;
      case OTTO_USE_START_TIMES:
         strcpy(tval->start_times, "\"");
         one_printed = OTTO_FALSE;
         for(i=0; i<24; i++)
         {
            for(j=0; j<60; j++)
            {
               if(item->start_times[i] & (1L << j))
               {
                  if(one_printed == OTTO_TRUE)
                     strcat(tval->start_times, ",");
                  otto_sprintf(&tval->start_times[strlen(tval->start_times)], "%d:%02d", i, j);
                  one_printed = OTTO_TRUE;
               }
            }
         }
         strcat(tval->start_times, "\"");
         break;
   }

   if(item->condition[0] != '\0')
   {
      s          = item->expression;
      t          = tval->expression;
      while(*s != '\0')
      {
         switch(*s)
         {
            case '&':
            case '|':
            case '(':
            case ')':
            default:
               *t++ = *s;
               *t   = '\0';
               break;

            case 'd':
            case 'f':
            case 'n':
            case 'r':
            case 's':
            case 't':
               *t++ = *s++;
               *t   = '\0';
               x = (char *)&index;
               *x++ = *s++;
               *x   = *s;
               if(index < 0)
                  strcpy(t, "----");
               else
                  otto_sprintf(t, "%04d", index);
               t += 4 ;
               *t = '\0';
               break;
         }

         s++;
      }

      s          = item->expression;
      t          = tval->expression2;
      while(*s != '\0')
      {
         switch(*s)
         {
            case '&':
            case '|':
            case '(':
            case ')':
            default:
               *t++ = *s;
               *t   = '\0';
               break;

            case 'd':
            case 'f':
            case 'n':
            case 'r':
            case 's':
            case 't':
               c.s = s;
               value = evaluate_stat(&c);
               if(value != OTTO_FAIL)
                  otto_sprintf(t, " (%d) ", value);
               else
                  otto_sprintf(t, " (-) ");
               s += 2;
               t += 5 ;
               *t = '\0';
               break;
         }

         s++;
      }


      otto_sprintf(tval->expr_fail, "%d", item->expr_fail);
   }

   if(item->on_noexec == OTTO_FALSE)
   {
      sstr = "SU";
   }
   else
   {
      sstr = "NE";
      dstr = "onnoexec";
   }

   if(item->on_autohold == OTTO_TRUE)
   {
      dstr = "autohold";
   }

   if(item->expr_fail == OTTO_TRUE)
   {
      dstr = "exprfail";
   }

   strcpy(rstr, "RU");
   if(item->loopname[0] != '\0')
   {
      if(item->loopnum < item->loopmin || item->loopnum > item->loopmax)
      {
         rstr[0] = '-';
         rstr[1] = '-';
      }
      else
      {
         rstr[0] = '0' + (item->loopnum * item->loopsgn)/10;
         rstr[1] = '0' + (item->loopnum * item->loopsgn)%10;
      }
   }

   switch(item->status)
   {
      case STAT_IN: strcpy(tval->status, "IN"); break;
      case STAT_AC: strcpy(tval->status, "AC"); break;
      case STAT_RU: strcpy(tval->status, rstr); break;
      case STAT_SU: strcpy(tval->status, sstr); break;
      case STAT_FA: strcpy(tval->status, "FA"); break;
      case STAT_TE: strcpy(tval->status, "TE"); break;
      case STAT_OH: strcpy(tval->status, "OH"); break;
      case STAT_BR: strcpy(tval->status, "BR"); break;
      default:      strcpy(tval->status, "--"); break;
   }

   otto_sprintf(tval->autohold,    "%d",  item->autohold);
   otto_sprintf(tval->on_autohold, "%d",  item->on_autohold);
   otto_sprintf(tval->autonoexec,  "%d",  item->autonoexec);
   otto_sprintf(tval->on_noexec,   "%d",  item->on_noexec);
   if(item->type != OTTO_BOX)
      otto_sprintf(tval->pid,      "%d",  item->pid);
   

   if(item->start == 0)
   {
      strcpy(tval->start, "--------");
   }
   else
   {
      tmp_time_t = item->start;
      tm_time = localtime(&tmp_time_t);
      strftime(tval->start, sizeof(tval->start), "%m/%d/%Y %T", tm_time);
   }

   if(item->finish == 0)
   {
      strcpy(tval->finish, "--------");
   }
   else
   {
      tmp_time_t = item->finish;
      tm_time = localtime(&tmp_time_t);
      strftime(tval->finish, sizeof(tval->finish), "%m/%d/%Y %T", tm_time);
   }

   if(item->start == 0 || item->finish == 0)
   {
      if(item->status == STAT_RU || dstr[0] == '\0')
      {
         if(item->start != 0 && cfg.show_sofar == OTTO_TRUE)
         {
            duration = now - item->start;
            otto_sprintf(tval->duration, "%02ld:%02ld:%02ld", duration / 3600, duration / 60 % 60, duration % 60);
         }
         else
         {
            strcpy(tval->duration, "--------");
         }
      }
      else
         strcpy(tval->duration, dstr);

   }
   else
   {
      duration = item->finish - item->start;
      if(duration >= 0)
      {
         otto_sprintf(tval->duration, "%02ld:%02ld:%02ld", duration / 3600, duration / 60 % 60, duration % 60);
      }
      else
      {
         strcpy(tval->duration, "--------");
      }
   }

   otto_sprintf(tval->exit_status, "%d",  item->exit_status);

   otto_sprintf(tval->loopname, "%s", item->loopname);
   otto_sprintf(tval->loopnum, "%d", (int)(item->loopnum * item->loopsgn));
   otto_sprintf(tval->loopmin, "%d", (int)(item->loopmin * item->loopsgn));
   otto_sprintf(tval->loopmax, "%d", (int)(item->loopmax * item->loopsgn));
   if(item->type == 'b')
   {
      if(item->loopname[0] != '\0')
      {
         switch(item->loopstat)
         {
            case LOOP_NOT_RUNNING:
               strcpy(tval->loopstat, "Not Running");
               break;
            case LOOP_RUNNING:
               strcpy(tval->loopstat, "Running");
               break;
            case LOOP_BROKEN:
               strcpy(tval->loopstat, "Broken");
               break;
            default:
               strcpy(tval->loopstat, "Unknown");
               break;
         }
      }
      else
      {
         strcpy(tval->loopstat, "Not configured to loop");
      }
   }
   else
   {
      strcpy(tval->loopstat, "Unable to loop");
   }

   switch(format)
   {
      case AS_CSV:
         mime_quote(tval->description, sizeof(tval->description));
         mime_quote(tval->environment, sizeof(tval->environment));
         mime_quote(tval->command,     sizeof(tval->command));
         mime_quote(tval->condition,   sizeof(tval->condition));
         mime_quote(tval->start_times, sizeof(tval->start_times));
         mime_quote(tval->expression,  sizeof(tval->expression));
         mime_quote(tval->expression2, sizeof(tval->expression2));
         break;

      case AS_HTML:
         escape_html(tval->description, sizeof(tval->description));
         escape_html(tval->environment, sizeof(tval->environment));
         escape_html(tval->command,     sizeof(tval->command));
         escape_html(tval->condition,   sizeof(tval->condition));
         escape_html(tval->start_times, sizeof(tval->start_times));
         escape_html(tval->expression,  sizeof(tval->expression));
         escape_html(tval->expression2, sizeof(tval->expression2));
         break;

      case AS_MSPDI:
         escape_mspdi(tval->description, sizeof(tval->description));
         escape_mspdi(tval->environment, sizeof(tval->environment));
         escape_mspdi(tval->command,     sizeof(tval->command));
         escape_mspdi(tval->condition,   sizeof(tval->condition));
         escape_mspdi(tval->start_times, sizeof(tval->start_times));
         escape_mspdi(tval->expression,  sizeof(tval->expression));
         escape_mspdi(tval->expression2, sizeof(tval->expression2));
         break;
   }

}



int
ottojob_print_name_errors(int error_mask, char *action, char *name, char *kind)
{
   int retval = 0;

   if(error_mask & OTTO_INVALID_NAME_CHARS)
   {
      fprintf(stderr, "ERROR for %s: %s < %sname contains one or more invalid characters or symbols >.\n", action, name, kind);
      retval++;
   }
   if(error_mask & OTTO_INVALID_NAME_LENGTH)
   {
      fprintf(stderr, "ERROR for %s: %s < %sname exceeds allowed length (%d bytes) >.\n", action, name, kind, NAMLEN);
      retval++;
   }

   return(retval);
}



int
ottojob_print_type_errors(int error_mask, char *action, char *name, char *type)
{
   int retval = 0;

   if(error_mask & OTTO_INVALID_VALUE)
   {
      fprintf(stderr, "ERROR for %s: %s < invalid value \"%c\" for keyword: \"job_type\" >.\n", action, name, type[0]);
      retval++;
   }

   return(retval);
}



int
ottojob_print_box_name_errors(int error_mask, char *action, char *name, char *box_name)
{
   int retval = 0;

   if(error_mask & OTTO_INVALID_NAME_CHARS)
   {
      fprintf(stderr, "ERROR for %s: %s < box_name %s contains one or more invalid characters or symbols >.\n", action, name, box_name);
      retval++;
   }
   if(error_mask & OTTO_INVALID_NAME_LENGTH)
   {
      fprintf(stderr, "ERROR for %s: %s < box_name %s exceeds allowed length (%d bytes) >.\n", action, name, box_name, NAMLEN);
      retval++;
   }
   if(error_mask & OTTO_SAME_JOB_BOX_NAMES)
   {
      fprintf(stderr, "ERROR for %s: %s < job_name and box_name cannot be same >.\n", action, name);
      retval++;
   }

   return(retval);
}



int
ottojob_print_command_errors(int error_mask, char *action, char *name, int outlen)
{
   int retval = 0;

   if(error_mask & OTTO_MISSING_REQUIRED_VALUE)
   {
      fprintf(stderr, "ERROR for %s: %s < required \"command\" is missing >.\n", action, name);
      retval++;
   }
   if(error_mask & OTTO_LENGTH_EXCEEDED)
   {
      fprintf(stderr, "ERROR for %s: %s < command exceeds allowed length (%d bytes) >.\n", action, name, outlen);
      retval++;
   }

   return(retval);
}



int
ottojob_print_condition_errors(int error_mask, char *action, char *name, int outlen)
{
   int retval = 0;

   if(error_mask & OTTO_CONDITION_BUFFER_EXCEEDED)
   {
      fprintf(stderr, "ERROR for %s: %s <  condition exceeds validation buffer (%d bytes) >.\n", action, name, VCNDLEN);
      retval++;
   }
   if(error_mask & OTTO_CONDITION_UNBALANCED)
   {
      fprintf(stderr, "ERROR for %s: %s <  unbalanced parentheses in condition >.\n", action, name);
      retval++;
   }
   if(error_mask & OTTO_LENGTH_EXCEEDED)
   {
      fprintf(stderr, "ERROR for %s: %s <  condition exceeds allowed length (%d bytes) >.\n", action, name, outlen);
      retval++;
   }
   if(error_mask & OTTO_CONDITION_ONE_OPERAND)
   {
      fprintf(stderr, "ERROR for %s: %s < logical operator in condition must have 2 operands >.\n", action, name);
      retval++;
   }
   if(error_mask & OTTO_CONDITION_MISSING_JOBNAME)
   {
      fprintf(stderr, "ERROR for %s: %s < missing job_name in condition >.\n", action, name);
      retval++;
   }
   if(error_mask & OTTO_INVALID_NAME_CHARS)
   {
      fprintf(stderr, "ERROR for %s: %s < invalid characters for job_name in condition >.\n", action, name);
      retval++;
   }
   if(error_mask & OTTO_INVALID_NAME_LENGTH)
   {
      fprintf(stderr, "ERROR for %s: %s < job_name in condition exceeds allowed length (%d bytes) >.\n", action, name, NAMLEN);
      retval++;
   }
   if(error_mask & OTTO_CONDITION_NEEDS_PARENS)
   {
      fprintf(stderr, "ERROR for %s: %s < using both ANDs and ORs in condition requires the use of parenthesis >.\n", action, name);
      retval++;
   }
   if(error_mask & OTTO_CONDITION_INVALID_KEYWORD)
   {
      fprintf(stderr, "ERROR for %s: %s < invalid keyword in condition >.\n", action, name);
      retval++;
   }

   return(retval);
}



int
ottojob_print_description_errors(int error_mask, char *action, char *name, int outlen)
{
   int retval = 0;

   if(error_mask & OTTO_LENGTH_EXCEEDED)
   {
      fprintf(stderr, "ERROR for %s: %s < description exceeds allowed length (%d bytes) >.\n", action, name, outlen);
      retval++;
   }

   return(retval);
}



int
ottojob_print_environment_errors(int error_mask, char *action, char *name, int outlen)
{
   int retval = 0;

   if(error_mask & OTTO_MALFORMED_ENV_ASSIGNMENT)
   {
      fprintf(stderr, "ERROR for %s: %s < environment contains a malformed assignment >.\n", action, name);
      retval++;
   }
   if(error_mask & OTTO_INVALID_ENVNAME_FIRSTCHAR)
   {
      fprintf(stderr, "ERROR for %s: %s < environment variable name begins with an invalid character >.\n", action, name);
      retval++;
   }
   if(error_mask & OTTO_INVALID_ENVNAME_CHARS)
   {
      fprintf(stderr, "ERROR for %s: %s < environment variable name contains one or more invalid characters >.\n", action, name);
      retval++;
   }
   if(error_mask & OTTO_INVALID_OVERRIDE)
   {
      fprintf(stderr, "ERROR for %s: %s < environment attempts to override one of OTTO_JOBNAME|HOME|USER|LOGNAME >.\n", action, name);
      retval++;
   }
   if(error_mask & OTTO_LENGTH_EXCEEDED)
   {
      fprintf(stderr, "ERROR for %s: %s < environment exceeds allowed length (%d bytes) >.\n", action, name, outlen);
      retval++;
   }

   return(retval);
}



int
ottojob_print_auto_hold_errors(int error_mask, char *action, char *name, char *auto_hold)
{
   int retval = 0;

   if(error_mask & OTTO_INVALID_VALUE)
   {
      fprintf(stderr, "ERROR for %s: %s < invalid value \"%c\" for keyword: \"auto_hold\" >.\n", action, name, auto_hold[0]);
      retval++;
   }

   return(retval);
}



int
ottojob_print_auto_noexec_errors(int error_mask, char *action, char *name, char *auto_noexec)
{
   int retval = 0;

   if(error_mask & OTTO_INVALID_VALUE)
   {
      fprintf(stderr, "ERROR for %s: %s < invalid value \"%c\" for keyword: \"auto_noexec\" >.\n", action, name, auto_noexec[0]);
      retval++;
   }

   return(retval);
}



int
ottojob_print_date_conditions_errors(int error_mask, char *action, char *name, char *date_conditions)
{
   int retval = 0;

   if(error_mask & OTTO_INVALID_VALUE)
   {
      fprintf(stderr, "ERROR for %s: %s < invalid value \"%c\" for keyword: \"date_conditions\" >.\n", action, name, date_conditions[0]);
      retval++;
   }
   if(error_mask & OTTO_INVALID_APPLICATION)
   {
      fprintf(stderr, "ERROR for %s: %s < invalid application of date conditions >.\n", action, name);
      fprintf(stderr, "Otto only allows date conditions on objects at level 0. (i.e. no parent box).\n");
      retval++;
   }
   if(error_mask & OTTO_INVALID_COMBINATION)
   {
      fprintf(stderr, "ERROR for %s: %s < invalid combination of date conditions >.\n", action, name);
      fprintf(stderr, "You must specify date_conditions, days_of_week, and either start_mins or start_times.\n");
      retval++;
   }

   return(retval);
}



int
ottojob_print_days_of_week_errors(int error_mask, char *action, char *name)
{
   int retval = 0;

   if(error_mask & OTTO_INVALID_VALUE)
   {
      fprintf(stderr, "ERROR for %s: %s < invalid value in days_of_week attribute. > Value must be in [mo|tu|we|th|fr|sa|su|all].\n", action, name);
      retval++;
   }

   return(retval);
}



int
ottojob_print_start_mins_errors(int error_mask, char *action, char *name)
{
   int retval = 0;

   if(error_mask & OTTO_INVALID_VALUE)
   {
      fprintf(stderr, "ERROR for %s: %s < invalid value for start_mins attribute >.Value must be in [0-59].\n", action, name);
      retval++;
   }

   return(retval);
}



int
ottojob_print_start_times_errors(int error_mask, char *action, char *name)
{
   int retval = 0;

   if(error_mask & OTTO_INVALID_VALUE)
   {
      fprintf(stderr, "ERROR for %s: %s < invalid value in start_times attribute >. Value must be in 24 hour HH:MM format.\n", action, name);
      retval++;
   }

   return(retval);
}



int
ottojob_print_loop_errors(int error_mask, char *action, char *name)
{
   int retval = 0;

   if(error_mask & OTTO_INVALID_ENVNAME_FIRSTCHAR)
   {
      fprintf(stderr, "ERROR for %s: %s < loop variable name begins with a digit >.\n", action, name);
      retval++;
   }
   if(error_mask & OTTO_INVALID_ENVNAME_CHARS)
   {
      fprintf(stderr, "ERROR for %s: %s < loop variable name contains one or more invalid characters >.\n", action, name);
      retval++;
   }
   if(error_mask & OTTO_INVALID_NAME_LENGTH)
   {
      fprintf(stderr, "ERROR for %s: %s < loop variable name exceeds allowed length (%d bytes) >.\n", action, name, VARLEN);
      retval++;
   }
   if(error_mask & OTTO_INVALID_MINNUM_CHARS)
   {
      fprintf(stderr, "ERROR for %s: %s < loop starting value contains one or more non-numeric characters >.\n", action, name);
      retval++;
   }
   if(error_mask & OTTO_INVALID_LOOPMIN_LEN)
   {
      fprintf(stderr, "ERROR for %s: %s < loop starting value must be between 0 and 99 >.\n", action, name);
      retval++;
   }
   if(error_mask & OTTO_INVALID_MAXNUM_CHARS)
   {
      fprintf(stderr, "ERROR for %s: %s < loop ending value contains one or more non-numeric characters >.\n", action, name);
      retval++;
   }
   if(error_mask & OTTO_INVALID_LOOPMAX_LEN)
   {
      fprintf(stderr, "ERROR for %s: %s < loop ending value must be between 0 and 99 >.\n", action, name);
      retval++;
   }
   if(error_mask & OTTO_MALFORMED_LOOP)
   {
      fprintf(stderr, "ERROR for %s: %s < loop expression contains errors >.\n", action, name);
      retval++;
   }

   return(retval);
}



int
ottojob_print_new_name_errors(int error_mask, char *action, char *name, char *new_name)
{
   int retval = 0;

   if(error_mask & OTTO_INVALID_NAME_CHARS)
   {
      fprintf(stderr, "ERROR for %s: %s < new_name %s contains one or more invalid characters or symbols >.\n", action, name, new_name);
      retval++;
   }
   if(error_mask & OTTO_INVALID_NAME_LENGTH)
   {
      fprintf(stderr, "ERROR for %s: %s < new_name %s exceeds allowed length (%d bytes) >.\n", action, name, new_name, NAMLEN);
      retval++;
   }
   if(error_mask & OTTO_SAME_NAME)
   {
      fprintf(stderr, "ERROR for %s: %s < job_name and new_name cannot be same >.\n", action, name);
      retval++;
   }
   if(error_mask & OTTO_SAME_JOB_BOX_NAMES)
   {
      fprintf(stderr, "ERROR for %s: %s < box_name and new_name cannot be same >.\n", action, name);
      retval++;
   }

   return(retval);
}



int
ottojob_print_start_errors(int error_mask, char *action, char *name)
{
   int retval = 0;

   if(error_mask & OTTO_INVALID_VALUE)
   {
      fprintf(stderr, "ERROR for %s: %s < start attribute contains errors >.\n", action, name);
      retval++;
   }

   return(retval);
}



int
ottojob_print_finish_errors(int error_mask, char *action, char *name)
{
   int retval = 0;

   if(error_mask & OTTO_INVALID_VALUE)
   {
      fprintf(stderr, "ERROR for %s: %s < finish attribute contains errors >.\n", action, name);
      retval++;
   }

   return(retval);
}



int
ottojob_print_task_name_errors(int error_mask, char *action, char *name, int task1, int task2)
{
   int retval = 0;

   if(error_mask & OTTO_SAME_NAME)
   {
      fprintf(stderr, "ERROR for %s: %s < name is duplicated between tasks %d and %d >.\n", action, name, task1, task2);

      retval++;
   }

   return(retval);
}





