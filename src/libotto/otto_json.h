#ifndef _H_OTTO_JSON_
#define _H_OTTO_JSON_

#include "otto.h"

typedef struct _st_JSON_WRITER_STATE_t {
    int indent_level;
    char current_box[NAMLEN+1];
} st_JSON_WRITER_STATE_t;

int parse_json(DYNBUF *b, JOBLIST *joblist);

int write_json(JOBLIST *joblist);
int json_write_job_tag(JOB *job, st_JSON_WRITER_STATE_t *state);
int json_open_box_tag(JOB *job, st_JSON_WRITER_STATE_t *state);
int json_close_box_tag(JOB *job, st_JSON_WRITER_STATE_t *state);

int json_open_tag(const char* name, st_JSON_WRITER_STATE_t *state);
int json_close_tag(st_JSON_WRITER_STATE_t *state);
int json_write_text_element(const char* name, const char *value, st_JSON_WRITER_STATE_t *state);
int json_open_array(const char* name, st_JSON_WRITER_STATE_t *state);
int json_close_array(st_JSON_WRITER_STATE_t *state);

#endif