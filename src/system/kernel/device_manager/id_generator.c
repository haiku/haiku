/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2002-2004, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*
	Part of the Device Manager

	ID generator.

	Generators are created on demand and deleted if all their
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


#include "device_manager_private.h"
#include <KernelExport.h>
#include <util/list.h>

#include <stdlib.h>
#include <string.h>


//#define TRACE_ID_GENERATOR
#ifdef TRACE_ID_GENERATOR
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


struct list sGenerators;
static benaphore sGeneratorLock;


/**	Create new generator
 *	sGeneratorLock must be held
 */

static id_generator *
create_generator(const char *name)
{
	id_generator *generator;

	TRACE(("create_generator(name: %s)\n", name));

	generator = malloc(sizeof(*generator));
	if (generator == NULL)
		return NULL;

	memset(generator, 0, sizeof(*generator));

	generator->name = strdup(name);
	if (generator->name == NULL) {
		free(generator);
		return NULL;
	}

	generator->ref_count = 1;

	list_add_link_to_head(&sGenerators, generator);
	return generator;
}


/** Allocate id */

static int32
create_id_internal(id_generator *generator)
{
	uint32 id;

	TRACE(("create_id_internal(name: %s)\n", generator->name));

	// see above: we use global instead of local lock	
	benaphore_lock(&sGeneratorLock);

	// simple bit search
	for (id = 0; id < GENERATOR_MAX_ID; ++id) {
		if ((generator->alloc_map[id / 8] & (1 << (id & 7))) == 0) {
			TRACE(("  id: %lu\n", id));

			generator->alloc_map[id / 8] |= 1 << (id & 7);
			++generator->num_ids;

			benaphore_unlock(&sGeneratorLock);
			return id;
		}
	}

	benaphore_unlock(&sGeneratorLock);

	return B_ERROR;
}


/**	Find generator by name
 *	sGeneratorLock must be held
 */

static id_generator *
find_generator(const char *name)
{
	id_generator *generator = NULL;

	TRACE(("find_generator(name: %s)\n", name));

	while ((generator = list_get_next_item(&sGenerators, generator)) != NULL) {
		if (!strcmp(generator->name, name)) {
			// increase ref_count, so it won't go away 
			++generator->ref_count;
			break;
		}
	}

	return generator;
}


/** Decrease ref_count, deleting generator if not used anymore */

static void
release_generator(id_generator *generator)
{
	TRACE(("release_generator(name: %s)\n", generator->name));

	benaphore_lock(&sGeneratorLock);

	if (--generator->ref_count == 0) {
		// no one messes with generator
		if (generator->num_ids == 0) {
			TRACE(("  Destroy %s\n", generator->name));
			// no IDs is allocated - destroy generator
			list_remove_link(generator);
			free(generator->name);
			free(generator);
		}
	}

	benaphore_unlock(&sGeneratorLock);
}


//	#pragma mark -
//	Private kernel API


status_t
dm_init_id_generator(void)
{
	list_init(&sGenerators);
	return benaphore_init(&sGeneratorLock, "id generator");
}


//	#pragma mark -
//	Public module API


/** Create automatic ID */

int32
dm_create_id(const char *name)
{
	id_generator *generator;
	int32 id;

	// find generator, create new if not there 
	benaphore_lock(&sGeneratorLock);

	generator = find_generator(name);
	if (generator == NULL)
		generator = create_generator(name);

	benaphore_unlock(&sGeneratorLock);

	if (generator == NULL)
		return B_NO_MEMORY;

	// get ID
	id = create_id_internal(generator);

	release_generator(generator);

	TRACE(("dm_create_id: name: %s, id: %ld\n", name, id));
	return id;
}


/**	Free automatically generated ID */

status_t
dm_free_id(const char *name, uint32 id)
{
	id_generator *generator;

	TRACE(("dm_free_id(name: %s, id: %ld)\n", name, id));

	// find generator	
	benaphore_lock(&sGeneratorLock);

	generator = find_generator(name);

	benaphore_unlock(&sGeneratorLock);

	if (generator == NULL) {
		TRACE(("  Generator %s doesn't exist\n", name));
		return B_NAME_NOT_FOUND;
	}

	// free ID

	// make sure it's really allocated
	// (very important to keep <num_ids> in sync
	if ((generator->alloc_map[id / 8] & (1 << (id & 7))) == 0) {
		dprintf("id %ld of generator %s wasn't allocated\n", id, generator->name);

		release_generator(generator);
		return B_BAD_VALUE;
	}

	generator->alloc_map[id / 8] &= ~(1 << (id & 7));
	generator->num_ids--;

	release_generator(generator);

	return B_OK;
}

