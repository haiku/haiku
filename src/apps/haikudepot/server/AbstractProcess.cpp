/*
 * Copyright 2018-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "AbstractProcess.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <AutoDeleter.h>
#include <AutoLocker.h>
#include <Locker.h>

#include "HaikuDepotConstants.h"
#include "Logger.h"


AbstractProcess::AbstractProcess()
	:
	fLock(),
	fListener(NULL),
	fWasStopped(false),
	fProcessState(PROCESS_INITIAL),
	fErrorStatus(B_OK)
{
}


AbstractProcess::~AbstractProcess()
{
}


void
AbstractProcess::SetListener(AbstractProcessListener* listener)
{
	AutoLocker<BLocker> locker(&fLock);
	fListener = BReference<AbstractProcessListener>(listener);
}


status_t
AbstractProcess::Run()
{
	{
		AutoLocker<BLocker> locker(&fLock);

		if (ProcessState() != PROCESS_INITIAL) {
			HDINFO("cannot start process as it is not idle");
			return B_NOT_ALLOWED;
		}

		if (fWasStopped) {
			HDINFO("cannot start process as it was stopped");
			return B_CANCELED;
		}

		fProcessState = PROCESS_RUNNING;
	}

	status_t runResult = RunInternal();

	if (runResult != B_OK)
		HDERROR("[%s] an error has arisen; %s", Name(), strerror(runResult));

	BReference<AbstractProcessListener> listener;

	{
		AutoLocker<BLocker> locker(&fLock);
		fProcessState = PROCESS_COMPLETE;
		fErrorStatus = runResult;
		listener = fListener;
	}

	// this process may be part of a larger bulk-load process and
	// if so, the process orchestration needs to know when this
	// process has completed.
	if (listener.Get() != NULL)
		listener->ProcessExited();

	return runResult;
}


bool
AbstractProcess::WasStopped()
{
	AutoLocker<BLocker> locker(&fLock);
	return fWasStopped;
}


status_t
AbstractProcess::ErrorStatus()
{
	AutoLocker<BLocker> locker(&fLock);
	return fErrorStatus;
}


/*! This method will stop the process.  The actual process may carry on to
    perform some tidy-ups on its thread so this does not stop the thread or
    change the state of the process; just indicates to the running thread that
    it should stop.  If it has not yet been started then it will be put into
    finished state.
*/

status_t
AbstractProcess::Stop()
{
	status_t result = B_CANCELED;
    BReference<AbstractProcessListener> listener = NULL;

	{
		AutoLocker<BLocker> locker(&fLock);

		if (!fWasStopped) {
			fWasStopped = true;
			result = StopInternal();

			if (fProcessState == PROCESS_INITIAL) {
				listener = fListener;
				fProcessState = PROCESS_COMPLETE;
			}
		}
	}

	if (listener.Get() != NULL)
		listener->ProcessExited();

	return result;
}


status_t
AbstractProcess::StopInternal()
{
	return B_NOT_ALLOWED;
}


bool
AbstractProcess::IsRunning()
{
	return ProcessState() == PROCESS_RUNNING;
}


process_state
AbstractProcess::ProcessState()
{
	AutoLocker<BLocker> locker(&fLock);
	return fProcessState;
}