/*
 * Copyright 2009, François Revol, revol@free.fr.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MULTIBOOT_H
#define _MULTIBOOT_H


#ifndef __ASSEMBLER__

#include <SupportDefs.h>

#endif /* __ASSEMBLER__ */


/* minimal part of the MultiBoot API */
/* we use the official names */

/* magics */
#define MULTIBOOT_MAGIC 0x1badb002
#define MULTIBOOT_MAGIC2 0x2badb002

/* header flags */
#define MULTIBOOT_PAGE_ALIGN 0x00000001
#define MULTIBOOT_MEMORY_INFO 0x00000002
#define MULTIBOOT_VIDEO_MODE 0x00000004
#define MULTIBOOT_AOUT_KLUDGE 0x00010000

/* info flags */
#define MULTIBOOT_INFO_MEMORY 0x00000001
#define MULTIBOOT_INFO_BOOTDEV 0x00000002
#define MULTIBOOT_INFO_CMDLINE 0x00000004
#define MULTIBOOT_INFO_MODS 0x00000008
#define MULTIBOOT_INFO_MEM_MAP 0x00000040

#ifndef __ASSEMBLER__

/* info struct passed to the loader */
struct multiboot_info {
	uint32 flags;
	uint32 mem_lower;
	uint32 mem_upper;
	uint32 boot_device;
	uint32 cmdline;
	uint32 mods_count;
	uint32 mods_addr;
	uint32 syms[4];
	uint32 mmap_length;
	uint32 mmap_addr;
	uint32 drives_length;
	uint32 drives_addr;
	uint32 config_table;
	uint32 boot_loader_name;
	uint32 apm_table;
	uint32 vbe_control_info;
	uint32 vbe_mode_info;
	uint16 vbe_interface_seg;
	uint16 vbe_interface_off;
	uint16 vbe_interface_len;
};


#ifdef __cplusplus
extern "C" {
#endif

extern void dump_multiboot_info(void);
extern status_t parse_multiboot_commandline(struct stage2_args *args);

#ifdef __cplusplus
}
#endif

#endif /* __ASSEMBLER__ */

#endif /* _MULTIBOOT_H */
