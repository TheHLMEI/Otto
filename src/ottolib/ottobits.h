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

#define OTTO_USE_START_TIMES   1
#define OTTO_USE_START_MINUTES 2

#define i64 int64_t
#define cde8int(a) ((i64)a[0]<<56|(i64)a[1]<<48|(i64)a[2]<<40|(i64)a[3]<<32|a[4]<<24|a[5]<<16|a[6]<<8|a[7])

#define cdeSU______     ((i64)'S'<<56 | (i64)'U'<<48 | (i64)' '<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeMO______     ((i64)'M'<<56 | (i64)'O'<<48 | (i64)' '<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeTU______     ((i64)'T'<<56 | (i64)'U'<<48 | (i64)' '<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeWE______     ((i64)'W'<<56 | (i64)'E'<<48 | (i64)' '<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeTH______     ((i64)'T'<<56 | (i64)'H'<<48 | (i64)' '<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeFR______     ((i64)'F'<<56 | (i64)'R'<<48 | (i64)' '<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeSA______     ((i64)'S'<<56 | (i64)'A'<<48 | (i64)' '<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')
#define cdeALL_____     ((i64)'A'<<56 | (i64)'L'<<48 | (i64)'L'<<40 | (i64)' '<<32 | ' '<<24 | ' '<<16 | ' '<<8 | ' ')

#define DTECOND_BIT      (1L)
#define DYSOFWK_BIT (1L << 1)
#define STRTMNS_BIT (1L << 2)
#define STRTTMS_BIT (1L << 3)

#define HAS_TYPE                 (1L)
#define HAS_BOX_NAME        (1L << 1) 
#define HAS_DESCRIPTION     (1L << 2)
#define HAS_COMMAND         (1L << 3)
#define HAS_CONDITION       (1L << 4)
#define HAS_DATE_CONDITIONS (1L << 5)
#define HAS_AUTO_HOLD       (1L << 6)

#endif
//
// vim: ts=3 sw=3 ai
//
