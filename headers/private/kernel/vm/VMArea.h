/*
 * Copyright 2009-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_VM_VM_AREA_H
#define _KERNEL_VM_VM_AREA_H


#include <vm_defs.h>

#include <lock.h>
#include <util/DoublyLinkedList.h>
#include <util/SinglyLinkedList.h>
#include <util/AVLTree.h>
#include <vm/vm_types.h>


struct VMAddressSpace;
struct VMCache;
struct VMKernelAddressSpace;
struct VMUserAddressSpace;


struct VMAreaUnwiredWaiter
	: public DoublyLinkedListLinkImpl<VMAreaUnwiredWaiter> {
	VMArea*					area;
	addr_t					base;
	size_t					size;
	ConditionVariable		condition;
	ConditionVariableEntry	waitEntry;
};

typedef DoublyLinkedList<VMAreaUnwiredWaiter> VMAreaUnwiredWaiterList;


struct VMAreaWiredRange : SinglyLinkedListLinkImpl<VMAreaWiredRange> {
	VMArea*					area;
	addr_t					base;
	size_t					size;
	bool					writable;
	bool					implicit;	// range created automatically
	VMAreaUnwiredWaiterList	waiters;

	VMAreaWiredRange()
	{
	}

	VMAreaWiredRange(addr_t base, size_t size, bool writable, bool implicit)
		:
		area(NULL),
		base(base),
		size(size),
		writable(writable),
		implicit(implicit)
	{
	}

	void SetTo(addr_t base, size_t size, bool writable, bool implicit)
	{
		this->area = NULL;
		this->base = base;
		this->size = size;
		this->writable = writable;
		this->implicit = implicit;
	}

	bool IntersectsWith(addr_t base, size_t size) const
	{
		return this->base + this->size - 1 >= base
			&& base + size - 1 >= this->base;
	}
};

typedef SinglyLinkedList<VMAreaWiredRange> VMAreaWiredRangeList;


struct VMPageWiringInfo {
	VMAreaWiredRange	range;
	phys_addr_t			physicalAddress;
							// the actual physical address corresponding to
							// the virtual address passed to vm_wire_page()
							// (i.e. with in-page offset)
	vm_page*			page;
};


struct VMAreasTreeNode {
	AVLTreeNode tree_node;
};


struct VMArea : private VMAreasTreeNode {
public:
	enum {
		// AddWaiterIfWired() flags
		IGNORE_WRITE_WIRED_RANGES	= 0x01,	// ignore existing ranges that
											// wire for writing
	};

public:
	area_id					id;
	char					name[B_OS_NAME_LENGTH];
	uint32					protection;
	uint32					protection_max;
	uint16					wiring;

private:
	uint16					memory_type;	// >> shifted by MEMORY_TYPE_SHIFT

public:
	VMCache*				cache;
	off_t					cache_offset;
	uint32					cache_type;
	VMAreaMappings			mappings;
	uint8*					page_protections;

	struct VMAddressSpace*	address_space;
	struct VMArea*			cache_next;
	struct VMArea*			cache_prev;

			addr_t				Base() const	{ return fBase; }
			size_t				Size() const	{ return fSize; }

	inline	uint32				MemoryType() const;
	inline	void				SetMemoryType(uint32 memoryType);

			bool				ContainsAddress(addr_t address) const
									{ return address >= fBase
										&& address <= fBase + (fSize - 1); }

			bool				IsWired() const
									{ return !fWiredRanges.IsEmpty(); }
			bool				IsWired(addr_t base, size_t size) const;

			void				Wire(VMAreaWiredRange* range);
			void				Unwire(VMAreaWiredRange* range);
			VMAreaWiredRange*	Unwire(addr_t base, size_t size, bool writable);

			bool				AddWaiterIfWired(VMAreaUnwiredWaiter* waiter);
			bool				AddWaiterIfWired(VMAreaUnwiredWaiter* waiter,
									addr_t base, size_t size, uint32 flags = 0);

protected:
								VMArea(VMAddressSpace* addressSpace,
									uint32 wiring, uint32 protection);
								~VMArea();

			status_t			Init(const char* name, uint32 allocationFlags);

protected:
			friend struct VMAreasTreeDefinition;
			friend struct VMKernelAddressSpace;
			friend struct VMUserAddressSpace;

protected:
			void				SetBase(addr_t base)	{ fBase = base; }
			void				SetSize(size_t size)	{ fSize = size; }

protected:
			addr_t				fBase;
			size_t				fSize;
			VMAreaWiredRangeList fWiredRanges;
};


struct VMAreasTreeDefinition {
	typedef area_id		Key;
	typedef VMArea		Value;

	AVLTreeNode* GetAVLTreeNode(VMArea* value) const
	{
		return &value->tree_node;
	}

	VMArea* GetValue(AVLTreeNode* node) const
	{
		const addr_t vmTreeNodeAddr = (addr_t)node
			- offsetof(VMAreasTreeNode, tree_node);
		VMAreasTreeNode* vmTreeNode =
			reinterpret_cast<VMAreasTreeNode*>(vmTreeNodeAddr);
		return static_cast<VMArea*>(vmTreeNode);
	}

	int Compare(area_id key, const VMArea* value) const
	{
		const area_id valueId = value->id;
		if (valueId == key)
			return 0;
		return key < valueId ? -1 : 1;
	}

	int Compare(const VMArea* a, const VMArea* b) const
	{
		return Compare(a->id, b);
	}
};

typedef AVLTree<VMAreasTreeDefinition> VMAreasTree;


struct VMAreas {
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
									{ return sTree.Find(id); }
	static	VMArea*				Lookup(area_id id);
	static	area_id				Find(const char* name);
	static	status_t			Insert(VMArea* area);
	static	void				Remove(VMArea* area);

	static	VMAreasTree::Iterator GetIterator()
									{ return sTree.GetIterator(); }

private:
	static	rw_lock				sLock;
	static	VMAreasTree			sTree;
};


uint32
VMArea::MemoryType() const
{
	return (uint32)memory_type << MEMORY_TYPE_SHIFT;
}


void
VMArea::SetMemoryType(uint32 memoryType)
{
	memory_type = memoryType >> MEMORY_TYPE_SHIFT;
}


#endif	// _KERNEL_VM_VM_AREA_H
