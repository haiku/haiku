/*	
 * Copyright (c) 2000-2008, Ingo Weinhold <ingo_weinhold@gmx.de>,
 * Copyright (c) 2000-2008, Stephan Aßmus <superstippi@gmx.de>,
 * All Rights Reserved. Distributed under the terms of the MIT license.
 */
#ifndef MESSAGE_EVENT_H
#define MESSAGE_EVENT_H


#include <Message.h>

#include "AbstractLOAdapter.h"
#include "Event.h"


enum {
	MSG_EVENT	= 'evnt'
};


class MessageEvent : public Event, public AbstractLOAdapter {
 public:
								MessageEvent(bigtime_t time,
									BHandler* handler,
									uint32 command = MSG_EVENT);
								MessageEvent(bigtime_t time,
									BHandler* handler,
									const BMessage& message);
								MessageEvent(bigtime_t time,
									const BMessenger& messenger);
	virtual						~MessageEvent();

	virtual	void				Execute();

 private:
			BMessage			fMessage;
};

#endif	// MESSAGE_EVENT_H
