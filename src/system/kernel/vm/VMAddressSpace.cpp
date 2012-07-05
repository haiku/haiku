/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <vm/VMAddressSpace.h>

#include <stdlib.h>

#include <new>

#include <KernelExport.h>

#include <util/OpenHashTable.h>

#include <heap.h>
#include <thread.h>
#include <vm/vm.h>
#include <vm/VMArea.h>
#include <vm/VMCache.h>

#include "VMKernelAddressSpace.h"
#include "VMUserAddressSpace.h"


//#define TRACE_VM
#ifdef TRACE_VM
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#define ASPACE_HASH_TABLE_SIZE 1024


// #pragma mark - AddressSpaceHashDefinition


struct AddressSpaceHashDefinition {
	typedef team_id			KeyType;
	typedef VMAddressSpace	ValueType;

	size_t HashKey(team_id key) const
	{
		return key;
	}

	size_t Hash(const VMAddressSpace* value) const
	{
		return HashKey(value->ID());
	}

	bool Compare(team_id key, const VMAddressSpace* value) const
	{
		return value->ID() == key;
	}

	VMAddressSpace*& GetLink(VMAddressSpace* value) const
	{
		return value->HashTableLink();
	}
};

typedef BOpenHashTable<AddressSpaceHashDefinition> AddressSpaceTable;

static AddressSpaceTable	sAddressSpaceTable;
static rw_lock				sAddressSpaceTableLock;

VMAddressSpace* VMAddressSpace::sKernelAddressSpace;


// #pragma mark - VMAddressSpace


VMAddressSpace::VMAddressSpace(team_id id, addr_t base, size_t size,
	const char* name)
	:
	fBase(base),
	fEndAddress(base + (size - 1)),
	fFreeSpace(size),
	fID(id),
	fRefCount(1),
	fFaultCount(0),
	fChangeCount(0),
	fTranslationMap(NULL),
	fDeleting(false)
{
	rw_lock_init(&fLock, name);
//	rw_lock_init(&fLock, kernel ? "kernel address space" : "address space");
}


VMAddressSpace::~VMAddressSpace()
{
	TRACE(("VMAddressSpace::~VMAddressSpace: called on aspace %" B_PRId32 "\n",
		ID()));

	WriteLock();

	delete fTranslationMap;

	rw_lock_destroy(&fLock);
}


/*static*/ status_t
VMAddressSpace::Init()
{
	rw_lock_init(&sAddressSpaceTableLock, "address spaces table");

	// create the area and address space hash tables
	{
		new(&sAddressSpaceTable) AddressSpaceTable;
		status_t error = sAddressSpaceTable.Init(ASPACE_HASH_TABLE_SIZE);
		if (error != B_OK)
			panic("vm_init: error creating aspace hash table\n");
	}

	// create the initial kernel address space
	if (Create(B_SYSTEM_TEAM, KERNEL_BASE, KERNEL_SIZE, true,
			&sKernelAddressSpace) != B_OK) {
		panic("vm_init: error creating kernel address space!\n");
	}

	add_debugger_command("aspaces", &_DumpListCommand,
		"Dump a list of all address spaces");
	add_debugger_command("aspace", &_DumpCommand,
		"Dump info about a particular address space");

	return B_OK;
}


/*! Deletes all areas in the specified address space, and the address
	space by decreasing all reference counters. It also marks the
	address space of being in deletion state, so that no more areas
	can be created in it.
	After this, the address space is not operational anymore, but might
	still be in memory until the last reference has been released.
*/
void
VMAddressSpace::RemoveAndPut()
{
	WriteLock();
	fDeleting = true;
	WriteUnlock();

	vm_delete_areas(this, true);
	Put();
}


status_t
VMAddressSpace::InitObject()
{
	return B_OK;
}


void
VMAddressSpace::Dump() const
{
	kprintf("dump of address space at %p:\n", this);
	kprintf("id: %" B_PRId32 "\n", fID);
	kprintf("ref_count: %" B_PRId32 "\n", fRefCount);
	kprintf("fault_count: %" B_PRId32 "\n", fFaultCount);
	kprintf("translation_map: %p\n", fTranslationMap);
	kprintf("base: %#" B_PRIxADDR "\n", fBase);
	kprintf("end: %#" B_PRIxADDR "\n", fEndAddress);
	kprintf("change_count: %" B_PRId32 "\n", fChangeCount);
}


/*static*/ status_t
VMAddressSpace::Create(team_id teamID, addr_t base, size_t size, bool kernel,
	VMAddressSpace** _addressSpace)
{
	VMAddressSpace* addressSpace = kernel
		? (VMAddressSpace*)new(std::nothrow) VMKernelAddressSpace(teamID, base,
			size)
		: (VMAddressSpace*)new(std::nothrow) VMUserAddressSpace(teamID, base,
			size);
	if (addressSpace == NULL)
		return B_NO_MEMORY;

	status_t status = addressSpace->InitObject();
	if (status != B_OK) {
		delete addressSpace;
		return status;
	}

	TRACE(("VMAddressSpace::Create(): team %" B_PRId32 " (%skernel): %#lx "
		"bytes starting at %#lx => %p\n", teamID, kernel ? "" : "!", size,
		base, addressSpace));

	// create the corresponding translation map
	status = arch_vm_translation_map_create_map(kernel,
		&addressSpace->fTranslationMap);
	if (status != B_OK) {
		delete addressSpace;
		return status;
	}

	// add the aspace to the global hash table
	rw_lock_write_lock(&sAddressSpaceTableLock);
	sAddressSpaceTable.InsertUnchecked(addressSpace);
	rw_lock_write_unlock(&sAddressSpaceTableLock);

	*_addressSpace = addressSpace;
	return B_OK;
}


/*static*/ VMAddressSpace*
VMAddressSpace::GetKernel()
{
	// we can treat this one a little differently since it can't be deleted
	sKernelAddressSpace->Get();
	return sKernelAddressSpace;
}


/*static*/ team_id
VMAddressSpace::CurrentID()
{
	Thread* thread = thread_get_current_thread();

	if (thread != NULL && thread->team->address_space != NULL)
		return thread->team->id;

	return B_ERROR;
}


/*static*/ VMAddressSpace*
VMAddressSpace::GetCurrent()
{
	Thread* thread = thread_get_current_thread();

	if (thread != NULL) {
		VMAddressSpace* addressSpace = thread->team->address_space;
		if (addressSpace != NULL) {
			addressSpace->Get();
			return addressSpace;
		}
	}

	return NULL;
}


/*static*/ VMAddressSpace*
VMAddressSpace::Get(team_id teamID)
{
	rw_lock_read_lock(&sAddressSpaceTableLock);
	VMAddressSpace* addressSpace = sAddressSpaceTable.Lookup(teamID);
	if (addressSpace)
		addressSpace->Get();
	rw_lock_read_unlock(&sAddressSpaceTableLock);

	return addressSpace;
}


/*static*/ VMAddressSpace*
VMAddressSpace::DebugFirst()
{
	return sAddressSpaceTable.GetIterator().Next();
}


/*static*/ VMAddressSpace*
VMAddressSpace::DebugNext(VMAddressSpace* addressSpace)
{
	if (addressSpace == NULL)
		return NULL;

	AddressSpaceTable::Iterator it
		= sAddressSpaceTable.GetIterator(addressSpace->ID());
	it.Next();
	return it.Next();
}


/*static*/ VMAddressSpace*
VMAddressSpace::DebugGet(team_id teamID)
{
	return sAddressSpaceTable.Lookup(teamID);
}


/*static*/ void
VMAddressSpace::_DeleteIfUnreferenced(team_id id)
{
	rw_lock_write_lock(&sAddressSpaceTableLock);

	bool remove = false;
	VMAddressSpace* addressSpace = sAddressSpaceTable.Lookup(id);
	if (addressSpace != NULL && addressSpace->fRefCount == 0) {
		sAddressSpaceTable.RemoveUnchecked(addressSpace);
		remove = true;
	}

	rw_lock_write_unlock(&sAddressSpaceTableLock);

	if (remove)
		delete addressSpace;
}


/*static*/ int
VMAddressSpace::_DumpCommand(int argc, char** argv)
{
	VMAddressSpace* aspace;

	if (argc < 2) {
		kprintf("aspace: not enough arguments\n");
		return 0;
	}

	// if the argument looks like a number, treat it as such

	{
		team_id id = strtoul(argv[1], NULL, 0);

		aspace = sAddressSpaceTable.Lookup(id);
		if (aspace == NULL) {
			kprintf("invalid aspace id\n");
		} else {
			aspace->Dump();
		}
		return 0;
	}
	return 0;
}


/*static*/ int
VMAddressSpace::_DumpListCommand(int argc, char** argv)
{
	kprintf("   address      id         base          end   area count   "
		" area size\n");

	AddressSpaceTable::Iterator it = sAddressSpaceTable.GetIterator();
	while (VMAddressSpace* space = it.Next()) {
		int32 areaCount = 0;
		off_t areaSize = 0;
		for (VMAddressSpace::AreaIterator areaIt = space->GetAreaIterator();
				VMArea* area = areaIt.Next();) {
			areaCount++;
			areaSize += area->Size();
		}
		kprintf("%p  %6" B_PRId32 "   %#010" B_PRIxADDR "   %#10" B_PRIxADDR
			"   %10" B_PRId32 "   %10" B_PRIdOFF "\n", space, space->ID(),
			space->Base(), space->EndAddress(), areaCount, areaSize);
	}

	return 0;
}
