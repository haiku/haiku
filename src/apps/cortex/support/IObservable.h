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
