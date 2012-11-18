/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2002-2004, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*!	Generators are created on demand and deleted if all their
	IDs are freed. They have a ref_count which is increased
	whenever someone messes with the generator.

	Whenever a generator is searched for, generator_lock is held.

	To find out which ID are allocated, a bitmap is used that
	contain up to GENERATOR_MAX_ID bits. This is a hard limit,
	but it's simple to implement. If someone really needs lots of
	IDs, we can still rewrite the code. For simplicity, we use
	sGeneratorLock instead of a generator specific lock during
	allocation; changing it is straightforward as everything
	is prepared for that.
*/


#include "id_generator.h"

#include <new>
#include <stdlib.h>
#include <string.h>

#include <KernelExport.h>

#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>


//#define TRACE_ID_GENERATOR
#ifdef TRACE_ID_GENERATOR
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define GENERATOR_MAX_ID 64

struct id_generator : DoublyLinkedListLinkImpl<id_generator> {
	id_generator(const char* name)
		:
		ref_count(1),
		name(strdup(name)),
		num_ids(0)
	{
		memset(&alloc_map, 0, sizeof(alloc_map));
	}

	~id_generator()
	{
		free(name);
	}

	int32		ref_count;
	char*		name;
	uint32		num_ids;
	uint8		alloc_map[(GENERATOR_MAX_ID + 7) / 8];
};

typedef DoublyLinkedList<id_generator> GeneratorList;


GeneratorList sGenerators;
static mutex sLock = MUTEX_INITIALIZER("id generator");


/*!	Create new generator.
	sLock must be held.
*/
static id_generator*
create_generator(const char* name)
{
	TRACE(("create_generator(name: %s)\n", name));

	id_generator* generator = new(std::nothrow) id_generator(name);
	if (generator == NULL)
		return NULL;

	if (generator->name == NULL) {
		delete generator;
		return NULL;
	}

	sGenerators.Add(generator);
	return generator;
}


/*! Allocate ID */
static int32
create_id_internal(id_generator* generator)
{
	uint32 id;

	TRACE(("create_id_internal(name: %s)\n", generator->name));

	// see above: we use global instead of local lock
	MutexLocker _(sLock);

	// simple bit search
	for (id = 0; id < GENERATOR_MAX_ID; ++id) {
		if ((generator->alloc_map[id / 8] & (1 << (id & 7))) == 0) {
			TRACE(("  id: %lu\n", id));

			generator->alloc_map[id / 8] |= 1 << (id & 7);
			generator->num_ids++;

			return id;
		}
	}

	return B_ERROR;
}


/*!	Gets a generator by name, and acquires a reference to it.
	sLock must be held.
*/
static id_generator*
get_generator(const char* name)
{
	TRACE(("find_generator(name: %s)\n", name));

	GeneratorList::Iterator iterator = sGenerators.GetIterator();
	while (iterator.HasNext()) {
		id_generator* generator = iterator.Next();

		if (!strcmp(generator->name, name)) {
			// increase ref_count, so it won't go away
			generator->ref_count++;
			return generator;
		}
	}

	return NULL;
}


/*! Decrease ref_count, deleting generator if not used anymore */
static void
release_generator(id_generator *generator)
{
	TRACE(("release_generator(name: %s)\n", generator->name));

	MutexLocker _(sLock);

	if (--generator->ref_count == 0) {
		// no one messes with generator
		if (generator->num_ids == 0) {
			TRACE(("  Destroy %s\n", generator->name));
			// no IDs is allocated - destroy generator
			sGenerators.Remove(generator);
			delete generator;
		}
	}
}


//	#pragma mark - Private kernel API


void
dm_init_id_generator(void)
{
	new(&sGenerators) GeneratorList;
}


//	#pragma mark - Public module API


/*! Create automatic ID */
int32
dm_create_id(const char* name)
{

	// find generator, create new if not there
	mutex_lock(&sLock);

	id_generator* generator = get_generator(name);
	if (generator == NULL)
		generator = create_generator(name);

	mutex_unlock(&sLock);

	if (generator == NULL)
		return B_NO_MEMORY;

	// get ID
	int32 id = create_id_internal(generator);

	release_generator(generator);

	TRACE(("dm_create_id: name: %s, id: %ld\n", name, id));
	return id;
}


/*!	Free automatically generated ID */
status_t
dm_free_id(const char* name, uint32 id)
{
	TRACE(("dm_free_id(name: %s, id: %ld)\n", name, id));

	// find generator
	mutex_lock(&sLock);

	id_generator* generator = get_generator(name);

	mutex_unlock(&sLock);

	if (generator == NULL) {
		TRACE(("  Generator %s doesn't exist\n", name));
		return B_NAME_NOT_FOUND;
	}

	// free ID

	// make sure it's really allocated
	// (very important to keep <num_ids> in sync
	if ((generator->alloc_map[id / 8] & (1 << (id & 7))) == 0) {
		dprintf("id %" B_PRIu32 " of generator %s wasn't allocated\n", id,
			generator->name);

		release_generator(generator);
		return B_BAD_VALUE;
	}

	generator->alloc_map[id / 8] &= ~(1 << (id & 7));
	generator->num_ids--;

	release_generator(generator);
	return B_OK;
}
