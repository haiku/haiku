/*
 * Copyright 2008-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <locks.h>


enum {
	STATE_UNINITIALIZED	= -1,	// keep in sync with INIT_ONCE_UNINITIALIZED
	STATE_INITIALIZING	= -2,
	STATE_SPINNING		= -3,
	STATE_INITIALIZED	= -4	// keep in sync with INIT_ONCE_INITIALIZED
};


status_t
__init_once(vint32* control, status_t (*initRoutine)(void*), void* data)
{
	// Algorithm:
	// The control variable goes through at most four states:
	// STATE_UNINITIALIZED: The initial uninitialized state.
	// STATE_INITIALIZING: Set by the first thread entering the function. It
	// will call initRoutine.
	// semaphore/STATE_SPINNING: Set by the second thread entering the function,
	// when the first thread is still executing initRoutine. The normal case is
	// that the thread manages to create a semaphore. This thread (and all
	// following threads) will block on the semaphore until the first thread is
	// done.
	// STATE_INITIALIZED: Set by the first thread when it returns from
	// initRoutine. All following threads will return right away.

	int32 value = atomic_test_and_set(control, STATE_INITIALIZING,
		STATE_UNINITIALIZED);

	if (value == STATE_INITIALIZED)
		return 0;

	if (value == STATE_UNINITIALIZED) {
		// we're the first -- perform the initialization
		initRoutine(data);

		value = atomic_set(control, STATE_INITIALIZED);

		// If someone else is waiting, we need to delete the semaphore.
		if (value >= 0)
			delete_sem(value);

		return 0;
	}

	if (value == STATE_INITIALIZING) {
		// someone is initializing -- we need to create a semaphore we can wait
		// on
		sem_id semaphore = create_sem(0, "pthread once");
		if (semaphore >= 0) {
			// successfully created -- set it
			value = atomic_test_and_set(control, semaphore, STATE_INITIALIZING);
			if (value == STATE_INITIALIZING)
				value = semaphore;
			else
				delete_sem(semaphore);
		} else {
			// Failed to create the semaphore. Can only happen when the system
			// runs out of semaphores, but we can still handle the situation
			// gracefully by spinning.
			value = atomic_test_and_set(control, STATE_SPINNING,
				STATE_INITIALIZING);
			if (value == STATE_INITIALIZING)
				value = STATE_SPINNING;
		}
	}

	if (value >= 0) {
		// wait on the semaphore
		while (acquire_sem(value) == B_INTERRUPTED);

		return 0;
	} else if (value == STATE_SPINNING) {
		// out of semaphores -- spin
		while (atomic_get(control) == STATE_SPINNING);
	}

	return 0;
}
