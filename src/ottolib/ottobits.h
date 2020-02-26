#ifndef _OTTOBITS_H_
#define _OTTOBITS_H_

#include <stdint.h>

#define OTTO_VERSION "v2.0.0"

// general defines
#define OTTO_UNKNOWN   0

#define OTTO_FALSE     0
#define OTTO_TRUE      1

#define OTTO_SUCCESS   0
#define OTTO_FAIL     -1

#define OTTO_QUIET     0
#define OTTO_VERBOSE   1

#define OTTO_SERVER    0
#define OTTO_CLIENT    1

#define OTTO_SUN      (1L)
#define OTTO_MON   (1L<<1)
#define OTTO_TUE   (1L<<2)
#define OTTO_WED   (1L<<3)
#define OTTO_THU   (1L<<4)
#define OTTO_FRI   (1L<<5)
#define OTTO_SAT   (1L<<6)
#define OTTO_ALL   (OTTO_SUN | OTTO_MON | OTTO_TUE | OTTO_WED | OTTO_THU | OTTO_FRI | OTTO_SAT)

#define OTTO_BOX  'b'
#define OTTO_CMD  'c'

#define OTTO_USE_STARTTIMES 1
#define OTTO_USE_STARTMINS  2

#define i64 int64_t
#define cde8int(a) ((i64)a[0]<<56|(i64)a[1]<<48|(i64)a[2]<<40|(i64)a[3]<<32|a[4]<<24|a[5]<<16|a[6]<<8|a[7])

#endif
//
// vim: ts=3 sw=3 ai
//
