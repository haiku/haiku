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
//	File Name:		PortLink.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Class for low-overhead packet-style port-based messaging
//  
//------------------------------------------------------------------------------
#ifndef _PORTLINK_H
#define _PORTLINK_H

#include <BeBuild.h>
#include <OS.h>
#include "Session.h"

class PortMessage;

class PortLink
{
public:
	PortLink(port_id port);
	PortLink(const PortLink &link);
	virtual ~PortLink(void);

	void SetOpCode(int32 code);

	void SetPort(port_id port);
	port_id	GetPort();

	status_t Flush(bigtime_t timeout = B_INFINITE_TIMEOUT);
	status_t FlushWithReply(PortMessage *msg,bigtime_t timeout=B_INFINITE_TIMEOUT);
	status_t FlushToSession();

	void MakeEmpty();
	status_t Attach(const void *data, size_t size);
	status_t AttachString(const char *string);
	template <class Type> status_t Attach(Type data)
	{
		int32 size	= sizeof(Type);

		if ( (SESSION_BUFFER_SIZE*4) - fSendPosition > size){
			memcpy(fSendBuffer + fSendPosition, &data, size);
			fSendPosition += size;
			*fDataSize+=size;
			return B_OK;
		}
		return B_NO_MEMORY;
	}
	
private:
	bool fPortValid;
	port_id	fSendPort;
	port_id fReceivePort;
	char	*fSendBuffer;
	int32	fSendPosition;
	int32	fSendCode;
	int32	*fDataSize;
};

#endif
