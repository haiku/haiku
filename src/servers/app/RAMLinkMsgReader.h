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
//	File Name:		RAMLinkMsgReader.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Class for reading Link messages from a memory buffer
//  
//------------------------------------------------------------------------------
#ifndef RAM_LINK_MSG_READER_H
#define RAM_LINK_MSG_READER_H

#include <LinkMsgReader.h>

/*
	This class is somewhat an inheritance hack to avoid some *serious* code
	duplication in the app_server. Its only intended use is for reading back 
	PortLink message sent over an area in cases where it is too large to fit 
	through a port.
	
	Expected format:
		int32 message code
		size_t buffer size
		[data buffer]
*/
class RAMLinkMsgReader : public LinkMsgReader
{
public:
	RAMLinkMsgReader(int8 *buffer);
	RAMLinkMsgReader(void);
	~RAMLinkMsgReader(void);
	
	void SetBuffer(int8 *buffer);
	int8 *GetBuffer(void);
	size_t GetBufferSize(void);
	int32 Code(void) { return fCode; }
	
	status_t Read(void *data, ssize_t size);
	status_t ReadString(char **string);
	template <class Type> status_t Read(Type *data)
	{
		return Read(data, sizeof(Type));
	}

	// These should never need to be called where this class is used. However, we do
	// need to make debugging easier for such contexts...
	status_t GetNextMessage(int32 *code, bigtime_t timeout);
	void SetPort(port_id port);
	port_id GetPort(void);
	
protected:
	
	
	int8 *fBuffer, *fAttachStart;
	int8 *fPosition;
	size_t fAttachSize;
	int32 fCode;
};

#endif

