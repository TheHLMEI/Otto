#define _XOPEN_SOURCE
#include <time.h>

char *otto_strptime(char *s, char *format, struct tm *tm)
{
   tzset();
   return(strptime(s, format, tm));
}
