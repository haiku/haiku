/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Akidau
 */

//! Base thread class for threads spawned and managed by the registrar


#include "RegistrarThread.h"
#include <string.h>


/*!	\class RegistrarThread
	\brief Base thread class for threads spawned and managed by the registrar
	
*/

// constructor
/*! \brief Creates a new RegistrarThread object, spawning the object's
	thread.
	
	Call Run() to actually get the thread running.
	
	\param name The desired name of the new thread
	\param priority The desired priority of the new thread
	\param managerMessenger A BMessenger to the thread manager to which this
	                        thread does or will belong.
*/
RegistrarThread::RegistrarThread(const char *name, int32 priority, BMessenger managerMessenger)
	: fManagerMessenger(managerMessenger)
	, fShouldExit(false)
	, fIsFinished(false)
	, fStatus(B_NO_INIT)
	, fId(-1)
	, fPriority(priority)
{
	fName[0] = 0;
	status_t err = name && fManagerMessenger.IsValid() ? B_OK : B_BAD_VALUE;
	if (err == B_OK)
		strcpy(fName, name);
	fStatus = err;
}

// destructor
/*! \brief Destroys the RegistrarThread object
*/
RegistrarThread::~RegistrarThread()
{
}

// InitCheck()
/*! \brief Returns the initialization status of the object
*/
status_t
RegistrarThread::InitCheck()
{
	return fStatus;
}

// Run
/*! \brief Begins executing the thread's ThreadFunction()
*/
status_t
RegistrarThread::Run()
{
	status_t err = InitCheck();
	if (err == B_OK) {
		fId = spawn_thread(&RegistrarThread::EntryFunction, fName,
			fPriority, (void*)this);
		err = fId >= 0 ? B_OK : fId;
		if (err == B_OK)	
			err = resume_thread(fId);
	}
	return err;
}

// Id
//! Returns the thread id
thread_id
RegistrarThread::Id() const
{
	return fId;
}

// Name
//! Returns the name of the thread 
const char*
RegistrarThread::Name() const
{
	return fName;
}
	
// AskToExit
/*! \brief Signals to thread that it needs to quit politely as soon
	as possible.
*/
void
RegistrarThread::AskToExit()
{
	fShouldExit = true;
}

// IsFinished
/*! \brief Returns \c true if the thread has finished executing
*/
bool
RegistrarThread::IsFinished() const
{
	return fIsFinished;
}

// EntryFunction
/*! \brief This is the function supplied to spawn_thread. It simply calls
	ThreadFunction() on the \a data parameter, which is assumed to be a pointer
	to a RegistrarThread object.
*/
int32
RegistrarThread::EntryFunction(void *data)
{
	return ((RegistrarThread*)data)->ThreadFunction();
}
