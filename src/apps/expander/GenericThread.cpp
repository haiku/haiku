// license: public domain
// authors: jonas.sundstrom@kirilla.com

#include "GenericThread.h"

#include <string.h>

GenericThread::GenericThread(const char * thread_name, int32 a_priority, BMessage * message)
	:
	fThreadDataStore(message),
	fThreadId(spawn_thread(private_thread_function, thread_name, a_priority, this)),
	fExecuteUnit(create_sem(1, "ExecuteUnit sem")),
	fQuitRequested(false),
	fThreadIsPaused(false)
{
	if (fThreadDataStore == NULL)
		fThreadDataStore	=	new BMessage();
}


GenericThread::~GenericThread()
{
	kill_thread(fThreadId);

	delete_sem(fExecuteUnit);
}


status_t
GenericThread::ThreadFunction(void)
{
	status_t  status  =  B_OK;

	status = ThreadStartup();	// Subclass and override this function
	if (status != B_OK)
	{
		ThreadStartupFailed(status);
		return (status);
		// is this the right thing to do?
	}

	while (1)
	{
		if (HasQuitBeenRequested())
		{
			status = ThreadShutdown();	// Subclass and override this function
			if (status != B_OK)
			{
				ThreadShutdownFailed(status);
				return (status);
				// what do we do?
			}

			delete this;		// destructor
			return B_OK;
		}

		BeginUnit();

		status = ExecuteUnit();	// Subclass and override

		if (status != B_OK)
			ExecuteUnitFailed(status);  // Subclass and override

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
GenericThread::ThreadStartupFailed(status_t a_status)
{
	// This function is virtual.
	// Subclass and override this function.

	Quit();
}


void
GenericThread::ExecuteUnitFailed(status_t a_status)
{
	// This function is virtual.
	// Subclass and override this function.

	Quit();
}


void
GenericThread::ThreadShutdownFailed(status_t a_status)
{
	// This function is virtual.
	// Subclass and override this function.

	// (is this good default behaviour?)
}


status_t
GenericThread::Start(void)
{
	status_t	status	=	B_OK;

	if (IsPaused())
	{
		status	=	release_sem(fExecuteUnit);
		if (status != B_OK)
			return status;

		fThreadIsPaused	=	false;
	}

	status	=	resume_thread(fThreadId);

	return status;
}


int32
GenericThread::private_thread_function(void * a_simple_thread_ptr)
{
	status_t	status	=	B_OK;

	status	=	((GenericThread *) a_simple_thread_ptr)-> ThreadFunction();

	return (status);
}


BMessage *
GenericThread::GetDataStore(void)
{
	return	(fThreadDataStore);
}


void
GenericThread::SetDataStore(BMessage * message)
{
	fThreadDataStore  =  message;
}


status_t
GenericThread::Pause(bool a_do_block, bigtime_t a_timeout)
{
	status_t	status	=	B_OK;

	if (a_do_block)
		status	=	acquire_sem(fExecuteUnit);
	// thread will wait on semaphore
	else
		status	=	acquire_sem_etc(fExecuteUnit, 1, B_RELATIVE_TIMEOUT, a_timeout);
	// thread will timeout

	if (status == B_OK)
	{
		fThreadIsPaused	=	true;
		return (B_OK);
	}

	return status;
}


void
GenericThread::Quit(void)
{
	fQuitRequested	=	true;
}


bool
GenericThread::HasQuitBeenRequested(void)
{
	return (fQuitRequested);
}


bool
GenericThread::IsPaused(void)
{
	return (fThreadIsPaused);
}


status_t
GenericThread::Suspend(void)
{
	return (suspend_thread(fThreadId));
}


status_t
GenericThread::Resume(void)
{
	release_sem(fExecuteUnit);		// to counteract Pause()
	fThreadIsPaused	=	false;

	return (resume_thread(fThreadId));	// to counteract Suspend()
}


status_t
GenericThread::Kill(void)
{
	return (kill_thread(fThreadId));
}


void
GenericThread::ExitWithReturnValue(status_t a_return_value)
{
	exit_thread(a_return_value);
}


status_t
GenericThread::SetExitCallback(void(*a_callback)(void*), void * a_data)
{
	return (on_exit_thread(a_callback, a_data));
}


status_t
GenericThread::WaitForThread(status_t * a_exit_value)
{
	return (wait_for_thread(fThreadId, a_exit_value));
}


status_t
GenericThread::Rename(char * a_name)
{
	return (rename_thread(fThreadId, a_name));
}


status_t
GenericThread::SendData(int32 a_code, void * a_buffer, size_t a_buffer_size)
{
	return (send_data(fThreadId, a_code, a_buffer, a_buffer_size));
}


int32
GenericThread::ReceiveData(thread_id * a_sender, void * a_buffer, size_t a_buffer_size)
{
	return (receive_data(a_sender, a_buffer, a_buffer_size));
}


bool
GenericThread::HasData(void)
{
	return (has_data(fThreadId));
}


status_t
GenericThread::SetPriority(int32 a_new_priority)
{
	return (set_thread_priority(fThreadId, a_new_priority));
}


void
GenericThread::Snooze(bigtime_t a_microseconds)
{
	Suspend();
	snooze(a_microseconds);
	Resume();
}


void
GenericThread::SnoozeUntil(bigtime_t a_microseconds, int a_timebase)
{
	Suspend();
	snooze_until(a_microseconds, a_timebase);
	Resume();
}


status_t
GenericThread::GetInfo(thread_info * thread_info)
{
	return (get_thread_info(fThreadId, thread_info));
}


thread_id
GenericThread::GetThread(void)
{
	thread_info	t_thread_info;
	GetInfo(& t_thread_info);
	return (t_thread_info.thread);
}


team_id
GenericThread::GetTeam(void)
{
	thread_info	t_thread_info;
	GetInfo(& t_thread_info);
	return (t_thread_info.team);
}


char *
GenericThread::GetName(void)
{
	thread_info	t_thread_info;
	GetInfo(& t_thread_info);
	return strdup(t_thread_info.name);
}


thread_state
GenericThread::GetState(void)
{
	thread_info	t_thread_info;
	GetInfo(& t_thread_info);
	return (t_thread_info.state);
}


sem_id
GenericThread::GetSemaphore(void)
{
	thread_info	t_thread_info;
	GetInfo(& t_thread_info);
	return (t_thread_info.sem);
}


int32
GenericThread::GetPriority(void)
{
	thread_info	t_thread_info;
	GetInfo(& t_thread_info);
	return (t_thread_info.priority);
}


bigtime_t
GenericThread::GetUserTime(void)
{
	thread_info	t_thread_info;
	GetInfo(& t_thread_info);
	return (t_thread_info.user_time);
}


bigtime_t
GenericThread::GetKernelTime(void)
{
	thread_info	t_thread_info;
	GetInfo(& t_thread_info);
	return (t_thread_info.kernel_time);
}


void *
GenericThread::GetStackBase(void)
{
	thread_info	t_thread_info;
	GetInfo(& t_thread_info);
	return (t_thread_info.stack_base);
}


void *
GenericThread::GetStackEnd(void)
{
	thread_info	t_thread_info;
	GetInfo(& t_thread_info);
	return (t_thread_info.stack_end);
}


void
GenericThread::BeginUnit(void)
{
	acquire_sem(fExecuteUnit); // thread can not be paused until it releases semaphore
}


void
GenericThread::EndUnit(void)
{
	release_sem(fExecuteUnit); // thread can now be paused
}

