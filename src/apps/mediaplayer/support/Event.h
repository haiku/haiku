/*	
 * Copyright (c) 2000-2008, Ingo Weinhold <ingo_weinhold@gmx.de>,
 * Copyright (c) 2000-2008, Stephan Aßmus <superstippi@gmx.de>,
 * All Rights Reserved. Distributed under the terms of the MIT license.
 */
#ifndef EVENT_H
#define EVENT_H


#include <OS.h>


class Event {
 public:
								Event(bool autoDelete = true);
								Event(bigtime_t time, bool autoDelete = true);
	virtual						~Event();

			void				SetTime(bigtime_t time);
			bigtime_t			Time() const;

			void				SetAutoDelete(bool autoDelete);
			bool				AutoDelete() const
									{ return fAutoDelete; }

	virtual	void				Execute();

 private:
			bigtime_t			fTime;
			bool				fAutoDelete;
};

#endif	// EVENT_H
