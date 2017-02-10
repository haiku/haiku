/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef _KERNEL_ARCH_PPC_MMU_H
#define _KERNEL_ARCH_PPC_MMU_H


#include <SupportDefs.h>
#include <string.h>

#include <arch_cpu.h>


/*** BAT - block address translation ***/

enum bat_length {
	BAT_LENGTH_128kB	= 0x0000,
	BAT_LENGTH_256kB	= 0x0001,
	BAT_LENGTH_512kB	= 0x0003,
	BAT_LENGTH_1MB		= 0x0007,
	BAT_LENGTH_2MB		= 0x000f,
	BAT_LENGTH_4MB		= 0x001f,
	BAT_LENGTH_8MB		= 0x003f,
	BAT_LENGTH_16MB		= 0x007f,
	BAT_LENGTH_32MB		= 0x00ff,
	BAT_LENGTH_64MB		= 0x01ff,
	BAT_LENGTH_128MB	= 0x03ff,
	BAT_LENGTH_256MB	= 0x07ff,
};

enum bat_protection {
	BAT_READ_ONLY = 1,
	BAT_READ_WRITE = 2,
};

struct block_address_translation {
	// upper 32 bit
	uint32	page_index : 15;				// BEPI, block effective page index
	uint32	_reserved0 : 4;
	uint32	length : 11;
	uint32	kernel_valid : 1;				// Vs, Supervisor-state valid
	uint32	user_valid : 1;					// Vp, User-state valid
	// lower 32 bit
	uint32	physical_block_number : 15;		// BPRN
	uint32	write_through : 1;				// WIMG
	uint32	caching_inhibited : 1;
	uint32	memory_coherent : 1;
	uint32	guarded : 1;
	uint32	_reserved1 : 1;
	uint32	protection : 2;

	block_address_translation()
	{
		Clear();
	}

	void SetVirtualAddress(void *address)
	{
		page_index = uint32(address) >> 17;
	}

	void SetPhysicalAddress(void *address)
	{
		physical_block_number = uint32(address) >> 17;
	}

	void Clear()
	{
		memset((void *)this, 0, sizeof(block_address_translation));
	}
};

struct segment_descriptor {
	uint32	type : 1;						// 0 for page translation descriptors
	uint32	kernel_protection_key : 1;		// Ks, Supervisor-state protection key
	uint32	user_protection_key : 1;		// Kp, User-state protection key
	uint32	no_execute_protection : 1;
	uint32	_reserved : 4;
	uint32	virtual_segment_id : 24;

	segment_descriptor()
	{
		Clear();
	}

	segment_descriptor(uint32 value)
	{
		*((uint32 *)this) = value;
	}

	void Clear()
	{
		memset((void *)this, 0, sizeof(segment_descriptor));
	}
};


/*** PTE - page table entry ***/

enum pte_protection {
	PTE_READ_ONLY	= 3,
	PTE_READ_WRITE	= 2,
};

struct page_table_entry {
	// upper 32 bit
	uint32	valid : 1;
	uint32	virtual_segment_id : 24;
	uint32	secondary_hash : 1;
	uint32	abbr_page_index : 6;
	// lower 32 bit
	uint32	physical_page_number : 20;
	uint32	_reserved0 : 3;
	uint32	referenced : 1;
	uint32	changed : 1;
	uint32	write_through : 1;				// WIMG
	uint32	caching_inhibited : 1;
	uint32	memory_coherent : 1;
	uint32	guarded : 1;
	uint32	_reserved1 : 1;
	uint32	page_protection : 2;

	static uint32 PrimaryHash(uint32 virtualSegmentID, uint32 virtualAddress);
	static uint32 SecondaryHash(uint32 virtualSegmentID, uint32 virtualAddress);
	static uint32 SecondaryHash(uint32 primaryHash);
};

struct page_table_entry_group {
	struct page_table_entry entry[8];
};

extern void ppc_get_page_table(page_table_entry_group **_pageTable, size_t *_size);
extern void ppc_set_page_table(page_table_entry_group *pageTable, size_t size);

static inline segment_descriptor
ppc_get_segment_register(void *virtualAddress)
{
	return (segment_descriptor)get_sr(virtualAddress);
}


static inline void
ppc_set_segment_register(void *virtualAddress, segment_descriptor segment)
{
	set_sr(virtualAddress, *(uint32 *)&segment);
}

#endif	/* _KERNEL_ARCH_PPC_MMU_H */
