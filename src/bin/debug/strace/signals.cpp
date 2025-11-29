/*
 * Copyright 2023, Trung Nguyen, trungnt282910@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>

#include <string>

#include "signals.h"
#include "strace.h"
#include "Context.h"
#include "MemoryReader.h"
#include "Syscall.h"
#include "TypeHandler.h"


struct enum_info {
	int index;
	const char *name;
};

#define ENUM_INFO_ENTRY(name) \
	{ name, #name }

static const enum_info kSignals[] = {
	{ 0, "SIG0"}, 					/* 0 */
	ENUM_INFO_ENTRY(SIGHUP),
	ENUM_INFO_ENTRY(SIGINT),
	ENUM_INFO_ENTRY(SIGQUIT),
	ENUM_INFO_ENTRY(SIGILL),
	ENUM_INFO_ENTRY(SIGCHLD),
	ENUM_INFO_ENTRY(SIGABRT),
	ENUM_INFO_ENTRY(SIGPIPE),
	ENUM_INFO_ENTRY(SIGFPE),
	ENUM_INFO_ENTRY(SIGKILL),
	ENUM_INFO_ENTRY(SIGSTOP),	/* 10 */
	ENUM_INFO_ENTRY(SIGSEGV),
	ENUM_INFO_ENTRY(SIGCONT),
	ENUM_INFO_ENTRY(SIGTSTP),
	ENUM_INFO_ENTRY(SIGALRM),
	ENUM_INFO_ENTRY(SIGTERM),
	ENUM_INFO_ENTRY(SIGTTIN),
	ENUM_INFO_ENTRY(SIGTTOU),
	ENUM_INFO_ENTRY(SIGUSR1),
	ENUM_INFO_ENTRY(SIGUSR2),
	ENUM_INFO_ENTRY(SIGWINCH),	/* 20 */
	ENUM_INFO_ENTRY(SIGKILLTHR),
	ENUM_INFO_ENTRY(SIGTRAP),
	ENUM_INFO_ENTRY(SIGPOLL),
	ENUM_INFO_ENTRY(SIGPROF),
	ENUM_INFO_ENTRY(SIGSYS),
	ENUM_INFO_ENTRY(SIGURG),
	ENUM_INFO_ENTRY(SIGVTALRM),
	ENUM_INFO_ENTRY(SIGXCPU),
	ENUM_INFO_ENTRY(SIGXFSZ),
	ENUM_INFO_ENTRY(SIGBUS),	/* 30 */
	ENUM_INFO_ENTRY(SIGRESERVED1),
	ENUM_INFO_ENTRY(SIGRESERVED2),
	{ 0, NULL }
};

static EnumTypeHandler::EnumMap kSignalsMap;


static const enum_info kSigmaskHow[] = {
	ENUM_INFO_ENTRY(SIG_BLOCK),
	ENUM_INFO_ENTRY(SIG_UNBLOCK),
	ENUM_INFO_ENTRY(SIG_SETMASK),

	{ 0, NULL }
};


#define sigmask(sig) (1 << ((sig) - 1))
#define FLAG_INFO_ENTRY(name) \
	{ sigmask( SIG##name ), #name }

static const FlagsTypeHandler::FlagInfo kSigmaskFlagsInfo[] = {
	FLAG_INFO_ENTRY(HUP),
	FLAG_INFO_ENTRY(INT),
	FLAG_INFO_ENTRY(QUIT),
	FLAG_INFO_ENTRY(ILL),
	FLAG_INFO_ENTRY(CHLD),
	FLAG_INFO_ENTRY(ABRT),
	FLAG_INFO_ENTRY(PIPE),
	FLAG_INFO_ENTRY(FPE),
	FLAG_INFO_ENTRY(KILL),
	FLAG_INFO_ENTRY(STOP),
	FLAG_INFO_ENTRY(SEGV),
	FLAG_INFO_ENTRY(CONT),
	FLAG_INFO_ENTRY(TSTP),
	FLAG_INFO_ENTRY(ALRM),
	FLAG_INFO_ENTRY(TERM),
	FLAG_INFO_ENTRY(TTIN),
	FLAG_INFO_ENTRY(TTOU),
	FLAG_INFO_ENTRY(USR1),
	FLAG_INFO_ENTRY(USR2),
	FLAG_INFO_ENTRY(WINCH),
	FLAG_INFO_ENTRY(KILLTHR),
	FLAG_INFO_ENTRY(TRAP),
	FLAG_INFO_ENTRY(POLL),
	FLAG_INFO_ENTRY(PROF),
	FLAG_INFO_ENTRY(SYS),
	FLAG_INFO_ENTRY(URG),
	FLAG_INFO_ENTRY(VTALRM),
	FLAG_INFO_ENTRY(XCPU),
	FLAG_INFO_ENTRY(XFSZ),
	FLAG_INFO_ENTRY(BUS),

	{ 0, NULL }
};


static FlagsTypeHandler::FlagsList kSigmaskFlags;
static EnumTypeHandler::EnumMap kSigmaskHowMap;


std::string
signal_name(int signal)
{
	EnumTypeHandler::EnumMap::const_iterator i = kSignalsMap.find(signal);
	if (i != kSignalsMap.end() && i->second != NULL)
		return i->second;

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


class SigsetTypeHandler : public FlagsTypeHandler {
public:
	SigsetTypeHandler()
		:
		FlagsTypeHandler(kSigmaskFlags)
	{
	}

	string GetParameterValue(Context &context, Parameter *param,
		const void *address)
	{
		void *data = *(void **)address;

		if (context.GetContents(Context::POINTER_VALUES)) {
			int32 bytesRead;
			sigset_t value;
			status_t err = context.Reader().Read(data, &value, sizeof(value), bytesRead);
			if (err != B_OK)
				return context.FormatPointer(data);

			uint32 count = 0;
#if __GNUC__ > 2
                count += __builtin_popcount(value);
#else
			{
				sigset_t mask = value;
				while (mask > 0) {
					if ((mask & 1) == 1)
						count++;
					mask >>= 1;
				}
			}
#endif
			string r;
			// when more than 2/3 of the signals are set, inverse
			if (count > SIGRESERVED2 * 2 / 3) {
				value = ~value;
				r += "~";
			}
			value &= (((sigset_t)1 << SIGRESERVED2) - 1);

			r += "[";
			r += RenderValue(context, value);
			r += "]";
			return r;
		}

		return context.FormatPointer(data);
	}

	string GetReturnValue(Context &context, uint64 value)
	{
		return context.FormatPointer((void *)value);
	}
};


void
patch_signal()
{
	for (int i = 0; kSigmaskFlagsInfo[i].name != NULL; i++)
		kSigmaskFlags.push_back(kSigmaskFlagsInfo[i]);
	for (int i = 0; kSigmaskHow[i].name != NULL; i++)
		kSigmaskHowMap[kSigmaskHow[i].index] = kSigmaskHow[i].name;
	for (int i = 0; kSignals[i].name != NULL; i++)
		kSignalsMap[kSignals[i].index] = kSignals[i].name;
	Syscall *setSignalMask = get_syscall("_kern_set_signal_mask");
	setSignalMask->GetParameter("how")->SetHandler(new EnumTypeHandler(kSigmaskHowMap));
	setSignalMask->GetParameter("set")->SetHandler(new SigsetTypeHandler());
	setSignalMask->GetParameter("oldSet")->SetHandler(new SigsetTypeHandler());
	setSignalMask->GetParameter("oldSet")->SetOut(true);

	Syscall *sigwait = get_syscall("_kern_sigwait");
	sigwait->GetParameter("set")->SetHandler(new SigsetTypeHandler());
	sigwait->GetParameter("info")->SetOut(true);

	Syscall *sigsuspend = get_syscall("_kern_sigsuspend");
	sigsuspend->GetParameter("mask")->SetHandler(new SigsetTypeHandler());

	Syscall *sigpending = get_syscall("_kern_sigpending");
	sigpending->GetParameter("set")->SetHandler(new SigsetTypeHandler());
	sigpending->GetParameter("set")->SetOut(true);

	Syscall *sendSignal = get_syscall("_kern_send_signal");
	sendSignal->GetParameter("signal")->SetHandler(new EnumTypeHandler(kSignalsMap));
	Syscall *sigaction = get_syscall("_kern_sigaction");
	sigaction->GetParameter("sig")->SetHandler(new EnumTypeHandler(kSignalsMap));
}
