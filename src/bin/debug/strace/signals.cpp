/*
 * Copyright 2023, Trung Nguyen, trungnt282910@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>

#include <string>

#include "signals.h"


// signal names
static const char *kSignalName[] = {
	/*  0 */ "SIG0",
	/*  1 */ "SIGHUP",
	/*  2 */ "SIGINT",
	/*  3 */ "SIGQUIT",
	/*  4 */ "SIGILL",
	/*  5 */ "SIGCHLD",
	/*  6 */ "SIGABRT",
	/*  7 */ "SIGPIPE",
	/*  8 */ "SIGFPE",
	/*  9 */ "SIGKILL",
	/* 10 */ "SIGSTOP",
	/* 11 */ "SIGSEGV",
	/* 12 */ "SIGCONT",
	/* 13 */ "SIGTSTP",
	/* 14 */ "SIGALRM",
	/* 15 */ "SIGTERM",
	/* 16 */ "SIGTTIN",
	/* 17 */ "SIGTTOU",
	/* 18 */ "SIGUSR1",
	/* 19 */ "SIGUSR2",
	/* 20 */ "SIGWINCH",
	/* 21 */ "SIGKILLTHR",
	/* 22 */ "SIGTRAP",
	/* 23 */ "SIGPOLL",
	/* 24 */ "SIGPROF",
	/* 25 */ "SIGSYS",
	/* 26 */ "SIGURG",
	/* 27 */ "SIGVTALRM",
	/* 28 */ "SIGXCPU",
	/* 29 */ "SIGXFSZ",
	/* 30 */ "SIGBUS",
	/* 31 */ "SIGRESERVED1",
	/* 32 */ "SIGRESERVED2",
};


std::string
signal_name(int signal)
{
	if (signal >= 0 && signal <= SIGRESERVED2)
		return kSignalName[signal];

	static char buffer[32];
	sprintf(buffer, "%d", signal);
	return buffer;
}


static const char *
signal_code(int signal, int code)
{
#define CASE(X) case X: return #X;
	switch (code) {
		CASE(SI_USER)
		CASE(SI_QUEUE)
		CASE(SI_TIMER)
		CASE(SI_ASYNCIO)
		CASE(SI_MESGQ)
	}

	switch (signal) {
		case SIGILL:
			switch (code) {
				CASE(ILL_ILLOPC)
				CASE(ILL_ILLOPN)
				CASE(ILL_ILLADR)
				CASE(ILL_ILLTRP)
				CASE(ILL_PRVOPC)
				CASE(ILL_PRVREG)
				CASE(ILL_COPROC)
				CASE(ILL_BADSTK)
			}
			break;
		case SIGFPE:
			switch (code) {
				CASE(FPE_INTDIV)
				CASE(FPE_INTOVF)
				CASE(FPE_FLTDIV)
				CASE(FPE_FLTOVF)
				CASE(FPE_FLTUND)
				CASE(FPE_FLTRES)
				CASE(FPE_FLTINV)
				CASE(FPE_FLTSUB)
			}
			break;
		case SIGSEGV:
			switch (code) {
				CASE(SEGV_MAPERR)
				CASE(SEGV_ACCERR)
			}
			break;
		case SIGBUS:
			switch (code) {
				CASE(BUS_ADRALN)
				CASE(BUS_ADRERR)
				CASE(BUS_OBJERR)
			}
			break;
		case SIGTRAP:
			switch (code) {
				CASE(TRAP_BRKPT)
				CASE(TRAP_TRACE)
			}
			break;
		case SIGCHLD:
			switch (code) {
				CASE(CLD_EXITED)
				CASE(CLD_KILLED)
				CASE(CLD_DUMPED)
				CASE(CLD_TRAPPED)
				CASE(CLD_STOPPED)
				CASE(CLD_CONTINUED)
			}
			break;
		case SIGPOLL:
			switch (code) {
				CASE(POLL_IN)
				CASE(POLL_OUT)
				CASE(POLL_MSG)
				CASE(POLL_ERR)
				CASE(POLL_PRI)
				CASE(POLL_HUP)
			}
			break;
	}
#undef CASE

	static char buffer[32];
	sprintf(buffer, "%d", code);
	return buffer;
}


std::string
signal_info(siginfo_t& info)
{
	static char buffer[32];
	std::string string;

	string.reserve(256);

	string += "{";
	string += "si_signo=";
	string += signal_name(info.si_signo);
	string += ", si_code=";
	string += signal_code(info.si_signo, info.si_code);

	if (info.si_errno != 0) {
		string += ", si_errno=0x";
		sprintf(buffer, "%x", info.si_errno);
		string += buffer;
	}

	string += ", si_pid=";
	sprintf(buffer, "%d", (int)info.si_pid);
	string += buffer;
	string += ", si_uid=";
	sprintf(buffer, "%d", (int)info.si_uid);
	string += buffer;

	if (info.si_signo == SIGILL || info.si_signo == SIGFPE || info.si_signo == SIGSEGV
		|| info.si_signo == SIGBUS || info.si_signo == SIGTRAP) {
		string += ", si_addr=";
		sprintf(buffer, "%p", info.si_addr);
		string += buffer;
	}

	if (info.si_signo == SIGCHLD) {
		string += ", si_status=";
		sprintf(buffer, "%d", info.si_status);
		string += buffer;
	}

	if (info.si_signo == SIGPOLL) {
		string += ", si_band=";
		sprintf(buffer, "%ld", info.si_band);
		string += buffer;
	}

	string += ", si_value=";
	sprintf(buffer, "%p", info.si_value.sival_ptr);
	string += buffer;
	string += "}";

	return string;
}
