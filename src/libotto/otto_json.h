#ifndef _H_OTTO_JSON_
#define _H_OTTO_JSON_

#include <stdlib.h>
#include "otto.h"

#pragma pack(1)
typedef struct _st_JSON_BOX_NODE_t_
{
    char name[NAMLEN + 1];
    struct _st_JSON_BOX_NODE_t_ *next;
} st_JSON_BOX_NODE_t;

typedef struct _st_JSON_WRITER_STATE_t_
{
    int indent_level;
    st_JSON_BOX_NODE_t *box_top;    
} st_JSON_WRITER_STATE_t;
#pragma pack(0)

int parse_json(DYNBUF *b, JOBLIST *joblist);

int write_json(JOBLIST *joblist);
int json_write_job_elements(JOB *job, st_JSON_WRITER_STATE_t *state);
int json_write_cmd_tag(JOB *job, st_JSON_WRITER_STATE_t *state);
int json_open_box_tag(JOB *job, st_JSON_WRITER_STATE_t *state);
int json_close_box_tag(JOB *job, st_JSON_WRITER_STATE_t *state);

int json_open_tag(const char *name, st_JSON_WRITER_STATE_t *state);
int json_close_tag(st_JSON_WRITER_STATE_t *state);
int json_open_array(const char *name, st_JSON_WRITER_STATE_t *state);
int json_close_array(st_JSON_WRITER_STATE_t *state);

int json_write_text_element(const char *name, const char *value, st_JSON_WRITER_STATE_t *state);
int json_write_char_element(const char *name, char value, st_JSON_WRITER_STATE_t *state);
int json_write_long_element(const char *name, int64_t value, st_JSON_WRITER_STATE_t *state);

#endif