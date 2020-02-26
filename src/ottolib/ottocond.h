#ifndef _OTTOCOND_H_
#define _OTTOCOND_H_

#define SELF_REFERENCE      (1)
#define MISS_REFERENCE   (1<<1)

enum JOBSTATS
{
	STAT_IN=0,
	STAT_AC,
	STAT_RU,
	STAT_SU,
	STAT_FA,
	STAT_TE,
	STAT_OH,
	STAT_MAX
};


typedef struct _ecnd_st
{
	char *s;
} ecnd_st;


int   compile_expression (char *output, char *condition, int outlen);
int   evaluate_expr (char *s);
int   evaluate_stat (ecnd_st *c);
int   evaluate_term (ecnd_st *c);
int   validate_dependencies(char *name, char *condition);

#endif
//
// vim: ts=3 sw=3 ai
//
