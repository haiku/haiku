/*
 * Copyright 2009-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the NewOS License.
 */
#ifndef VM_USER_AREA_H
#define VM_USER_AREA_H


#include <vm/VMArea.h>


struct VMUserAddressSpace;


struct VMUserArea : VMArea {
								VMUserArea(VMAddressSpace* addressSpace,
									uint32 wiring, uint32 protection);
								~VMUserArea();

	static	VMUserArea*			Create(VMAddressSpace* addressSpace,
									const char* name, uint32 wiring,
									uint32 protection, uint32 allocationFlags);
	static	VMUserArea*			CreateReserved(VMAddressSpace* addressSpace,
									uint32 flags, uint32 allocationFlags);

			DoublyLinkedListLink<VMUserArea>& AddressSpaceLink()
									{ return fAddressSpaceLink; }
			const DoublyLinkedListLink<VMUserArea>& AddressSpaceLink() const
									{ return fAddressSpaceLink; }

private:
			DoublyLinkedListLink<VMUserArea> fAddressSpaceLink;
};


struct VMUserAreaGetLink {
    inline DoublyLinkedListLink<VMUserArea>* operator()(
		VMUserArea* area) const
    {
        return &area->AddressSpaceLink();
    }

    inline const DoublyLinkedListLink<VMUserArea>* operator()(
		const VMUserArea* area) const
    {
        return &area->AddressSpaceLink();
    }
};

typedef DoublyLinkedList<VMUserArea, VMUserAreaGetLink> VMUserAreaList;


#endif	// VM_USER_AREA_H
