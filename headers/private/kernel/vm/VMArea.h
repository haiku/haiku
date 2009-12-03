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
#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>
#include <vm/vm_types.h>


struct VMAddressSpace;
struct VMCache;


struct VMArea {
	char*					name;
	area_id					id;
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
	struct VMArea*			cache_next;
	struct VMArea*			cache_prev;
	struct VMArea*			hash_next;

			addr_t				Base() const	{ return fBase; }
			size_t				Size() const	{ return fSize; }

			void				SetBase(addr_t base)	{ fBase = base; }
			void				SetSize(size_t size)	{ fSize = size; }

			bool				ContainsAddress(addr_t address) const
									{ return address >= fBase
										&& address <= fBase + (fSize - 1); }

	static	VMArea*				Create(VMAddressSpace* addressSpace,
									const char* name, uint32 wiring,
									uint32 protection);
	static	VMArea*				CreateReserved(VMAddressSpace* addressSpace,
									uint32 flags);

			DoublyLinkedListLink<VMArea>& AddressSpaceLink()
									{ return fAddressSpaceLink; }
			const DoublyLinkedListLink<VMArea>& AddressSpaceLink() const
									{ return fAddressSpaceLink; }

private:
			DoublyLinkedListLink<VMArea> fAddressSpaceLink;
			addr_t				fBase;
			size_t				fSize;
};


struct VMAddressSpaceAreaGetLink {
    inline DoublyLinkedListLink<VMArea>* operator()(VMArea* area) const
    {
        return &area->AddressSpaceLink();
    }

    inline const DoublyLinkedListLink<VMArea>* operator()(
		const VMArea* area) const
    {
        return &area->AddressSpaceLink();
    }
};

typedef DoublyLinkedList<VMArea, VMAddressSpaceAreaGetLink>
	VMAddressSpaceAreaList;


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
