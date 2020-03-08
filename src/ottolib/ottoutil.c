#include <ctype.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


#include "ottobits.h"
#include "ottocfg.h"
#include "ottoutil.h"
#include "simplelog.h"

extern SIMPLELOG *logp;

#define cdeAMP_____     ((i64)'A'<<56 | (i64)'M'<<48 | (i64)'P'<<40 | (i64)';'<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeAPOS____     ((i64)'A'<<56 | (i64)'P'<<48 | (i64)'O'<<40 | (i64)'S'<<32 | ';'<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeGT______     ((i64)'G'<<56 | (i64)'T'<<48 | (i64)';'<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeLT______     ((i64)'L'<<56 | (i64)'T'<<48 | (i64)';'<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeQUOT____     ((i64)'Q'<<56 | (i64)'U'<<48 | (i64)'O'<<40 | (i64)'T'<<32 | ';'<<24 | ' '<<16 | ' '<<8 | ' ')

void
output_xml(char *s)
{
	while(*s != '\0')
	{
		switch(*s)
		{
			case '<':  printf("&lt;");   break;
			case '>':  printf("&gt;");   break;
			case '&':  printf("&amp;");  break;
			case '"':  printf("&quot;"); break;
			case '\'': printf("&apos;"); break;
			default:   printf("%c", *s); break;
		}

		s++;
	}
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



char *
format_timestamp(time_t t)
{
	struct tm *now;
	static char timestamp[128];

	now = localtime(&t);
	sprintf(timestamp, "%4d-%02d-%02dT%02d:%02d:%02d",
			(now->tm_year + 1900), (now->tm_mon + 1), now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);

	return(timestamp);
}



char *
format_duration(time_t t)
{
	int32_t seconds = (int32_t)t;
	int32_t hours;
	int32_t minutes;
	static char duration[128];

	// adjust duration to minimum granularity of MS Project (1/10th of a minute)
	if(seconds < 6)
		seconds = 6;

	// convert seconds to minutes
	minutes = seconds/60;

	// convert minutes to hours
	hours = minutes/60;

	// normalize minutes and seconds
	minutes %= 60;
	seconds %= 60;

	// create the duration string
	sprintf(duration, "PT%dH%dM%dS", hours, minutes, seconds);

	return(duration);
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
read_stdin(DYNBUF *b, char *prompt)
{
	int retval = OTTO_SUCCESS;
   int nread;
	char buffer[8192];
	int display_prompt = OTTO_FALSE;

	if(b != NULL)
	{
		memset(b, 0, sizeof(DYNBUF));

		if(isatty(STDIN_FILENO))
		{
			display_prompt = OTTO_TRUE;
		}

		if(display_prompt == OTTO_TRUE)
		{
			printf("%s> ", prompt);
			fflush(stdout);
		}

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
			if(display_prompt == OTTO_TRUE)
			{
				printf("%s> ", prompt);
				fflush(stdout);
			}
		}

		b->s = b->buffer;

		if(display_prompt == OTTO_TRUE)
		{
			printf("\n");
			fflush(stdout);
		}

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
remove_jil_comments(DYNBUF *b)
{
	char *s, last_char;

	// remove comments from buffer
	last_char = '\n';

	s = b->buffer;
	while(*s != '\0')
	{
		if(last_char == '\n')
		{
			while(isspace(*s) && *s != '\n' && *s != '\0')
				s++;
		}

		switch(*s)
		{
			case '#':
				if(last_char == '\n')
				{
					// convert line to spaces
					while(*s != '\0' && *s != '\n')
						*s++ = ' ';
				}
				break;
#ifdef STRING_LITERALS
			case '"':
				// read string literal
				c++;
				while((*c = fgetc(fp)) != EOF)
				{
					if(*c == '"' && *(c-1) != '\\')
						break;
					c++;
				}
				break;
#endif
			case '*':
				if(last_char == '/')
				{
					// convert comment block to spaces
					*(s-1) = ' ';
					*s++   = ' ';
					while(*s != '\0')
					{
						if(*s == '*' && *(s+1) == '/')
						{
							*s++ = ' ';
							*s   = ' ';
							break;
						}
						if(*s != '\n')
							*s = ' ';
						s++;
					}
				}
				break;
		}

		last_char = *s;
		s++;

	}
}



void
advance_word(DYNBUF *b)
{
	// consume non-space
	while(!isspace(*(b->s)) && *(b->s) != '\0')
		b->s++;

	// consume space
	while(isspace(*(b->s)) && *(b->s) != '\0')
	{
		if(*(b->s) == '\n')
			b->line++;
		b->s++;
	}
}



void
advance_jilword(DYNBUF *b)
{
	// consume non-space
	while(!isspace(*(b->s)) && *(b->s) != ':' && *(b->s) != '\0')
		b->s++;

	// consume space
	while((isspace(*(b->s)) || *(b->s) == ':') && *(b->s) != '\0')
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



//
// vim: ts=3 sw=3 ai
//
