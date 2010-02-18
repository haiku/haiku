/*
 * Copyright 2005, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef CPU_GENERIC_x86_H
#define CPU_GENERIC_x86_H


#include <SupportDefs.h>


struct x86_mtrr_info;

extern uint64 gPhysicalMask;

#ifdef __cplusplus
extern "C" {
#endif

extern uint32	generic_count_mtrrs(void);
extern void		generic_init_mtrrs(uint32 count);
extern void		generic_set_mtrr(uint32 index, uint64 base, uint64 length, uint8 type);
extern status_t generic_get_mtrr(uint32 index, uint64 *_base, uint64 *_length,
					uint8 *_type);
extern void		generic_set_mtrrs(uint8 defaultType,
					const struct x86_mtrr_info* infos,
					uint32 count, uint32 maxCount);
extern status_t generic_mtrr_compute_physical_mask(void);

extern void		generic_dump_mtrrs(uint32 count);

#ifdef __cplusplus
}
#endif

#endif	// CPU_GENERIC_x86_H
