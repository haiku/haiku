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
//	File Name:		MessageRunner.h
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	A BMessageRunner periodically sends a message to a
//                  specified target.
//------------------------------------------------------------------------------
#ifndef _MESSAGE_RUNNER_H
#define _MESSAGE_RUNNER_H

#include <Messenger.h>

class BMessageRunner {
public:
	BMessageRunner(BMessenger target, const BMessage *message,
				   bigtime_t interval, int32 count = -1);
	BMessageRunner(BMessenger target, const BMessage *message,
				   bigtime_t interval, int32 count, BMessenger replyTo);
	virtual ~BMessageRunner();

	status_t InitCheck() const;

	status_t SetInterval(bigtime_t interval);
	status_t SetCount(int32 count);
	status_t GetInfo(bigtime_t *interval, int32 *count) const;


private:
	BMessageRunner(const BMessageRunner &);
	BMessageRunner &operator=(const BMessageRunner &);

	void InitData(BMessenger target, const BMessage *message,
				  bigtime_t interval, int32 count, BMessenger replyTo);
	status_t SetParams(bool resetInterval, bigtime_t interval, bool resetCount,
					   int32 count);

	virtual void _ReservedMessageRunner1();
	virtual void _ReservedMessageRunner2();
	virtual void _ReservedMessageRunner3();
	virtual void _ReservedMessageRunner4();
	virtual void _ReservedMessageRunner5();
	virtual void _ReservedMessageRunner6();

	int32	fToken;
	uint32	_reserved[6];
};

#endif	// _MESSAGE_RUNNER_H
