/*
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


// ObservableHandler.h
// * PURPOSE
//   Implementation of an observable BHandler.
//
// * HISTORY
//   e.moon		19aug99		Begun.

#ifndef __ObservableHandler_H__
#define __ObservableHandler_H__

#include <Handler.h>

#include "observe.h"
#include "IObservable.h"
#include "MultiInvoker.h"

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class ObservableHandler :
	public	BHandler,
	public	IObservable,
	private	MultiInvoker {
	
	typedef	BHandler _inherited;

public:											// *** deletion
	// clients must call release() rather than deleting,
	// to ensure that all observers are notified of the
	// object's demise.  if the object has already been
	// released, return an error.

	virtual status_t release();

public:											// *** ctor/dtor
	virtual ~ObservableHandler();
	ObservableHandler(
		const char*							name=0);
	ObservableHandler(
		BMessage*								archive);
		
public:											// *** accessors
	// return true if release() has been called, false otherwise.
	bool isReleased() const;

protected:									// *** hooks
	// sends M_OBSERVER_ADDED to the newly-added observer
	virtual void observerAdded(
		const BMessenger&				observer);

	// sends M_OBSERVER_REMOVED to the newly-removed observer		
	virtual void observerRemoved(
		const BMessenger&				observer);

protected:									// *** internal operations
	// call to send the given message to all observers.
	// Responsibility for deletion of the message remains with
	// the caller.
	// * LOCKING: the BLooper must be locked.
	virtual status_t notify(
		BMessage*								message);

	// sends M_RELEASE_OBSERVABLE
	virtual void notifyRelease();

public:											// *** BHandler
	virtual void MessageReceived(
		BMessage*								message);

public:											// *** BArchivable
	// * LOCKING: the BLooper must be locked.
	virtual status_t Archive(
		BMessage*								archive,
		bool										deep=true) const;
		
private:										// implementation
	void _handleAddObserver(
		BMessage*								message); //nyi
		
	void _handleRemoveObserver(
		BMessage*								message); //nyi

private:										// members
	bool											m_released;
};

__END_CORTEX_NAMESPACE
#endif /*__ObservableHandler_H__*/

