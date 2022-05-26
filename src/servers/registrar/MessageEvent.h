//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		MessageEvent.h
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//					YellowBites (http://www.yellowbites.com)
//	Description:	A special event that sends a message when executed.
//------------------------------------------------------------------------------

#ifndef MESSAGE_EVENT_H
#define MESSAGE_EVENT_H

#include <Message.h>
#include <Messenger.h>

#include "Event.h"

class BHandler;

class MessageEvent : public Event {
public:
	MessageEvent(bigtime_t time, BHandler* handler, uint32 command);
	MessageEvent(bigtime_t time, BHandler* handler, const BMessage *message);
	MessageEvent(bigtime_t time, const BMessenger& messenger, uint32 command);
	MessageEvent(bigtime_t time, const BMessenger& messenger,
				 const BMessage *message);
	virtual ~MessageEvent();

	virtual bool Do(EventQueue *queue);

protected:
	BMessage			fMessage;
	BMessenger			fMessenger;
	BHandler			*fHandler;
};

#endif	// MESSAGE_EVENT_H
