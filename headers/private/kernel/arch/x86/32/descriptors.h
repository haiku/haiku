/*
 * Copyright 2002-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_ARCH_X86_32_DESCRIPTORS_H
#define _KERNEL_ARCH_X86_32_DESCRIPTORS_H


#define KERNEL_CODE_SEG	0x8
#define KERNEL_DATA_SEG	0x10

#define USER_CODE_SEG	0x1b
#define USER_DATA_SEG	0x23

#define APM_CODE32_SEGMENT	0x28
#define APM_CODE16_SEGMENT	0x30
#define APM_DATA_SEGMENT	0x38

#define BIOS_DATA_SEGMENT	0x40

#ifndef _ASSEMBLER
	// this file can also be included from assembler as well
	// (and is in arch_interrupts.S)

#define DOUBLE_FAULT_TSS_BASE_SEGMENT 9
#define TSS_BASE_SEGMENT (DOUBLE_FAULT_TSS_BASE_SEGMENT + smp_get_num_cpus())
#define TLS_BASE_SEGMENT (TSS_BASE_SEGMENT + smp_get_num_cpus())
#define APM_BASE_SEGMENT (TLS_BASE_SEGMENT + smp_get_num_cpus())


// defines entries in the GDT/LDT

typedef struct segment_descriptor {
	uint16 limit_00_15;				// bit	 0 - 15
	uint16 base_00_15;				//		16 - 31
	uint32 base_23_16 : 8;			//		 0 -  7
	uint32 type : 4;				//		 8 - 11
	uint32 desc_type : 1;			//		12		(0 = system, 1 = code/data)
	uint32 privilege_level : 2;		//		13 - 14
	uint32 present : 1;				//		15
	uint32 limit_19_16 : 4;			//		16 - 19
	uint32 available : 1;			//		20
	uint32 zero : 1;				//		21
	uint32 d_b : 1;					//		22
	uint32 granularity : 1;			//		23
	uint32 base_31_24 : 8;			//		24 - 31
} segment_descriptor;

struct tss {
	uint16 prev_task;
	uint16 unused0;
	uint32 sp0;
	uint32 ss0;
	uint32 sp1;
	uint32 ss1;
	uint32 sp2;
	uint32 ss2;
	uint32 cr3;
	uint32 eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
	uint32 es, cs, ss, ds, fs, gs;
	uint32 ldt_seg_selector;
	uint16 unused1;
	uint16 io_map_base;
};


static inline void
clear_segment_descriptor(struct segment_descriptor *desc)
{
	*(long long *)desc = 0;
}


static inline void
set_segment_descriptor_base(struct segment_descriptor *desc, addr_t base)
{
	desc->base_00_15 = (addr_t)base & 0xffff;	// base is 32 bits long
	desc->base_23_16 = ((addr_t)base >> 16) & 0xff;
	desc->base_31_24 = ((addr_t)base >> 24) & 0xff;
}


static inline void
set_segment_descriptor(struct segment_descriptor *desc, addr_t base, uint32 limit,
	uint8 type, uint8 privilegeLevel)
{
	set_segment_descriptor_base(desc, base);

	// limit is 20 bits long
	if (limit & 0xfff00000) {
		desc->limit_00_15 = ((addr_t)limit >> 12) & 0x0ffff;
		desc->limit_19_16 = ((addr_t)limit >> 28) & 0xf;
		desc->granularity = 1;	// 4 KB granularity
	} else {
		desc->limit_00_15 = (addr_t)limit & 0x0ffff;
		desc->limit_19_16 = ((addr_t)limit >> 16) & 0xf;
		desc->granularity = 0;	// 1 byte granularity
	}
		limit >>= 12;


	desc->type = type;
	desc->desc_type = DT_CODE_DATA_SEGMENT;
	desc->privilege_level = privilegeLevel;

	desc->present = 1;
	desc->available = 0;	// system available bit is currently not used
	desc->d_b = 1;			// 32-bit code

	desc->zero = 0;
}


static inline void
set_tss_descriptor(struct segment_descriptor *desc, addr_t base, uint32 limit)
{
	// the TSS descriptor has a special layout different from the standard descriptor
	set_segment_descriptor_base(desc, base);

	desc->limit_00_15 = (addr_t)limit & 0x0ffff;
	desc->limit_19_16 = 0;

	desc->type = DT_TSS;
	desc->desc_type = DT_SYSTEM_SEGMENT;
	desc->privilege_level = DPL_KERNEL;

	desc->present = 1;
	desc->granularity = 0;	// 1 Byte granularity
	desc->available = 0;	// system available bit is currently not used
	desc->d_b = 0;

	desc->zero = 0;
}

#endif	/* _ASSEMBLER */

#endif	/* _KERNEL_ARCH_X86_32_DESCRIPTORS_H */
