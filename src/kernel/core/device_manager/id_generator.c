/*
** Copyright 2002-04, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of the Device Manager

	ID generator.
	
	Generators are created on demand and deleted if all their
	IDs are freed. They have a ref_count which is increased
	whenever someone messes with the generator. 
	
	Whenever a generator is search for, generator_lock is hold.
	
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


#define TRACE_ID_GENERATOR
#ifdef TRACE_ID_GENERATOR
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


struct list sGenerators;
static benaphore sGeneratorLock;


// create new generator
// sGeneratorLock must be hold

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
//	ADD_DL_LIST_HEAD( generator, generator_list, );
	return generator;
}


// allocate id

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
			TRACE(("id: %lu\n", id));

			generator->alloc_map[id / 8] |= 1 << (id & 7);
			++generator->num_ids;

			benaphore_unlock(&sGeneratorLock);
			return id;
		}
	}

	benaphore_unlock(&sGeneratorLock);

	return B_ERROR;
}


// find generator by name
// sGeneratorLock must be hold

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


// decrease ref_count, deleting generator if not used anymore

static void
release_generator(id_generator *generator)
{
	TRACE(("release_generator(name: %s)\n", generator->name));

	benaphore_lock(&sGeneratorLock);

	if (--generator->ref_count == 0) {
		// noone messes with generator
		if (generator->num_ids == 0) {
			TRACE(("Destroy %s\n", generator->name));
			// no IDs is allocated - destroy generator
			list_remove_link(generator);
			//REMOVE_DL_LIST( generator, generator_list, );
			free(generator->name);
			free(generator);
		}
	}

	benaphore_unlock(&sGeneratorLock);
}


// public: create automatic ID			

int32
pnp_create_id(const char *name)
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

	TRACE(("create_id: name: %s, id: %ld", name, id));
	return id;
}


// public: free automatically generated ID

status_t
pnp_free_id(const char *name, uint32 id)
{
	id_generator *generator;

	TRACE(("free_id(name: %s, id: %ld)\n", name, id));

	// find generator	
	benaphore_lock(&sGeneratorLock);

	generator = find_generator(name);

	benaphore_unlock(&sGeneratorLock);

	if (generator == NULL) {
		dprintf("Generator %s doesn't exist\n", name);
		return B_NAME_NOT_FOUND;
	}

	// free ID

	// make sure it's really allocated
	// (very important to keep <num_ids> in sync
	if ((generator->alloc_map[id / 8] & (1 << (id & 7))) == 0) {
		dprintf("id %ld of generator %s wasn't allocated", id, generator->name);

		release_generator(generator);
		return B_BAD_VALUE;
	}

	generator->alloc_map[id / 8] &= ~(1 << (id & 7));
	generator->num_ids--;

	release_generator(generator);

	return B_OK;
}


status_t
id_generator_init(void)
{
	list_init(&sGenerators);
	return benaphore_init(&sGeneratorLock, "id generator");
}

