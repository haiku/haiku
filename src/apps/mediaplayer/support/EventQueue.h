/*	
 * Copyright (c) 2000-2008, Ingo Weinhold <ingo_weinhold@gmx.de>,
 * Copyright (c) 2000-2008, Stephan Aßmus <superstippi@gmx.de>,
 * All Rights Reserved. Distributed under the terms of the MIT license.
 */
#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H


#include <List.h>
#include <Locker.h>
#include <OS.h>


class Event;


class EventQueue : public BLocker {
 public:
 								EventQueue();
 	virtual						~EventQueue();

			status_t			InitCheck();

	static	EventQueue*			CreateDefault();
	static	void				DeleteDefault();
	static	EventQueue&			Default();

			void				AddEvent(Event* event);
			bool				RemoveEvent(Event* event);
			void				ChangeEvent(Event* event,
									bigtime_t newTime);

 private:
			void				_AddEvent(Event* event);
			Event*				_EventAt(int32 index) const;

	static	int32				_execute_events_(void *cookie);
			int32				_ExecuteEvents();
			void				_Reschedule();

			BList				fEvents;
			thread_id			fEventExecutor;
			sem_id				fThreadControl;
	volatile bigtime_t			fNextEventTime;
			status_t			fStatus;
	static	EventQueue*			fDefaultQueue;
};

#endif	// EVENT_QUEUE_H
