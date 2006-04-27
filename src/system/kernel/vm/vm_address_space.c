/*
 * Copyright 2002-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


//#include <OS.h>
#include <KernelExport.h>

#include <vm.h>
#include <vm_address_space.h>
#include <vm_priv.h>
#include <thread.h>
#include <util/khash.h>

#include <stdlib.h>


//#define TRACE_VM
#ifdef TRACE_VM
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

vm_address_space *kernel_aspace;

#define ASPACE_HASH_TABLE_SIZE 1024
static void *aspace_table;
static sem_id aspace_hash_sem;


static void
_dump_aspace(vm_address_space *aspace)
{
	vm_area *area;

	dprintf("dump of address space at %p:\n", aspace);
	dprintf("id: 0x%lx\n", aspace->id);
	dprintf("ref_count: %ld\n", aspace->ref_count);
	dprintf("fault_count: %ld\n", aspace->fault_count);
	dprintf("working_set_size: 0x%lx\n", aspace->working_set_size);
	dprintf("translation_map: %p\n", &aspace->translation_map);
	dprintf("base: 0x%lx\n", aspace->base);
	dprintf("size: 0x%lx\n", aspace->size);
	dprintf("change_count: 0x%lx\n", aspace->change_count);
	dprintf("sem: 0x%lx\n", aspace->sem);
	dprintf("area_hint: %p\n", aspace->area_hint);
	dprintf("area_list:\n");
	for (area = aspace->areas; area != NULL; area = area->address_space_next) {
		dprintf(" area 0x%lx: ", area->id);
		dprintf("base_addr = 0x%lx ", area->base);
		dprintf("size = 0x%lx ", area->size);
		dprintf("name = '%s' ", area->name);
		dprintf("protection = 0x%lx\n", area->protection);
	}
}


static int
dump_aspace(int argc, char **argv)
{
	vm_address_space *aspace;

	if (argc < 2) {
		dprintf("aspace: not enough arguments\n");
		return 0;
	}

	// if the argument looks like a number, treat it as such

	{
		team_id id = strtoul(argv[1], NULL, 0);

		aspace = hash_lookup(aspace_table, &id);
		if (aspace == NULL) {
			dprintf("invalid aspace id\n");
		} else {
			_dump_aspace(aspace);
		}
		return 0;
	}
	return 0;
}


static int
dump_aspace_list(int argc, char **argv)
{
	vm_address_space *as;
	struct hash_iterator iter;

	dprintf("addr\tid\tbase\t\tsize\n");

	hash_open(aspace_table, &iter);
	while ((as = hash_next(aspace_table, &iter)) != NULL) {
		dprintf("%p\t0x%lx\t0x%lx\t\t0x%lx\n",
			as, as->id, as->base, as->size);
	}
	hash_close(aspace_table, &iter, false);
	return 0;
}


static int
aspace_compare(void *_a, const void *key)
{
	vm_address_space *aspace = _a;
	const team_id *id = key;

	if (aspace->id == *id)
		return 0;

	return -1;
}


static uint32
aspace_hash(void *_a, const void *key, uint32 range)
{
	vm_address_space *aspace = _a;
	const team_id *id = key;

	if (aspace != NULL)
		return aspace->id % range;

	return *id % range;
}


/** When this function is called, all references to this address space
 *	have been released, so it's safe to remove it.
 */

static void
delete_address_space(vm_address_space *addressSpace)
{
	TRACE(("delete_address_space: called on aspace 0x%lx\n", addressSpace->id));

	if (addressSpace == kernel_aspace)
		panic("tried to delete the kernel aspace!\n");

	// put this aspace in the deletion state
	// this guarantees that no one else will add regions to the list
	acquire_sem_etc(addressSpace->sem, WRITE_COUNT, 0, 0);

	addressSpace->state = VM_ASPACE_STATE_DELETION;

	(*addressSpace->translation_map.ops->destroy)(&addressSpace->translation_map);

	delete_sem(addressSpace->sem);
	free(addressSpace);
}


//	#pragma mark -


vm_address_space *
vm_get_address_space_by_id(team_id aid)
{
	vm_address_space *aspace;

	acquire_sem_etc(aspace_hash_sem, READ_COUNT, 0, 0);
	aspace = hash_lookup(aspace_table, &aid);
	if (aspace)
		atomic_add(&aspace->ref_count, 1);
	release_sem_etc(aspace_hash_sem, READ_COUNT, 0);

	return aspace;
}


vm_address_space *
vm_get_kernel_address_space(void)
{
	/* we can treat this one a little differently since it can't be deleted */
	atomic_add(&kernel_aspace->ref_count, 1);
	return kernel_aspace;
}


vm_address_space *
vm_kernel_address_space(void)
{
	return kernel_aspace;
}


team_id
vm_kernel_address_space_id(void)
{
	return kernel_aspace->id;
}


vm_address_space *
vm_get_current_user_address_space(void)
{
	struct thread *thread = thread_get_current_thread();

	if (thread != NULL) {
		vm_address_space *addressSpace = thread->team->address_space;
		if (addressSpace != NULL) {
			atomic_add(&addressSpace->ref_count, 1);
			return addressSpace;
		}
	}

	return NULL;
}


team_id
vm_current_user_address_space_id(void)
{
	struct thread *thread = thread_get_current_thread();

	if (thread != NULL && thread->team->address_space != NULL)
		return thread->team->id;

	return B_ERROR;
}


void
vm_put_address_space(vm_address_space *aspace)
{
	bool remove = false;

	acquire_sem_etc(aspace_hash_sem, WRITE_COUNT, 0, 0);
	if (atomic_add(&aspace->ref_count, -1) == 1) {
		hash_remove(aspace_table, aspace);
		remove = true;
	}
	release_sem_etc(aspace_hash_sem, WRITE_COUNT, 0);

	if (remove)
		delete_address_space(aspace);
}


/** Deletes all areas in the specified address space, and the address
 *	space by decreasing all reference counters. It also marks the
 *	address space of being in deletion state, so that no more areas
 *	can be created in it.
 *	After this, the address space is not operational anymore, but might
 *	still be in memory until the last reference has been released.
 */

void
vm_delete_address_space(vm_address_space *addressSpace)
{
	acquire_sem_etc(addressSpace->sem, WRITE_COUNT, 0, 0);
	addressSpace->state = VM_ASPACE_STATE_DELETION;
	release_sem_etc(addressSpace->sem, WRITE_COUNT, 0);

	vm_delete_areas(addressSpace);
	vm_put_address_space(addressSpace);
}


status_t
vm_create_address_space(team_id id, addr_t base, addr_t size,
	bool kernel, vm_address_space **_addressSpace)
{
	vm_address_space *addressSpace;
	status_t status;

	addressSpace = (vm_address_space *)malloc(sizeof(vm_address_space));
	if (addressSpace == NULL)
		return B_NO_MEMORY;

	TRACE(("vm_create_aspace: %s: %lx bytes starting at 0x%lx => %p\n",
		name, size, base, addressSpace));

	addressSpace->base = base;
	addressSpace->size = size;
	addressSpace->areas = NULL;
	addressSpace->area_hint = NULL;
	addressSpace->change_count = 0;
	if (!kernel) {
		// the kernel address space will create its semaphore later
		addressSpace->sem = create_sem(WRITE_COUNT, "address space");
		if (addressSpace->sem < B_OK) {
			status_t status = addressSpace->sem;
			free(addressSpace);
			return status;
		}
	}

	addressSpace->id = id;
	addressSpace->ref_count = 1;
	addressSpace->state = VM_ASPACE_STATE_NORMAL;
	addressSpace->fault_count = 0;
	addressSpace->scan_va = base;
	addressSpace->working_set_size = kernel ? DEFAULT_KERNEL_WORKING_SET : DEFAULT_WORKING_SET;
	addressSpace->max_working_set = DEFAULT_MAX_WORKING_SET;
	addressSpace->min_working_set = DEFAULT_MIN_WORKING_SET;
	addressSpace->last_working_set_adjust = system_time();

	// initialize the corresponding translation map
	status = arch_vm_translation_map_init_map(&addressSpace->translation_map, kernel);
	if (status < B_OK) {
		free(addressSpace);
		return status;
	}

	// add the aspace to the global hash table
	acquire_sem_etc(aspace_hash_sem, WRITE_COUNT, 0, 0);
	hash_insert(aspace_table, addressSpace);
	release_sem_etc(aspace_hash_sem, WRITE_COUNT, 0);

	*_addressSpace = addressSpace;
	return B_OK;
}


void
vm_address_space_walk_start(struct hash_iterator *iterator)
{
	hash_open(aspace_table, iterator);
}


vm_address_space *
vm_address_space_walk_next(struct hash_iterator *iterator)
{
	vm_address_space *aspace;

	acquire_sem_etc(aspace_hash_sem, READ_COUNT, 0, 0);
	aspace = hash_next(aspace_table, iterator);
	if (aspace)
		atomic_add(&aspace->ref_count, 1);
	release_sem_etc(aspace_hash_sem, READ_COUNT, 0);
	return aspace;
}


status_t
vm_address_space_init(void)
{
	aspace_hash_sem = -1;

	// create the area and address space hash tables
	{
		vm_address_space *aspace;
		aspace_table = hash_init(ASPACE_HASH_TABLE_SIZE, (addr_t)&aspace->hash_next - (addr_t)aspace,
			&aspace_compare, &aspace_hash);
		if (aspace_table == NULL)
			panic("vm_init: error creating aspace hash table\n");
	}

	kernel_aspace = NULL;

	// create the initial kernel address space
	if (vm_create_address_space(1, KERNEL_BASE, KERNEL_SIZE,
			true, &kernel_aspace) != B_OK)
		panic("vm_init: error creating kernel address space!\n");

	add_debugger_command("aspaces", &dump_aspace_list, "Dump a list of all address spaces");
	add_debugger_command("aspace", &dump_aspace, "Dump info about a particular address space");

	return B_OK;
}


status_t
vm_address_space_init_post_sem(void)
{
	status_t status = arch_vm_translation_map_init_kernel_map_post_sem(&kernel_aspace->translation_map);
	if (status < B_OK)
		return status;

	status = kernel_aspace->sem = create_sem(WRITE_COUNT, "kernel_aspacelock");
	if (status < B_OK)
		return status;

	status = aspace_hash_sem = create_sem(WRITE_COUNT, "aspace_hash_sem");
	if (status < B_OK)
		return status;

	return B_OK;
}
