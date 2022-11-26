#include <signal.h>
#include <stdio.h>

#include "otto.h"

/*--------------------------- signal support functions ---------------------------*/
void
init_signals(void (* func)(int))
{
	sig_set(SIGTERM,   func);
	sig_set(SIGHUP,    func);
	sig_set(SIGQUIT,   func);
	sig_set(SIGILL,    func);
	sig_set(SIGABRT,   func);
	sig_set(SIGFPE,    func);
	sig_set(SIGSEGV,   func);
	sig_set(SIGPIPE,   func);
	sig_set(SIGALRM,   func);
	sig_set(SIGTERM,   func);
	sig_set(SIGUSR1,   func);
	sig_set(SIGUSR2,   func);
	sig_set(SIGTRAP,   func);
	sig_set(SIGIOT,    func);
	sig_set(SIGBUS,    func);
	sig_set(SIGIO,     func);
	sig_set(SIGPOLL,   func);
	sig_set(SIGXCPU,   func);
	sig_set(SIGXFSZ,   func);
	sig_set(SIGVTALRM, func);
	sig_set(SIGPROF,   func);
	sig_set(SIGPWR,    func);
}



int
sig_set(int num, void (* func)(int))
{
	struct sigaction act;

	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, num);

	act.sa_handler = func;
	act.sa_flags   = 0;

	if(num == SIGCLD)
		act.sa_flags |= SA_NOCLDSTOP;

	return( sigaction(num, &act, NULL) );
}



int
sig_hold(int num)
{
	sigset_t set;

	sigemptyset(&set);
	sigaddset(&set, num);

	return( sigprocmask(SIG_BLOCK, &set, NULL) );
}



int
sig_rlse(int num)
{
	sigset_t set;

	sigemptyset(&set);
	sigaddset(&set, num);

	return( sigprocmask(SIG_UNBLOCK, &set, NULL) );
}



char *
strsignal(int signo)
{
	char *retval = NULL;
	static char msg[32];

	switch(signo)
	{
		case SIGHUP:        retval = "SIGHUP";        break;
		case SIGINT:        retval = "SIGINT";        break;
		case SIGQUIT:       retval = "SIGQUIT";       break;
		case SIGILL:        retval = "SIGILL";        break;
		case SIGTRAP:       retval = "SIGTRAP";       break;
		case SIGABRT:       retval = "SIGABRT";       break;
//    case SIGEMT:        retval = "SIGEMT";        break;
		case SIGFPE :       retval = "SIGFPE";        break;
		case SIGKILL:       retval = "SIGKILL";       break;
		case SIGBUS:        retval = "SIGBUS";        break;
		case SIGSEGV:       retval = "SIGSEGV";       break;
		case SIGSYS:        retval = "SIGSYS";        break;
		case SIGPIPE:       retval = "SIGPIPE";       break;
		case SIGALRM:       retval = "SIGALRM";       break;
		case SIGTERM:       retval = "SIGTERM";       break;
		case SIGUSR1:       retval = "SIGUSR1";       break;
		case SIGUSR2:       retval = "SIGUSR2";       break;
		case SIGCHLD:       retval = "SIGCHLD";       break;
		case SIGPWR:        retval = "SIGPWR";        break;
		case SIGVTALRM:     retval = "SIGVTALRM";     break;
		case SIGPROF:       retval = "SIGPROF";       break;
		case SIGIO:         retval = "SIGIO";         break;
		case SIGWINCH:      retval = "SIGWINCH";      break;
		case SIGSTOP:       retval = "SIGSTOP";       break;
		case SIGTSTP:       retval = "SIGTSTP";       break;
		case SIGCONT:       retval = "SIGCONT";       break;
		case SIGTTIN:       retval = "SIGTTIN";       break;
		case SIGTTOU:       retval = "SIGTTOU";       break;
		case SIGURG:        retval = "SIGURG";        break;
//    case SIGLOST:       retval = "SIGLOST";       break;
//    case SIGRESERVE:    retval = "SIGRESERVE";    break;
//    case SIGOBSOLETE32: retval = "SIGOBSOLETE32"; break;
		case SIGXCPU:       retval = "SIGXCPU";       break;
		case SIGXFSZ:       retval = "SIGXFSZ";       break;
//    case SIGCANCEL:     retval = "SIGCANCEL";     break;
//    case SIGGFAULT:     retval = "SIGGFAULT";     break;
//    case SIGRTMIN:      retval = "SIGRTMIN";      break;
//    case SIGRTMAX:      retval = "SIGRTMAX";      break;
		default: sprintf(msg, "%d", signo); retval = msg; break;
	}

	return(retval);
}



//
// vim: ts=3 sw=3 ai
//
