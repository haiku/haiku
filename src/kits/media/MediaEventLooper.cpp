/*
 * Copyright (c) 2015 Dario Casalinuovo <b.vitruvio@gmail.com>
 * Copyright (c) 2002, 2003 Marcus Overhagen <Marcus@Overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files or portions
 * thereof (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice
 *    in the  binary, as well as this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided with
 *    the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <MediaEventLooper.h>
#include <TimeSource.h>
#include <scheduler.h>
#include <Buffer.h>
#include <ServerInterface.h>
#include "MediaDebug.h"

/*************************************************************
 * protected BMediaEventLooper
 *************************************************************/

/* virtual */
BMediaEventLooper::~BMediaEventLooper()
{
	CALLED();

	// don't call Quit(); here, except if the user was stupid
	if (fControlThread != -1) {
		printf("You MUST call BMediaEventLooper::Quit() in your destructor!\n");
		Quit();
	}
}

/* explicit */
BMediaEventLooper::BMediaEventLooper(uint32 apiVersion) :
	BMediaNode("called by BMediaEventLooper"),
	fControlThread(-1),
	fCurrentPriority(B_URGENT_PRIORITY),
	fSetPriority(B_URGENT_PRIORITY),
	fRunState(B_UNREGISTERED),
	fEventLatency(0),
	fSchedulingLatency(0),
	fBufferDuration(0),
	fOfflineTime(0),
	fApiVersion(apiVersion)
{
	CALLED();
	fEventQueue.SetCleanupHook(BMediaEventLooper::_CleanUpEntry, this);
	fRealTimeQueue.SetCleanupHook(BMediaEventLooper::_CleanUpEntry, this);
}

/* virtual */ void
BMediaEventLooper::NodeRegistered()
{
	CALLED();
	// Calling Run() should be done by the derived class,
	// at least that's how it is documented by the BeBook.
	// It appears that BeOS R5 called it here. Calling Run()
	// twice doesn't hurt, and some nodes need it to be called here.
	Run();
}


/* virtual */ void
BMediaEventLooper::Start(bigtime_t performance_time)
{
	CALLED();
	// This hook function is called when a node is started
	// by a call to the BMediaRoster. The specified
	// performanceTime, the time at which the node
	// should start running, may be in the future.
	fEventQueue.AddEvent(media_timed_event(performance_time, BTimedEventQueue::B_START));
}


/* virtual */ void
BMediaEventLooper::Stop(bigtime_t performance_time,
						bool immediate)
{
	CALLED();
	// This hook function is called when a node is stopped
	// by a call to the BMediaRoster. The specified performanceTime,
	// the time at which the node should stop, may be in the future.
	// If immediate is true, your node should ignore the performanceTime
	// value and synchronously stop performance. When Stop() returns,
	// you're promising not to write into any BBuffers you may have
	// received from your downstream consumers, and you promise not
	// to send any more buffers until Start() is called again.

	if (immediate) {
		// always be sure to add to the front of the queue so we can make sure it is
		// handled before any buffers are sent!
		performance_time = 0;
	}
	fEventQueue.AddEvent(media_timed_event(performance_time, BTimedEventQueue::B_STOP));
}


/* virtual */ void
BMediaEventLooper::Seek(bigtime_t media_time,
						bigtime_t performance_time)
{
	CALLED();
	// This hook function is called when a node is asked to seek to
	// the specified mediaTime by a call to the BMediaRoster.
	// The specified performanceTime, the time at which the node
	// should begin the seek operation, may be in the future.
	fEventQueue.AddEvent(media_timed_event(performance_time, BTimedEventQueue::B_SEEK, NULL,
		BTimedEventQueue::B_NO_CLEANUP, 0, media_time, NULL));
}


/* virtual */ void
BMediaEventLooper::TimeWarp(bigtime_t at_real_time,
							bigtime_t to_performance_time)
{
	CALLED();
	// This hook function is called when the time source to which the
	// node is slaved is repositioned (via a seek operation) such that
	// there will be a sudden jump in the performance time progression
	// as seen by the node. The to_performance_time argument indicates
	// the new performance time; the change should occur at the real
	// time specified by the at_real_time argument.

	// place in the realtime queue
	fRealTimeQueue.AddEvent(media_timed_event(at_real_time,	BTimedEventQueue::B_WARP,
		NULL, BTimedEventQueue::B_NO_CLEANUP, 0, to_performance_time, NULL));

	// BeBook: Your implementation of TimeWarp() should call through to BMediaNode::TimeWarp()
	// BeBook: as well as all other inherited forms of TimeWarp()
	// XXX should we do this here?
	BMediaNode::TimeWarp(at_real_time, to_performance_time);
}


/* virtual */ status_t
BMediaEventLooper::AddTimer(bigtime_t at_performance_time,
							int32 cookie)
{
	CALLED();

	media_timed_event event(at_performance_time,
		BTimedEventQueue::B_TIMER, NULL,
		BTimedEventQueue::B_EXPIRE_TIMER);
	event.data = cookie;
	return EventQueue()->AddEvent(event);
}


/* virtual */ void
BMediaEventLooper::SetRunMode(run_mode mode)
{
	CALLED();
	// The SetRunMode() hook function is called when someone requests that your node's run mode be changed.

	// bump or reduce priority when switching from/to offline run mode
	int32 priority;
	priority = (mode == B_OFFLINE) ? min_c(B_NORMAL_PRIORITY, fSetPriority) : fSetPriority;
	if (priority != fCurrentPriority) {
		fCurrentPriority = priority;
		if (fControlThread > 0) {
			set_thread_priority(fControlThread, fCurrentPriority);
			fSchedulingLatency = estimate_max_scheduling_latency(fControlThread);
			printf("BMediaEventLooper: SchedulingLatency is %" B_PRId64 "\n",
				fSchedulingLatency);
		}
	}

	BMediaNode::SetRunMode(mode);
}


/* virtual */ void
BMediaEventLooper::CleanUpEvent(const media_timed_event *event)
{
	CALLED();
	// Implement this function to clean up after custom events you've created
	// and added to your queue. It's called when a custom event is removed from
	// the queue, to let you handle any special tidying-up that the event might require.
}


/* virtual */ bigtime_t
BMediaEventLooper::OfflineTime()
{
	CALLED();
	return fOfflineTime;
}


/* virtual */ void
BMediaEventLooper::ControlLoop()
{
	CALLED();

	status_t err = B_OK;
	bigtime_t waitUntil = B_INFINITE_TIMEOUT;
	bool hasRealtime = false;
	bool hasEvent = false;

	// While there are no events or it is not time for the earliest event,
	// process messages using WaitForMessages. Whenever this funtion times out,
	// we need to handle the next event

	fSchedulingLatency = estimate_max_scheduling_latency(fControlThread);
	while (RunState() != B_QUITTING) {
		if (err == B_TIMED_OUT
				|| err == B_WOULD_BLOCK) {
			// NOTE: The reference for doing the lateness calculus this way can
			// be found in the BeBook article "A BMediaEventLooper Example".
			// The value which we are going to calculate, is referred there as
			// 'lateness'.
			media_timed_event event;
			if (hasEvent)
				err = fEventQueue.RemoveFirstEvent(&event);
			else if (hasRealtime)
				err = fRealTimeQueue.RemoveFirstEvent(&event);

			if (err == B_OK) {
				// The general idea of lateness is to allow
				// the client code to detect when the buffer
				// is handled late or early.
				bigtime_t lateness = TimeSource()->RealTime() - waitUntil;

				DispatchEvent(&event, lateness, hasRealtime);
			}
		} else if (err != B_OK)
			return;

		// BMediaEventLooper compensates your performance time by adding
		// the event latency (see SetEventLatency()) and the scheduling
		// latency (or, for real-time events, only the scheduling latency).

		hasRealtime = fRealTimeQueue.HasEvents();
		hasEvent = fEventQueue.HasEvents();

		if (hasEvent) {
			waitUntil = TimeSource()->RealTimeFor(
				fEventQueue.FirstEventTime(),
				fEventLatency + fSchedulingLatency);
		} else if (!hasRealtime)
			waitUntil = B_INFINITE_TIMEOUT;

		if (hasRealtime) {
			bigtime_t realtimeWait = fRealTimeQueue.FirstEventTime()
				- fSchedulingLatency;

			if (!hasEvent || realtimeWait <= waitUntil) {
				waitUntil = realtimeWait;
				hasEvent = false;
			} else
				hasRealtime = false;
		}

		if (waitUntil != B_INFINITE_TIMEOUT
				&& TimeSource()->RealTime() >= waitUntil) {
			err = WaitForMessage(0);
		} else
			err = WaitForMessage(waitUntil);
	}
}


thread_id
BMediaEventLooper::ControlThread()
{
	CALLED();
	return fControlThread;
}

/*************************************************************
 * protected BMediaEventLooper
 *************************************************************/


BTimedEventQueue *
BMediaEventLooper::EventQueue()
{
	CALLED();
	return &fEventQueue;
}


BTimedEventQueue *
BMediaEventLooper::RealTimeQueue()
{
	CALLED();
	return &fRealTimeQueue;
}


int32
BMediaEventLooper::Priority() const
{
	CALLED();
	return fCurrentPriority;
}


int32
BMediaEventLooper::RunState() const
{
	PRINT(6, "CALLED BMediaEventLooper::RunState()\n");
	return fRunState;
}


bigtime_t
BMediaEventLooper::EventLatency() const
{
	CALLED();
	return fEventLatency;
}


bigtime_t
BMediaEventLooper::BufferDuration() const
{
	CALLED();
	return fBufferDuration;
}


bigtime_t
BMediaEventLooper::SchedulingLatency() const
{
	CALLED();
	return fSchedulingLatency;
}


status_t
BMediaEventLooper::SetPriority(int32 priority)
{
	CALLED();

	// clamp to a valid value
	if (priority < 5)
		priority = 5;

	if (priority > 120)
		priority = 120;

	fSetPriority = priority;
	fCurrentPriority = (RunMode() == B_OFFLINE) ? min_c(B_NORMAL_PRIORITY, fSetPriority) : fSetPriority;

	if (fControlThread > 0) {
		set_thread_priority(fControlThread, fCurrentPriority);
		fSchedulingLatency = estimate_max_scheduling_latency(fControlThread);
		printf("BMediaEventLooper: SchedulingLatency is %" B_PRId64 "\n",
			fSchedulingLatency);
	}

	return B_OK;
}


void
BMediaEventLooper::SetRunState(run_state state)
{
	CALLED();

	// don't allow run state changes while quitting,
	// also needed for correct terminating of the ControlLoop()
	if (fRunState == B_QUITTING && state != B_TERMINATED)
		return;

	fRunState = state;
}


void
BMediaEventLooper::SetEventLatency(bigtime_t latency)
{
	CALLED();
	// clamp to a valid value
	if (latency < 0)
		latency = 0;

	fEventLatency = latency;
	write_port_etc(ControlPort(), GENERAL_PURPOSE_WAKEUP, 0, 0, B_TIMEOUT, 0);
}


void
BMediaEventLooper::SetBufferDuration(bigtime_t duration)
{
	CALLED();

	if (duration < 0)
		duration = 0;

	fBufferDuration = duration;
}


void
BMediaEventLooper::SetOfflineTime(bigtime_t offTime)
{
	CALLED();
	fOfflineTime = offTime;
}


void
BMediaEventLooper::Run()
{
	CALLED();

	if (fControlThread != -1)
		return; // thread already running

	// until now, the run state is B_UNREGISTERED, but we need to start in B_STOPPED state.
	SetRunState(B_STOPPED);

	char threadName[32];
	sprintf(threadName, "%.20s control", Name());
	fControlThread = spawn_thread(_ControlThreadStart, threadName, fCurrentPriority, this);
	resume_thread(fControlThread);

	// get latency information
	fSchedulingLatency = estimate_max_scheduling_latency(fControlThread);
}


void
BMediaEventLooper::Quit()
{
	CALLED();

	if (fRunState == B_TERMINATED)
		return;

	SetRunState(B_QUITTING);
	close_port(ControlPort());
	if (fControlThread != -1) {
		status_t err;
		wait_for_thread(fControlThread, &err);
		fControlThread = -1;
	}
	SetRunState(B_TERMINATED);
}


void
BMediaEventLooper::DispatchEvent(const media_timed_event *event,
								 bigtime_t lateness,
								 bool realTimeEvent)
{
	PRINT(6, "CALLED BMediaEventLooper::DispatchEvent()\n");

	HandleEvent(event, lateness, realTimeEvent);

	switch (event->type) {
		case BTimedEventQueue::B_START:
			SetRunState(B_STARTED);
			break;

		case BTimedEventQueue::B_STOP:
			SetRunState(B_STOPPED);
			break;

		case BTimedEventQueue::B_SEEK:
			/* nothing */
			break;

		case BTimedEventQueue::B_WARP:
			/* nothing */
			break;

		case BTimedEventQueue::B_TIMER:
			TimerExpired(event->event_time, event->data);
			break;

		default:
			break;
	}

	_DispatchCleanUp(event);
}

/*************************************************************
 * private BMediaEventLooper
 *************************************************************/


/* static */ int32
BMediaEventLooper::_ControlThreadStart(void *arg)
{
	CALLED();
	((BMediaEventLooper *)arg)->SetRunState(B_STOPPED);
	((BMediaEventLooper *)arg)->ControlLoop();
	((BMediaEventLooper *)arg)->SetRunState(B_QUITTING);
	return 0;
}


/* static */ void
BMediaEventLooper::_CleanUpEntry(const media_timed_event *event,
								 void *context)
{
	PRINT(6, "CALLED BMediaEventLooper::_CleanUpEntry()\n");
	((BMediaEventLooper *)context)->_DispatchCleanUp(event);
}


void
BMediaEventLooper::_DispatchCleanUp(const media_timed_event *event)
{
	PRINT(6, "CALLED BMediaEventLooper::_DispatchCleanUp()\n");

	// this function to clean up after custom events you've created
	if (event->cleanup >= BTimedEventQueue::B_USER_CLEANUP)
		CleanUpEvent(event);
}

/*
// unimplemented
BMediaEventLooper::BMediaEventLooper(const BMediaEventLooper &)
BMediaEventLooper &BMediaEventLooper::operator=(const BMediaEventLooper &)
*/

/*************************************************************
 * protected BMediaEventLooper
 *************************************************************/


status_t
BMediaEventLooper::DeleteHook(BMediaNode *node)
{
	CALLED();
	// this is the DeleteHook that gets called by the media server
	// before the media node is deleted
	Quit();
	return BMediaNode::DeleteHook(node);
}

/*************************************************************
 * private BMediaEventLooper
 *************************************************************/

status_t BMediaEventLooper::_Reserved_BMediaEventLooper_0(int32 arg,...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_1(int32 arg,...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_2(int32 arg,...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_3(int32 arg,...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_4(int32 arg,...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_5(int32 arg,...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_6(int32 arg,...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_7(int32 arg,...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_8(int32 arg,...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_9(int32 arg,...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_10(int32 arg,...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_11(int32 arg,...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_12(int32 arg,...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_13(int32 arg,...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_14(int32 arg,...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_15(int32 arg,...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_16(int32 arg,...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_17(int32 arg,...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_18(int32 arg,...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_19(int32 arg,...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_20(int32 arg,...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_21(int32 arg,...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_22(int32 arg,...) { return B_ERROR; }
status_t BMediaEventLooper::_Reserved_BMediaEventLooper_23(int32 arg,...) { return B_ERROR; }

