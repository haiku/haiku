/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef _KERNEL_ARCH_PPC_MMU_AMCC440_H
#define _KERNEL_ARCH_PPC_MMU_AMCC440_H


#include <SupportDefs.h>
#include <string.h>

#include <arch_cpu.h>


/*** TLB - translation lookaside buffer ***/

#define TLB_COUNT	64

/** valid tlb length values */
enum tlb_length {
	TLB_LENGTH_1kB		= 0x0,
	TLB_LENGTH_4kB		= 0x1,
	TLB_LENGTH_16kB		= 0x2,
	TLB_LENGTH_64kB		= 0x3,
	TLB_LENGTH_256kB	= 0x4,
	TLB_LENGTH_1MB		= 0x5,
	TLB_LENGTH_16MB		= 0x7,
	TLB_LENGTH_256MB	= 0x9,
};

#define TLB_V	0x200

/** structure of a real TLB entry */
//FIXME
struct tlb_entry {
	// word 0
	uint32	effective_page_number : 22;
	uint32	valid : 1;
	uint32	translation_address_space : 1;
	uint32	page_size : 4;
	uint32	parity_1 : 4;
	//uint32	translation_id : 8;
//FIXME:rest is Classic stuff
	// word 0
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

	tlb_entry()
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
		memset((void *)this, 0, sizeof(tlb_entry));
	}
};

#if 0 // XXX:Classic
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

#endif

#endif	/* _KERNEL_ARCH_PPC_MMU_AMCC440_H */
