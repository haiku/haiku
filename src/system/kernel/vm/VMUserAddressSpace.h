/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef VM_USER_ADDRESS_SPACE_H
#define VM_USER_ADDRESS_SPACE_H


#include <vm/VMAddressSpace.h>


struct VMUserAddressSpace : VMAddressSpace {
public:
								VMUserAddressSpace(team_id id, addr_t base,
									size_t size);
	virtual						~VMUserAddressSpace();

	virtual	VMArea*				FirstArea() const;
	virtual	VMArea*				NextArea(VMArea* area) const;

	virtual	VMArea*				LookupArea(addr_t address) const;
	virtual	status_t			InsertArea(void** _address, uint32 addressSpec,
									addr_t size, VMArea* area);
	virtual	void				RemoveArea(VMArea* area);

	virtual	bool				CanResizeArea(VMArea* area, size_t newSize);
	virtual	status_t			ResizeArea(VMArea* area, size_t newSize);
	virtual	status_t			ResizeAreaHead(VMArea* area, size_t size);
	virtual	status_t			ResizeAreaTail(VMArea* area, size_t size);

	virtual	status_t			ReserveAddressRange(void** _address,
									uint32 addressSpec, size_t size,
									uint32 flags);
	virtual	status_t			UnreserveAddressRange(addr_t address,
									size_t size);
	virtual	void				UnreserveAllAddressRanges();

	virtual	void				Dump() const;

private:
			status_t			_InsertAreaIntoReservedRegion(addr_t start,
									size_t size, VMArea* area);
			status_t			_InsertAreaSlot(addr_t start, addr_t size,
									addr_t end, uint32 addressSpec,
									VMArea* area);

private:
			VMAddressSpaceAreaList fAreas;
	mutable	VMArea*				fAreaHint;
};


#endif	/* VM_USER_ADDRESS_SPACE_H */
