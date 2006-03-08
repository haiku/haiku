// observe.cpp

#include "observe.h"

#include <Debug.h>
#include <Message.h>

//__USE_CORTEX_NAMESPACE

// -------------------------------------------------------- //
// *** HELPERS ***
// -------------------------------------------------------- //

// * Asynchronous

status_t __CORTEX_NAMESPACE__ add_observer(
	const BMessenger&				observer,
	const BMessenger&				target) {
//	ASSERT(observer.IsValid());
//	ASSERT(target.IsValid());
	
	BMessage m(M_ADD_OBSERVER);
	m.AddMessenger("observer", observer);
	return target.SendMessage(&m, observer);
}
	
status_t __CORTEX_NAMESPACE__ remove_observer(
	const BMessenger&				observer,
	const BMessenger&				target) {
//	ASSERT(observer.IsValid());
//	ASSERT(target.IsValid());
	
	BMessage m(M_REMOVE_OBSERVER);
	m.AddMessenger("observer", observer);
	return target.SendMessage(&m, observer);
}

// * Synchronous

status_t __CORTEX_NAMESPACE__ add_observer(
	const BMessenger&				observer,
	const BMessenger&				target,
	BMessage&								reply,
	bigtime_t								timeout) {
//	ASSERT(observer.IsValid());
//	ASSERT(target.IsValid());
	
	BMessage m(M_ADD_OBSERVER);
	m.AddMessenger("observer", observer);
	return target.SendMessage(&m, &reply, timeout);
}

status_t __CORTEX_NAMESPACE__ remove_observer(
	const BMessenger&				observer,
	const BMessenger&				target,
	BMessage&								reply,
	bigtime_t								timeout) {
//	ASSERT(observer.IsValid());
//	ASSERT(target.IsValid());
	
	BMessage m(M_REMOVE_OBSERVER);
	m.AddMessenger("observer", observer);
	return target.SendMessage(&m, &reply, timeout);
}

// END -- observe.cpp --
