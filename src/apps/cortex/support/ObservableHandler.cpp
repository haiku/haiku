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


// ObservableHandler.cpp

#include "ObservableHandler.h"

#include <Debug.h>
#include <Looper.h>

__USE_CORTEX_NAMESPACE

// ---------------------------------------------------------------- //
// *** deletion
// ---------------------------------------------------------------- //

// clients must call release() rather than deleting,
// to ensure that all observers are notified of the
// object's demise.  if the object has already been
// released, return an error.

status_t ObservableHandler::release() {
	if(m_released)
		return B_NOT_ALLOWED;
	
//	PRINT((
//		"ObservableHandler::release(): %ld targets\n", CountTargets()));
	
	if(!LockLooper()) {
		ASSERT(!"failed to lock looper");
	}

	m_released = true;
	
	if(CountTargets()) {
		// notify
		notifyRelease();
		UnlockLooper();
	}
	else {
		releaseComplete();	
		UnlockLooper();
		delete this;
	}
	
	return B_OK;
}

// ---------------------------------------------------------------- //
// *** ctor/dtor
// ---------------------------------------------------------------- //

ObservableHandler::~ObservableHandler() {
	if(CountTargets()) {
		PRINT((
			"*** ~ObservableHandler() '%s': %ld observers remain\n",
			Name(), CountTargets()));
	}
}

ObservableHandler::ObservableHandler(
	const char*							name) :
	BHandler(name),
	m_released(false) {}

ObservableHandler::ObservableHandler(
	BMessage*								archive) :
	BHandler(archive),
	m_released(false) {}
		
// ---------------------------------------------------------------- //
// *** accessors
// ---------------------------------------------------------------- //

// return true if release() has been called, false otherwise.
bool ObservableHandler::isReleased() const {
	return m_released;
}

// ---------------------------------------------------------------- //
// *** hooks
// ---------------------------------------------------------------- //

// sends M_OBSERVER_ADDED to the newly-added observer
void ObservableHandler::observerAdded(
	const BMessenger&				observer) {
	
	BMessage m(M_OBSERVER_ADDED);
	m.AddMessenger("target", BMessenger(this));
	observer.SendMessage(&m);
}
		
// sends M_OBSERVER_REMOVED to the newly-removed observer		
void ObservableHandler::observerRemoved(
	const BMessenger&				observer) {

	BMessage m(M_OBSERVER_REMOVED);
	m.AddMessenger("target", BMessenger(this));
	observer.SendMessage(&m);
}

// ---------------------------------------------------------------- //
// *** internal operations
// ---------------------------------------------------------------- //

// call to send the given message to all observers.
// Responsibility for deletion of the message remains with
// the caller.

status_t ObservableHandler::notify(
	BMessage*								message) {
#if DEBUG
	BLooper* l = Looper();
	ASSERT(l);
	ASSERT(l->IsLocked());
#endif

	return Invoke(message);
}

// sends M_RELEASE_OBSERVABLE
void ObservableHandler::notifyRelease() {
	BMessage m(M_RELEASE_OBSERVABLE);
	m.AddMessenger("target", BMessenger(this));
	notify(&m);
}

// ---------------------------------------------------------------- //
// *** BHandler
// ---------------------------------------------------------------- //

void ObservableHandler::MessageReceived(
	BMessage*								message) {

//	PRINT((
//		"### ObservableHandler::MessageReceived()\n"));
//	message->PrintToStream();

	switch(message->what) {
		case M_ADD_OBSERVER:
			_handleAddObserver(message);
			break;

		case M_REMOVE_OBSERVER:
			_handleRemoveObserver(message);
			break;
		
		case M_KILL_OBSERVABLE:
			// +++++ this should be an optional feature
			releaseComplete();
			delete this; // BOOM!
			break;

		default:
			_inherited::MessageReceived(message);
	}	
}

// ---------------------------------------------------------------- //
// *** BArchivable
// ---------------------------------------------------------------- //

status_t ObservableHandler::Archive(
	BMessage*								archive,
	bool										deep) const {
	
#if DEBUG
	BLooper* l = Looper();
	ASSERT(l);
	ASSERT(l->IsLocked());
#endif
	if(m_released)
		return B_NOT_ALLOWED; // can't archive a dead object
		
	return _inherited::Archive(archive, deep);
}
		
// ---------------------------------------------------------------- //
// implementation
// ---------------------------------------------------------------- //

void ObservableHandler::_handleAddObserver(
	BMessage*								message) {

#if DEBUG
	BLooper* l = Looper();
	ASSERT(l);
	ASSERT(l->IsLocked());
#endif
	BMessage reply;

	BMessenger observer;
	status_t err = message->FindMessenger(
		"observer", &observer);
	if(err < B_OK) {
		PRINT((
			"* ObservableHandler::_handleAddObserver(): no observer specified!\n"));
		// send reply? +++++
		return;
	}

	if(m_released) {
		// already quitting
		reply.what = M_BAD_TARGET;
		reply.AddMessenger("target", BMessenger(this));
		reply.AddMessenger("observer", observer);
		message->SendReply(&reply);
		
		return;
	}
	else if(IndexOfTarget(observer.Target(0)) != -1) {
		// observer already added
		reply.what = M_BAD_OBSERVER;
		reply.AddMessenger("target", BMessenger(this));
		reply.AddMessenger("observer", observer);
		message->SendReply(&reply);
		
		return;
	}	

	// valid observer given

	// add it
	err = AddTarget(observer.Target(0));
	ASSERT(err == B_OK);

	// call hook
	observerAdded(observer);
}
		
void ObservableHandler::_handleRemoveObserver(
	BMessage*								message) {

#if DEBUG
	BLooper* l = Looper();
	ASSERT(l);
	ASSERT(l->IsLocked());
#endif
	BMessage reply;

	BMessenger observer;
	status_t err = message->FindMessenger(
		"observer", &observer);
	if(err < B_OK) {
		PRINT((
			"* ObservableHandler::_handleRemoveObserver(): no observer specified!\n"));
		// send reply? +++++
		return;
	}

	int32 index = IndexOfTarget(observer.Target(0));
	if(index == -1) {
		reply.what = M_BAD_OBSERVER;
		
		reply.AddMessenger("target", BMessenger(this));
		reply.AddMessenger("observer", observer);
		message->SendReply(&reply);
		return;
	}
	
	// valid observer given; remove it & call notification hook
	RemoveTarget(index);
	observerRemoved(observer);
	
	// time to shut down?
	if(m_released && !CountTargets()) {
		releaseComplete();
		delete this; // BOOM!
	}
}


// END -- ObservableHandler.cpp --
