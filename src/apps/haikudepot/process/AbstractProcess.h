/*
 * Copyright 2018-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef ABSTRACT_PROCESS_H
#define ABSTRACT_PROCESS_H

#include <String.h>
#include <Url.h>

#include "StandardMetaData.h"
#include "Stoppable.h"


typedef enum process_state {
	PROCESS_INITIAL			= 1 << 0,
	PROCESS_RUNNING			= 1 << 1,
	PROCESS_COMPLETE		= 1 << 2
} process_state;


class ProcessListener;


/*! This is the superclass of all Processes. */

class AbstractProcess : public Stoppable {
public:
								AbstractProcess();
	virtual						~AbstractProcess();

	virtual	const char*			Name() const = 0;
	virtual	const char*			Description() const = 0;
	virtual float				Progress();
			status_t			Run();
			status_t			Stop();
			status_t			ErrorStatus();
			bool				IsRunning();
			bool				WasStopped();
			process_state		ProcessState();

	virtual	BString				LogReport();

			void				SetListener(ProcessListener* listener);

protected:
	virtual	status_t			RunInternal() = 0;
	virtual	status_t			StopInternal();
			void				_NotifyChanged();

	static	char				_ProcessStateIdentifier(process_state value);

			bool				_ShouldProcessProgress();

protected:
			BLocker				fLock;

private:
			ProcessListener*	fListener;
			bool				fWasStopped;
			process_state		fProcessState;
			status_t			fErrorStatus;
			double				fDurationSeconds;
			bigtime_t			fLastProgressUpdate;
};

#endif // ABSTRACT_PROCESS_H
