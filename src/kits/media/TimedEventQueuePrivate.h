/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: TimedEventQueuePrivate.cpp
 *  DESCR: implements _event_queue_imp used by BTimedEventQueue,
 *         not thread save!
 ***********************************************************************/

#ifndef _TIMED_EVENT_QUEUE_PRIVATE_H
#define _TIMED_EVENT_QUEUE_PRIVATE_H

#include <MediaDefs.h>
#include <Locker.h>
#include "MediaDebug.h"

struct _event_queue_imp
{
	_event_queue_imp();
	~_event_queue_imp();
		
	status_t	AddEvent(const media_timed_event &event);
	status_t	RemoveEvent(const media_timed_event *event);
	status_t  	RemoveFirstEvent(media_timed_event * outEvent);
		
	bool		HasEvents() const;
	int32		EventCount() const;

	const media_timed_event *	FirstEvent() const;
	bigtime_t					FirstEventTime() const;
	const media_timed_event *	LastEvent() const;
	bigtime_t					LastEventTime() const;
	
	const media_timed_event *	FindFirstMatch(
									bigtime_t eventTime,
									BTimedEventQueue::time_direction direction,
									bool inclusive,
									int32 eventType);
		
	status_t	DoForEach(
					BTimedEventQueue::for_each_hook hook,
					void *context,
					bigtime_t eventTime,
					BTimedEventQueue::time_direction direction,
					bool inclusive,
					int32 eventType);
		
	void		SetCleanupHook(BTimedEventQueue::cleanup_hook hook, void *context);
	status_t	FlushEvents(
					bigtime_t eventTime,
					BTimedEventQueue::time_direction direction,
					bool inclusive,
					int32 eventType);

#if DEBUG > 1
	void		Dump() const;
#endif
					
private:
	struct event_queue_entry
	{	
		struct event_queue_entry *prev;
		struct event_queue_entry *next;
		media_timed_event event;
	};

	void RemoveEntry(event_queue_entry *entry);
	void CleanupEvent(media_timed_event *event);
	
	event_queue_entry *GetEnd_BeforeTime(bigtime_t eventTime, bool inclusive);
	event_queue_entry *GetStart_AfterTime(bigtime_t eventTime, bool inclusive);

	BLocker *			fLock;
	int32				fEventCount;
	event_queue_entry 	*fFirstEntry;
	event_queue_entry 	*fLastEntry;
	void * 				fCleanupHookContext;
	BTimedEventQueue::cleanup_hook 	fCleanupHook;
};

#endif //_TIMED_EVENT_QUEUE_PRIVATE_H
