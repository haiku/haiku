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
	void SetBuffer(const void *buffer, const ssize_t &size, const bool &copy=false);

	int32 Code(void) { return _code; }
	void *Buffer(void) { return _buffer; }
	ssize_t BufferSize(void) { return _buffersize; }
	
	status_t Read(bool *data);
	status_t Read(BPoint *data);
	status_t Read(BRect *data);
	status_t Read(float *data);
	status_t Read(int *data);
	status_t Read(long *data);
	status_t Read(double *data);
	status_t Read(void *data, ssize_t size);

	void Rewind(void);
	
private:
	int32 _code;
	uint8 *_buffer;
	ssize_t _buffersize;
	uint8 *_index;
};

#endif
