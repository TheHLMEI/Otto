/*------------------------------- ottomsp export functions -------------------------------*/
int
ottomsp_export(void)
{
	int retval = OTTO_SUCCESS;
	int i, id=-1;
	JOBLIST joblist;

	copy_jobwork();

	for(i=0; i<cfg.ottodb_maxjobs; i++)
	{
		jobwork[i].id = -1;

		if(jobname[0] != '\0' &&
			jobwork[i].name[0] != '\0' &&
			strcmp(jobname, jobwork[i].name) == 0)
			id = i;
	}


	if(id == -1)
	{
		if(jobname[0] != '\0')
			retval = OTTO_FAIL;
		else
			id = root_job->head;
	}

	if(retval == OTTO_SUCCESS)
		prep_tasks(id);

	if(retval == OTTO_SUCCESS)
		write_mpp_xml(&joblist, autoschedule);

	return(retval);
}



void
prep_tasks(int16_t id)
{
	while(id != -1)
	{
		jobwork[id].id = jobid++;
		if(start == -1)
		{
			if(jobwork[id].start != 0)
			{
				start = id;
			}
		}
		else
		{
			if(jobwork[id].start != 0 && jobwork[id].start < jobwork[start].start)
			{
				start = id;
			}
		}
		if(jobwork[id].type == 'b')
			prep_tasks(jobwork[id].head);

		if(jobname[0] != '\0' && strcmp(jobname, jobwork[id].name) == 0)
			break;

		id = jobwork[id].next;
	}
}



//
// vim: ts=3 sw=3 ai
//
