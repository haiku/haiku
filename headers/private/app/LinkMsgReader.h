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
//	File Name:		LinkMsgReader.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Pahtz <pahtz@yahoo.com.au>
//	Description:	Class for receiving low-overhead port-based messages
//  
//------------------------------------------------------------------------------
#ifndef _LINKMSGREADER_H
#define _LINKMSGREADER_H

#include <OS.h>

class LinkMsgReader
{
public:
	LinkMsgReader(port_id port);
	virtual ~LinkMsgReader(void);

	virtual void SetPort(port_id port);
	virtual port_id	GetPort(void);
	
	virtual status_t GetNextMessage(int32 *code, bigtime_t timeout = B_INFINITE_TIMEOUT);
	virtual status_t Read(void *data, ssize_t size);
	virtual status_t ReadString(char **string);
	template <class Type> status_t Read(Type *data)
	{
		return Read(data, sizeof(Type));
	}
	
protected:
	virtual status_t ReadFromPort(bigtime_t timeout);
	virtual status_t AdjustReplyBuffer(bigtime_t timeout);
	void ResetBuffer();
	
	port_id fReceivePort;
	
	char	*fRecvBuffer;

	int32	fRecvPosition;	//current read position

	int32	fRecvStart;	//start of current message
	
	int32	fRecvBufferSize;

	int32	fDataSize;	//size of data in recv buffer
	int32	fReplySize;	//size of current reply message
	
	status_t fReadError;	//Read failed for current message
};

#endif
