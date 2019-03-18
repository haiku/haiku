/*
 * Copyright 2019, Adrien Destugues <pulkomandy@pulkomandy.tk>
 * Copyright 2010, Ithamar R. Adema <ithamar.adema@team-embedded.nl>
 * Copyright 2009, Johannes Wischert, johanneswi@gmail.com.
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * Copyright 2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>

#include <elf_priv.h>
#include <arch/elf.h>


//#define TRACE_ARCH_ELF
#ifdef TRACE_ARCH_ELF
#	define TRACE(x) dprintf x
#	define CHATTY 1
#else
#	define TRACE(x) ;
#	define CHATTY 0
#endif


#ifndef _BOOT_MODE
static bool
is_in_image(struct elf_image_info *image, addr_t address)
{
	return (address >= image->text_region.start
			&& address < image->text_region.start + image->text_region.size)
		|| (address >= image->data_region.start
			&& address < image->data_region.start + image->data_region.size);
}
#endif	// !_BOOT_MODE


int
arch_elf_relocate_rel(struct elf_image_info *image,
	struct elf_image_info *resolve_image, Elf64_Rel *rel, int rel_len)
{
	// TODO: rel entries in RISCV64 elf
	return B_NO_ERROR;
}


static inline void
write_32(addr_t P, Elf32_Word value)
{
	*(Elf32_Word*)P = value;
}


static inline void
write_16(addr_t P, Elf32_Word value)
{
	// bits 16:29
	*(Elf32_Half*)P = (Elf32_Half)value;
}


static inline bool
write_16_check(addr_t P, Elf32_Word value)
{
	// bits 15:0
	if ((value & 0xffff0000) && (~value & 0xffff8000))
		return false;
	*(Elf32_Half*)P = (Elf32_Half)value;
	return true;
}


static inline bool
write_8(addr_t P, Elf32_Word value)
{
	// bits 7:0
	*(uint8 *)P = (uint8)value;
	return true;
}


static inline bool
write_8_check(addr_t P, Elf32_Word value)
{
	// bits 7:0
	if ((value & 0xffffff00) && (~value & 0xffffff80))
		return false;
	*(uint8 *)P = (uint8)value;
	return true;
}


int
arch_elf_relocate_rela(struct elf_image_info *image,
	struct elf_image_info *resolve_image, Elf64_Rela *rel, int rel_len)
{
	return B_OK;
}
