//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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
//	File Name:		PortQueue.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Facilitation class for reading and queueing messages from ports
//  
//------------------------------------------------------------------------------
#ifndef PORTQUEUE_H_
#define PORTQUEUE_H_

#include <OS.h>
#include <queue>

class PortMessage;

class PortQueue
{
public:
	PortQueue(const port_id &port);
	~PortQueue(void);
	bool IsInitialized(void) { return _init; }

	bool SetPort(const port_id &port);
	port_id GetPort(void) { return _port; }
	
	bool MessagesWaiting(void) { return !_q.empty(); }
	
	void GetMessagesFromPort(bool wait_for_messages=false);
	PortMessage *GetMessageFromQueue(void);

	void MakeEmpty(void);
	
private:
	std::queue<PortMessage*> _q;
	bool _init;
	port_id _port;
};

#endif
