////////////////////////////////////////////////////////////
// MultiInvoker.h
// --------------
// Declares the MultiInvoker class.
//
// Copyright 1999, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample
// Code License.

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
