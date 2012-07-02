/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_X86_64_DESCRIPTORS_H
#define _KERNEL_ARCH_X86_64_DESCRIPTORS_H


// Segment definitions.
// Note that the ordering of these is important to SYSCALL/SYSRET.
#define KERNEL_CODE_SEG		0x08
#define KERNEL_DATA_SEG		0x10
#define USER_DATA_SEG		0x1b
#define USER_CODE_SEG		0x23


#ifndef _ASSEMBLER


#define TSS_BASE_SEGMENT	5
#define TLS_BASE_SEGMENT	(TSS_BASE_SEGMENT + smp_get_num_cpus())


// Structure of a segment descriptor.
struct segment_descriptor {
	uint32 limit0 : 16;
	uint32 base0 : 24;
	uint32 type : 4;
	uint32 desc_type : 1;
	uint32 dpl : 2;
	uint32 present : 1;
	uint32 limit1 : 4;
	uint32 available : 1;
	uint32 long_mode : 1;
	uint32 d_b : 1;
	uint32 granularity : 1;
	uint32 base1 : 8;
} _PACKED;

// Structure of a TSS segment descriptor.
struct tss_descriptor {
	uint32 limit0 : 16;
	uint32 base0 : 24;
	uint32 type : 4;
	uint32 desc_type : 1;
	uint32 dpl : 2;
	uint32 present : 1;
	uint32 limit1 : 4;
	uint32 available : 1;
	uint32 unused1 : 2;
	uint32 granularity : 1;
	uint32 base1 : 8;
	uint32 base2 : 32;
	uint32 unused2 : 32;
} _PACKED;

// Structure of an interrupt descriptor.
struct interrupt_descriptor {
	uint32 base0 : 16;
	uint32 sel : 16;
	uint32 ist : 3;
	uint32 unused1 : 5;
	uint32 type : 4;
	uint32 unused2 : 1;
	uint32 dpl : 2;
	uint32 present : 1;
	uint32 base1 : 16;
	uint32 base2 : 32;
	uint32 reserved : 32;
} _PACKED;

struct gdt_idt_descr {
	uint16 limit;
	addr_t base;
} _PACKED;

struct tss {
	uint32 _reserved1;
	uint64 rsp0;
	uint64 rsp1;
	uint64 rsp2;
	uint64 _reserved2;
	uint64 ist1;
	uint64 ist2;
	uint64 ist3;
	uint64 ist4;
	uint64 ist5;
	uint64 ist6;
	uint64 ist7;
	uint64 _reserved3;
	uint16 _reserved4;
	uint16 io_bitmap;
} _PACKED;


static inline void
clear_segment_descriptor(segment_descriptor* desc)
{
	*(uint64*)desc = 0;
}


static inline void
set_segment_descriptor(segment_descriptor* desc, uint8 type, uint8 dpl)
{
	clear_segment_descriptor(desc);

	// In 64-bit mode the CPU ignores the base/limit of code/data segments,
	// it always treats base as 0 and does no limit checks.
	desc->base0 = 0;
	desc->base1 = 0;
	desc->limit0 = 0xffff;
	desc->limit1 = 0xf;
	desc->granularity = 1;

	desc->type = type;
	desc->desc_type = DT_CODE_DATA_SEGMENT;
	desc->dpl = dpl;
	desc->present = 1;

	desc->long_mode = (type & DT_CODE_EXECUTE_ONLY) ? 1 : 0;
		// Must be set to 1 for code segments only.
}


static inline void
set_tss_descriptor(segment_descriptor* _desc, uint64 base, uint32 limit)
{
	clear_segment_descriptor(_desc);
	clear_segment_descriptor(&_desc[1]);

	// The TSS descriptor is a special format in 64-bit mode, it is 16 bytes
	// instead of 8.
	tss_descriptor* desc = (tss_descriptor*)_desc;

	desc->base0 = base & 0xffffff;
	desc->base1 = (base >> 24) & 0xff;
	desc->base2 = (base >> 32);
	desc->limit0 = limit & 0xffff;
	desc->limit1 = (limit >> 16) & 0xf;

	desc->present = 1;
	desc->type = DT_TSS;
	desc->desc_type = DT_SYSTEM_SEGMENT;
	desc->dpl = DPL_KERNEL;
}


static inline void
set_interrupt_descriptor(interrupt_descriptor* desc, uint64 addr, uint32 type,
	uint16 seg, uint32 dpl, uint32 ist)
{
	desc->base0 = addr & 0xffff;
	desc->base1 = (addr >> 16) & 0xffff;
	desc->base2 = (addr >> 32) & 0xffffffff;
	desc->sel = seg;
	desc->ist = ist;
	desc->type = type;
	desc->dpl = dpl;
	desc->present = 1;
	desc->unused1 = 0;
	desc->unused2 = 0;
	desc->reserved = 0;
}


#endif	/* _ASSEMBLER */

#endif	/* _KERNEL_ARCH_X86_64_DESCRIPTORS_H */
