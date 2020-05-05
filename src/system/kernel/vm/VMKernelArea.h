/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the NewOS License.
 */
#ifndef VM_KERNEL_AREA_H
#define VM_KERNEL_AREA_H


#include <util/AVLTree.h>

#include <vm/VMArea.h>


struct ObjectCache;
struct VMKernelAddressSpace;
struct VMKernelArea;


struct VMKernelAddressRange : AVLTreeNode {
public:
	// range types
	enum {
		RANGE_FREE,
		RANGE_RESERVED,
		RANGE_AREA
	};

public:
	DoublyLinkedListLink<VMKernelAddressRange>		listLink;
	addr_t											base;
	size_t											size;
	union {
		VMKernelArea*								area;
		struct {
			addr_t									base;
			uint32									flags;
		} reserved;
		DoublyLinkedListLink<VMKernelAddressRange>	freeListLink;
	};
	int												type;

public:
	VMKernelAddressRange(addr_t base, size_t size, int type)
		:
		base(base),
		size(size),
		type(type)
	{
	}

	VMKernelAddressRange(addr_t base, size_t size,
		const VMKernelAddressRange* other)
		:
		base(base),
		size(size),
		type(other->type)
	{
		if (type == RANGE_RESERVED) {
			reserved.base = other->reserved.base;
			reserved.flags = other->reserved.flags;
		}
	}
};


struct VMKernelAddressRangeTreeDefinition {
	typedef addr_t					Key;
	typedef VMKernelAddressRange	Value;

	AVLTreeNode* GetAVLTreeNode(Value* value) const
	{
		return value;
	}

	Value* GetValue(AVLTreeNode* node) const
	{
		return static_cast<Value*>(node);
	}

	int Compare(addr_t a, const Value* _b) const
	{
		addr_t b = _b->base;
		if (a == b)
			return 0;
		return a < b ? -1 : 1;
	}

	int Compare(const Value* a, const Value* b) const
	{
		return Compare(a->base, b);
	}
};

typedef AVLTree<VMKernelAddressRangeTreeDefinition> VMKernelAddressRangeTree;


struct VMKernelAddressRangeGetFreeListLink {
	typedef DoublyLinkedListLink<VMKernelAddressRange> Link;

	inline Link* operator()(VMKernelAddressRange* range) const
	{
		return &range->freeListLink;
	}

	inline const Link* operator()(const VMKernelAddressRange* range) const
	{
		return &range->freeListLink;
	}
};


struct VMKernelArea : VMArea, AVLTreeNode {
								VMKernelArea(VMAddressSpace* addressSpace,
									uint32 wiring, uint32 protection);
								~VMKernelArea();

	static	VMKernelArea*		Create(VMAddressSpace* addressSpace,
									const char* name, uint32 wiring,
									uint32 protection, ObjectCache* objectCache,
									uint32 allocationFlags);

			VMKernelAddressRange* Range() const
									{ return fRange; }
			void				SetRange(VMKernelAddressRange* range)
									{ fRange = range; }

private:
			VMKernelAddressRange* fRange;
};


#endif	// VM_KERNEL_AREA_H
