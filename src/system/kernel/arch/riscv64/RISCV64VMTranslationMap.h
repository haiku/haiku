/*
 * Copyright 2020-2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *   X512 <danger_mail@list.ru>
 */
#ifndef _RISCV64VMTRANSLATIONMAP_H_
#define _RISCV64VMTRANSLATIONMAP_H_


#include <vm/VMTranslationMap.h>
#include <arch_cpu_defs.h>


struct RISCV64VMTranslationMap: public VMTranslationMap {
								RISCV64VMTranslationMap(bool kernel,
									phys_addr_t pageTable = 0);
	virtual						~RISCV64VMTranslationMap();

	virtual	bool				Lock();
	virtual	void				Unlock();

	virtual	addr_t				MappedSize() const;
	virtual	size_t				MaxPagesNeededToMap(addr_t start,
									addr_t end) const;

	virtual	status_t			Map(addr_t virtualAddress,
									phys_addr_t physicalAddress,
									uint32 attributes, uint32 memoryType,
									vm_page_reservation* reservation);
	virtual	status_t			Unmap(addr_t start, addr_t end);

	virtual	status_t			DebugMarkRangePresent(addr_t start, addr_t end,
									bool markPresent);

	virtual	status_t			UnmapPage(VMArea* area, addr_t address,
									bool updatePageQueue);
	virtual	void				UnmapPages(VMArea* area, addr_t base,
									size_t size, bool updatePageQueue);
	virtual	void				UnmapArea(VMArea* area,
									bool deletingAddressSpace,
									bool ignoreTopCachePageFlags);

	virtual	status_t			Query(addr_t virtualAddress,
									phys_addr_t* _physicalAddress,
									uint32* _flags);
	virtual	status_t			QueryInterrupt(addr_t virtualAddress,
									phys_addr_t* _physicalAddress,
									uint32* _flags);

	virtual	status_t			Protect(addr_t base, addr_t top,
									uint32 attributes, uint32 memoryType);
			status_t			ProtectPage(VMArea* area, addr_t address,
									uint32 attributes);
			status_t			ProtectArea(VMArea* area,
									uint32 attributes);

			status_t			SetFlags(addr_t virtualAddress,
									uint32 flags);

	virtual	status_t			ClearFlags(addr_t virtualAddress,
									uint32 flags);

	virtual	bool				ClearAccessedAndModified(
									VMArea* area, addr_t address,
									bool unmapIfUnaccessed,
									bool& _modified);

	virtual	void				Flush();

	virtual	void				DebugPrintMappingInfo(addr_t virtualAddress);
	virtual	bool				DebugGetReverseMappingInfo(
									phys_addr_t physicalAddress,
									ReverseMappingInfoCallback& callback);

	inline	phys_addr_t			PageTable();
	inline	uint64				Satp();

			status_t			MemcpyToMap(addr_t to, const char *from,
									size_t size);
			status_t			MemcpyFromMap(char *to, addr_t from,
									size_t size);
			status_t			MemsetToMap(addr_t to, char c, size_t count);
			ssize_t				StrlcpyFromMap(char *to, addr_t from,
									size_t size);
			ssize_t				StrlcpyToMap(addr_t to, const char *from,
									size_t size);

private:
			Pte*				LookupPte(addr_t virtAdr, bool alloc,
									vm_page_reservation* reservation);
			phys_addr_t			LookupAddr(addr_t virtAdr);

			bool				fIsKernel;
			phys_addr_t			fPageTable;
			uint64_t			fPageTableSize; // in page units
};


inline phys_addr_t RISCV64VMTranslationMap::PageTable()
{
	return fPageTable;
}

inline uint64 RISCV64VMTranslationMap::Satp()
{
	SatpReg satp;
	satp.ppn = fPageTable / B_PAGE_SIZE;
	satp.asid = 0;
	satp.mode = satpModeSv39;
	return satp.val;
}


struct RISCV64VMPhysicalPageMapper: public VMPhysicalPageMapper {
								RISCV64VMPhysicalPageMapper();
	virtual						~RISCV64VMPhysicalPageMapper();

	virtual	status_t			GetPage(phys_addr_t physicalAddress,
									addr_t* _virtualAddress,
									void** _handle);
	virtual	status_t			PutPage(addr_t virtualAddress,
									void* handle);

	virtual	status_t			GetPageCurrentCPU(
									phys_addr_t physicalAddress,
									addr_t* _virtualAddress,
									void** _handle);
	virtual	status_t			PutPageCurrentCPU(addr_t virtualAddress,
									void* _handle);

	virtual	status_t			GetPageDebug(phys_addr_t physicalAddress,
									addr_t* _virtualAddress,
									void** _handle);
	virtual	status_t			PutPageDebug(addr_t virtualAddress,
									void* handle);

	virtual	status_t			MemsetPhysical(phys_addr_t address, int value,
									phys_size_t length);
	virtual	status_t			MemcpyFromPhysical(void* to, phys_addr_t from,
									size_t length, bool user);
	virtual	status_t			MemcpyToPhysical(phys_addr_t to,
									const void* from, size_t length,
									bool user);
	virtual	void				MemcpyPhysicalPage(phys_addr_t to,
									phys_addr_t from);
};


#endif	// _RISCV64VMTRANSLATIONMAP_H_
