/*
 * Copyright 2009-2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _TIMED_EVENT_QUEUE_H
#define _TIMED_EVENT_QUEUE_H


#include <MediaDefs.h>


struct media_timed_event {
								media_timed_event();
								media_timed_event(bigtime_t inTime, int32 inType);
								media_timed_event(bigtime_t inTime, int32 inType,
									void* inPointer, uint32 inCleanup);
								media_timed_event(bigtime_t inTime, int32 inType,
									void* inPointer, uint32 inCleanup,
									int32 inData, int64 inBigdata,
									const char* inUserData, size_t dataSize = 0);

								media_timed_event(
									const media_timed_event& other);

								~media_timed_event();

			void				operator=(const media_timed_event& other);

public:
			bigtime_t			event_time;
			int32				type;
			void*				pointer;
			uint32				cleanup;
			int32				data;
			int64				bigdata;
			char				user_data[64];

private:
			uint32				_reserved_media_timed_event_[8];
};


bool operator==(const media_timed_event& a, const media_timed_event& b);
bool operator!=(const media_timed_event& a, const media_timed_event& b);
bool operator<(const media_timed_event& a, const media_timed_event& b);
bool operator>(const media_timed_event& a, const media_timed_event&b);


namespace BPrivate {
	class TimedEventQueueData;
};

class BTimedEventQueue {
public:
			enum event_type {
				B_NO_EVENT = -1,
				B_ANY_EVENT = 0,

				B_START,
				B_STOP,
				B_SEEK,
				B_WARP,
				B_TIMER,
				B_HANDLE_BUFFER,
				B_DATA_STATUS,
				B_HARDWARE,
				B_PARAMETER,

				B_USER_EVENT = 0x4000
			};

			enum cleanup_flag {
				B_NO_CLEANUP = 0,
				B_RECYCLE_BUFFER,
				B_EXPIRE_TIMER,
				B_USER_CLEANUP = 0x4000
			};

			enum time_direction {
				B_ALWAYS = -1,
				B_BEFORE_TIME = 0,
				B_AT_TIME,
				B_AFTER_TIME
			};

public:
			void*				operator new(size_t size);
			void				operator delete(void* ptr, size_t size);

								BTimedEventQueue();
	virtual						~BTimedEventQueue();

			typedef void (*cleanup_hook)(const media_timed_event* event,
				void* context);
			void				SetCleanupHook(cleanup_hook hook,
									void* context);

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

			status_t			FlushEvents(bigtime_t eventTime,
									time_direction direction,
									bool inclusive = true,
									int32 eventType = B_ANY_EVENT);

private:
	// FBC padding and forbidden methods
	virtual	void				_ReservedTimedEventQueue0();
	virtual	void				_ReservedTimedEventQueue1();
	virtual	void				_ReservedTimedEventQueue2();
	virtual	void				_ReservedTimedEventQueue3();
	virtual	void				_ReservedTimedEventQueue4();
	virtual	void				_ReservedTimedEventQueue5();
	virtual	void				_ReservedTimedEventQueue6();
	virtual	void				_ReservedTimedEventQueue7();
	virtual	void				_ReservedTimedEventQueue8();
	virtual	void				_ReservedTimedEventQueue9();
	virtual	void				_ReservedTimedEventQueue10();
	virtual	void				_ReservedTimedEventQueue11();
	virtual	void				_ReservedTimedEventQueue12();
	virtual	void				_ReservedTimedEventQueue13();
	virtual	void				_ReservedTimedEventQueue14();
	virtual	void				_ReservedTimedEventQueue15();
	virtual	void				_ReservedTimedEventQueue16();
	virtual	void				_ReservedTimedEventQueue17();
	virtual	void				_ReservedTimedEventQueue18();
	virtual	void				_ReservedTimedEventQueue19();
	virtual	void				_ReservedTimedEventQueue20();
	virtual	void				_ReservedTimedEventQueue21();
	virtual	void				_ReservedTimedEventQueue22();
	virtual	void				_ReservedTimedEventQueue23();

								BTimedEventQueue(const BTimedEventQueue&);
			BTimedEventQueue&	operator=(const BTimedEventQueue&);

private:
	static	int					_Match(const media_timed_event& event,
										 bigtime_t eventTime,
										 time_direction direction,
										 bool inclusive, int32 eventType);

private:
			BPrivate::TimedEventQueueData* fData;

			uint32 				_reserved[6];
};


#endif
