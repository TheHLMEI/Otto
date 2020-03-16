#ifndef _OTTOCRUD_H_
#define _OTTOCRUD_H_

#include "ottoipc.h"

int  create_job(ottoipc_create_job_pdu_st *pdu);
int  report_job(ottoipc_simple_pdu_st     *pdu);
int  update_job(ottoipc_update_job_pdu_st *pdu);
void delete_job(ottoipc_simple_pdu_st     *pdu);
void delete_box(ottoipc_simple_pdu_st     *pdu);

#endif
//
// vim: ts=3 sw=3 ai
//
