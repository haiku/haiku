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
//	File Name:		PortMessage.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Package class for port messaging-based data
//  
//------------------------------------------------------------------------------
#ifndef PORTMESSAGE_H_
#define PORTMESSAGE_H_

#include <OS.h>
#include <ServerProtocol.h>

class PortMessage
{
public:
	PortMessage(const int32 &code, const void *buffer, const ssize_t &buffersize,
			 const bool &copy);
	PortMessage(void);
	~PortMessage(void);
	
	status_t ReadFromPort(const port_id &port, const bigtime_t &timeout=B_INFINITE_TIMEOUT);
	status_t WriteToPort(const port_id &port);	
	
	void SetCode(const int32 &code);
	void SetProtocol(int32 protocol) { _protocol=protocol; }
	void SetBuffer(const void *buffer, const ssize_t &size, const bool &copy=false);

	int32 Code(void) { return _code; }
	void *Buffer(void) { return _buffer; }
	ssize_t BufferSize(void) { return _buffersize; }
	int32 Protocol(void) const { return _protocol; }
	
	status_t Read(void *data, ssize_t size);
	status_t ReadString(char **string);
	template <class Type> status_t Read(Type *data)
		{
			int32 size = sizeof(Type);

			if(!data)
				return B_BAD_VALUE;

			if( !_buffer || 
				(_buffersize < size) ||
				(_index+size > _buffer+_buffersize) )
				return B_NO_MEMORY;
	
			*data=*((Type*)_index);
			_index+=size;
	
			return B_OK;
		}

	void Rewind(void);
private:
	int32 _code;
	uint8 *_buffer;
	ssize_t _buffersize;
	uint8 *_index;
	int32 _protocol;
};

#endif
