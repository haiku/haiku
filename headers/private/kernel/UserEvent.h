/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_USER_EVENT_H
#define _KERNEL_USER_EVENT_H


#include <signal.h>

#include <SupportDefs.h>

#include <DPC.h>
#include <thread.h>


namespace BKernel {


struct Team;
struct Thread;


struct UserEvent {
	virtual						~UserEvent();

	virtual	status_t			Fire() = 0;
};


struct SignalEvent : UserEvent {
	virtual						~SignalEvent();

			void				SetUserValue(union sigval userValue);

protected:
			struct EventSignal;

protected:
								SignalEvent(EventSignal* signal);

protected:
			EventSignal*		fSignal;
};


struct TeamSignalEvent : SignalEvent {
	static	TeamSignalEvent*	Create(Team* team, uint32 signalNumber,
									int32 signalCode, int32 errorCode);

	virtual	status_t			Fire();

private:
								TeamSignalEvent(Team* team,
									EventSignal* signal);

private:
			Team*				fTeam;
};


struct ThreadSignalEvent : SignalEvent {
	static	ThreadSignalEvent*	Create(Thread* thread, uint32 signalNumber,
									int32 signalCode, int32 errorCode,
									pid_t sendingTeam);

	virtual	status_t			Fire();

private:
								ThreadSignalEvent(Thread* thread,
									EventSignal* signal);

private:
			Thread*				fThread;
};


struct CreateThreadEvent : UserEvent, private DPCCallback {
								~CreateThreadEvent();

	static	CreateThreadEvent*	Create(
									const ThreadCreationAttributes& attributes);

	virtual	status_t			Fire();

private:
								CreateThreadEvent(
									const ThreadCreationAttributes& attributes);

	virtual	void				DoDPC(DPCQueue* queue);

private:
			ThreadCreationAttributes fCreationAttributes;
			char				fThreadName[B_OS_NAME_LENGTH];
			bool				fPendingDPC;
};


}	// namespace BKernel


using BKernel::CreateThreadEvent;
using BKernel::SignalEvent;
using BKernel::TeamSignalEvent;
using BKernel::ThreadSignalEvent;
using BKernel::UserEvent;


#endif	// _KERNEL_USER_EVENT_H
