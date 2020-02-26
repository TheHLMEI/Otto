int
ottomsp_list(JOBLIST *joblist)
{
	int retval = OTTO_SUCCESS;
	int  i;

	for(i=1; i<joblist->nitems; i++)
	{
		printf("%*.*s%s\n", joblist->item[i].level, joblist->item[i].level, " ", joblist->item[i].name);
	}

	return(retval);
}



//
// vim: ts=3 sw=3 ai
//
