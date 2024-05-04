#include <string.h>
#include "otto.h"
#include "otto_json.h"

#define JSON_TAB_STOP 2
#define JSON_TAB_CHAR "                                                                                                                             "

int parse_json(DYNBUF *b, JOBLIST *joblist)
{
    int ret = OTTO_SUCCESS;

    if (joblist == NULL)
        return OTTO_FAIL;

    memset(joblist, 0, sizeof(JOBLIST));

    b->line = 0;
    b->s = b->buffer;
    while (ret == OTTO_SUCCESS && b->s[0] != '\0')
    {
        // TODO
        advance_word(b);
    }

    return ret;
}

int write_json(JOBLIST *joblist)
{
    int ret = OTTO_SUCCESS;
    int i;
    st_JSON_WRITER_STATE_t state;
    JOB *job;

    if (joblist == NULL)
        ret = OTTO_FAIL;

    state.indent_level = 0;
    printf("{\n");
    ret = json_open_array("jobs", &state);
    if (ret == OTTO_SUCCESS)
    {
        for (i = 0; i < joblist->nitems; i++)
        {
            job = &joblist->item[i];
            switch (job->type)
            {
            case OTTO_BOX:
                json_open_box_tag(&joblist->item[i], &state);
                json_close_box_tag(&joblist->item[i], &state);
                break;

            case OTTO_CMD:
                json_write_job_tag(&joblist->item[i], &state);
                break;

            default:
                break;
            }

        }
    }
    json_close_array(&state);
    printf("}\n");

    return ret;
}

int json_open_box_tag(JOB *job, st_JSON_WRITER_STATE_t *state)
{
    int ret = OTTO_SUCCESS;

    if (state == NULL)
        return OTTO_FAIL;

    ret &= json_open_tag("box", state);
    ret &= json_write_text_element("name", job->name, state);
    ret &= json_write_text_element("description", job->description, state);
    ret &= json_write_text_element("box_name", job->box_name, state);
    ret &= json_write_text_element("condition", job->condition, state);

    return ret;
}

int json_close_box_tag(JOB *job, st_JSON_WRITER_STATE_t *state)
{
    return json_close_tag(state);
}

int json_write_job_tag(JOB *job, st_JSON_WRITER_STATE_t *state)
{
    int ret = OTTO_SUCCESS;

    if (state == NULL)
        return OTTO_FAIL;

    ret &= json_open_tag("command", state);
    ret &= json_write_text_element("name", job->name, state);
    ret &= json_write_text_element("description", job->description, state);
    ret &= json_write_text_element("box_name", job->box_name, state);
    ret &= json_write_text_element("command", job->command, state);
    ret &= json_write_text_element("condition", job->condition, state);
    ret &= json_close_tag(state);

    return ret;
}

// Generic JSON methods
int json_open_tag(const char *name, st_JSON_WRITER_STATE_t *state)
{
    int ret = OTTO_SUCCESS;
    printf("%.*s\"%s\": {\n", state->indent_level * JSON_TAB_STOP, JSON_TAB_CHAR, name);
    state->indent_level++;
    return ret;
}

int json_close_tag(st_JSON_WRITER_STATE_t *state)
{
    int ret = OTTO_SUCCESS;
    state->indent_level--;
    printf("%.*s},\n", state->indent_level * JSON_TAB_STOP, JSON_TAB_CHAR);
    return ret;
}

int json_write_text_element(const char *name, const char *value, st_JSON_WRITER_STATE_t *state)
{
    int ret = OTTO_SUCCESS;
    printf("%.*s\"%s\": \"%s\",\n", state->indent_level * JSON_TAB_STOP, JSON_TAB_CHAR, name, value);
    return ret;
}

int json_open_array(const char* name, st_JSON_WRITER_STATE_t *state)
{
    int ret = OTTO_SUCCESS;

    if (state == NULL)
        return OTTO_FAIL;

    printf("%.*s\"%s\": [\n", state->indent_level * JSON_TAB_STOP, JSON_TAB_CHAR, name);
    state->indent_level++;

    return ret;
}

int json_close_array(st_JSON_WRITER_STATE_t *state)
{
    int ret = OTTO_SUCCESS;

    if (state == NULL)
        return OTTO_FAIL;

    state->indent_level--;
    printf("%.*s],\n", state->indent_level * JSON_TAB_STOP, JSON_TAB_CHAR);

    return ret;
}
