/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "pthread_private.h"

#include <limits.h>
#include <stdlib.h>


static pthread_key sKeyTable[PTHREAD_KEYS_MAX];
static int32 sNextSequence = 1;


/*!	Retrieves the destructor of a key locklessly.
	Returns the destructor's sequence in \a sequence.
*/
static pthread_key_destructor
get_key_destructor(uint32 key, int32& sequence)
{
	pthread_key_destructor destructor = NULL;

	do {
		sequence = sKeyTable[key].sequence;
		if (sequence == PTHREAD_UNUSED_SEQUENCE)
			return NULL;

		destructor = sKeyTable[key].destructor;
	} while (sKeyTable[key].sequence != sequence);

	return destructor;
}


/*!	Function to get the thread specific value of a key in a lockless
	way. The thread specific value is reset to NULL.
	\a sequence must be the sequence of the key table that this value
	has to fit to.
*/
static void*
get_key_value(pthread_thread* thread, uint32 key, int32 sequence)
{
	pthread_key_data& keyData = thread->specific[key];
	int32 specificSequence;
	void* value;

	do {
		specificSequence = keyData.sequence;
		if (specificSequence != sequence)
			return NULL;

		value = keyData.value;
	} while (specificSequence != sequence);

	keyData.value = NULL;

	return value;
}


void
__pthread_key_call_destructors(pthread_thread* thread)
{
	for (uint32 key = 0; key < PTHREAD_KEYS_MAX; key++) {
		int32 sequence;
		pthread_key_destructor destructor = get_key_destructor(key, sequence);
		void* value = get_key_value(thread, key, sequence);

		if (value != NULL && destructor != NULL)
			destructor(value);
	}
}


//	#pragma mark - public API


int
pthread_key_create(pthread_key_t* _key, void (*destructor)(void*))
{
	int32 nextSequence = atomic_add(&sNextSequence, 1);

	for (uint32 key = 0; key < PTHREAD_KEYS_MAX; key++) {
		int32 sequence = sKeyTable[key].sequence;
		if (sequence != PTHREAD_UNUSED_SEQUENCE)
			continue;

		// try to acquire this slot

		if (atomic_test_and_set(&sKeyTable[key].sequence, nextSequence,
				sequence) != sequence)
			continue;

		sKeyTable[key].destructor = destructor;
		*_key = key;
		return 0;
	}

	return EAGAIN;
}


int
pthread_key_delete(pthread_key_t key)
{
	if (key < 0 || key >= PTHREAD_KEYS_MAX)
		return EINVAL;

	int32 sequence = atomic_get_and_set(&sKeyTable[key].sequence,
		PTHREAD_UNUSED_SEQUENCE);
	if (sequence == PTHREAD_UNUSED_SEQUENCE)
		return EINVAL;

	return 0;
}


void*
pthread_getspecific(pthread_key_t key)
{
	pthread_thread* thread = pthread_self();

	if (key < 0 || key >= PTHREAD_KEYS_MAX)
		return NULL;

	// check if this key is used, and our value belongs to its current meaning
	int32 sequence = atomic_get(&sKeyTable[key].sequence);
	if (sequence == PTHREAD_UNUSED_SEQUENCE
		|| thread->specific[key].sequence != sequence)
		return NULL;

	return thread->specific[key].value;
}


int
pthread_setspecific(pthread_key_t key, const void* value)
{
	if (key < 0 || key >= PTHREAD_KEYS_MAX)
		return EINVAL;

	int32 sequence = atomic_get(&sKeyTable[key].sequence);
	if (sequence == PTHREAD_UNUSED_SEQUENCE)
		return EINVAL;

	pthread_key_data& keyData = pthread_self()->specific[key];
	keyData.sequence = sequence;
	keyData.value = const_cast<void*>(value);
	return 0;
}

