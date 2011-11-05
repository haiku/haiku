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


// ObservableLooper.cpp

#include "ObservableLooper.h"

#include <Debug.h>
#include <MessageRunner.h>

__USE_CORTEX_NAMESPACE


// ---------------------------------------------------------------- //
// *** deletion
// ---------------------------------------------------------------- //

// clients must call release() rather than deleting,
// to ensure that all observers are notified of the
// object's demise.  if the object has already been
// released, return an error.

status_t ObservableLooper::release() {

	// +++++ what if I'm not running?
	// +++++ is a lock necessary?
	
	if(isReleased())
		return B_NOT_ALLOWED;
	
	// send request through proper channels
	BMessenger(this).SendMessage(B_QUIT_REQUESTED);

	return B_OK;
}

// ---------------------------------------------------------------- //
// *** ctor/dtor
// ---------------------------------------------------------------- //

ObservableLooper::~ObservableLooper() {
	if(CountTargets()) {
		PRINT((
			"*** ~ObservableLooper() '%s': %ld observers remain\n",
			Name(), CountTargets()));
	}
	if(m_executioner)
		delete m_executioner;
}

ObservableLooper::ObservableLooper(
	const char*							name,
	int32										priority,
	int32										portCapacity,
	bigtime_t								quitTimeout) :
	BLooper(name, priority, portCapacity),
	m_quitTimeout(quitTimeout),
	m_executioner(0),
	m_quitting(false) {}

ObservableLooper::ObservableLooper(
	BMessage*								archive) :
	BLooper(archive),
	m_quitTimeout(B_INFINITE_TIMEOUT),
	m_executioner(0),
	m_quitting(false) {

	archive->FindInt64("quitTimeout", (int64*)&m_quitTimeout);
}

// ---------------------------------------------------------------- //
// *** accessors
// ---------------------------------------------------------------- //

bool ObservableLooper::isReleased() const {
	return m_quitting;
}

// ---------------------------------------------------------------- //
// *** hooks
// ---------------------------------------------------------------- //

// sends M_OBSERVER_ADDED to the newly-added observer
void ObservableLooper::observerAdded(
	const BMessenger&				observer) {
	
	BMessage m(M_OBSERVER_ADDED);
	m.AddMessenger("target", BMessenger(this));
	observer.SendMessage(&m);
}
		
// sends M_OBSERVER_REMOVED to the newly-removed observer		
void ObservableLooper::observerRemoved(
	const BMessenger&				observer) {

	BMessage m(M_OBSERVER_REMOVED);
	m.AddMessenger("target", BMessenger(this));
	observer.SendMessage(&m);
}

// ---------------------------------------------------------------- //
// *** internal operations
// ---------------------------------------------------------------- //

status_t ObservableLooper::notify(
	BMessage*								message) {
	ASSERT(IsLocked());

	return Invoke(message);
}

// sends M_RELEASE_OBSERVABLE
void ObservableLooper::notifyRelease() {
	BMessage m(M_RELEASE_OBSERVABLE);
	m.AddMessenger("target", BMessenger(this));
	notify(&m);
}

// ---------------------------------------------------------------- //
// *** BLooper
// ---------------------------------------------------------------- //

void ObservableLooper::Quit() {
	ASSERT(IsLocked());
	
	if(QuitRequested()) {
		releaseComplete();
		_inherited::Quit();
	}
	else
		Unlock();
}

bool ObservableLooper::QuitRequested() {

	if(CountTargets()) {
		if(!m_quitting) {
			m_quitting = true;
			
			// no release request yet sent
			notifyRelease();

			if(m_quitTimeout != B_INFINITE_TIMEOUT) {
				// Initiate a timer to force quit -- if an observer
				// has died, it shouldn't take me down with it.
				ASSERT(!m_executioner);
				m_executioner = new BMessageRunner(
					BMessenger(this),
					new BMessage(M_KILL_OBSERVABLE),
					m_quitTimeout,
					1);
			}			
		}

		// targets remain, so don't quit.
		return false;
	}
	
	// okay to quit
	return true;
}
	
// ---------------------------------------------------------------- //
// *** BHandler
// ---------------------------------------------------------------- //

void ObservableLooper::MessageReceived(
	BMessage*								message) {
	
//	PRINT((
//		"### ObservableLooper::MessageReceived()\n"));
//	message->PrintToStream();

	switch(message->what) {
		case M_ADD_OBSERVER:
			_handleAddObserver(message);
			break;

		case M_REMOVE_OBSERVER:
			_handleRemoveObserver(message);
			break;
		
		case M_KILL_OBSERVABLE:
			releaseComplete();
			BLooper::Quit();
			break;

		default:
			_inherited::MessageReceived(message);
	}
}

// ---------------------------------------------------------------- //
// *** BArchivable
// ---------------------------------------------------------------- //

status_t ObservableLooper::Archive(
	BMessage*								archive,
	bool										deep) const {
	
	ASSERT(IsLocked());

	// can't archive an object in limbo
	if(m_quitting)
		return B_NOT_ALLOWED;
	
	status_t err = _inherited::Archive(archive, deep);
	if(err < B_OK)
		return err;
	
	archive->AddInt64("quitTimeout", m_quitTimeout);
	return B_OK;
}		

// ---------------------------------------------------------------- //
// implementation
// ---------------------------------------------------------------- //

void ObservableLooper::_handleAddObserver(
	BMessage*								message) {

	BMessage reply;

	BMessenger observer;
	status_t err = message->FindMessenger(
		"observer", &observer);
	if(err < B_OK) {
		PRINT((
			"* ObservableLooper::_handleAddObserver(): no observer specified!\n"));
		// send reply? +++++
		return;
	}

	// at this point, a reply of some sort will be sent		
	reply.AddMessenger("target", BMessenger(this));
	reply.AddMessenger("observer", observer);

	if(m_quitting) {
		// already quitting
		reply.what = M_BAD_TARGET;
	}
	else if(IndexOfTarget(observer.Target(0)) != -1) {
		// observer already added
		reply.what = M_BAD_OBSERVER;
	}	
	else {
		// add it
		err = AddTarget(observer.Target(0));
		ASSERT(err == B_OK);
		reply.what = M_OBSERVER_ADDED;
	}
	
	// send reply
	message->SendReply(&reply);
	
	// call hook
	observerAdded(observer);
}
		
void ObservableLooper::_handleRemoveObserver(
	BMessage*								message) {

//	PRINT(("ObservableLooper::_handleRemoveObserver():\n"
//		"  %ld targets\n", CountTargets()));
	BMessage reply;

	BMessenger observer;
	status_t err = message->FindMessenger(
		"observer", &observer);
	if(err < B_OK) {
		PRINT((
			"* ObservableLooper::_handleRemoveObserver(): no observer specified!\n"));
		// send reply? +++++
		return;
	}

	// at this point, a reply of some sort will be sent		
	reply.AddMessenger("target", BMessenger(this));
	reply.AddMessenger("observer", observer);
	
	int32 index = IndexOfTarget(observer.Target(0));
	if(index == -1) {
		reply.what = M_BAD_OBSERVER;
	}
	else {
		RemoveTarget(index);
		reply.what = M_OBSERVER_REMOVED;
	}
	
	message->SendReply(&reply);
	
	// call hook
	observerRemoved(observer);
	
	// time to shut down?
	if(m_quitting && !CountTargets()) {
		releaseComplete();
		BLooper::Quit();
	}
}

// END -- ObservableLooper.cpp --
