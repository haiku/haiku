// Observe.h (cortex)
// * PURPOSE
//   Messages used for implementation of the Observer pattern.
//
// * HISTORY
//   e.moon		19aug99		Begun

#ifndef __Observe_H__
#define __Observe_H__

#include <Messenger.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

// -------------------------------------------------------- //
// *** MESSAGES ***
// -------------------------------------------------------- //

// messages sent to the Observable (source)
enum observer_message_t {
	// Requests that an observer be added to a given
	// observable (target).
	// - "observer" (BMessenger)
	M_ADD_OBSERVER							= Observer_message_base,
	
	// Requests that a given observable (target) stop
	// sending notifications to a given observer.
	// Should be sent in response to M_RELEASE_OBSERVABLE
	// in order to allow the observable object to be deleted.
	// - "observer" (BMessenger)
	M_REMOVE_OBSERVER,
	
	// Requests that an observable quit immediately without
	// waiting for its observers to acknowledge that they're done.
	// This should only be posted to an Observable in an
	// emergency
	M_KILL_OBSERVABLE
};


// messages sent by the Observable (target)
enum target_message_t {
	// sent when the target is no longer needed; the
	// observer should reply with M_REMOVE_OBSERVER in order
	// to release (allow the deletion of) the target
	// - "target" (BMessenger)
	M_RELEASE_OBSERVABLE				= Observable_message_base,
	
	// SUGGESTED BUT NOT REQUIRED FOR IObservable IMPLEMENTATION:
	// *** IObservables are encouraged to send other replies!
	// sent upon successful receipt of M_ADD_OBSERVER
	// - "target" (BMessenger)
	M_OBSERVER_ADDED,
	
	// SUGGESTED BUT NOT REQUIRED FOR IObservable IMPLEMENTATION:
	// *** IObservables are encouraged to send other replies!
	// sent upon successful receipt of M_REMOVE_OBSERVER
	// - "target" (BMessenger)
	M_OBSERVER_REMOVED,
	
	// sent when no matching observer was found for an
	// M_REMOVE_OBSERVER message, or if the observer specified
	// in an M_ADD_OBSERVER message was previously added.
	// - "target" (BMessenger)
	// - "observer" (BMessenger)
	M_BAD_OBSERVER,
	
	// sent when the target receiving an M_ADD_OBSERVER
	// or M_REMOVE_OBSERVER didn't match the target described
	// in the message.  also sent if the target is currently
	// in the process of quitting (it's already sent M_RELEASE_OBSERVABLE
	// to its current observers, and is waiting for them to acknowledge
	// with M_REMOVE_OBSERVER so that it can delete itself.)
	// - "target" (BMessenger)
	// - "observer" (BMessenger)
	M_BAD_TARGET
};

// -------------------------------------------------------- //
// *** FUNCTIONS ***
// -------------------------------------------------------- //

// * Asynchronous

status_t add_observer(
	const BMessenger&				observer,
	const BMessenger&				target);
	
status_t remove_observer(
	const BMessenger&				observer,
	const BMessenger&				target);

// * Synchronous

status_t add_observer(
	const BMessenger&				observer,
	const BMessenger&				target,
	BMessage&								reply,
	bigtime_t								timeout =B_INFINITE_TIMEOUT);

status_t remove_observer(
	const BMessenger&				observer,
	const BMessenger&				target,
	BMessage&								reply,
	bigtime_t								timeout =B_INFINITE_TIMEOUT);
		
// -------------------------------------------------------- //
// *** TOOL CLASSES ***
// -------------------------------------------------------- //

template <class _observable_t>
class observer_handle {

public:										// ctor/dtor

	virtual ~observer_handle() {}

	observer_handle(
		const BMessenger&			observer,
		_observable_t*				target,
		bigtime_t							timeout =B_INFINITE_TIMEOUT) :
		
		_observer_messenger(observer),
		_target_messenger(target),
		_target_cache(target),
		_timeout(timeout),
		_valid(false) {
		
		BMessage reply;
		status_t err = add_observer(
			_observer_messenger,
			_target_messenger,
			reply, timeout);
		if(err < B_OK) {
			PRINT((
				"! observer_handle<>(): add_observer() failed:\n"
				"  %s\n", strerror(err)));
#if DEBUG
			PRINT((
				"  * target's reply:\n"));
			reply.PrintToStream();
#endif
		}
		else _valid = true;
	}

public:										// interface

	virtual void release() {
		if(!_valid) {
			PRINT((
				"! observer_handle<>::release(): invalid or already released.\n"));
			return;
		}
	
		BMessage reply;
		status_t err = remove_observer(
			_observer_messenger,
			_target_messenger,
			reply,
			timeout);
		if(err < B_OK) {
			PRINT((
				"! observer_handle<>::release(): remove_observer() failed:\n"
				"  %s\n", strerror(err)));
#if DEBUG
			PRINT((
				"  * target's reply:\n"));
			reply.PrintToStream();
#endif
		}
		
		_valid = false;
	}

private:
	const BMessenger				_observer_messenger;
	BMessenger							_target_messenger;
	_observable_t*					_target_cache;
	bigtime_t								_timeout;
	bool										_valid;
};


__END_CORTEX_NAMESPACE
#endif /*__Observe_H__*/
