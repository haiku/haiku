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
//	File Name:		SessionStreamReader.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Reader for BSession message streams
//					
//  
//------------------------------------------------------------------------------

#include <stdio.h>
#include "SessionStreamReader.h"

SessionMessage::SessionMessage(void)
{
}

SessionMessage::SessionMessage(const int32 &code, void *buffer, size_t buffersize)
{
	SetCode(code);
	SetBuffer(buffer,buffersize);
}

SessionMessage::~SessionMessage(void)
{
}

void SessionMessage::SetData(const int32 &code, void *buffer, size_t buffersize)
{
	SetCode(code);
	SetBuffer(buffer,buffersize);
}


SessionStreamReader::SessionStreamReader(msg_dispatcher_func *func)
{
	_buffer=NULL;
	_dispatcher=func;
	_msgsize=0;
	_msgcode=0;
	_currentbuffersize=0;
}

SessionStreamReader::~SessionStreamReader(void)
{
	if(_buffer)
	{
		printf("DEBUG: Deleting non-empty SessionStreamReader!\n");
		delete _buffer;
	}
}

void SessionStreamReader::DispatchMessages(void *msgbuffer, size_t buffersize)
{
	// This is the main functionality of the stream reader - it takes a BSession message buffer
	// and reads as much as it can. The entire function centers around _buffer. If _buffer is
	// NULL, then the stream reader is not waiting on the completion of a message.
	
	if(!msgbuffer || buffersize==0)
		return;
	
	int8 *index=(int8*)msgbuffer;
		
	// do we have a message waiting to be completed?
	while(index<((int8*)msgbuffer+buffersize))
	{
		if(_buffer)
		{
			// We need to see if the data we have received in this call
			// will complete the message or not
			if(_currentbuffersize+buffersize>_msgsize)
			{
				// It will, so copy the data to the current location in the buffer (indicated by
				// _currentbuffersize), call the Dispatch function, make the SSR empty, and iterate
								
				memcpy(_buffer+_currentbuffersize, msgbuffer, _msgsize-_currentbuffersize);
				index+=_msgsize-_currentbuffersize;

				_msg.SetData(_msgcode,_buffer,_msgsize);
				(*_dispatcher)(&_msg);
				
				delete _buffer;
				_buffer=NULL;
				_currentbuffersize=0;
				_msgsize=0;
				_msgcode=0;
				
				continue;
			}
			else
			{
				// We will not be completing the message here, so simply copy the buffer and exit
				memcpy(_buffer+_currentbuffersize, msgbuffer, buffersize);
				_currentbuffersize+=buffersize;
				break;
			}
			
		}
		else
		{
			// Get basic message data
			_msgcode=*((int32*)index); index+=sizeof(int32);
			_msgsize=*((int32*)index); index+=sizeof(int32);
			
			// Now that we have basic stuff for the message, let's see if it is complete
			if((index+_msgsize)>((int8*)msgbuffer+buffersize))
			{
				// We have an incomplete message. Allocate a buffer to contain all attached
				// data, copy it over, and exit
				_buffer=new int8[_msgsize];
				memcpy(_buffer, msgbuffer, ((int8*)msgbuffer+buffersize)-index);
				break;
			}
			else
			{
				// This message is complete, so set the appropriate SessionMessage data and
				// call the Dispatch function
				
				_msg.SetData(_msgcode,index,_msgsize);
				(*_dispatcher)(&_msg);
				index+=_msgsize;
			}
		}
	}
}

