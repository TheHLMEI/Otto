#include <string.h>

#include "ottobits.h"
#include "ottocfg.h"
#include "ottocond.h"
#include "ottodb.h"
#include "ottojob.h"
#include "simplelog.h"

extern SIMPLELOG *logp;


int
compile_expression(char *output, char *condition, int outlen)
{
	int retval = OTTO_SUCCESS;
	int exit_loop;
	char name[NAMLEN+1];
	char *i, *n, *s, *t;
	int16_t index;
	int l;

	s = condition;
	t = output;
	l = outlen;

	while(l > 0 && *s != '\0')
	{
		switch(*s)
		{
			case '(':
			case ')':
			case '&':
			case '|':
				*t++ = *s;
				l--;
				break;
			case 'd':
			case 'f':
			case 'n':
			case 'r':
			case 's':
			case 't':
				*t++ = *s++;
				l--;
				if(l > 2)
				{
					n = name;
					exit_loop = OTTO_FALSE;
					while(exit_loop == OTTO_FALSE && *s != '\0')
					{
						switch(*s)
						{
							case '(':
								break;
							case ')':
								*n = '\0';
								exit_loop = OTTO_TRUE;
								break;
							default:
								*n++ = *s;
								if(n - name == NAMLEN)
								{
									lprintf(logp, MAJR, "Condition compile name exceeds allowed length (%d bytes).\n", NAMLEN);
									return(OTTO_FAIL);
								}
								break;
						}
						s++;
					}
					index = find_jobname(name);
					// lprintf(logp, INFO, "Lookup name = '%s' index = %d id = %d\n", name, index, jobwork[index].id);
					if(index != -1)
					{
						index = jobwork[index].id;
					}
					else
					{
						retval = OTTO_FAIL;
					}
					i = (char *)&index;
					*t++ = *i++;
					*t++ = *i;
					l -= 2;
				}
				else
				{
					lprintf(logp, MAJR, "Condition exceeds compile buffer (%d bytes).\n", outlen);
					return(OTTO_FAIL);
				}
				break;
		}

		s++;
	}
	*t = '\0';

	return(retval);
}



int
evaluate_expr(char *s)
{
	ecnd_st c;

	c.s = s;

	return(evaluate_term(&c));
}



int
evaluate_term(ecnd_st *c)
{
	int retval     = OTTO_FALSE;
	int exit_loop  = OTTO_FALSE;
	int term       = OTTO_FALSE;
	int evaluate   = OTTO_FALSE;
	int operation  = '|';

	if(*(c->s) == '(')
	{
		// eat it
		c->s++;
	}

	while(*(c->s) != '\0' && exit_loop == OTTO_FALSE)
	{
		evaluate = OTTO_FALSE;
		switch(*(c->s))
		{
			case '&': operation = '&';       break;
			case '|': operation = '|';       break;
			case ')': exit_loop = OTTO_TRUE; break;
			default:  retval = OTTO_FALSE; exit_loop = OTTO_TRUE; break;
			
			case '(': term = evaluate_term(c); evaluate = OTTO_TRUE; break;
			case 'd': 
			case 'f': 
			case 'n': 
			case 'r': 
			case 's': 
			case 't': term = evaluate_stat(c); evaluate = OTTO_TRUE; break;
		}

		if(term == OTTO_FAIL)
			return(OTTO_FAIL);

		c->s++;

		if(evaluate == OTTO_TRUE)
		{
			if(operation == '&')
			{
				if(retval && term)
					retval = OTTO_TRUE;
				else
					retval = OTTO_FALSE;
			}
			else
			{
				if(retval || term)
					retval = OTTO_TRUE;
				else
					retval = OTTO_FALSE;
			}

			if(operation == '&' && retval == OTTO_FALSE)
				exit_loop = OTTO_TRUE;
		}
	}

	return(retval);
}



int
evaluate_stat(ecnd_st *c)
{
	int retval = OTTO_FALSE;
	int16_t index;
	char *i;
	char test;

	i = (char *)&index;

	test = *(c->s++);
	*i++ = *(c->s++);
	*i   = *(c->s);

	if(index >= 0)
	{
		switch(test)
		{
			case 'd':
				if(job[index].status == STAT_SU ||
					job[index].status == STAT_FA ||
					job[index].status == STAT_TE)
					retval = OTTO_TRUE;
				break;

			case 'f':
				if(job[index].status == STAT_FA)
					retval = OTTO_TRUE;
				break;

			case 'n':
				if(job[index].status != STAT_RU)
					retval = OTTO_TRUE;
				break;

			case 'r':
				if(job[index].status == STAT_RU)
					retval = OTTO_TRUE;
				break;

			case 's':
				if(job[index].status == STAT_SU)
					retval = OTTO_TRUE;
				break;

			case 't':
				if(job[index].status == STAT_TE)
					retval = OTTO_TRUE;
				break;

			default:
				retval = OTTO_FALSE;
				break;
		}
	}
	else
	{
		retval = OTTO_FAIL;
	}

	return(retval);
}


int
validate_dependencies(char *name, char *condition)
{
	int retval = 0;
	char tmpnam[NAMLEN+1];
	char *s = condition, *t;

	// set up the work space
	sort_jobwork(BY_NAME);

	while(*s != '\0')
	{
		// look for open paren
		while(*s != '(' && *s != '\0')
			s++;

		if(*s != '\0')
		{
			// advance to job name
			s++;

			//copy name into tmpnam
			t = tmpnam;
			while(*s != ')' && *s != '\0')
				*t++ = *s++;

			// null terminate
			*t = '\0';

			// make sure the job isn't directly referencing itself (fatal)
			if(strcmp(name, tmpnam) == 0)
			{
				retval |= SELF_REFERENCE;
			}

			// check whether a referenced job exists (non-fatal)
			if(find_jobname(tmpnam) == -1)
			{
				retval |= MISS_REFERENCE;
			}
		}
	}

	return(retval);
}



//
// vim: ts=3 sw=3 ai
//
