#ifndef _OTTOSIGNAL_H_
#define _OTTOSIGNAL_H_

int init_signals(void (* func)(int));
int sig_set(int num, void (* func)(int));
int sig_hold(int num);
int sig_rlse(int num);
char * strsignal(int signo);

#endif
//
// vim: ts=3 sw=3 ai
//
