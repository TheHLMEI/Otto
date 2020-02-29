/*------------------ job structure copy and validation functions ------------------*/
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ottobits.h"
#include "ottocfg.h"
#include "ottodb.h"
#include "ottolog.h"
#include "ottojob.h"
#include "ottoutil.h"



#define OTTO_TERM        0
#define OTTO_CONJUNCTION 1

#define VCNDLEN             4096

#define i64 int64_t
#define cdeSU______     ((i64)'S'<<56 | (i64)'U'<<48 | (i64)' '<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeMO______     ((i64)'M'<<56 | (i64)'O'<<48 | (i64)' '<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeTU______     ((i64)'T'<<56 | (i64)'U'<<48 | (i64)' '<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeWE______     ((i64)'W'<<56 | (i64)'E'<<48 | (i64)' '<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeTH______     ((i64)'T'<<56 | (i64)'H'<<48 | (i64)' '<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeFR______     ((i64)'F'<<56 | (i64)'R'<<48 | (i64)' '<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeSA______     ((i64)'S'<<56 | (i64)'A'<<48 | (i64)' '<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeALL_____     ((i64)'A'<<56 | (i64)'L'<<48 | (i64)'L'<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')



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
				lprintf(MAJR, "Command exceeds allowed length (%d bytes).\n", outlen);
				retval = OTTO_FAIL;
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
				lprintf(MAJR, "CONDITION ERROR: Condition exceeds validation buffer (%d bytes).\n", VCNDLEN);
				return(OTTO_FAIL);
			}
		}
		*p = '\0';

		if(paren_count != 0)
		{
			lprintf(MAJR, "CONDITION ERROR: Unbalanced parentheses in logical expression.\n");
			return(OTTO_FAIL);
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
				lprintf(MAJR, "CONDITION ERROR: Condition exceeds allowed length (%d bytes).\n", outlen);
				retval = OTTO_FAIL;
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
					lprintf(MAJR, "DAYS_OF_WEEK ERROR: Unrecognized value (%s).  Value must be in [mo|tu|we|th|fr|sa|su|all].\n", word);
					retval = OTTO_FAIL;
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
		*t++ = *s++;
		l--;

		// now get up to the next quote
		while(*s != '"' && *s != '\0')
		{
			if(l > 0)
			{
				*t++ = *s++;
				l--;
			}
			else
			{
				lprintf(MAJR, "Description exceeds allowed length (%d bytes).\n", outlen);
				retval = OTTO_FAIL;
			}
		}
		// add the quote
		if(l > 0)
		{
			*t++ = *s++;
			l--;
		}
		else
		{
			lprintf(MAJR, "Description exceeds allowed length (%d bytes).\n", outlen);
			retval = OTTO_FAIL;
		}

		*t = '\0';
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
ottojob_copy_start_mins(int64_t *output, char *start_mins)
{
	int retval = OTTO_SUCCESS;
	char *s, *t;
	char word[8128];
	int exit_loop = OTTO_FALSE;
	int comma_found = OTTO_FALSE;
	int min = 0, time_is_valid = OTTO_FALSE;

	s = start_mins;
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
				lprintf(MAJR, "START_MINS ERROR: Unrecognized value (%s).  Value must be in [0-59].\n", word);
				retval = OTTO_FAIL;
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
				lprintf(MAJR, "START_TIMES ERROR: Unrecognized value (%s).  Value must be in 24 hour HH:MM format.\n", word);
				retval = OTTO_FAIL;
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

		// default to 'c'
		*t = 'c';

		if(s != NULL && t != NULL && outlen >= 1)
		{
			*t = tolower(*s);

			if(*t != 'b' && *t != 'c')
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
	int  exit_loop  = OTTO_FALSE;
	int  last       = OTTO_CONJUNCTION;
	char tmpstr[VCNDLEN];

	if(*(c->s) == '(')
	{
		*(c->t)++ = '(';
		c->s++;
		open_paren = OTTO_TRUE;
	}

	while(*(c->s) != '\0' && exit_loop == OTTO_FALSE)
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
						lprintf(MAJR, "CONDITION ERROR: Unbalanced parentheses in logical expression.\n");
						retval = OTTO_FAIL;
					}
					exit_loop = OTTO_TRUE;
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
						lprintf(MAJR, "CONDITION ERROR: Logical operator must have 2 operands.\n");
						retval = OTTO_FAIL;
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
						lprintf(MAJR, "CONDITION ERROR: Logical operator must have 2 operands.\n");
						retval = OTTO_FAIL;
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
							lprintf(MAJR, "CONDITION ERROR: Logical operator must have 2 operands.\n");
							retval = OTTO_FAIL;
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
							lprintf(MAJR, "CONDITION ERROR: Logical operator must have 2 operands.\n");
							retval = OTTO_FAIL;
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
								*(c->t)++ = tolower(*(c->s));;
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
										lprintf(MAJR, "CONDITION ERROR: Invalid job_name: < job_name is missing >.\n");
										retval = OTTO_FAIL;
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
												lprintf(MAJR, "CONDITION ERROR: Invalid job_name: < job_name contains one or more invalid characters or symbols >.\n");
												retval = OTTO_FAIL;
												break;
											}
										}

										if(namelen > NAMLEN)
										{
											lprintf(MAJR, "CONDITION ERROR: Invalid job_name: < job_name exceeds allowed length (%d bytes) >.\n", NAMLEN);
											retval = OTTO_FAIL;
										}

										while(*(c->s) != '\0' && isspace(*(c->s)))
											c->s++;

										if(*(c->s) == ')')
										{
											*(c->t)++ = ')';
										}
										else
										{
											lprintf(MAJR, "CONDITION ERROR: Invalid job_name: < job_name contains one or more invalid characters or symbols >.\n");
											retval = OTTO_FAIL;
										}
									}
								}
								last = OTTO_TERM;
							}
							else
							{
								lprintf(MAJR, "CONDITION ERROR: Logical operator must have 2 operands.\n");
								retval = OTTO_FAIL;
							}
							break;
						default:
							ottojob_copy_condition_word(tmpstr, c->s, sizeof(tmpstr));
							lprintf(MAJR, "CONDITION ERROR: Invalid key word: %s.\n", tmpstr);
							retval = OTTO_FAIL;
							break;
					}
					break;

				default:
					ottojob_copy_condition_word(tmpstr, c->s, sizeof(tmpstr));
					lprintf(MAJR, "CONDITION ERROR: Invalid key word: %s.\n", tmpstr);
					retval = OTTO_FAIL;
					break;
			}
		}

		c->s++;
		if(retval == OTTO_FAIL)
			exit_loop = OTTO_TRUE;
	}
	*(c->t) = '\0';

	if(and_count > 0 && or_count > 0)
	{
		lprintf(MAJR, "CONDITION ERROR: Using both ANDs and ORs requires the use of parenthesis.\n");
		retval = OTTO_FAIL;
	}

	if(last == OTTO_CONJUNCTION && strlen(c->output) != 0)
	{
		lprintf(MAJR, "CONDITION ERROR: Logical operator must have 2 operands.\n");
		retval = OTTO_FAIL;
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

	lsprintf(INFO, "Job structure layout:\n");
	lsprintf(CATI, "id              at %4d for %4d type integer(4)  notnull\n", offsetof(JOB, id),              sizeof(j.id));
	lsprintf(CATI, "parent          at %4d for %4d type integer(4)  notnull\n", offsetof(JOB, parent),          sizeof(j.parent));
	lsprintf(CATI, "head            at %4d for %4d type integer(4)  notnull\n", offsetof(JOB, head),            sizeof(j.head));
	lsprintf(CATI, "tail            at %4d for %4d type integer(4)  notnull\n", offsetof(JOB, tail),            sizeof(j.tail));
	lsprintf(CATI, "prev            at %4d for %4d type integer(4)  notnull\n", offsetof(JOB, prev),            sizeof(j.prev));
	lsprintf(CATI, "next            at %4d for %4d type integer(4)  notnull\n", offsetof(JOB, next),            sizeof(j.next));
	lsprintf(CATI, "level           at %4d for %4d type integer(4)  notnull\n", offsetof(JOB, level),           sizeof(j.level));
	lsprintf(CATI, "name            at %4d for %4d type char        notnull\n", offsetof(JOB, name),            sizeof(j.name));
	lsprintf(CATI, "type            at %4d for %4d type char        notnull\n", offsetof(JOB, type),            sizeof(j.type));
	lsprintf(CATI, "box_name        at %4d for %4d type char        notnull\n", offsetof(JOB, box_name),        sizeof(j.box_name));
	lsprintf(CATI, "description     at %4d for %4d type char        notnull\n", offsetof(JOB, description),     sizeof(j.description));
	lsprintf(CATI, "command         at %4d for %4d type char        notnull\n", offsetof(JOB, command),         sizeof(j.command));
	lsprintf(CATI, "condition       at %4d for %4d type char        notnull\n", offsetof(JOB, condition),       sizeof(j.condition));
	lsprintf(CATI, "expression      at %4d for %4d type char        notnull\n", offsetof(JOB, expression),      sizeof(j.expression));
	lsprintf(CATI, "expr_fail       at %4d for %4d type char        notnull\n", offsetof(JOB, expr_fail),       sizeof(j.expr_fail));
	lsprintf(CATI, "auto_hold       at %4d for %4d type char        notnull\n", offsetof(JOB, auto_hold),       sizeof(j.auto_hold));
	lsprintf(CATI, "date_conditions at %4d for %4d type char        notnull\n", offsetof(JOB, date_conditions), sizeof(j.date_conditions));
	lsprintf(CATI, "days_of_week    at %4d for %4d type char        notnull\n", offsetof(JOB, days_of_week),    sizeof(j.days_of_week));
	lsprintf(CATI, "start_mins      at %4d for %4d type char        notnull\n", offsetof(JOB, start_mins),      sizeof(j.start_mins));
	lsprintf(CATI, "start_times     at %4d for %4d type char        notnull\n", offsetof(JOB, start_times),     sizeof(j.start_times));
	lsprintf(CATI, "on_noexec       at %4d for %4d type char        notnull\n", offsetof(JOB, on_noexec),       sizeof(j.on_noexec));
	lsprintf(CATI, "status          at %4d for %4d type char        notnull\n", offsetof(JOB, status),          sizeof(j.status));
	lsprintf(CATI, "pid             at %4d for %4d type integer(9)  notnull\n", offsetof(JOB, pid),             sizeof(j.pid));
	lsprintf(CATI, "exit_status     at %4d for %4d type integer(9)  notnull\n", offsetof(JOB, exit_status),     sizeof(j.exit_status));
	lsprintf(CATI, "start           at %4d for %4d type integer(10) notnull\n", offsetof(JOB, start),           sizeof(j.start));
	lsprintf(CATI, "finish          at %4d for %4d type integer(10) notnull\n", offsetof(JOB, finish),          sizeof(j.finish));
	lsprintf(CATI, "duration        at %4d for %4d type integer(10) notnull\n", offsetof(JOB, duration),        sizeof(j.duration));
	lsprintf(CATI, "base_auto_hold  at %4d for %4d type char        notnull\n", offsetof(JOB, base_auto_hold),  sizeof(j.base_auto_hold));
	lsprintf(CATI, "gpflag          at %4d for %4d type char        notnull\n", offsetof(JOB, gpflag),          sizeof(j.gpflag));
	lsprintf(CATI, "bitmask         at %4d for %4d type integer(4)  notnull\n", offsetof(JOB, bitmask),         sizeof(j.bitmask));
	lsprintf(END, "");
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



// 
// vim: ts=3 sw=3 ai
// 
