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


// IObservable.h
// * PURPOSE
//   Defines a general observable-object interface.
// * HISTORY
//   e.moon		19aug99		Begun

#ifndef __IObservable_H__
#define __IObservable_H__

class BMessenger;

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class IObservable {

public:
	IObservable() { }
	virtual ~IObservable() { }
public:											// *** deletion
	// clients must call release() rather than deleting,
	// to ensure that all observers are notified of the
	// object's demise.  if the object has already been
	// released, return an error.
	virtual status_t release()=0;

public:											// *** accessors
	// return true if release() has been called, false otherwise.
	virtual bool isReleased() const=0;

protected:									// *** hooks
	// must be called after an observer has been added (this is a good
	// place to send an acknowledgement)
	virtual void observerAdded(
		const BMessenger&				observer) {TOUCH(observer);}
		
	// must be called after an observer has been removed
	virtual void observerRemoved(
		const BMessenger&				observer) {TOUCH(observer);}

	// must be called once all observers have been removed
	virtual void releaseComplete() {}

protected:									// *** operations
	// call to send the given message to all observers.
	// Responsibility for deletion of the message remains with
	// the caller.
	virtual status_t notify(
		BMessage*								message)=0;

	// must be called by release() if targets remain to be
	// released.  should notify all targets with an appropriate
	// release-notification message (such as M_RELEASE_OBSERVABLE).
	virtual void notifyRelease()=0;
};

__END_CORTEX_NAMESPACE
#endif /*__IObservable_H__*/
