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
//	File Name:		PortQueue.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Facilitation class for reading and queueing messages from ports
//  
//------------------------------------------------------------------------------
#include "PortQueue.h"
#include "PortMessage.h"

PortQueue::PortQueue(const port_id &port)
{
	port_info pi;
	_init=(get_port_info(port,&pi)==B_OK)?true:false;
	_port=port;
}

PortQueue::~PortQueue(void)
{
	MakeEmpty();
}

bool PortQueue::SetPort(const port_id &port)
{
	port_info pi;
	_init=(get_port_info(port,&pi)==B_OK)?true:false;
	_port=port;

	return _init;
}

void PortQueue::GetMessagesFromPort(bool wait_for_messages)
{
	if(_init)
	{
		PortMessage *msg;
		ssize_t size;
		port_info pi;
		get_port_info(_port, &pi);
		status_t stat;

		if(pi.queue_count==0 && wait_for_messages)
		{
			size=port_buffer_size(_port);
			// now that there is a message, we'll just fall through to the
			// port-emptying loop
			get_port_info(_port, &pi);
		}
		
		for(int32 i=0; i<pi.queue_count; i++)
		{
			msg=new PortMessage();
			stat=msg->ReadFromPort(_port,0);
			
			if(stat!=B_OK)
			{
				delete msg;
				break;
			}
			msg->BSessionWorkaround();
			_q.push(msg);
		}
		
	}
}

PortMessage *PortQueue::GetMessageFromQueue(void)
{
	if(_q.empty())
		return NULL;

	PortMessage *msg=_q.front();
	_q.pop();
	msg->BSessionWorkaround();
	return msg;
}

void PortQueue::MakeEmpty(void)
{
	PortMessage *msg;
	while(!_q.empty())
	{
		msg=_q.front();
		_q.pop();
		delete msg;
	}
}

