/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <time.h>

#include <errno.h>

#include <new>

#include <AutoDeleter.h>
#include <syscall_utils.h>

#include <syscalls.h>
#include <thread_defs.h>
#include <user_timer_defs.h>

#include <libroot_private.h>
#include <pthread_private.h>
#include <time_private.h>
#include <user_thread.h>


static void
info_to_itimerspec(const user_timer_info& info, itimerspec& spec)
{
	bigtime_to_timespec(info.interval, spec.it_interval);

	// A remaining_time of B_INFINITE_TIMEOUT means the timer isn't scheduled.
	if (info.remaining_time != B_INFINITE_TIMEOUT) {
		bigtime_to_timespec(info.remaining_time, spec.it_value);
	} else {
		spec.it_value.tv_sec = 0;
		spec.it_value.tv_nsec = 0;
	}
}


static bool
itimerspec_to_bigtimes(const itimerspec& spec, bigtime_t& _nextTime,
	bigtime_t& _interval)
{
	if (!timespec_to_bigtime(spec.it_interval, _interval)
		|| !timespec_to_bigtime(spec.it_value, _nextTime)) {
		return false;
	}

	if (_nextTime == 0)
		_nextTime = B_INFINITE_TIMEOUT;

	return true;
}


static status_t
timer_thread_entry(void* _entry, void* _value)
{
	// init a pthread
	pthread_thread thread;
	__init_pthread(&thread, NULL, NULL);
	thread.flags |= THREAD_DETACHED;

	get_user_thread()->pthread = &thread;

	// we have been started with deferred signals -- undefer them
	undefer_signals();

	// prepare the arguments
	union sigval value;
	value.sival_ptr = _value;
	void (*entry)(union sigval) = (void (*)(union sigval))_entry;

	// call the entry function
	entry(value);

	return B_OK;
}


// #pragma mark -


int
timer_create(clockid_t clockID, struct sigevent* event, timer_t* _timer)
{
	// create a timer object
	__timer_t* timer = new(std::nothrow) __timer_t;
	if (timer == NULL)
		RETURN_AND_SET_ERRNO(ENOMEM);
	ObjectDeleter<__timer_t> timerDeleter(timer);

	// If the notification method is SIGEV_THREAD, initialize thread creation
	// attributes.
	bool isThreadEvent = event != NULL && event->sigev_notify == SIGEV_THREAD;
	thread_creation_attributes threadAttributes;
	if (isThreadEvent) {
		status_t error = __pthread_init_creation_attributes(
			event->sigev_notify_attributes, NULL, &timer_thread_entry,
			(void*)event->sigev_notify_function, event->sigev_value.sival_ptr,
			"timer notify", &threadAttributes);
		if (error != B_OK)
			RETURN_AND_SET_ERRNO(error);

		threadAttributes.flags |= THREAD_CREATION_FLAG_DEFER_SIGNALS;
	}

	// create the timer
	int32 timerID = _kern_create_timer(clockID, -1, 0, event,
		isThreadEvent ? &threadAttributes : NULL);
	if (timerID < 0)
		RETURN_AND_SET_ERRNO(timerID);

	// init the object members
	timer->SetTo(timerID, -1);

	*_timer = timerDeleter.Detach();
	return 0;
}


int
timer_delete(timer_t timer)
{
	status_t error = _kern_delete_timer(timer->id, timer->thread);
	if (error != B_OK)
		RETURN_AND_SET_ERRNO(error);

	delete timer;
	return 0;
}


int
timer_gettime(timer_t timer, struct itimerspec* value)
{
	user_timer_info info;
	status_t error = _kern_get_timer(timer->id, timer->thread, &info);
	if (error != B_OK)
		RETURN_AND_SET_ERRNO(error);

	info_to_itimerspec(info, *value);

	return 0;
}


int
timer_settime(timer_t timer, int flags, const struct itimerspec* value,
	struct itimerspec* oldValue)
{
	// translate new value
	bigtime_t nextTime;
	bigtime_t interval;
	if (!itimerspec_to_bigtimes(*value, nextTime, interval))
		RETURN_AND_SET_ERRNO(EINVAL);

	uint32 timeoutFlags = (flags & TIMER_ABSTIME) != 0
		? B_ABSOLUTE_TIMEOUT : B_RELATIVE_TIMEOUT;

	// set the timer
	user_timer_info oldInfo;
	status_t error = _kern_set_timer(timer->id, timer->thread, nextTime,
		interval, timeoutFlags, oldValue != NULL ? &oldInfo : NULL);
	if (error != B_OK)
		RETURN_AND_SET_ERRNO(error);

	// translate old info back
	if (oldValue != NULL)
		info_to_itimerspec(oldInfo, *oldValue);

	return 0;
}


int
timer_getoverrun(timer_t timer)
{
	user_timer_info info;
	status_t error = _kern_get_timer(timer->id, timer->thread, &info);
	if (error != B_OK)
		RETURN_AND_SET_ERRNO(error);

	return info.overrun_count;
}
