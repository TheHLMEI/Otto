#include <string.h>
#include <inttypes.h>
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

    memset(&state, 0, sizeof(state));
    state.indent_level = 0;
    state.box_top = NULL;

    printf("{\n");
    ret = json_open_array("jobs", &state);
    if (ret == OTTO_SUCCESS)
    {
        for (i = 0; i < joblist->nitems; i++)
        {
            job = &joblist->item[i];

            if (state.box_top != NULL && strcmp(state.box_top->name, job->box_name) != 0) 
            {
                json_close_box_tag(job, &state);
            }

            switch (job->type)
            {
            case OTTO_BOX:
                json_open_box_tag(job, &state);
                break;

            case OTTO_CMD:
                json_write_cmd_tag(job, &state);
                break;

            default:
                break;
            }
        }
    }

    while (state.box_top != NULL) 
    {
        json_close_box_tag(NULL, &state);
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

    // push the box on the stack
    st_JSON_BOX_NODE_t *box_node = (st_JSON_BOX_NODE_t *) malloc(sizeof(st_JSON_BOX_NODE_t));
    strcpy(box_node->name, job->name);
    if (state->box_top == NULL)
        box_node->next = NULL;
    else
        box_node->next = state->box_top;
    state->box_top = box_node;

    ret &= json_open_tag(NULL, state);
    ret &= json_write_job_elements(job, state);
    ret &= json_open_array("jobs", state);

    return ret;
}

int json_close_box_tag(JOB *job, st_JSON_WRITER_STATE_t *state)
{
    int ret = OTTO_SUCCESS;

    // pop the node from the stack
    if (state->box_top == NULL)
        return OTTO_FAIL;

    st_JSON_BOX_NODE_t *box_node = state->box_top;
    state->box_top = box_node->next;
    free(box_node);    

    ret &= json_close_array(state);
    ret &= json_close_tag(state);

    return ret;
}

int json_write_cmd_tag(JOB *job, st_JSON_WRITER_STATE_t *state)
{
    int ret = OTTO_SUCCESS;

    if (state == NULL)
        return OTTO_FAIL;

    ret &= json_open_tag(NULL, state);
    ret &= json_write_job_elements(job, state);
    ret &= json_write_text_element("command", job->command, state);
    ret &= json_close_tag(state);

    return ret;
}

int json_write_job_elements(JOB *job, st_JSON_WRITER_STATE_t *state)
{
    int ret = OTTO_SUCCESS;
    JOBTVAL tval;

    if (state == NULL)
        return OTTO_FAIL;

    ottojob_prepare_txt_values(&tval, job, AS_ASCII);
    ret &= json_write_text_element("name", job->name, state);

    ret &= json_write_text_element("type", tval.type, state);
    ret &= json_write_text_element("description", job->description, state);

    if (strlen(job->box_name) > 0)
        ret &= json_write_text_element("box_name", job->box_name, state);

    if (strlen(job->condition) > 0)
        ret &= json_write_text_element("condition", job->condition, state);

    if (job->date_conditions != OTTO_FALSE)
    {
        ret &= json_write_text_element("date_conditions", tval.date_conditions, state);
        ret &= json_write_text_element("days_of_week", tval.days_of_week, state);

        switch (job->date_conditions)
        {
        case OTTO_USE_START_MINUTES:
            ret &= json_write_text_element("start_minutes", tval.start_minutes, state);
            break;
        case OTTO_USE_START_TIMES:
            ret &= json_write_text_element("start_times", tval.start_times, state);
            break;
        }
    }

    ret &= json_write_text_element("autohold", tval.autohold, state);

    if (strlen(job->environment) > 0)
        ret &= json_write_text_element("environment", tval.environment, state);

    return ret;
}

// Generic JSON methods
int json_open_tag(const char *name, st_JSON_WRITER_STATE_t *state)
{
    int ret = OTTO_SUCCESS;

    if (name != NULL)
    {
        printf("%.*s\"%s\": {\n", state->indent_level * JSON_TAB_STOP, JSON_TAB_CHAR, name);
    }
    else
    {
        printf("%.*s{\n", state->indent_level * JSON_TAB_STOP, JSON_TAB_CHAR);
    }
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

int json_open_array(const char *name, st_JSON_WRITER_STATE_t *state)
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

int json_write_text_element(const char *name, const char *value, st_JSON_WRITER_STATE_t *state)
{
    int ret = OTTO_SUCCESS;
    printf("%.*s\"%s\": \"%s\",\n", state->indent_level * JSON_TAB_STOP, JSON_TAB_CHAR, name, value);
    return ret;
}

int json_write_char_element(const char *name, const char value, st_JSON_WRITER_STATE_t *state)
{
    int ret = OTTO_SUCCESS;
    printf("%.*s\"%s\": \"%c\",\n", state->indent_level * JSON_TAB_STOP, JSON_TAB_CHAR, name, value);
    return ret;
}

int json_write_long_element(const char *name, int64_t value, st_JSON_WRITER_STATE_t *state)
{
    int ret = OTTO_SUCCESS;
    printf("%.*s\"%s\": \"%" PRId64 "\",\n", state->indent_level * JSON_TAB_STOP, JSON_TAB_CHAR, name, value);
    return ret;
}