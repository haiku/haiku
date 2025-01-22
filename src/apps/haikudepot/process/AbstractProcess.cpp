/*
 * Copyright 2018-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "AbstractProcess.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <AutoDeleter.h>
#include <AutoLocker.h>
#include <Locker.h>
#include <StopWatch.h>

#include "HaikuDepotConstants.h"
#include "Logger.h"
#include "ProcessListener.h"


static const bigtime_t kProcessUpdateMinimumDelay = 250000;


AbstractProcess::AbstractProcess()
	:
	fLock(),
	fListener(NULL),
	fWasStopped(false),
	fProcessState(PROCESS_INITIAL),
	fErrorStatus(B_OK),
	fDurationSeconds(0.0),
	fLastProgressUpdate(0)
{
}


AbstractProcess::~AbstractProcess()
{
}


void
AbstractProcess::SetListener(ProcessListener* listener)
{
	if (fListener != listener) {
		AutoLocker<BLocker> locker(&fLock);
		fListener = listener;
	}
}


status_t
AbstractProcess::Run()
{
	ProcessListener* listener;

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
		listener = fListener;
	}

	if (listener != NULL)
		listener->ProcessChanged();

	BStopWatch stopWatch("process", true);
	status_t runResult = RunInternal();
	fDurationSeconds = static_cast<double>(stopWatch.ElapsedTime()) / 1000000.0;

	if (runResult != B_OK)
		HDERROR("[%s] an error has arisen; %s", Name(), strerror(runResult));

	{
		AutoLocker<BLocker> locker(&fLock);
		fProcessState = PROCESS_COMPLETE;
		fErrorStatus = runResult;
	}

	// this process may be part of a larger bulk-load process and
	// if so, the process orchestration needs to know when this
	// process has completed.
	if (listener != NULL)
		listener->ProcessChanged();

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


/*!	This method will stop the process.  The actual process may carry on to
	perform some tidy-ups on its thread so this does not stop the thread or
	change the state of the process; just indicates to the running thread that
	it should stop.  If it has not yet been started then it will be put into
	finished state.
*/

status_t
AbstractProcess::Stop()
{
	status_t result = B_CANCELED;
	ProcessListener* listener = NULL;

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

	if (listener != NULL)
		listener->ProcessChanged();

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


float
AbstractProcess::Progress()
{
	return kProgressIndeterminate;
}


void
AbstractProcess::_NotifyChanged()
{
	ProcessListener* listener = NULL;
	{
		AutoLocker<BLocker> locker(&fLock);
		listener = fListener;
	}
	if (listener != NULL)
		listener->ProcessChanged();
}


BString
AbstractProcess::LogReport()
{
	BString result;
	AutoLocker<BLocker> locker(&fLock);
	result.SetToFormat("%s [%c] %6.3f", Name(), _ProcessStateIdentifier(ProcessState()),
		fDurationSeconds);
	return result;
}


/*!	This method should be called before processing any progress. It will prevent
	progress updates from coming through too frequently.
*/
bool
AbstractProcess::_ShouldProcessProgress()
{
	bigtime_t now = system_time();

	if (now - fLastProgressUpdate < kProcessUpdateMinimumDelay)
		return false;

	fLastProgressUpdate = now;
	return true;
}


/*static*/ char
AbstractProcess::_ProcessStateIdentifier(process_state value)
{
	switch (value) {
		case PROCESS_INITIAL:
			return 'I';
		case PROCESS_RUNNING:
			return 'R';
		case PROCESS_COMPLETE:
			return 'C';
		default:
			return '?';
	}
}
