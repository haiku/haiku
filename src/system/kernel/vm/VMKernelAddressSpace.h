/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VM_KERNEL_ADDRESS_SPACE_H
#define VM_KERNEL_ADDRESS_SPACE_H


#include <vm/VMAddressSpace.h>

#include "VMKernelArea.h"


struct VMKernelAddressSpace : VMAddressSpace {
public:
								VMKernelAddressSpace(team_id id, addr_t base,
									size_t size);
	virtual						~VMKernelAddressSpace();

	virtual	status_t			InitObject();

	virtual	VMArea*				FirstArea() const;
	virtual	VMArea*				NextArea(VMArea* area) const;

	virtual	VMArea*				LookupArea(addr_t address) const;
	virtual	VMArea*				CreateArea(const char* name, uint32 wiring,
									uint32 protection);
	virtual	void				DeleteArea(VMArea* area);
	virtual	status_t			InsertArea(void** _address, uint32 addressSpec,
									size_t size, VMArea* area);
	virtual	void				RemoveArea(VMArea* area);

	virtual	bool				CanResizeArea(VMArea* area, size_t newSize);
	virtual	status_t			ResizeArea(VMArea* area, size_t newSize);
	virtual	status_t			ShrinkAreaHead(VMArea* area, size_t newSize);
	virtual	status_t			ShrinkAreaTail(VMArea* area, size_t newSize);

	virtual	status_t			ReserveAddressRange(void** _address,
									uint32 addressSpec, size_t size,
									uint32 flags);
	virtual	status_t			UnreserveAddressRange(addr_t address,
									size_t size);
	virtual	void				UnreserveAllAddressRanges();

	virtual	void				Dump() const;

private:
			typedef VMKernelAddressRange Range;
			typedef VMKernelAddressRangeTree RangeTree;
			typedef DoublyLinkedList<Range,
				DoublyLinkedListMemberGetLink<Range, &Range::listLink> >
					RangeList;
			typedef DoublyLinkedList<Range, VMKernelAddressRangeGetFreeListLink>
				RangeFreeList;

private:
	inline	void				_FreeListInsertRange(Range* range, size_t size);
	inline	void				_FreeListRemoveRange(Range* range, size_t size);

			void				_InsertRange(Range* range);
			void				_RemoveRange(Range* range);

			status_t			_AllocateRange(addr_t address,
									uint32 addressSpec, size_t size,
									bool allowReservedRange, Range*& _range);
			Range*				_FindFreeRange(addr_t start, size_t size,
									size_t alignment, uint32 addressSpec,
									bool allowReservedRange,
									addr_t& _foundAddress);
			void				_FreeRange(Range* range);

			void				_CheckStructures() const;

private:
			RangeTree			fRangeTree;
			RangeList			fRangeList;
			RangeFreeList*		fFreeLists;
			int					fFreeListCount;
};


#endif	/* VM_KERNEL_ADDRESS_SPACE_H */
