/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VM_USER_ADDRESS_SPACE_H
#define VM_USER_ADDRESS_SPACE_H


#include <vm/VMAddressSpace.h>

#include "VMUserArea.h"


struct VMUserAddressSpace : VMAddressSpace {
public:
								VMUserAddressSpace(team_id id, addr_t base,
									size_t size);
	virtual						~VMUserAddressSpace();

	virtual	VMArea*				FirstArea() const;
	virtual	VMArea*				NextArea(VMArea* area) const;

	virtual	VMArea*				LookupArea(addr_t address) const;
	virtual	VMArea*				FindClosestArea(addr_t address, bool less)
									const;
	virtual	VMArea*				CreateArea(const char* name, uint32 wiring,
									uint32 protection, uint32 allocationFlags);
	virtual	void				DeleteArea(VMArea* area,
									uint32 allocationFlags);
	virtual	status_t			InsertArea(VMArea* area, size_t size,
									const virtual_address_restrictions*
										addressRestrictions,
									uint32 allocationFlags, void** _address);
	virtual	void				RemoveArea(VMArea* area,
									uint32 allocationFlags);

	virtual	bool				CanResizeArea(VMArea* area, size_t newSize);
	virtual	status_t			ResizeArea(VMArea* area, size_t newSize,
									uint32 allocationFlags);
	virtual	status_t			ShrinkAreaHead(VMArea* area, size_t newSize,
									uint32 allocationFlags);
	virtual	status_t			ShrinkAreaTail(VMArea* area, size_t newSize,
									uint32 allocationFlags);

	virtual	status_t			ReserveAddressRange(size_t size,
									const virtual_address_restrictions*
										addressRestrictions,
									uint32 flags, uint32 allocationFlags,
									void** _address);
	virtual	status_t			UnreserveAddressRange(addr_t address,
									size_t size, uint32 allocationFlags);
	virtual	void				UnreserveAllAddressRanges(
									uint32 allocationFlags);

	virtual	void				Dump() const;

private:
	inline	bool				_IsRandomized(uint32 addressSpec) const;
	static	addr_t				_RandomizeAddress(addr_t start, addr_t end,
									size_t alignment, bool initial = false);

			status_t			_InsertAreaIntoReservedRegion(addr_t start,
									size_t size, VMUserArea* area,
									uint32 allocationFlags);
			status_t			_InsertAreaSlot(addr_t start, addr_t size,
									addr_t end, uint32 addressSpec,
									size_t alignment, VMUserArea* area,
									uint32 allocationFlags);

private:
	static	const addr_t		kMaxRandomize;
	static	const addr_t		kMaxInitialRandomize;

			VMUserAreaTree		fAreas;
			addr_t				fNextInsertHint;
};


#endif	/* VM_USER_ADDRESS_SPACE_H */
