#ifndef _OTTODB_H_
#define _OTTODB_H_

#include "ottojob.h"

#define OTTODB_VERSION JOB_LAYOUT_VERSION
#define OTTODB_MAXJOBS 1000

enum SORTS
{
	BY_UNKNOWN,
	BY_ACTIVE,
	BY_ID,
	BY_LINKED_LIST,
	BY_NAME,
	BY_PID,
	BY_DATE_COND
};


extern JOB  *job;
extern JOB  *root_job;
extern JOB  *jobwork;
extern ino_t ottodb_inode;
extern int   ottodbfd;


// ottodb functions
int   open_ottodb(int type);
void  copy_jobwork();
void  save_jobwork();
void  sort_jobwork(int sort_mode);
int   find_jobname(char *jobname);
void  clear_job(int job_index);
void  clear_jobwork(int job_index);

#endif
//
// vim: ts=3 sw=3 ai
//

