#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "ottobits.h"
#include "otto_pid_writer.h"



int
write_pid(JOBLIST *joblist)
{
	int retval = OTTO_SUCCESS;
	int i;

	if(joblist == NULL)
		retval = OTTO_FAIL;

	if(retval == OTTO_SUCCESS)
	{
		for(i=0; i<joblist->nitems; i++)
		{
			if(joblist->item[i].type == OTTO_BOX || joblist->item[i].pid <= 0)
				printf("-1\n");
			else
				printf("%d\n",  joblist->item[i].pid);
		}
	}

	return(retval);
}



//
// vim: ts=3 sw=3 ai
//
