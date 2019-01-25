/*
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SIGNAL_SET_H
#define SIGNAL_SET_H


#include <signal.h>


class SignalSet {
public:
								SignalSet();
								SignalSet(int signal);
								SignalSet(const sigset_t& signals);

			void				SetTo(const sigset_t& signals);
			void				SetTo(int signal);

			const sigset_t&		Signals() const	{ return fSignals; }

			bool				ContainsSignal(int signal) const;

			SignalSet&			AddSignal(int signal);
			SignalSet&			AddSignals(const SignalSet& signals);
			SignalSet&			RemoveSignal(int signal);
			SignalSet&			RemoveSignals(const SignalSet& signals);

			status_t			BlockInCurrentThread(
									SignalSet* oldMask = NULL) const;
			status_t			UnblockInCurrentThread(
									SignalSet* oldMask = NULL) const;
			status_t			SetCurrentThreadSignalMask(
									SignalSet* oldMask = NULL) const;

	static	SignalSet			CurrentThreadSignalMask();

private:
			sigset_t			fSignals;
};


SignalSet::SignalSet()
{
	sigemptyset(&fSignals);
}


SignalSet::SignalSet(int signal)
{
	SetTo(signal);
}


SignalSet::SignalSet(const sigset_t& signals)
	:
	fSignals(signals)
{
}


void
SignalSet::SetTo(const sigset_t& signals)
{
	fSignals = signals;
}


void
SignalSet::SetTo(int signal)
{
	sigemptyset(&fSignals);
	sigaddset(&fSignals, signal);
}


bool
SignalSet::ContainsSignal(int signal) const
{
	return sigismember(&fSignals, signal) != 0;
}


SignalSet&
SignalSet::AddSignal(int signal)
{
	sigaddset(&fSignals, signal);
	return *this;
}


SignalSet&
SignalSet::AddSignals(const SignalSet& signals)
{
	// NOTE: That is not portable.
	fSignals |= signals.fSignals;
	return *this;
}


SignalSet&
SignalSet::RemoveSignal(int signal)
{
	sigdelset(&fSignals, signal);
	return *this;
}


SignalSet&
SignalSet::RemoveSignals(const SignalSet& signals)
{
	// NOTE: That is not portable.
	fSignals &= ~signals.fSignals;
	return *this;
}


status_t
SignalSet::BlockInCurrentThread(SignalSet* oldMask) const
{
	return pthread_sigmask(SIG_BLOCK, &fSignals,
		oldMask != NULL ? &oldMask->fSignals : NULL);
}


status_t
SignalSet::UnblockInCurrentThread(SignalSet* oldMask) const
{
	return pthread_sigmask(SIG_UNBLOCK, &fSignals,
		oldMask != NULL ? &oldMask->fSignals : NULL);
}


status_t
SignalSet::SetCurrentThreadSignalMask(SignalSet* oldMask) const
{
	return pthread_sigmask(SIG_SETMASK, &fSignals,
		oldMask != NULL ? &oldMask->fSignals : NULL);
}


/*static*/ SignalSet
SignalSet::CurrentThreadSignalMask()
{
	SignalSet signals;
	pthread_sigmask(SIG_BLOCK, NULL, &signals.fSignals);
	return signals;
}


#endif	// SIGNAL_SET_H
