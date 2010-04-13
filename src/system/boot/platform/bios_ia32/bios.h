/*
 * Copyright 2004-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BIOS_H
#define BIOS_H


#include <SupportDefs.h>


// The values in this structure are passed to the BIOS call, and
// are updated to the register contents after that call.

struct bios_regs {
	uint32	eax;
	uint32	ebx;
	uint32	ecx;
	uint32	edx;
	uint32	esi;
	uint32	edi;
	uint16	es;
	uint16	flags;
};

#define CARRY_FLAG	0x01
#define ZERO_FLAG	0x40

#define ADDRESS_SEGMENT(address) ((addr_t)(address) >> 4)
#define ADDRESS_OFFSET(address) ((addr_t)(address) & 0xf)
#define LINEAR_ADDRESS(segment, offset) \
	(((addr_t)(segment) << 4) + (addr_t)(offset))
#define SEGMENTED_TO_LINEAR(segmented) \
	LINEAR_ADDRESS((addr_t)(segmented) >> 16, (addr_t)(segmented) & 0xffff)


static const addr_t kDataSegmentScratch = 0x10020;	// about 768 bytes
static const addr_t kDataSegmentBase = 0x10000;
static const addr_t kExtraSegmentScratch = 0x2000;	// about 24 kB


#ifdef __cplusplus
extern "C" {
#endif

void	call_bios(uint8 num, struct bios_regs* regs);
uint32	boot_key_in_keyboard_buffer(void);

#ifdef __cplusplus
}
#endif


#endif	/* BIOS_H */
