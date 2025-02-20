/*
 * Copyright 2021-2022 Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
 */

#ifndef _ARM64_ARCH_MMU_H
#define _ARM64_ARCH_MMU_H


/*
 * Quotes taken from:
 * Arm(C) Architecture Reference Manual
 * Armv8, for Armv8-A architecture profile
 * Chapter: D5.3 VMSAv8-64 translation table format descriptors
 */
class ARMv8TranslationTableDescriptor {

	/* Descriptor bit[0] identifies whether the descriptor is valid,
	 * and is 1 for a valid descriptor. If a lookup returns an invalid
	 * descriptor, the associated input address is unmapped, and any
	 * attempt to access it generates a Translation fault.
	 *
	 * Descriptor bit[1] identifies the descriptor type, and is encoded as:
	 * 0, Block The descriptor gives the base address of a block of memory,
	 * and the attributes for that memory region.
	 * 1, Table The descriptor gives the address of the next level of
	 * translation table, and for a stage 1 translation, some attributes for
	 * that translation.
	 */

	static constexpr uint64_t kTypeMask = 0x3u;

	static constexpr uint64_t kTypeInvalid = 0x0u;
	static constexpr uint64_t kTypeBlock = 0x1u;
	static constexpr uint64_t kTypeTable = 0x3u;
	static constexpr uint64_t kTypePage = 0x3u;

	// TODO: Place TABLE PAGE BLOCK prefixes accordingly
	struct UpperAttributes {
		static constexpr uint64_t TABLE_PXN	= (1UL << 59);
		static constexpr uint64_t TABLE_XN	= (1UL << 60);
		static constexpr uint64_t TABLE_AP	= (1UL << 61);
		static constexpr uint64_t TABLE_NS	= (1UL << 63);
		static constexpr uint64_t BLOCK_PXN	= (1UL << 53);
		static constexpr uint64_t BLOCK_UXN	= (1UL << 54);
	};

	struct LowerAttributes {
		static constexpr uint64_t BLOCK_NS			= (1 << 5);
		static constexpr uint64_t BLOCK_NON_SHARE	= (0 << 8);
		static constexpr uint64_t BLOCK_OUTER_SHARE	= (2 << 8);
		static constexpr uint64_t BLOCK_INNER_SHARE	= (3 << 8);
		static constexpr uint64_t BLOCK_AF			= (1UL << 10);
		static constexpr uint64_t BLOCK_NG			= (1UL << 11);
	};

public:

	static constexpr uint64 DefaultPeripheralAttribute = LowerAttributes::BLOCK_AF
		| LowerAttributes::BLOCK_NON_SHARE
		| UpperAttributes::BLOCK_PXN
		| UpperAttributes::BLOCK_UXN;

	static constexpr uint64 DefaultCodeAttribute = LowerAttributes::BLOCK_AF
		| LowerAttributes::BLOCK_OUTER_SHARE;

	ARMv8TranslationTableDescriptor(uint64_t* descriptor)
		: fDescriptor(descriptor)
	{}

	ARMv8TranslationTableDescriptor(uint64_t descriptor)
		: fDescriptor(reinterpret_cast<uint64_t*>(descriptor))
	{}

	bool IsInvalid() {
		return (*fDescriptor & kTypeMask) == kTypeInvalid;
	}

	bool IsBlock() {
		return (*fDescriptor & kTypeMask) == kTypeBlock;
	}

	bool IsPage() {
		return (*fDescriptor & kTypeMask) == kTypePage;
	}

	bool IsTable() {
		return (*fDescriptor & kTypeMask) == kTypeTable;
	}


	uint64_t* Dereference() {
		if (IsTable())
			// TODO: Use ATTR_MASK
			return reinterpret_cast<uint64_t*>((*fDescriptor) & 0x0000fffffffff000ULL);
		else
			return NULL;
	}

	void SetToTable(uint64* descriptor, uint64_t attributes) {
		*fDescriptor = reinterpret_cast<uint64_t>(descriptor) | kTypeTable;
	}

	void SetAsPage(uint64_t* physical, uint64_t attributes) {
		*fDescriptor = CleanAttributes(reinterpret_cast<uint64_t>(physical)) | attributes | kTypePage;
	}

	void SetAsBlock(uint64_t* physical, uint64_t attributes) {
		*fDescriptor = CleanAttributes(reinterpret_cast<uint64_t>(physical)) | attributes | kTypeBlock;
	}

	void Next() {
		fDescriptor++;
	}

	void JumpTo(uint16 slot) {
		fDescriptor += slot;
	}

	uint64 Value() {
		return *fDescriptor;
	}

	uint64 Location() {
		return reinterpret_cast<uint64_t>(fDescriptor);
	}

private:

	static uint64 CleanAttributes(uint64 address) {
		return address & ~ATTR_MASK;
	}

	uint64_t* fDescriptor;
};


class MemoryAttributeIndirection {

public:
	MemoryAttributeIndirection(uint8 el = kInvalidExceptionLevel)
	{
		if (el == kInvalidExceptionLevel) {
			el = arch_exception_level();
		}

		switch(el)
		{
			case 1:
				fMair = READ_SPECIALREG(MAIR_EL1);
				break;
			case 2:
				fMair = READ_SPECIALREG(MAIR_EL2);
				break;
			case 3:
				fMair = READ_SPECIALREG(MAIR_EL3);
				break;
			default:
				fMair = 0x00u;
				break;
		}
	}

	uint8 IndexOf(uint8 requirement) {

		uint64 processedMair = fMair;
		uint8 index = 0;

		while (((processedMair & 0xFF) != requirement) && (index < 8)) {
			index++;
			processedMair = (processedMair >> 8);
		}

		return (index < 8)?index:0xff;
	}


	uint64 MaskOf(uint8 requirement) {
		return IndexOf(requirement) << 2;
	}

private:
	uint64 fMair;
};


class ARMv8TranslationRegime {

	static const uint8 skTranslationLevels = 4;
public:

	struct TranslationLevel {
		uint8 shift;
		uint64 mask;
		bool blocks;
		bool tables;
		bool pages;
	};

	typedef struct TranslationLevel TranslationDescriptor[skTranslationLevels];

	ARMv8TranslationRegime(TranslationDescriptor& regime)
		: fRegime(regime)
	{}

	uint16 DescriptorIndex(addr_t virt_addr, uint8 level) {
		return (virt_addr >> fRegime[level].shift) & fRegime[level].mask;
	}

	bool BlocksAllowed(uint8 level) {
		return fRegime[level].blocks;
	}

	bool TablesAllowed(uint8 level) {
		return fRegime[level].tables;
	}

	bool PagesAllowed(uint8 level) {
		return fRegime[level].pages;
	}

	uint64 Mask(uint8 level) {
		return EntrySize(level) - 1;
	}

	bool Aligned(addr_t address, uint8 level) {
		return (address & Mask(level)) == 0;
	}

	uint64 EntrySize(uint8 level) {
		return 1ul << fRegime[level].shift;
	}

	uint64 TableSize(uint8 level) {
		return EntrySize(level) * arch_mmu_entries_per_granularity(Granularity());
	}

	uint64* AllocatePage(void) {
		uint64 size = Granularity();
		uint64* page = NULL;
#if 0
		// BUG: allocation here overlaps assigned memory ...
		if (platform_allocate_region((void **)&page, size, 0) == B_OK) {
#else
		// TODO: luckly size == B_PAGE_SIZE == 4KB ...
		page = reinterpret_cast<uint64*>(mmu_allocate_page());
		if (page != NULL) {
#endif
			memset(page, 0, size);
			if ((reinterpret_cast<uint64>(page) & (size - 1)) != 0) {
				panic("Memory requested not %lx aligned\n", size - 1);
			}
			return page;
		} else {
			panic("Unavalable memory for descriptors\n");
			return NULL;
		}
	}

	uint8 MaxLevels() {
		return skTranslationLevels;
	}

	uint64 Granularity() {
		// Size of the last level ...
		return EntrySize(skTranslationLevels - 1);
	}

private:
	TranslationDescriptor& fRegime;
};

#endif /* _ARM64_ARCH_MMU_H */
