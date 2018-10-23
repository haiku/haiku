/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: TimedEventQueue.cpp
 *  DESCR: used by BMediaEventLooper
 ***********************************************************************/

#include <TimedEventQueue.h>
#include <string.h>
#include "TimedEventQueuePrivate.h"
#include "MediaDebug.h"

/*************************************************************
 * struct media_timed_event
 *************************************************************/

media_timed_event::media_timed_event()
{
	CALLED();
	memset(this, 0, sizeof(*this));
}


media_timed_event::media_timed_event(bigtime_t inTime,
									 int32 inType)
{
	CALLED();
	memset(this, 0, sizeof(*this));
	event_time = inTime;
	type = inType;
}


media_timed_event::media_timed_event(bigtime_t inTime,
									 int32 inType,
									 void *inPointer,
									 uint32 inCleanup)
{
	CALLED();
	memset(this, 0, sizeof(*this));
	event_time = inTime;
	type = inType;
	pointer = inPointer;
	cleanup = inCleanup;
}


media_timed_event::media_timed_event(bigtime_t inTime,
									 int32 inType,
									 void *inPointer,
									 uint32 inCleanup,
									 int32 inData,
									 int64 inBigdata,
									 char *inUserData,
									 size_t dataSize)
{
	CALLED();
	memset(this, 0, sizeof(*this));
	event_time = inTime;
	type = inType;
	pointer = inPointer;
	cleanup = inCleanup;
	data = inData;
	bigdata = inBigdata;
	memcpy(user_data,inUserData,min_c(sizeof(media_timed_event::user_data),dataSize));
}


media_timed_event::media_timed_event(const media_timed_event &clone)
{
	CALLED();
	*this = clone;
}


void
media_timed_event::operator=(const media_timed_event &clone)
{
	CALLED();
	memcpy(this, &clone, sizeof(*this));
}


media_timed_event::~media_timed_event()
{
	CALLED();
}

/*************************************************************
 * global operators
 *************************************************************/

bool operator==(const media_timed_event & a, const media_timed_event & b)
{
	CALLED();
	return (0 == memcmp(&a,&b,sizeof(media_timed_event)));
}

bool operator!=(const media_timed_event & a, const media_timed_event & b)
{
	CALLED();
	return (0 != memcmp(&a,&b,sizeof(media_timed_event)));
}

bool operator<(const media_timed_event & a, const media_timed_event & b)
{
	CALLED();
	return a.event_time < b.event_time;
}

bool operator>(const media_timed_event & a, const media_timed_event &b)
{
	CALLED();
	return a.event_time > b.event_time;
}


/*************************************************************
 * public BTimedEventQueue
 *************************************************************/


void *
BTimedEventQueue::operator new(size_t s)
{
	CALLED();
	return ::operator new(s);
}


void
BTimedEventQueue::operator delete(void *p, size_t s)
{
	CALLED();
	return ::operator delete(p);
}


BTimedEventQueue::BTimedEventQueue() : 
	fImp(new _event_queue_imp)
{
	CALLED();
}


BTimedEventQueue::~BTimedEventQueue()
{
	CALLED();
	delete fImp;
}


status_t
BTimedEventQueue::AddEvent(const media_timed_event &event)
{
	CALLED();
	return fImp->AddEvent(event);
}


status_t
BTimedEventQueue::RemoveEvent(const media_timed_event *event)
{
	CALLED();
	return fImp->RemoveEvent(event);
}


status_t
BTimedEventQueue::RemoveFirstEvent(media_timed_event *outEvent)
{
	CALLED();
	return fImp->RemoveFirstEvent(outEvent);
}


bool
BTimedEventQueue::HasEvents() const
{
	CALLED();
	return fImp->HasEvents();
}


int32
BTimedEventQueue::EventCount() const
{
	CALLED();
	return fImp->EventCount();
}


const media_timed_event *
BTimedEventQueue::FirstEvent() const
{
	CALLED();
	return fImp->FirstEvent();
}


bigtime_t
BTimedEventQueue::FirstEventTime() const
{
	CALLED();
	return fImp->FirstEventTime();
}


const media_timed_event *
BTimedEventQueue::LastEvent() const
{
	CALLED();
	return fImp->LastEvent();
}


bigtime_t
BTimedEventQueue::LastEventTime() const
{
	CALLED();
	return fImp->LastEventTime();
}


const media_timed_event *
BTimedEventQueue::FindFirstMatch(bigtime_t eventTime,
								 time_direction direction,
								 bool inclusive,
								 int32 eventType)
{
	CALLED();
	return fImp->FindFirstMatch(eventTime, direction, inclusive, eventType);
}


status_t
BTimedEventQueue::DoForEach(for_each_hook hook,
							void *context,
							bigtime_t eventTime,
							time_direction direction,
							bool inclusive,
							int32 eventType)
{
	CALLED();
	return fImp->DoForEach(hook, context, eventTime, direction, inclusive, eventType);
}


void
BTimedEventQueue::SetCleanupHook(cleanup_hook hook,
								 void *context)
{
	CALLED();
	fImp->SetCleanupHook(hook, context);
}


status_t
BTimedEventQueue::FlushEvents(bigtime_t eventTime,
							  time_direction direction,
							  bool inclusive,
							  int32 eventType)
{
	CALLED();
	return fImp->FlushEvents(eventTime, direction, inclusive, eventType);
}

/*************************************************************
 * private BTimedEventQueue
 *************************************************************/

/*
// unimplemented
BTimedEventQueue::BTimedEventQueue(const BTimedEventQueue &other)
BTimedEventQueue &BTimedEventQueue::operator=(const BTimedEventQueue &other)
*/

status_t BTimedEventQueue::_Reserved_BTimedEventQueue_0(void *, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_1(void *, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_2(void *, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_3(void *, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_4(void *, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_5(void *, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_6(void *, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_7(void *, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_8(void *, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_9(void *, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_10(void *, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_11(void *, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_12(void *, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_13(void *, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_14(void *, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_15(void *, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_16(void *, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_17(void *, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_18(void *, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_19(void *, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_20(void *, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_21(void *, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_22(void *, ...) { return B_ERROR; }
status_t BTimedEventQueue::_Reserved_BTimedEventQueue_23(void *, ...) { return B_ERROR; }
