int insert_task(JOB *item);



int
ottomsp_import(JOBLIST *joblist)
{
	int retval = OTTO_SUCCESS;
	int  i, joblevel=0;

	for(i=1; i<joblist->nitems; i++)
	{
		if(strcmp(joblist->item[i].name, jobname) == 0)
		{
			joblevel = joblist->item[i].level;
			break;
		}
	}

	if(i == joblist->nitems)
		i = 1;

	// loop through remaining jobs printing them if necessary
	for(; i<=joblist->nitems; i++)
	{
		if(jobname[0] != '\0' && joblist->item[i].level <= joblevel)
			break;

		insert_task(&joblist->item[i]);
	}

	return(retval);
}



int
insert_task(JOB *item)
{
	int retval = OTTO_SUCCESS;
// placeholder
	return(retval);
}



//
// vim: ts=3 sw=3 ai
//
