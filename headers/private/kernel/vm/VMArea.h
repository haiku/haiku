/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_VM_VM_AREA_H
#define _KERNEL_VM_VM_AREA_H


#include <lock.h>
#include <util/OpenHashTable.h>
#include <vm/vm_types.h>


struct VMAddressSpace;
struct VMCache;


struct VMArea {
	char*					name;
	area_id					id;
	addr_t					base;
	addr_t					size;
	uint32					protection;
	uint16					wiring;
	uint16					memory_type;

	VMCache*				cache;
	vint32					no_cache_change;
	off_t					cache_offset;
	uint32					cache_type;
	VMAreaMappings			mappings;
	uint8*					page_protections;

	struct VMAddressSpace*	address_space;
	struct VMArea*			address_space_next;
	struct VMArea*			cache_next;
	struct VMArea*			cache_prev;
	struct VMArea*			hash_next;

	bool ContainsAddress(addr_t address) const
		{ return address >= base && address <= base + (size - 1); }
};


struct VMAreaHashDefinition {
	typedef area_id		KeyType;
	typedef VMArea		ValueType;

	size_t HashKey(area_id key) const
	{
		return key;
	}

	size_t Hash(const VMArea* value) const
	{
		return HashKey(value->id);
	}

	bool Compare(area_id key, const VMArea* value) const
	{
		return value->id == key;
	}

	VMArea*& GetLink(VMArea* value) const
	{
		return value->hash_next;
	}
};

typedef BOpenHashTable<VMAreaHashDefinition> VMAreaHashTable;


struct VMAreaHash {
	static	status_t			Init();

	static	status_t			ReadLock()
									{ return rw_lock_read_lock(&sLock); }
	static	void				ReadUnlock()
									{ rw_lock_read_unlock(&sLock); }
	static	status_t			WriteLock()
									{ return rw_lock_write_lock(&sLock); }
	static	void				WriteUnlock()
									{ rw_lock_write_unlock(&sLock); }

	static	VMArea*				LookupLocked(area_id id)
									{ return sTable.Lookup(id); }
	static	VMArea*				Lookup(area_id id);
	static	area_id				Find(const char* name);
	static	void				Insert(VMArea* area);
	static	void				Remove(VMArea* area);

	static	VMAreaHashTable::Iterator GetIterator()
									{ return sTable.GetIterator(); }

private:
	static	rw_lock				sLock;
	static	VMAreaHashTable		sTable;
};


#endif	// _KERNEL_VM_VM_AREA_H
