/*
** Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk. All rights reserved.
** Distributed under the terms of the MIT License.
*/


#include <arch_mmu.h>

#include <arch_cpu.h>
#include <debug.h>


// Address space identifiers for the MMUs
// Ultrasparc User Manual, Table 6-10
enum {
	instruction_control_asi = 0x50,
	data_control_asi = 0x58,
	instruction_8k_tsb_asi = 0x51,
	data_8k_tsb_asi = 0x59,
	instruction_64k_tsb_asi = 0x52,
	data_64k_tsb_asi = 0x5A,
	data_direct_tsb_asi = 0x5B,
	instruction_tlb_in_asi = 0x54,
	data_tlb_in_asi = 0x5C,
	instruction_tlb_access_asi = 0x55,
	data_tlb_access_asi = 0x5D,
	instruction_tlb_read_asi = 0x56,
	data_tlb_read_asi = 0x5E,
	instruction_tlb_demap_asi = 0x57,
	data_tlb_demap_asi = 0x5F,
};


// MMU register addresses
// Ultrasparc User Manual, Table 6-10
enum {
	tsb_tag_target = 0x00,            // I/D, RO
	primary_context = 0x08,           //   D, RW
	secondary_context = 0x10,         //   D, RW
	synchronous_fault_status = 0x18,  // I/D, RW
	synchronous_fault_address = 0x20, //   D, RO
	tsb = 0x28,                       // I/D, RW
	tlb_tag_access = 0x30,            // I/D, RW
	virtual_watchpoint = 0x38,        //   D, RW
	physical_watchpoint = 0x40        //   D, RW
};


extern void sparc_get_instruction_tsb(TsbEntry **_pageTable, size_t *_size)
{
	uint64_t tsbEntry;
	asm("ldxa [%[mmuRegister]] 0x50, %[destination]"
		: [destination] "=r"(tsbEntry)
		: [mmuRegister] "r"(tsb));

	*_pageTable = (TsbEntry*)(tsbEntry & ~((1ll << 13) - 1));
	*_size = 512 * (1 << (tsbEntry & 3)) * sizeof(TsbEntry);
	if (tsbEntry & (1 << 12))
		*_size *= 2;
}


extern void sparc_get_data_tsb(TsbEntry **_pageTable, size_t *_size)
{
	uint64_t tsbEntry;
	asm("ldxa [%[mmuRegister]] 0x58, %[destination]"
		: [destination] "=r"(tsbEntry)
		: [mmuRegister] "r"(tsb));

	*_pageTable = (TsbEntry*)(tsbEntry & ~((1ll << 13) - 1));
	*_size = 512 * (1 << (tsbEntry & 3)) * sizeof(TsbEntry);
	if (tsbEntry & (1 << 12))
		*_size *= 2;
}


