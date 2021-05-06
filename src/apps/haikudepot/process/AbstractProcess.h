/*
 * Copyright 2018-2021, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef ABSTRACT_PROCESS_H
#define ABSTRACT_PROCESS_H

#include <String.h>
#include <Referenceable.h>
#include <Url.h>

#include "StandardMetaData.h"
#include "Stoppable.h"


typedef enum process_state {
	PROCESS_INITIAL			= 1 << 0,
	PROCESS_RUNNING			= 1 << 1,
	PROCESS_COMPLETE		= 1 << 2
} process_state;


/*! Clients are able to subclass from this 'interface' in order to accept
    call-backs when a process has exited; either through success or through
    failure.
 */

class AbstractProcessListener : public BReferenceable {
public:
	virtual	void				ProcessChanged() = 0;
};


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
			void				SetListener(AbstractProcessListener* listener);

protected:
	virtual	status_t			RunInternal() = 0;
	virtual	status_t			StopInternal();
			void				_NotifyChanged();

protected:
			BLocker				fLock;

private:
			BReference<AbstractProcessListener>
								fListener;
			bool				fWasStopped;
			process_state		fProcessState;
			status_t			fErrorStatus;
};

#endif // ABSTRACT_PROCESS_H
