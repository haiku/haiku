/*
 * Copyright 2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _TIMED_EVENT_QUEUE_H
#define _TIMED_EVENT_QUEUE_H

#include <MediaDefs.h>

struct _event_queue_imp;


struct media_timed_event {
								media_timed_event();
								media_timed_event(bigtime_t inTime,
									int32 inType);
								media_timed_event(bigtime_t inTime,
									int32 inType, void*inPointer,
									uint32 inCleanup);
								media_timed_event(
									bigtime_t inTime, int32 inType,
									void* inPointer, uint32 inCleanup,
									int32 inData, int64 inBigdata,
									char* inUserData, size_t dataSize = 0);

								media_timed_event(
									const media_timed_event& other);

								~media_timed_event();

			// TODO: Should this not return "media_timed_event&" ?!
			void				operator=(const media_timed_event& other);

			bigtime_t			event_time;
			int32				type;
			void*				pointer;
			uint32				cleanup;
			int32				data;
			int64				bigdata;
			char				user_data[64];
			bigtime_t			queued_time;	// Real time when put in queue

			uint32				_reserved_media_timed_event_[6];
};


bool operator==(const media_timed_event& a, const media_timed_event& b);
bool operator!=(const media_timed_event& a, const media_timed_event& b);
bool operator<(const media_timed_event& a, const media_timed_event& b);
bool operator>(const media_timed_event& a, const media_timed_event&b);


/*!	A priority queue for holding media_timed_events. Sorts by increasing time,
	so events near the front of the queue are to occur earlier.
*/
class BTimedEventQueue {
public:

			enum event_type {
				B_NO_EVENT = -1,	// Pushing this type will always fail.
				B_ANY_EVENT = 0,	// Pushing this type will always fail.
				B_START,
				B_STOP,
				B_SEEK,
				B_WARP,
				B_TIMER,
				B_HANDLE_BUFFER,
				B_DATA_STATUS,
				B_HARDWARE,
				B_PARAMETER,

				// User defined events are greater than this value.
				B_USER_EVENT = 0x4000
			};

			enum cleanup_flag {
				B_NO_CLEANUP = 0,
				B_RECYCLE_BUFFER,	// Specifies to recycle buffers handled by
									// BTimedEventQueue.
				B_EXPIRE_TIMER,		// Specifies to call TimerExpired() on the
									// event->data.
				B_USER_CLEANUP = 0x4000
			};

			enum time_direction {
				B_ALWAYS = -1,
				B_BEFORE_TIME = 0,
				B_AT_TIME,
				B_AFTER_TIME
			};


			void*				operator new(size_t size);
			void				operator delete(void* ptr, size_t size);

								BTimedEventQueue();
	virtual						~BTimedEventQueue();

			status_t			AddEvent(const media_timed_event& event);
			status_t			RemoveEvent(const media_timed_event* event);
			status_t  			RemoveFirstEvent(
									media_timed_event* _event = NULL);

			bool				HasEvents() const;
			int32				EventCount() const;


			const media_timed_event* FirstEvent() const;
			bigtime_t			FirstEventTime() const;
			const media_timed_event* LastEvent() const;
			bigtime_t			LastEventTime() const;

			const media_timed_event* FindFirstMatch(bigtime_t eventTime,
								time_direction direction,
								bool inclusive = true,
								int32 eventType = B_ANY_EVENT);


	// Queue manipulation
	// Call DoForEach to perform a function on each event in the queue.
	// Return an appropriate status defining an action to take for that event.
	// DoForEach is an atomic operation ensuring the consistency of the queue
	// during the call.
			enum queue_action {
				B_DONE = -1,
				B_NO_ACTION = 0,
				B_REMOVE_EVENT,
				B_RESORT_QUEUE
			};
			typedef queue_action (*for_each_hook)(media_timed_event* event,
				void* context);

			status_t			DoForEach(for_each_hook hook, void* context,
									bigtime_t eventTime = 0,
									time_direction direction = B_ALWAYS,
									bool inclusive = true,
									int32 eventType = B_ANY_EVENT);


	// Flushing events
	// The cleanup hook is called when events are flushed from the queue and
	// the cleanup is not B_NO_CLEANUP or B_RECYCLE_BUFFER.
			typedef void (*cleanup_hook)(const media_timed_event* event,
				void* context);
			void				SetCleanupHook(cleanup_hook hook,
									void* context);
			status_t			FlushEvents(bigtime_t eventTime,
									time_direction direction,
									bool inclusive = true,
									int32 eventType = B_ANY_EVENT);

private:
	// FBC padding and forbidden methods
								BTimedEventQueue(
									const BTimedEventQueue& other);
			BTimedEventQueue&	operator=(const BTimedEventQueue& other);

	virtual	status_t			_Reserved_BTimedEventQueue_0(void*, ...);
	virtual	status_t			_Reserved_BTimedEventQueue_1(void*, ...);
	virtual	status_t			_Reserved_BTimedEventQueue_2(void*, ...);
	virtual	status_t			_Reserved_BTimedEventQueue_3(void*, ...);
	virtual	status_t			_Reserved_BTimedEventQueue_4(void*, ...);
	virtual	status_t			_Reserved_BTimedEventQueue_5(void*, ...);
	virtual	status_t			_Reserved_BTimedEventQueue_6(void*, ...);
	virtual	status_t			_Reserved_BTimedEventQueue_7(void*, ...);
	virtual	status_t			_Reserved_BTimedEventQueue_8(void*, ...);
	virtual	status_t			_Reserved_BTimedEventQueue_9(void*, ...);
	virtual	status_t			_Reserved_BTimedEventQueue_10(void*, ...);
	virtual	status_t			_Reserved_BTimedEventQueue_11(void*, ...);
	virtual	status_t			_Reserved_BTimedEventQueue_12(void*, ...);
	virtual	status_t			_Reserved_BTimedEventQueue_13(void*, ...);
	virtual	status_t			_Reserved_BTimedEventQueue_14(void*, ...);
	virtual	status_t			_Reserved_BTimedEventQueue_15(void*, ...);
	virtual	status_t			_Reserved_BTimedEventQueue_16(void*, ...);
	virtual	status_t			_Reserved_BTimedEventQueue_17(void*, ...);
	virtual	status_t			_Reserved_BTimedEventQueue_18(void*, ...);
	virtual	status_t			_Reserved_BTimedEventQueue_19(void*, ...);
	virtual	status_t			_Reserved_BTimedEventQueue_20(void*, ...);
	virtual	status_t			_Reserved_BTimedEventQueue_21(void*, ...);
	virtual	status_t			_Reserved_BTimedEventQueue_22(void*, ...);
	virtual	status_t			_Reserved_BTimedEventQueue_23(void*, ...);

private:
			_event_queue_imp* 	fImp;

			uint32 				_reserved_timed_event_queue_[6];
};

#endif
