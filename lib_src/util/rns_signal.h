/**************************************************************************
 * Copyright (c) 2012-2020, YoonGoo TECHNOLOGIES (SHENZHEN) LTD.
 * File        : rns_signal.h
 * Version     : YMS 1.0.0
 * Description : -

 * modification history
 * ------------------------
 * author : guoyao 2017-03-13 14:56:30
 * -------------------------
**************************************************************************/

#ifndef _RNS_SIGNAL_H_
#define _RNS_SIGNAL_H_

#include "comm.h"
#include "rns_epoll.h"
#include <signal.h>
/*
#define SIGHUP		 1
#define SIGINT		 2
#define SIGQUIT		 3
#define SIGILL		 4
#define SIGTRAP		 5
#define SIGABRT		 6
#define SIGIOT		 6
#define SIGBUS		 7
#define SIGFPE		 8
#define SIGKILL		 9
#define SIGUSR1		10
#define SIGSEGV		11
#define SIGUSR2		12
#define SIGPIPE		13
#define SIGALRM		14
#define SIGTERM		15
#define SIGSTKFLT	16
#define SIGCHLD		17
#define SIGCONT		18
#define SIGSTOP		19
#define SIGTSTP		20
#define SIGTTIN		21
#define SIGTTOU		22
#define SIGURG		23
#define SIGXCPU		24
#define SIGXFSZ		25
#define SIGVTALRM	26
#define SIGPROF		27
#define SIGWINCH	28
#define SIGIO		29
#define SIGPOLL		SIGIO

#define SIGPWR		30
#define SIGSYS		31
#define	SIGUNUSED	31
*/

typedef struct rns_sgn_s
{
    rns_epoll_cb_t ecb;
    int32_t fd;
    sigset_t mask;  
}rns_sgn_t;


rns_sgn_t* rns_sgn_create(uint32_t sid);
void rns_sgn_destroy(rns_sgn_t** sgn);

#endif

