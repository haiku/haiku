/*
 * Copyright 1999, Be Incorporated.
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


////////////////////////////////////////////////////////////
// MultiInvoker.h
// --------------
// Declares the MultiInvoker class.
//


#ifndef _MultiInvoker_h
#define _MultiInvoker_h

#include <List.h>

////////////////////////////////////////////////////////////
// Class: MultiInvoker
// -------------------
// A BInvoker-like class that allows you to send messages
// to multiple targets. In addition, it will work with
// any BMessenger-derived class that you create.

class MultiInvoker
{
public:
		MultiInvoker();
		MultiInvoker(const MultiInvoker& src);
virtual	~MultiInvoker();

		MultiInvoker&	operator=(const MultiInvoker& src);

virtual	void		SetMessage(BMessage* message);
		BMessage*	Message() const;
		uint32		Command() const;

virtual	status_t	AddTarget(const BHandler* h, const BLooper* loop=0);

		// For this version of AddTarget, the MultiInvoker
		// will assume ownership of the messenger.
virtual status_t	AddTarget(BMessenger* messenger);
virtual	void		RemoveTarget(const BHandler* h);
virtual void		RemoveTarget(int32 index);
		int32		IndexOfTarget(const BHandler* h) const;
		int32		CountTargets() const;
		BHandler*	TargetAt(int32 index, BLooper** looper=0) const;
		BMessenger*	MessengerAt(int32 index) const;
		bool		IsTargetLocal(int32 index) const;

		void		SetTimeout(bigtime_t timeout);
		bigtime_t	Timeout() const;

virtual	void		SetHandlerForReply(BHandler* h);
		BHandler*	HandlerForReply() const;

virtual status_t	Invoke(BMessage* msg =0);

private:
		void		Clear();
		void		Clone(const MultiInvoker& src);

		BMessage*	m_message;
		BList		m_messengers;
		bigtime_t	m_timeout;
		BHandler*	m_replyHandler;
};

#endif /* _MultiInvoker_h */
