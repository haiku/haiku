/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: MediaEventLooper.cpp
 *  DESCR: 
 ***********************************************************************/
#include <MediaEventLooper.h>
#include <TimeSource.h>
#include <scheduler.h>
#include "debug.h"

// XXX The bebook says that the latency is always calculated in realtime
// XXX This is not currently done in this code

/*************************************************************
 * protected BMediaEventLooper
 *************************************************************/

/* virtual */
BMediaEventLooper::~BMediaEventLooper()
{
	CALLED();
	// don't call Quit(); here
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
	fEventQueue.SetCleanupHook(BMediaEventLooper::_CleanUpEntry,this);
	fRealTimeQueue.SetCleanupHook(BMediaEventLooper::_CleanUpEntry,this);
}

/* virtual */ void
BMediaEventLooper::NodeRegistered()
{
	CALLED();
	// don't call Run(); here, must be done by the derived class (yes, that's stupid)
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
		performance_time = fEventQueue.FirstEventTime();
		performance_time = (performance_time == B_INFINITE_TIMEOUT) ? 0 : performance_time - 1;
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
	// XXX what do we need to do here?
	return BMediaNode::AddTimer(at_performance_time,cookie);
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
		if(fControlThread > 0) {
			set_thread_priority(fControlThread, fCurrentPriority);
			fSchedulingLatency = estimate_max_scheduling_latency(fControlThread);
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

	bool is_realtime;
	status_t err;
	bigtime_t latency;
	bigtime_t waituntil;
	for (;;) {
		// while there are no events or it is not time for the earliest event,
		// process messages using WaitForMessages. Whenever this funtion times out,
		// we need to handle the next event
		for (;;) {
			if (RunState() == B_QUITTING)
				return;
			// BMediaEventLooper compensates your performance time by adding the event latency
			// (see SetEventLatency()) and the scheduling latency (or, for real-time events, 
			// only the scheduling latency). 
			// latency = fOut.downstream_latency + fOut.processing_latency + fSchedulingLatency;
			// XXX well, fix this later
			latency = fEventLatency + fSchedulingLatency; 
			
			if (fEventQueue.HasEvents() && (TimeSource()->Now() - latency) >= fEventQueue.FirstEventTime()) {
				is_realtime = false;
				break;
			}
			if (fRealTimeQueue.HasEvents() && (TimeSource()->RealTimeFor(TimeSource()->Now(),fSchedulingLatency)) >= fRealTimeQueue.FirstEventTime()) {
				is_realtime = true;
				break;
			}
			waituntil = B_INFINITE_TIMEOUT;
			if (fEventQueue.HasEvents()) {
				waituntil = TimeSource()->RealTimeFor(fEventQueue.FirstEventTime(), latency);
				is_realtime = false;
			}
			if (fRealTimeQueue.HasEvents()) {
				bigtime_t temp;
				temp = TimeSource()->RealTimeFor(TimeSource()->PerformanceTimeFor(fRealTimeQueue.FirstEventTime()), fSchedulingLatency);
				if (temp < waituntil) {
					waituntil = temp;
					is_realtime = true;
				}
			}
			err = WaitForMessage(waituntil);
			if (err == B_TIMED_OUT)
				break;
		}

		/// we have timed out - so handle the next event
		media_timed_event event;
		if (is_realtime)
			err = fRealTimeQueue.RemoveFirstEvent(&event);
		else
			err = fEventQueue.RemoveFirstEvent(&event);

		if (err == B_OK) {
			bigtime_t lateness;
			if (is_realtime)
				lateness = TimeSource()->RealTime() - event.event_time;
			else
				lateness = TimeSource()->Now() - event.event_time;
			DispatchEvent(&event,lateness,is_realtime);
		}
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
	CALLED();
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
	if (priority < 1)
		priority = 1;
		
	if (priority > 120)
		priority = 120;
		
	fSetPriority = priority;
	fCurrentPriority = (RunMode() == B_OFFLINE) ? min_c(B_NORMAL_PRIORITY, fSetPriority) : fSetPriority;

	if(fControlThread > 0) {
		set_thread_priority(fControlThread, fCurrentPriority);
		fSchedulingLatency = estimate_max_scheduling_latency(fControlThread);
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
}


void
BMediaEventLooper::SetBufferDuration(bigtime_t duration)
{
	CALLED();
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
	status_t err;

	if (fRunState == B_TERMINATED)
		return;
	
	SetRunState(B_QUITTING);
	
	close_port(ControlPort());
	if (fControlThread != -1)
		wait_for_thread(fControlThread, &err);
	fControlThread = -1;

	SetRunState(B_TERMINATED);
}


void
BMediaEventLooper::DispatchEvent(const media_timed_event *event,
								 bigtime_t lateness,
								 bool realTimeEvent)
{
	CALLED();

	HandleEvent(event,lateness,realTimeEvent);

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
		
		default:
			break;
	}

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
	CALLED();
	((BMediaEventLooper *)context)->_DispatchCleanUp(event);
}


void
BMediaEventLooper::_DispatchCleanUp(const media_timed_event *event)
{
	CALLED();

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

