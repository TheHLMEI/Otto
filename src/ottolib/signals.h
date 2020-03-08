#ifndef _SIGNALS_H_
#define _SIGNALS_H_

void  init_signals(void (* func)(int));
int   sig_set(int num, void (* func)(int));
int   sig_hold(int num);
int   sig_rlse(int num);
char *strsignal(int signo);

#endif
//
// vim: ts=3 sw=3 ai
//
