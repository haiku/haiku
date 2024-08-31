/*
 * Copyright 2022 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef VMSA_V8_TRANSLATION_MAP_H
#define VMSA_V8_TRANSLATION_MAP_H


#include <arch_cpu_defs.h>
#include <vm/VMTranslationMap.h>


struct VMSAv8TranslationMap : public VMTranslationMap {
public:
	VMSAv8TranslationMap(
		bool kernel, phys_addr_t pageTable, int pageBits, int vaBits, int minBlockLevel);
	~VMSAv8TranslationMap();

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

	virtual	status_t			UnmapPage(VMArea* area, addr_t address,
									bool updatePageQueue);
/*
	virtual	void				UnmapPages(VMArea* area, addr_t base,
									size_t size, bool updatePageQueue);
	virtual	void				UnmapArea(VMArea* area,
									bool deletingAddressSpace,
									bool ignoreTopCachePageFlags);
*/

	virtual	status_t			Query(addr_t virtualAddress,
									phys_addr_t* _physicalAddress,
									uint32* _flags);
	virtual	status_t			QueryInterrupt(addr_t virtualAddress,
									phys_addr_t* _physicalAddress,
									uint32* _flags);

	virtual	status_t			Protect(addr_t base, addr_t top,
									uint32 attributes, uint32 memoryType);

	virtual	status_t			ClearFlags(addr_t virtualAddress,
									uint32 flags);

	virtual	bool				ClearAccessedAndModified(
									VMArea* area, addr_t address,
									bool unmapIfUnaccessed,
									bool& _modified);

	virtual	void				Flush();

	enum HWFeature {
		// Can HW update Access and Dirty flags, respectively?
		HW_ACCESS = 0x1,
		HW_DIRTY = 0x2,

		// Can we use the CNP bit to indicate that ASIDs are consistent across cores?
		HW_COMMON_NOT_PRIVATE = 0x4
	};

	static uint32_t fHwFeature;
	static uint64_t fMair;

	static uint64_t GetMemoryAttr(uint32 attributes, uint32 memoryType, bool isKernel);
	static int CalcStartLevel(int vaBits, int pageBits);

	static void SwitchUserMap(VMSAv8TranslationMap *from, VMSAv8TranslationMap *to);

private:
	bool fIsKernel;
	phys_addr_t fPageTable;
	int fPageBits;
	int fVaBits;
	int fMinBlockLevel;
	int fInitialLevel;
	int fASID;
	int fRefcount;

private:
	static uint8_t MairIndex(uint8_t type);
	bool ValidateVa(addr_t va);
	uint64_t* TableFromPa(phys_addr_t pa);
	template<typename EntryRemoved>
	void FreeTable(phys_addr_t ptPa, uint64_t va, int level, EntryRemoved &&entryRemoved);
	phys_addr_t GetOrMakeTable(phys_addr_t ptPa, int level, int index, vm_page_reservation* reservation);
	template<typename UpdatePte>
	void ProcessRange(phys_addr_t ptPa, int level, addr_t va, size_t size,
		vm_page_reservation* reservation, UpdatePte &&updatePte);
	void PerformPteBreakBeforeMake(uint64_t* ptePtr, addr_t va);
	void FlushVAFromTLBByASID(addr_t va);
};


#endif
