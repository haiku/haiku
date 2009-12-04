/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the NewOS License.
 */
#ifndef VM_KERNEL_AREA_H
#define VM_KERNEL_AREA_H


#include <vm/VMArea.h>


struct VMKernelAddressSpace;


struct VMKernelArea : VMArea {
								VMKernelArea(VMAddressSpace* addressSpace,
									uint32 wiring, uint32 protection);
								~VMKernelArea();

	static	VMKernelArea*		Create(VMAddressSpace* addressSpace,
									const char* name, uint32 wiring,
									uint32 protection);
	static	VMKernelArea*		CreateReserved(VMAddressSpace* addressSpace,
									uint32 flags);

			DoublyLinkedListLink<VMKernelArea>& AddressSpaceLink()
									{ return fAddressSpaceLink; }
			const DoublyLinkedListLink<VMKernelArea>& AddressSpaceLink() const
									{ return fAddressSpaceLink; }

private:
			DoublyLinkedListLink<VMKernelArea> fAddressSpaceLink;
};


struct VMKernelAreaGetLink {
    inline DoublyLinkedListLink<VMKernelArea>* operator()(
		VMKernelArea* area) const
    {
        return &area->AddressSpaceLink();
    }

    inline const DoublyLinkedListLink<VMKernelArea>* operator()(
		const VMKernelArea* area) const
    {
        return &area->AddressSpaceLink();
    }
};

typedef DoublyLinkedList<VMKernelArea, VMKernelAreaGetLink> VMKernelAreaList;


#endif	// VM_KERNEL_AREA_H
