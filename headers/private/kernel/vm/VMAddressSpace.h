/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
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
#include <vm/vm_translation_map.h>
#include <vm/VMArea.h>


struct VMAddressSpace {
public:
			class AreaIterator;

public:
								VMAddressSpace(team_id id, addr_t base,
									size_t size, const char* name);
	virtual						~VMAddressSpace();

	static	status_t			Init();
	static	status_t			InitPostSem();

			team_id				ID() const				{ return fID; }
			addr_t				Base() const			{ return fBase; }
			addr_t				EndAddress() const		{ return fEndAddress; }
			size_t				FreeSpace() const		{ return fFreeSpace; }
			bool				IsBeingDeleted() const	{ return fDeleting; }

			vm_translation_map&	TranslationMap()	{ return fTranslationMap; }

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

			void				Get()	{ atomic_add(&fRefCount, 1); }
			void 				Put();
			void				RemoveAndPut();

			void				IncrementFaultCount()
									{ atomic_add(&fFaultCount, 1); }
			void				IncrementChangeCount()
									{ fChangeCount++; }

	inline	AreaIterator		GetAreaIterator();

			VMAddressSpace*&	HashTableLink()	{ return fHashTableLink; }

	virtual	VMArea*				FirstArea() const = 0;
	virtual	VMArea*				NextArea(VMArea* area) const = 0;

	virtual	VMArea*				LookupArea(addr_t address) const = 0;
	virtual	status_t			InsertArea(void** _address, uint32 addressSpec,
									addr_t size, VMArea* area) = 0;
	virtual	void				RemoveArea(VMArea* area) = 0;

	virtual	bool				CanResizeArea(VMArea* area, size_t newSize) = 0;
	virtual	status_t			ResizeArea(VMArea* area, size_t newSize) = 0;
	virtual	status_t			ResizeAreaHead(VMArea* area, size_t size) = 0;
	virtual	status_t			ResizeAreaTail(VMArea* area, size_t size) = 0;

	virtual	status_t			ReserveAddressRange(void** _address,
									uint32 addressSpec, size_t size,
									uint32 flags) = 0;
	virtual	status_t			UnreserveAddressRange(addr_t address,
									size_t size) = 0;
	virtual	void				UnreserveAllAddressRanges() = 0;

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

protected:
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
			vm_translation_map	fTranslationMap;
			bool				fDeleting;
	static	VMAddressSpace*		sKernelAddressSpace;
};


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


inline VMAddressSpace::AreaIterator
VMAddressSpace::GetAreaIterator()
{
	return AreaIterator(this);
}


#ifdef __cplusplus
extern "C" {
#endif

status_t vm_delete_areas(struct VMAddressSpace *aspace);
#define vm_swap_address_space(from, to) arch_vm_aspace_swap(from, to)

#ifdef __cplusplus
}
#endif


#endif	/* _KERNEL_VM_VM_ADDRESS_SPACE_H */
