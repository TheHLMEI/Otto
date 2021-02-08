#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
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

		if(pdu->type == OTTO_BOX && pdu->command[0] != '\0')
		{
			memset(pdu->command, 0, sizeof(pdu->command));
			pdu->option = BOX_COMMAND;
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
		// get actual indexes before re-sorting
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



int
report_job(ottoipc_simple_pdu_st *pdu)
{
	int     retval = OTTO_SUCCESS;
	JOBLIST joblist;
	int     level_limit = MAXLVL;
	int     h, i;
	ottoipc_report_job_pdu_st response;

	if((joblist.item = (JOB *)calloc(cfg.ottodb_maxjobs, sizeof(JOB))) == NULL)
		retval = OTTO_FAIL;

	joblist.nitems = 0;
	level_limit    = pdu->option;
	if(level_limit > MAXLVL)
		level_limit = MAXLVL;

	build_joblist(&joblist, pdu->name, root_job->head, 0, pdu->option, OTTO_TRUE);

	if(joblist.nitems == 0)
	{
		pdu->option = BOX_NOT_FOUND;
		ottoipc_enqueue_simple_pdu((ottoipc_simple_pdu_st *)pdu);
	}
	else
	{
		for(i=0; i<joblist.nitems; i++)
		{
			memset(&response, 0, sizeof(response));

			response.pdu_type = pdu->pdu_type;
			response.opcode   = pdu->opcode;

			memcpy(response.name,        joblist.item[i].name,        NAMLEN+1);
			memcpy(response.box_name,    joblist.item[i].box_name,    NAMLEN+1);
			memcpy(response.description, joblist.item[i].description, DSCLEN+1);
			memcpy(response.command,     joblist.item[i].command,     CMDLEN+1);
			memcpy(response.condition,   joblist.item[i].condition,   CNDLEN+1);

			response.id              = joblist.item[i].id;
			response.level           = joblist.item[i].level;
			response.box             = joblist.item[i].box;
			response.head            = joblist.item[i].head;
			response.tail            = joblist.item[i].tail;
			response.prev            = joblist.item[i].prev;
			response.next            = joblist.item[i].next;
			response.type            = joblist.item[i].type;
			response.date_conditions = joblist.item[i].date_conditions;
			response.days_of_week    = joblist.item[i].days_of_week;
			response.start_minutes   = joblist.item[i].start_minutes;
			for(h=0; h<24; h++)
				response.start_times[h] = joblist.item[i].start_times[h];
			response.autohold        = joblist.item[i].autohold;
			response.expr_fail       = joblist.item[i].expr_fail;
			response.status          = joblist.item[i].status;
			response.on_autohold     = joblist.item[i].on_autohold;
			response.on_noexec       = joblist.item[i].on_noexec;
			response.pid             = joblist.item[i].pid;
			response.start           = joblist.item[i].start;
			response.finish          = joblist.item[i].finish;
			response.duration        = joblist.item[i].duration;
			response.exit_status     = joblist.item[i].exit_status;

			ottoipc_enqueue_report_job(&response);
		}
	}

	return(retval);
}



int
update_job(ottoipc_update_job_pdu_st *pdu)
{
	int retval = OTTO_SUCCESS;
	int16_t id, box, new_box, tmp_id;
	int     i;
	int     rc;

	copy_jobwork();

	ottoipc_initialize_send();

	if((id = find_jobname(pdu->name)) == -1)
	{
		pdu->option = JOB_NOT_FOUND;
		ottoipc_enqueue_simple_pdu((ottoipc_simple_pdu_st *)pdu);
	}
	else
	{
		// get actual indexes before re-sorting
		box = jobwork[id].box;
		id  = jobwork[id].id;

		// get new box index if this pdu requests a box change
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

		sort_jobwork(BY_ID);

		// check validity of requested changes
		if(pdu->attributes & HAS_BOX_NAME)
		{
			// check to see if the new box is actually a child of the
			// current job in some way
			if(jobwork[id].type == OTTO_BOX)
			{
				tmp_id = jobwork[new_box].box;
				while(tmp_id != -1)
				{
					if(tmp_id == box)
					{
						pdu->option = GRANDFATHER_PARADOX;
						ottoipc_enqueue_simple_pdu((ottoipc_simple_pdu_st *)pdu);
						retval = OTTO_FAIL;
						break;
					}
					tmp_id = jobwork[tmp_id].box;
				}
			}
		}

		if(pdu->attributes & HAS_COMMAND && jobwork[id].type == OTTO_BOX)
		{
			pdu->attributes ^= HAS_COMMAND;
			pdu->option = BOX_COMMAND;
			ottoipc_enqueue_simple_pdu((ottoipc_simple_pdu_st *)pdu);
		}

		if(pdu->attributes & HAS_CONDITION)
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


		// update fields specifically appearing in the request according
		// to the pdu->attributes bitmask
		if(retval == OTTO_SUCCESS)
		{
			if(pdu->attributes & HAS_BOX_NAME)
			{
				// unlink the job from its current parent and siblings
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

	return(retval);
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
		// get actual indexes before re-sorting
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
		// get actual indexes before re-sorting
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
