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

PortMessage::PortMessage(const int32 &code, const void *buffer, const ssize_t &buffersize,
		const bool &copy)
{
	_code=code;
	SetBuffer(buffer,buffersize,copy);
}

PortMessage::PortMessage(void)
{
	_code=0;
	_buffer=NULL;
	_buffersize=0;
}

PortMessage::~PortMessage(void)
{
	if(_buffersize>0 && _buffer!=NULL)
		delete _buffer;
}


status_t PortMessage::ReadFromPort(const port_id &port, const bigtime_t &timeout=B_INFINITE_TIMEOUT)
{
	if(_buffersize>0 && _buffer!=NULL)
		delete _buffer;
	_buffer=NULL;
	_buffersize=0;

	if(timeout==B_INFINITE_TIMEOUT)
	{
		_buffersize=port_buffer_size(port);
		if(_buffersize==B_BAD_PORT_ID)
			return (status_t)_buffersize;

		if(_buffersize>0)
			_buffer=new char[_buffersize];
		read_port(port, &_code, _buffer, _buffersize);
	}
	else
	{
		_buffersize=port_buffer_size_etc(port,B_TIMEOUT, timeout);
		if(_buffersize==B_TIMED_OUT || _buffersize==B_BAD_PORT_ID || 
				_buffersize==B_WOULD_BLOCK)
			return (status_t)_buffersize;
			
		if(_buffersize>0)
			_buffer=new char[_buffersize];
		read_port(port, &_code, _buffer, _buffersize);
	}
	
	return B_OK;
}

status_t PortMessage::WriteToPort(const port_id &port)
{
	// Check port validity
	port_info pi;
	if(get_port_info(port,&pi)!=B_OK)
		return B_BAD_VALUE;
	
	write_port(port,_code, _buffer, _buffersize);
	
	if(_buffer && _buffersize>0)
	{
		delete _buffer;
		_buffersize=0;
		_buffer=NULL;
	}
	return B_OK;
}
	
void PortMessage::SetCode(const int32 &code)
{
	_code=code;
}

void PortMessage::SetBuffer(const void *buffer, const ssize_t &size, const bool &copy)
{
	if(_buffersize>0 && _buffer!=NULL)
		delete _buffer;
	_buffer=NULL;
	_buffersize=0;

	if(!buffer || size<1)
		return;

	if(copy)
	{
		_buffersize=size;
		_buffer=new char[_buffersize];
		memcpy(_buffer,buffer,_buffersize);
	}
	else
	{
		_buffersize=size;
		_buffer=(void *)buffer;
	}
}


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

void PortQueue::GetMessagesFromPort(bool wait_for_messages=false)
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

