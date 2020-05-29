/*
 * Copyright 2009-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_VM_VM_ADDRESS_SPACE_H
#define _KERNEL_VM_VM_ADDRESS_SPACE_H


#include <OS.h>

#include <vm/vm_priv.h>
#include <vm/VMArea.h>
#include <vm/VMTranslationMap.h>


struct virtual_address_restrictions;


struct VMAddressSpace {
public:
			class AreaIterator;
			class AreaRangeIterator;

public:
								VMAddressSpace(team_id id, addr_t base,
									size_t size, const char* name);
	virtual						~VMAddressSpace();

	static	status_t			Init();

			team_id				ID() const				{ return fID; }
			addr_t				Base() const			{ return fBase; }
			addr_t				EndAddress() const		{ return fEndAddress; }
			size_t				Size() const { return fEndAddress - fBase + 1; }
			size_t				FreeSpace() const		{ return fFreeSpace; }
			bool				IsBeingDeleted() const	{ return fDeleting; }

			VMTranslationMap*	TranslationMap()	{ return fTranslationMap; }

			status_t			ReadLock()
									{ return rw_lock_read_lock(&fLock); }
			void				ReadUnlock()
									{ rw_lock_read_unlock(&fLock); }
			status_t			WriteLock()
									{ return rw_lock_write_lock(&fLock); }
			void				WriteUnlock()
									{ rw_lock_write_unlock(&fLock); }

			int32				RefCount() const
									{ return fRefCount; }

	inline	void				Get()	{ atomic_add(&fRefCount, 1); }
	inline	void				Put();
			void				RemoveAndPut();

			void				IncrementFaultCount()
									{ atomic_add(&fFaultCount, 1); }
			void				IncrementChangeCount()
									{ fChangeCount++; }

	inline	bool				IsRandomizingEnabled() const
									{ return fRandomizingEnabled; }
	inline	void				SetRandomizingEnabled(bool enabled)
									{ fRandomizingEnabled = enabled; }

	inline	AreaIterator		GetAreaIterator();
	inline	AreaRangeIterator	GetAreaRangeIterator(addr_t address,
									addr_t size);

			VMAddressSpace*&	HashTableLink()	{ return fHashTableLink; }

	virtual	status_t			InitObject();

	virtual	VMArea*				FirstArea() const = 0;
	virtual	VMArea*				NextArea(VMArea* area) const = 0;

	virtual	VMArea*				LookupArea(addr_t address) const = 0;
	virtual	VMArea*				FindClosestArea(addr_t address, bool less) const
									= 0;
	virtual	VMArea*				CreateArea(const char* name, uint32 wiring,
									uint32 protection,
									uint32 allocationFlags) = 0;
	virtual	void				DeleteArea(VMArea* area,
									uint32 allocationFlags) = 0;
	virtual	status_t			InsertArea(VMArea* area, size_t size,
									const virtual_address_restrictions*
										addressRestrictions,
									uint32 allocationFlags, void** _address)
										= 0;
	virtual	void				RemoveArea(VMArea* area,
									uint32 allocationFlags) = 0;

	virtual	bool				CanResizeArea(VMArea* area, size_t newSize) = 0;
	virtual	status_t			ResizeArea(VMArea* area, size_t newSize,
									uint32 allocationFlags) = 0;
	virtual	status_t			ShrinkAreaHead(VMArea* area, size_t newSize,
									uint32 allocationFlags) = 0;
	virtual	status_t			ShrinkAreaTail(VMArea* area, size_t newSize,
									uint32 allocationFlags) = 0;

	virtual	status_t			ReserveAddressRange(size_t size,
									const virtual_address_restrictions*
										addressRestrictions,
									uint32 flags, uint32 allocationFlags,
									void** _address) = 0;
	virtual	status_t			UnreserveAddressRange(addr_t address,
									size_t size, uint32 allocationFlags) = 0;
	virtual	void				UnreserveAllAddressRanges(
									uint32 allocationFlags) = 0;

	virtual	void				Dump() const;

	static	status_t			Create(team_id teamID, addr_t base, size_t size,
									bool kernel,
									VMAddressSpace** _addressSpace);

	static	team_id				KernelID()
									{ return sKernelAddressSpace->ID(); }
	static	VMAddressSpace*		Kernel()
									{ return sKernelAddressSpace; }
	static	VMAddressSpace*		GetKernel();

	static	team_id				CurrentID();
	static	VMAddressSpace*		GetCurrent();

	static	VMAddressSpace*		Get(team_id teamID);

	static	VMAddressSpace*		DebugFirst();
	static	VMAddressSpace*		DebugNext(VMAddressSpace* addressSpace);
	static	VMAddressSpace*		DebugGet(team_id teamID);

protected:
	static	void				_DeleteIfUnreferenced(team_id id);

	static	int					_DumpCommand(int argc, char** argv);
	static	int					_DumpListCommand(int argc, char** argv);

protected:
			struct HashDefinition;

protected:
			VMAddressSpace*		fHashTableLink;
			addr_t				fBase;
			addr_t				fEndAddress;		// base + (size - 1)
			size_t				fFreeSpace;
			rw_lock				fLock;
			team_id				fID;
			int32				fRefCount;
			int32				fFaultCount;
			int32				fChangeCount;
			VMTranslationMap*	fTranslationMap;
			bool				fRandomizingEnabled;
			bool				fDeleting;
	static	VMAddressSpace*		sKernelAddressSpace;
};


void
VMAddressSpace::Put()
{
	team_id id = fID;
	if (atomic_add(&fRefCount, -1) == 1)
		_DeleteIfUnreferenced(id);
}


class VMAddressSpace::AreaIterator {
public:
	AreaIterator()
	{
	}

	AreaIterator(VMAddressSpace* addressSpace)
		:
		fAddressSpace(addressSpace),
		fNext(addressSpace->FirstArea())
	{
	}

	bool HasNext() const
	{
		return fNext != NULL;
	}

	VMArea* Next()
	{
		VMArea* result = fNext;
		if (fNext != NULL)
			fNext = fAddressSpace->NextArea(fNext);
		return result;
	}

	void Rewind()
	{
		fNext = fAddressSpace->FirstArea();
	}

private:
	VMAddressSpace*	fAddressSpace;
	VMArea*			fNext;
};


class VMAddressSpace::AreaRangeIterator : public VMAddressSpace::AreaIterator {
public:
	AreaRangeIterator()
	{
	}

	AreaRangeIterator(VMAddressSpace* addressSpace, addr_t address, addr_t size)
		:
		fAddressSpace(addressSpace),
		fNext(NULL),
		fAddress(address),
		fEndAddress(address + size - 1)
	{
		Rewind();
	}

	bool HasNext() const
	{
		return fNext != NULL;
	}

	VMArea* Next()
	{
		VMArea* result = fNext;
		if (fNext != NULL) {
			fNext = fAddressSpace->NextArea(fNext);
			if (fNext != NULL && fNext->Base() > fEndAddress)
				fNext = NULL;
		}

		return result;
	}

	void Rewind()
	{
		fNext = fAddressSpace->FindClosestArea(fAddress, true);
		if (fNext != NULL && !fNext->ContainsAddress(fAddress))
			Next();
	}

private:
	VMAddressSpace*	fAddressSpace;
	VMArea*			fNext;
	addr_t			fAddress;
	addr_t			fEndAddress;
};


inline VMAddressSpace::AreaIterator
VMAddressSpace::GetAreaIterator()
{
	return AreaIterator(this);
}


inline VMAddressSpace::AreaRangeIterator
VMAddressSpace::GetAreaRangeIterator(addr_t address, addr_t size)
{
	return AreaRangeIterator(this, address, size);
}


#ifdef __cplusplus
extern "C" {
#endif

void vm_delete_areas(struct VMAddressSpace *aspace, bool deletingAddressSpace);
#define vm_swap_address_space(from, to) arch_vm_aspace_swap(from, to)

#ifdef __cplusplus
}
#endif


#endif	/* _KERNEL_VM_VM_ADDRESS_SPACE_H */
