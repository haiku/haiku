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
//					Pahtz <pahtz@yahoo.com.au>
//	Description:	Class for low-overhead port-based messaging
//  
//------------------------------------------------------------------------------
#ifndef _PORTLINK_H
#define _PORTLINK_H

#include <OS.h>

/*
	Error checking rules: (for if you don't want to check every return code)
	
	Calling EndMessage() is optional, implied by Flush() or StartMessage().

	If you are sending just one message you only need to test Flush() == B_OK

	If you are buffering multiple messages without calling Flush() you must
		check EndMessage() == B_OK, or the last Attach() for each message. Check
		Flush() at the end.

	If you are reading, check the last Read() or ReadString() you perform.

*/

class BPortLink
{
public:
	BPortLink(port_id send = -1, port_id reply = -1);
	virtual ~BPortLink();

	status_t StartMessage(int32 code);
	void CancelMessage();
	status_t EndMessage();

	status_t Flush(bigtime_t timeout = B_INFINITE_TIMEOUT);
	
	// see BPrivate::BAppServerLink which inherits from BPortLink
	//status_t FlushWithReply(int32 *code);

	void SetSendPort(port_id port);
	port_id	GetSendPort();
	void SetReplyPort(port_id port);
	port_id	GetReplyPort();

	status_t Attach(const void *data, ssize_t size);
	status_t AttachString(const char *string);
	template <class Type> status_t Attach(const Type& data)
	{
		return Attach(&data, sizeof(Type));
	}

	status_t GetNextReply(int32 *code, bigtime_t timeout = B_INFINITE_TIMEOUT);
	status_t Read(void *data, ssize_t size);
	status_t ReadString(char **string);
	template <class Type> status_t Read(Type *data)
	{
		return Read(data, sizeof(Type));
	}
	
protected:
	status_t FlushCompleted(ssize_t newbuffersize);
	status_t ReadFromPort(bigtime_t timeout);
	status_t AdjustReplyBuffer(bigtime_t timeout);
	void ResetReplyBuffer();
	
	port_id	fSendPort;
	port_id fReceivePort;

	char	*fSendBuffer;
	char	*fRecvBuffer;

	int32	fSendPosition;	//current append position
	int32	fRecvPosition;	//current read position

	int32	fSendStart;	//start of current message
	int32	fRecvStart;	//start of current message
	
	int32	fSendBufferSize;
	int32	fRecvBufferSize;

	int32	fSendCount;	//number of completed messages in buffer

	int32	fDataSize;	//size of data in recv buffer
	int32	fReplySize;	//size of current reply message
	
	status_t fWriteError;	//Attach failed for current message
	status_t fReadError;	//Read failed for current message
};

#endif
