#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "ottobits.h"
#include "ottocfg.h"
#include "ottocond.h"
#include "ottocrud.h"
#include "ottodb.h"
#include "ottoipc.h"
#include "ottoutil.h"


void delete_box_chain(int id);



int
create_job(ottoipc_create_job_pdu_st *pdu)
{
	int retval = OTTO_SUCCESS;
	int     i, rc;
	int16_t id;
	int16_t box = -1;

	ottoipc_initialize_send();

	copy_jobwork();

	if(find_jobname(pdu->name) != -1)
	{
		pdu->option = JOB_ALREADY_EXISTS;
		ottoipc_enqueue_simple_pdu((ottoipc_simple_pdu_st *)pdu);
		retval = OTTO_FAIL;
	}

	if(retval == OTTO_SUCCESS &&
		pdu->box_name[0] != '\0' &&
		(box=find_jobname(pdu->box_name)) == -1)
	{
		pdu->option = BOX_NOT_FOUND;
		ottoipc_enqueue_simple_pdu((ottoipc_simple_pdu_st *)pdu);
		retval = OTTO_FAIL;
	}

	if(retval == OTTO_SUCCESS)
	{
		rc = validate_dependencies(pdu->name, pdu->condition);

		// make sure the job isn't directly referencing itself (fatal)
		if(rc & SELF_REFERENCE)
		{
			pdu->option = JOB_DEPENDS_ON_ITSELF;
			ottoipc_enqueue_simple_pdu((ottoipc_simple_pdu_st *)pdu);
			retval = OTTO_FAIL;
		}

		// check whether a referenced job exists (non-fatal)
		if(rc & MISS_REFERENCE)
		{
			pdu->option = JOB_DEPENDS_ON_MISSING_JOB;
			ottoipc_enqueue_simple_pdu((ottoipc_simple_pdu_st *)pdu);
		}
	}

	// reset the work space if necessary
	sort_jobwork(BY_NAME);

	if(retval == OTTO_SUCCESS)
	{
		// assume the last job is the best job to pick based on sort criteria
		id = cfg.ottodb_maxjobs-1;
		if(jobwork[id].name[0] != '\0')
		{
			pdu->option = NO_SPACE_AVAILABLE;
			ottoipc_enqueue_simple_pdu((ottoipc_simple_pdu_st *)pdu);
			retval = OTTO_FAIL;
		}
	}

	if(retval == OTTO_SUCCESS)
	{
		// get actual index
		id = jobwork[id].id;
		if(box != -1)
			box = jobwork[box].id;

		sort_jobwork(BY_ID);

		// copy pdu values to job storage
		clear_jobwork(id);

		memcpy(jobwork[id].name,        pdu->name,        NAMLEN+1);
		memcpy(jobwork[id].box_name,    pdu->box_name,    NAMLEN+1);
		memcpy(jobwork[id].description, pdu->description, DSCLEN+1);
		memcpy(jobwork[id].command,     pdu->command,     CMDLEN+1);
		memcpy(jobwork[id].condition,   pdu->condition,   CNDLEN+1);
		memset(jobwork[id].expression,  0,                CNDLEN+1);
		jobwork[id].type            = tolower(pdu->type);
		jobwork[id].autohold        = pdu->autohold;
		jobwork[id].date_conditions = pdu->date_conditions;
		jobwork[id].days_of_week    = pdu->days_of_week;
		jobwork[id].start_minutes   = pdu->start_minutes;
		jobwork[id].status          = STAT_IN;
		jobwork[id].on_autohold     = pdu->autohold;
		for(i=0; i<24; i++)
			jobwork[id].start_times[i] = pdu->start_times[i];

		// link the new job into the job stream
		jobwork[id].box  = box;
		jobwork[id].head = -1;
		jobwork[id].tail = -1;
		jobwork[id].prev = -1;
		jobwork[id].next = -1;

		// add the job at the tail of its parent box
		if(box == -1)
		{
			if(root_job->head == -1)
			{
				root_job->head = id;
			}
			else
			{
				jobwork[id].prev = root_job->tail;
				jobwork[root_job->tail].next = id;
			}
			root_job->tail = id;
		}
		else
		{
			if(jobwork[box].head == -1)
			{
				jobwork[box].head = id;
			}
			else
			{
				jobwork[id].prev = jobwork[box].tail;
				jobwork[jobwork[box].tail].next = id;
			}
			jobwork[box].tail = id;
		}
		save_jobwork();
		pdu->option = JOB_CREATED;
		ottoipc_enqueue_simple_pdu((ottoipc_simple_pdu_st *)pdu);
	}

	return(retval);
}



void
report_job(ottoipc_simple_pdu_st *pdu)
{
	pdu->option = NACK;
	ottoipc_enqueue_simple_pdu(pdu);
}



void
update_job(ottoipc_update_job_pdu_st *pdu)
{
	int16_t id, old_box, new_box;
	int     i;

	copy_jobwork();

	ottoipc_initialize_send();

	if((id = find_jobname(pdu->name)) == -1)
	{
		pdu->option = JOB_NOT_FOUND;
		ottoipc_enqueue_simple_pdu((ottoipc_simple_pdu_st *)pdu);
	}
	else
	{
		// get actual index
		old_box = jobwork[id].box;
		id      = jobwork[id].id;

		// get old and new box indexes if this pdu requests a box change
		if(pdu->attributes & HAS_BOX_NAME)
		{
			if(pdu->box_name[0] == '\0')
			{
				new_box = -1;
			}
			else
			{
				new_box = find_jobname(pdu->box_name);
				if(new_box == -1)
				{
					pdu->option = BOX_NOT_FOUND;
					ottoipc_enqueue_simple_pdu((ottoipc_simple_pdu_st *)pdu);
				}
				else
				{
					new_box = jobwork[new_box].id;
				}
			}
		}
		// printf("box_name = '%s' new_box = %d\n", pdu->box_name, new_box);

		sort_jobwork(BY_ID);

		// update fields specifically appearing in the request according
		// to the pdu->attributes bitmask

		if(pdu->attributes & HAS_BOX_NAME)
		{
			// unlink the job from its current parent and siblings
			if(jobwork[id].prev != -1) { jobwork[jobwork[id].prev].next = jobwork[id].next; }
			if(jobwork[id].next != -1) { jobwork[jobwork[id].next].prev = jobwork[id].prev; }

			if(old_box == -1)
			{
				if(root_job->head == id) { root_job->head = jobwork[id].next; }
				if(root_job->tail == id) { root_job->tail = jobwork[id].prev; }
				// printf("old_box == -1\n");
			}
			else
			{
				// printf("old_box == %d\n", old_box);
				if(jobwork[old_box].head == id) { jobwork[old_box].head = jobwork[id].next; }
				if(jobwork[old_box].tail == id) { jobwork[old_box].tail = jobwork[id].prev; }
			}

			// initialize job's new linkage
			jobwork[id].box  = new_box;
			jobwork[id].prev = -1;
			jobwork[id].next = -1;

			// link the job to its new parent and siblings
			if(new_box == -1)
			{
				if(root_job->head == -1)
				{
					root_job->head = id;
				}
				else
				{
					jobwork[id].prev = root_job->tail;
					jobwork[root_job->tail].next = id;
				}
				root_job->tail = id;
			}
			else
			{
				if(jobwork[new_box].head == -1)
				{
					jobwork[new_box].head = id;
				}
				else
				{
					jobwork[id].prev = jobwork[new_box].tail;
					jobwork[jobwork[new_box].tail].next = id;
				}
				jobwork[new_box].tail = id;
			}
		}

		if(pdu->attributes & HAS_DESCRIPTION)
		{
			memcpy(jobwork[id].description, pdu->description, sizeof(jobwork[id].description));
		}

		if(pdu->attributes & HAS_COMMAND)
		{
			memcpy(jobwork[id].command, pdu->command, sizeof(jobwork[id].command));
		}

		if(pdu->attributes & HAS_CONDITION)
		{
			memcpy(jobwork[id].condition, pdu->condition, sizeof(jobwork[id].condition));
		}

		if(pdu->attributes & HAS_DATE_CONDITIONS)
		{
			jobwork[id].date_conditions = pdu->date_conditions;
			jobwork[id].days_of_week    = pdu->days_of_week;
			jobwork[id].start_minutes   = pdu->start_minutes;
			for(i=0; i<24; i++)
				jobwork[id].start_times[i] = pdu->start_times[i];
		}

		if(pdu->attributes & HAS_AUTO_HOLD)
		{
			jobwork[id].autohold    = pdu->autohold;
			jobwork[id].on_autohold = pdu->autohold;
		}

		save_jobwork();
		pdu->option = JOB_UPDATED;
		ottoipc_enqueue_simple_pdu((ottoipc_simple_pdu_st *)pdu);
	}
}



void
delete_job(ottoipc_simple_pdu_st *pdu)
{
	int16_t id, box;

	copy_jobwork();

	ottoipc_initialize_send();

	if((id = find_jobname(pdu->name)) == -1 || jobwork[id].type == OTTO_BOX)
	{
		pdu->option = JOB_NOT_FOUND;
		ottoipc_enqueue_simple_pdu(pdu);
	}
	else
	{
		// get actual index
		box = jobwork[id].box;
		id  = jobwork[id].id;

		sort_jobwork(BY_ID);

		if(jobwork[id].prev != -1) { jobwork[jobwork[id].prev].next = jobwork[id].next; }
		if(jobwork[id].next != -1) { jobwork[jobwork[id].next].prev = jobwork[id].prev; }

		if(box == -1)
		{
			if(root_job->head == id) { root_job->head = jobwork[id].next; }
			if(root_job->tail == id) { root_job->tail = jobwork[id].prev; }
		}
		else
		{
			if(jobwork[box].head == id) { jobwork[box].head = jobwork[id].next; }
			if(jobwork[box].tail == id) { jobwork[box].tail = jobwork[id].prev; }
		}

		clear_jobwork(id);
		save_jobwork();
		pdu->option = JOB_DELETED;
		ottoipc_enqueue_simple_pdu(pdu);
	}
}



void
delete_box(ottoipc_simple_pdu_st *pdu)
{
	int16_t id, box, head;

	ottoipc_initialize_send();

	copy_jobwork();

	if((id = find_jobname(pdu->name)) == -1 || jobwork[id].type != OTTO_BOX)
	{
		pdu->option = JOB_NOT_FOUND;
		ottoipc_enqueue_simple_pdu(pdu);
	}
	else
	{
		// get actual index
		box = jobwork[id].box;
		id  = jobwork[id].id;

		sort_jobwork(BY_ID);

		if(jobwork[id].prev != -1) { jobwork[jobwork[id].prev].next = jobwork[id].next; }
		if(jobwork[id].next != -1) { jobwork[jobwork[id].next].prev = jobwork[id].prev; }

		if(box == -1)
		{
			if(root_job->head == id) { root_job->head = jobwork[id].next; }
			if(root_job->tail == id) { root_job->tail = jobwork[id].prev; }
		}
		else
		{
			if(jobwork[box].head == id) { jobwork[box].head = jobwork[id].next; }
			if(jobwork[box].tail == id) { jobwork[box].tail = jobwork[id].prev; }
		}

		head = jobwork[id].head;
		clear_jobwork(id);
		pdu->option = BOX_DELETED;
		ottoipc_enqueue_simple_pdu(pdu);
		delete_box_chain(head);
		save_jobwork();
	}
}



void
delete_box_chain(int id)
{
	int next, head, type;
	ottoipc_simple_pdu_st pdu;

	while(id != -1)
	{
		// save values for use after they're cleared
		next = jobwork[id].next;
		head = jobwork[id].head;
		type = jobwork[id].type;

		memcpy(pdu.name, jobwork[id].name, sizeof(pdu.name));
		pdu.opcode  = DELETE_JOB;
		pdu.option  = JOB_DELETED;
		if(type == OTTO_BOX)
		{
			pdu.opcode  = DELETE_BOX;
			pdu.option  = BOX_DELETED;
		}
		ottoipc_enqueue_simple_pdu(&pdu);

		clear_jobwork(id);

		if(type == OTTO_BOX)
			delete_box_chain(head);

		id = next;
	}
}



//
// vim: ts=3 sw=3 ai
//
