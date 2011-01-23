/*
 * Copyright 2003-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jonas Sundström, jonas@kirilla.com
 */


#include "GenericThread.h"

#include <string.h>


GenericThread::GenericThread(const char* thread_name, int32 priority,
	BMessage* message)	
	:
	fThreadDataStore(message),
	fThreadId(spawn_thread(_ThreadFunction, thread_name, priority, this)),
	fExecuteUnitSem(create_sem(1, "fExecuteUnitSem")),
	fQuitRequested(false),
	fThreadIsPaused(false)
{
	if (fThreadDataStore == NULL)
		fThreadDataStore = new BMessage();
}


GenericThread::~GenericThread()
{
	kill_thread(fThreadId);
	
	delete_sem(fExecuteUnitSem);
}


status_t 
GenericThread::ThreadFunction()
{
	status_t status = B_OK;
	
	status = ThreadStartup();
		// Subclass and override this function
		
	if (status != B_OK) {
		ThreadStartupFailed(status);
		return status;
		// is this the right thing to do?
	}
	
	while (1) {
		if (HasQuitBeenRequested()) {
			status = ThreadShutdown();
				// Subclass and override this function
				
			if (status != B_OK) {
				ThreadShutdownFailed(status);
				return status;
			}
			
			delete this;
			return B_OK;
		}

		BeginUnit();

		status = ExecuteUnit();
			// Subclass and override

		if (status != B_OK)
			ExecuteUnitFailed(status);
				// Subclass and override
		
		EndUnit();	
	}

	return B_OK;
}


status_t 
GenericThread::ThreadStartup()
{
	// This function is virtual.
	// Subclass and override this function.

	return B_OK;
}


status_t 
GenericThread::ExecuteUnit()
{
	// This function is virtual.

	// You would normally subclass and override this function
	// as it will provide you with Pause and Quit functionality
	// thanks to the unit management done by GenericThread::ThreadFunction()

	return B_OK;
}


status_t 
GenericThread::ThreadShutdown()
{
	// This function is virtual.
	// Subclass and override this function.

	return B_OK;
}


void 
GenericThread::ThreadStartupFailed(status_t status)
{
	// This function is virtual.
	// Subclass and override this function.
	
	Quit();
}


void 
GenericThread::ExecuteUnitFailed(status_t status)
{
	// This function is virtual.
	// Subclass and override this function.
	
	Quit();
}


void 
GenericThread::ThreadShutdownFailed(status_t status)
{
	// This function is virtual.
	// Subclass and override this function.
	
	// (is this good default behaviour?)
}


status_t 
GenericThread::Start()
{
	status_t status = B_OK;
	
	if (IsPaused()) {
		status = release_sem(fExecuteUnitSem);
		if (status != B_OK)
			return status;

		fThreadIsPaused = false;
	}
	
	status = resume_thread(fThreadId);
	
	return status;
}


int32 
GenericThread::_ThreadFunction(void* simple_thread_ptr)
{
	status_t status = B_OK;

	status = ((GenericThread*) simple_thread_ptr)->ThreadFunction();

	return status;
}


BMessage*	
GenericThread::GetDataStore()
{
	return fThreadDataStore;
}


void
GenericThread::SetDataStore(BMessage* message)
{
	fThreadDataStore = message;
}


status_t	
GenericThread::Pause(bool doBlock, bigtime_t timeout)
{
	status_t status = B_OK;
	
	if (doBlock)
		status = acquire_sem(fExecuteUnitSem); 
			// thread will wait on semaphore
	else
		status = acquire_sem_etc(fExecuteUnitSem, 1, B_RELATIVE_TIMEOUT,
			timeout); 
			// thread will timeout

	if (status == B_OK) {
		fThreadIsPaused = true;
		return B_OK;
	}
	
	return status;
}


void	
GenericThread::Quit()
{
	fQuitRequested = true;
}


bool 
GenericThread::HasQuitBeenRequested()
{
	return fQuitRequested;
}


bool	
GenericThread::IsPaused()
{
	return fThreadIsPaused;
}


status_t	
GenericThread::Suspend()
{
	return suspend_thread(fThreadId);
}


status_t	
GenericThread::Resume()
{
	release_sem(fExecuteUnitSem);
		// to counteract Pause()
	fThreadIsPaused = false;
	
	return resume_thread(fThreadId);
		// to counteract Suspend()
}


status_t	
GenericThread::Kill()
{
	return kill_thread(fThreadId);
}


void	
GenericThread::ExitWithReturnValue(status_t return_value)
{
	exit_thread(return_value);
}


status_t	
GenericThread::SetExitCallback(void (*callback)(void*), void* data)
{
	return on_exit_thread(callback, data);
}


status_t	
GenericThread::WaitForThread(status_t* exitValue)
{
	return wait_for_thread(fThreadId, exitValue);
}


status_t	
GenericThread::Rename(char* name)
{
	return rename_thread(fThreadId, name);
}


status_t	
GenericThread::SendData(int32 code, void* buffer, size_t buffer_size)
{
	return send_data(fThreadId, code, buffer, buffer_size);
}


int32	
GenericThread::ReceiveData(thread_id* sender, void* buffer, size_t buffer_size)
{
	return receive_data(sender, buffer, buffer_size);
}


bool	
GenericThread::HasData()
{
	return has_data(fThreadId);
}


status_t	
GenericThread::SetPriority(int32 newPriority)
{
	return set_thread_priority(fThreadId, newPriority);
}


void	
GenericThread::Snooze(bigtime_t microseconds)
{
	Suspend();
	snooze(microseconds);
	Resume();
}


void	
GenericThread::SnoozeUntil(bigtime_t microseconds, int timebase)
{
	Suspend();
	snooze_until(microseconds, timebase);
	Resume();
}


status_t	
GenericThread::GetInfo(thread_info* threadInfo)
{
	return get_thread_info(fThreadId, threadInfo);
}


thread_id	
GenericThread::GetThread()
{
	thread_info	threadInfo;
	GetInfo(&threadInfo);
	return threadInfo.thread;
}


team_id	
GenericThread::GetTeam()
{
	thread_info	threadInfo;
	GetInfo(&threadInfo);
	return threadInfo.team;
}


char*	
GenericThread::GetName()
{
	thread_info	threadInfo;
	GetInfo(&threadInfo);
	return strdup(threadInfo.name);
}


thread_state	
GenericThread::GetState()
{
	thread_info	threadInfo;
	GetInfo(&threadInfo);
	return threadInfo.state;
}


sem_id	
GenericThread::GetSemaphore()
{
	thread_info	threadInfo;
	GetInfo(&threadInfo);
	return threadInfo.sem;
}


int32	
GenericThread::GetPriority()
{
	thread_info	threadInfo;
	GetInfo(&threadInfo);
	return threadInfo.priority;
}


bigtime_t	
GenericThread::GetUserTime()
{
	thread_info	threadInfo;
	GetInfo(&threadInfo);
	return threadInfo.user_time;
}


bigtime_t	
GenericThread::GetKernelTime()
{
	thread_info	threadInfo;
	GetInfo(&threadInfo);
	return threadInfo.kernel_time;
}


void*	
GenericThread::GetStackBase()
{
	thread_info	threadInfo;
	GetInfo(&threadInfo);
	return threadInfo.stack_base;
}


void*	
GenericThread::GetStackEnd()
{
	thread_info	threadInfo;
	GetInfo(&threadInfo);
	return threadInfo.stack_end;
}


void	
GenericThread::BeginUnit()
{
	acquire_sem(fExecuteUnitSem);
		// thread can not be paused until it releases semaphore
}


void	
GenericThread::EndUnit()
{
	release_sem(fExecuteUnitSem);
		// thread can now be paused
}

