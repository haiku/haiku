/*
 * Copyright 2009-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the NewOS License.
 */
#ifndef VM_USER_AREA_H
#define VM_USER_AREA_H


#include <util/AVLTree.h>

#include <vm/VMArea.h>


struct VMUserAddressSpace;


struct VMUserArea : VMArea, AVLTreeNode {
								VMUserArea(VMAddressSpace* addressSpace,
									uint32 wiring, uint32 protection);
								~VMUserArea();

	static	VMUserArea*			Create(VMAddressSpace* addressSpace,
									const char* name, uint32 wiring,
									uint32 protection, uint32 allocationFlags);
	static	VMUserArea*			CreateReserved(VMAddressSpace* addressSpace,
									uint32 flags, uint32 allocationFlags);
};


struct VMUserAreaTreeDefinition {
	typedef addr_t					Key;
	typedef VMUserArea				Value;

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
		addr_t b = _b->Base();
		if (a == b)
			return 0;
		return a < b ? -1 : 1;
	}

	int Compare(const Value* a, const Value* b) const
	{
		return Compare(a->Base(), b);
	}
};

typedef AVLTree<VMUserAreaTreeDefinition> VMUserAreaTree;


#endif	// VM_USER_AREA_H
