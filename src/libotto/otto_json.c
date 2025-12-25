#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include "otto.h"
#include "otto_json.h"

#define JSON_TAB_STOP 2
#define JSON_TAB_CHAR "                                                                                                                             "

// TODOs
// 1. Stop putting a comma afer every element.  There should be a way to tell the function this is the last or there were others before
// 2. The entire parse_json thing for ottoimp

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

// Remove a trailing comma (and trailing whitespace after it) from b->buffer.
static void json_trim_trailing_comma(DYNBUF *b)
{
    if (b == NULL || b->buffer == NULL || b->eob == 0)
        return;

    int i = (int)b->eob - 1;
    // Skip whitespace at the end
    while (i >= 0 && (b->buffer[i] == ' ' || b->buffer[i] == '\t' ||
                      b->buffer[i] == '\r' || b->buffer[i] == '\n'))
    {
        i--;
    }

    if (i >= 0 && b->buffer[i] == ',')
    {
        // Delete the comma and any whitespace that followed it
        int j = i + 1;
        while (j < (int)b->eob && (b->buffer[j] == ' ' || b->buffer[j] == '\t' ||
                                   b->buffer[j] == '\r' || b->buffer[j] == '\n'))
        {
            j++;
        }
        memmove(&b->buffer[i], &b->buffer[j], b->eob - j);
        b->eob -= (j - i);
        b->buffer[b->eob] = '\0';
    }
}

// Escape JSON string value: quotes, backslashes, and control characters
static char *json_escape(const char *value)
{
    if (value == NULL)
        return strdup("");

    size_t len = strlen(value);
    size_t cap = len * 6 + 1; // worst-case \u00XX per char
    char *out = (char *)malloc(cap);
    if (!out)
        return strdup("");

    char *t = out;
    for (size_t k = 0; k < len; k++)
    {
        unsigned char c = (unsigned char)value[k];
        switch (c)
        {
            case '"': *t++ = '\\'; *t++ = '"'; break;
            case '\\': *t++ = '\\'; *t++ = '\\'; break;
            case '\b': *t++ = '\\'; *t++ = 'b';  break;
            case '\f': *t++ = '\\'; *t++ = 'f';  break;
            case '\n': *t++ = '\\'; *t++ = 'n';  break;
            case '\r': *t++ = '\\'; *t++ = 'r';  break;
            case '\t': *t++ = '\\'; *t++ = 't';  break;
            default:
                if (c < 0x20)
                {
                    t += sprintf(t, "\\u%04x", c);
                }
                else
                {
                    *t++ = c;
                }
        }
    }
    *t = '\0';
    return out;
}

/**
 *
 */
int write_json(JOBLIST *joblist)
{
    int ret = OTTO_SUCCESS;
    DYNBUF b;

    buffer_json(&b, joblist);
    printf("%s\n", b.buffer);

    return ret;
}

int json_open_box_tag(DYNBUF *b, JOB *job, st_JSON_WRITER_STATE_t *state)
{
    int ret = OTTO_SUCCESS;

    if (state == NULL)
        return OTTO_FAIL;

    // push the box on the stack
    st_JSON_BOX_NODE_t *box_node = (st_JSON_BOX_NODE_t *)malloc(sizeof(st_JSON_BOX_NODE_t));
    strcpy(box_node->name, job->name);
    if (state->box_top == NULL)
        box_node->next = NULL;
    else
        box_node->next = state->box_top;
    state->box_top = box_node;

    ret &= json_open_tag(b, NULL, state);
    ret &= json_write_job_elements(b, job, state);
    ret &= json_open_array(b, "jobs", state);

    return ret;
}

int json_close_box_tag(DYNBUF *b, JOB *job, st_JSON_WRITER_STATE_t *state)
{
    int ret = OTTO_SUCCESS;

    // pop the node from the stack
    if (state->box_top == NULL)
        return OTTO_FAIL;

    st_JSON_BOX_NODE_t *box_node = state->box_top;
    state->box_top = box_node->next;
    free(box_node);

    ret &= json_close_array(b, state);
    ret &= json_close_tag(b, state);

    return ret;
}

int json_write_cmd_tag(DYNBUF *b, JOB *job, st_JSON_WRITER_STATE_t *state)
{
    int ret = OTTO_SUCCESS;

    if (state == NULL)
        return OTTO_FAIL;

    ret &= json_open_tag(b, NULL, state);
    ret &= json_write_job_elements(b, job, state);
    ret &= json_write_text_element(b, "command", job->command, state);
    ret &= json_close_tag(b, state);

    return ret;
}

int json_write_job_elements(DYNBUF *b, JOB *job, st_JSON_WRITER_STATE_t *state)
{
    int ret = OTTO_SUCCESS;
    JOBTVAL tval;

    if (state == NULL)
        return OTTO_FAIL;

    ottojob_prepare_txt_values(&tval, job, AS_ASCII);
    ret &= json_write_text_element(b, "name", job->name, state);

    ret &= json_write_text_element(b, "type", tval.type, state);
    ret &= json_write_text_element(b, "description", job->description, state);

    if (strlen(job->box_name) > 0)
        ret &= json_write_text_element(b, "box_name", job->box_name, state);

    if (strlen(job->condition) > 0)
        ret &= json_write_text_element(b, "condition", job->condition, state);

    if (job->date_conditions != OTTO_FALSE)
    {
        ret &= json_write_text_element(b, "date_conditions", tval.date_conditions, state);
        ret &= json_write_text_element(b, "days_of_week", tval.days_of_week, state);

        switch (job->date_conditions)
        {
        case OTTO_USE_START_MINUTES:
            ret &= json_write_text_element(b, "start_minutes", tval.start_minutes, state);
            break;
        case OTTO_USE_START_TIMES:
            ret &= json_write_text_element(b, "start_times", tval.start_times, state);
            break;
        }
    }

    if (job->autohold == OTTO_TRUE)
        ret &= json_write_long_element(b, "autohold", 1, state);

    if (strlen(job->environment) > 0)
        ret &= json_write_text_element(b, "environment", tval.environment, state);

    // Emit numeric epoch seconds for start/finish and numeric duration
    ret &= json_write_long_element(b, "start", (int64_t)job->start, state);
    ret &= json_write_long_element(b, "finish", (int64_t)job->finish, state);
    ret &= json_write_long_element(b, "duration", (int64_t)job->duration, state);
    ret &= json_write_text_element(b, "status", tval.status, state);

    return ret;
}

// Generic JSON methods
int json_open_tag(DYNBUF *b, const char *name, st_JSON_WRITER_STATE_t *state)
{
    int ret = OTTO_SUCCESS;

    if (name != NULL)
    {
        bprintf(b, "%.*s\"%s\": {\n", state->indent_level * JSON_TAB_STOP, JSON_TAB_CHAR, name);
    }
    else
    {
        bprintf(b, "%.*s{\n", state->indent_level * JSON_TAB_STOP, JSON_TAB_CHAR);
    }
    state->indent_level++;
    return ret;
}

int json_close_tag(DYNBUF *b, st_JSON_WRITER_STATE_t *state)
{
    int ret = OTTO_SUCCESS;
    if (b != NULL) json_trim_trailing_comma(b);
    state->indent_level--;
    bprintf(b, "%.*s},\n", state->indent_level * JSON_TAB_STOP, JSON_TAB_CHAR);
    return ret;
}

int json_open_array(DYNBUF *b, const char *name, st_JSON_WRITER_STATE_t *state)
{
    int ret = OTTO_SUCCESS;

    if (state == NULL)
        return OTTO_FAIL;

    bprintf(b, "%.*s\"%s\": [\n", state->indent_level * JSON_TAB_STOP, JSON_TAB_CHAR, name);
    state->indent_level++;

    return ret;
}

int json_close_array(DYNBUF *b, st_JSON_WRITER_STATE_t *state)
{
    int ret = OTTO_SUCCESS;

    if (state == NULL)
        return OTTO_FAIL;

    if (b != NULL) json_trim_trailing_comma(b);
    state->indent_level--;
    bprintf(b, "%.*s],\n", state->indent_level * JSON_TAB_STOP, JSON_TAB_CHAR);

    return ret;
}

int json_write_text_element(DYNBUF *b, const char *name, const char *value, st_JSON_WRITER_STATE_t *state)
{
    int ret = OTTO_SUCCESS;
    char *esc = json_escape(value);
    bprintf(b, "%.*s\"%s\": \"%s\",\n", state->indent_level * JSON_TAB_STOP, JSON_TAB_CHAR, name, esc);
    free(esc);
    return ret;
}

int json_write_char_element(DYNBUF *b, const char *name, const char value, st_JSON_WRITER_STATE_t *state)
{
    int ret = OTTO_SUCCESS;
    bprintf(b, "%.*s\"%s\": \"%c\",\n", state->indent_level * JSON_TAB_STOP, JSON_TAB_CHAR, name, value);
    return ret;
}

int json_write_long_element(DYNBUF *b, const char *name, int64_t value, st_JSON_WRITER_STATE_t *state)
{
    int ret = OTTO_SUCCESS;
    // Emit numeric value (unquoted)
    bprintf(b, "%.*s\"%s\": %" PRId64 ",\n", state->indent_level * JSON_TAB_STOP, JSON_TAB_CHAR, name, value);
    return ret;
}

int buffer_json(DYNBUF *b, JOBLIST *joblist)
{
    int ret = OTTO_SUCCESS;

    if (joblist == NULL)
        ret = OTTO_FAIL;

    st_JSON_WRITER_STATE_t state;
    memset(&state, 0, sizeof(state));
    state.indent_level = 0;
    state.box_top = NULL;

    bprintf(b, "{\n");
    // Add a schema version to aid future compatibility
    bprintf(b, "%.*s\"schema_version\": %d,\n", state.indent_level * JSON_TAB_STOP, JSON_TAB_CHAR, 2);
    ret = json_open_array(b, "jobs", &state);
    if (ret == OTTO_SUCCESS)
    {
        for (int i = 0; i < joblist->nitems; i++)
        {
            job = &joblist->item[i];

            while (state.box_top != NULL && strcmp(state.box_top->name, job->box_name) != 0)
            {
                json_close_box_tag(b, job, &state);
            }

            switch (job->type)
            {
            case OTTO_BOX:
                json_open_box_tag(b, job, &state);
                break;

            case OTTO_CMD:
                json_write_cmd_tag(b, job, &state);
                break;

            default:
                break;
            }
        }
    }

    while (state.box_top != NULL)
    {
        json_close_box_tag(b, NULL, &state);
    }

    json_close_array(b, &state);
    // Trim any trailing comma after the last property before final close
    json_trim_trailing_comma(b);
    bprintf(b, "}\n");

    return ret;
}