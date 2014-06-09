// license: public domain
// authors: jonas.sundstrom@kirilla.com


#include "GenericThread.h"

#include <string.h>


GenericThread::GenericThread(const char* threadName, int32 priority,
	BMessage* message)
	:
	fThreadDataStore(message),
	fThreadId(spawn_thread(private_thread_function, threadName, priority,
		this)),
	fExecuteUnit(create_sem(1, "ExecuteUnit sem")),
	fQuitRequested(false),
	fThreadIsPaused(false)
{
	if (fThreadDataStore == NULL)
		fThreadDataStore = new BMessage();
}


GenericThread::~GenericThread()
{
	kill_thread(fThreadId);

	delete_sem(fExecuteUnit);
}


status_t
GenericThread::ThreadFunction(void)
{
	status_t status = B_OK;

	status = ThreadStartup();
		// Subclass and override this function
	if (status != B_OK) {
		ThreadStartupFailed(status);
		return (status);
			// is this the right thing to do?
	}

	while (1) {
		if (HasQuitBeenRequested()) {
			status = ThreadShutdown();
				// Subclass and override this function
			if (status != B_OK){
				ThreadShutdownFailed(status);
				return (status);
					// what do we do?
			}

			delete this;
				// destructor
			return B_OK;
		}

		BeginUnit();

		status = ExecuteUnit();
			// Subclass and override

		// Subclass and override
		if (status != B_OK)
			ExecuteUnitFailed(status);

		EndUnit();
	}

	return (B_OK);
}


status_t
GenericThread::ThreadStartup(void)
{
	// This function is virtual.
	// Subclass and override this function.

	return B_OK;
}


status_t
GenericThread::ExecuteUnit(void)
{
	// This function is virtual.

	// You would normally subclass and override this function
	// as it will provide you with Pause and Quit functionality
	// thanks to the unit management done by GenericThread::ThreadFunction()

	return B_OK;
}


status_t
GenericThread::ThreadShutdown(void)
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
GenericThread::Start(void)
{
	status_t status = B_OK;

	if (IsPaused()) {
		status = release_sem(fExecuteUnit);
		if (status != B_OK)
			return status;

		fThreadIsPaused = false;
	}

	status = resume_thread(fThreadId);

	return status;
}


int32
GenericThread::private_thread_function(void* pointer)
{
	return ((GenericThread*)pointer)->ThreadFunction();
}


BMessage*
GenericThread::GetDataStore(void)
{
	return fThreadDataStore;
}


void
GenericThread::SetDataStore(BMessage* message)
{
	fThreadDataStore  =  message;
}


status_t
GenericThread::Pause(bool shouldBlock, bigtime_t timeout)
{
	status_t status = B_OK;

	if (shouldBlock) {
		// thread will wait on semaphore
		status = acquire_sem(fExecuteUnit);
	} else {
		// thread will timeout
		status = acquire_sem_etc(fExecuteUnit, 1, B_RELATIVE_TIMEOUT, timeout);
	}

	if (status == B_OK) {
		fThreadIsPaused = true;
		return B_OK;
	}

	return status;
}


void
GenericThread::Quit(void)
{
	fQuitRequested = true;
}


bool
GenericThread::HasQuitBeenRequested(void)
{
	return fQuitRequested;
}


bool
GenericThread::IsPaused(void)
{
	return fThreadIsPaused;
}


status_t
GenericThread::Suspend(void)
{
	return suspend_thread(fThreadId);
}


status_t
GenericThread::Resume(void)
{
	release_sem(fExecuteUnit);
		// to counteract Pause()
	fThreadIsPaused = false;

	return (resume_thread(fThreadId));
		// to counteract Suspend()
}


status_t
GenericThread::Kill(void)
{
	return (kill_thread(fThreadId));
}


void
GenericThread::ExitWithReturnValue(status_t returnValue)
{
	exit_thread(returnValue);
}


status_t
GenericThread::SetExitCallback(void (*callback)(void*), void* data)
{
	return (on_exit_thread(callback, data));
}


status_t
GenericThread::WaitForThread(status_t* exitValue)
{
	return (wait_for_thread(fThreadId, exitValue));
}


status_t
GenericThread::Rename(char* name)
{
	return (rename_thread(fThreadId, name));
}


status_t
GenericThread::SendData(int32 code, void* buffer, size_t size)
{
	return (send_data(fThreadId, code, buffer, size));
}


int32
GenericThread::ReceiveData(thread_id* sender, void* buffer, size_t size)
{
	return (receive_data(sender, buffer, size));
}


bool
GenericThread::HasData(void)
{
	return (has_data(fThreadId));
}


status_t
GenericThread::SetPriority(int32 priority)
{
	return (set_thread_priority(fThreadId, priority));
}


void
GenericThread::Snooze(bigtime_t delay)
{
	Suspend();
	snooze(delay);
	Resume();
}


void
GenericThread::SnoozeUntil(bigtime_t delay, int timeBase)
{
	Suspend();
	snooze_until(delay, timeBase);
	Resume();
}


status_t
GenericThread::GetInfo(thread_info* info)
{
	return get_thread_info(fThreadId, info);
}


thread_id
GenericThread::GetThread(void)
{
	thread_info info;
	GetInfo(&info);
	return info.thread;
}


team_id
GenericThread::GetTeam(void)
{
	thread_info info;
	GetInfo(&info);
	return info.team;
}


char*
GenericThread::GetName(void)
{
	thread_info info;
	GetInfo(&info);
	return strdup(info.name);
}


thread_state
GenericThread::GetState(void)
{
	thread_info info;
	GetInfo(&info);
	return info.state;
}


sem_id
GenericThread::GetSemaphore(void)
{
	thread_info info;
	GetInfo(&info);
	return info.sem;
}


int32
GenericThread::GetPriority(void)
{
	thread_info info;
	GetInfo(&info);
	return info.priority;
}


bigtime_t
GenericThread::GetUserTime(void)
{
	thread_info info;
	GetInfo(&info);
	return info.user_time;
}


bigtime_t
GenericThread::GetKernelTime(void)
{
	thread_info info;
	GetInfo(&info);
	return info.kernel_time;
}


void*
GenericThread::GetStackBase(void)
{
	thread_info info;
	GetInfo(&info);
	return info.stack_base;
}


void*
GenericThread::GetStackEnd(void)
{
	thread_info info;
	GetInfo(&info);
	return info.stack_end;
}


void
GenericThread::BeginUnit(void)
{
	acquire_sem(fExecuteUnit);
		// thread can not be paused until it releases semaphore
}


void
GenericThread::EndUnit(void)
{
	release_sem(fExecuteUnit);
		// thread can now be paused
}
