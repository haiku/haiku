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
//	File Name:		SessionStreamReader.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Reader for BSession message streams
//					
//  
//------------------------------------------------------------------------------
#ifndef SSREADER_H_
#define SSREADER_H_

#include <PortMessage.h>

typedef void *msg_dispatcher_func(PortMessage *msg);

class SessionMessage : public PortMessage
{
public:
	SessionMessage(void);
	SessionMessage(const int32 &code, void *buffer, size_t buffersize);
	~SessionMessage(void);
	void SetData(const int32 &code, void *buffer, size_t buffersize);
};

class SessionStreamReader
{
public:
	SessionStreamReader(msg_dispatcher_func *func);
	~SessionStreamReader(void);
	void DispatchMessages(void *msgbuffer, size_t buffersize);

protected:
	int8 *fBuffer;
	uint32 fMsgSize,fMsgCode, fCurrentBufferSize;
	msg_dispatcher_func *fDispatcherFunc;
	SessionMessage fSessionMessage;
};

#endif
