/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */

#ifdef _BOOT_MODE
#	include <boot/arch.h>
#endif

#include <KernelExport.h>

#include <elf.h>
#include <elf_priv.h>

#include <arch/elf.h>


#ifndef _BOOT_MODE
// Currently got generic elf.cpp #ifdef'd out for x86_64, define stub versions here.

status_t
elf_load_user_image(const char *path, Team *team, int flags, addr_t *_entry)
{
	return B_ERROR;
}

image_id
load_kernel_add_on(const char *path)
{
	return 0;
}

status_t
unload_kernel_add_on(image_id id)
{
	return B_ERROR;
}

status_t
elf_debug_lookup_symbol_address(addr_t address, addr_t *_baseAddress,
	const char **_symbolName, const char **_imageName, bool *_exactMatch)
{
	return B_ERROR;
}

addr_t
elf_debug_lookup_symbol(const char* searchName)
{
	return 0;
}

struct elf_image_info *
elf_get_kernel_image()
{
	return NULL;
}

image_id
elf_create_memory_image(const char* imageName, addr_t text, size_t textSize,
	addr_t data, size_t dataSize)
{
	return B_ERROR;
}

status_t
elf_add_memory_image_symbol(image_id id, const char* name, addr_t address,
	size_t size, int32 type)
{
	return B_ERROR;
}

status_t
elf_init(struct kernel_args *args)
{
	return B_OK;
}

status_t
get_image_symbol(image_id image, const char *name, int32 symbolType,
	void **_symbolLocation)
{
	return B_OK;
}

status_t
_user_read_kernel_image_symbols(image_id id, struct Elf32_Sym* symbolTable,
	int32* _symbolCount, char* stringTable, size_t* _stringTableSize,
	addr_t* _imageDelta)
{
	return B_ERROR;
}
#endif

#ifdef _BOOT_MODE
status_t
boot_arch_elf_relocate_rel(preloaded_elf64_image* image, Elf64_Rel* rel,
	int relLength)
//#else
//int
//arch_elf_relocate_rel(struct elf_image_info *image,
//	struct elf_image_info *resolveImage, struct Elf32_Rel *rel, int relLength)
//#endif
{
	dprintf("arch_elf_relocate_rel: not supported on x86_64\n");
	return B_ERROR;
}


//#ifdef _BOOT_MODE
status_t
boot_arch_elf_relocate_rela(preloaded_elf64_image* image, Elf64_Rela* rel,
	int relLength)
//#else
//int
//arch_elf_relocate_rela(struct elf_image_info *image,
//	struct elf_image_info *resolveImage, struct Elf32_Rela *rel, int relLength)
//#endif
{
	for (int i = 0; i < relLength / (int)sizeof(Elf64_Rela); i++) {
		int type = ELF64_R_TYPE(rel[i].r_info);
		int symIndex = ELF64_R_SYM(rel[i].r_info);
		Elf64_Addr symAddr = 0;

		// Resolve the symbol, if any.
		if (symIndex != 0) {
			Elf64_Sym* symbol = SYMBOL(image, symIndex);

			status_t status;
//#ifdef _BOOT_MODE
			status = boot_elf_resolve_symbol(image, symbol, &symAddr);
//#else
//			status = elf_resolve_symbol(image, symbol, resolveImage, &S);
//#endif
			if (status < B_OK)
				return status;
		}

		// Address of the relocation.
		Elf64_Addr* resolveAddr = (Elf64_Addr *)(image->text_region.delta
			+ rel[i].r_offset);

		// Perform the relocation.
		switch(type) {
			case R_X86_64_NONE:
				break;
			case R_X86_64_64:
				*resolveAddr = symAddr + rel[i].r_addend;
				break;
			case R_X86_64_PC32:
				*resolveAddr = symAddr + rel[i].r_addend - rel[i].r_offset;
				break;
			case R_X86_64_GLOB_DAT:
			case R_X86_64_JUMP_SLOT:
				*resolveAddr = symAddr + rel[i].r_addend;
				break;
			case R_X86_64_RELATIVE:
				*resolveAddr = image->text_region.delta + rel[i].r_addend;
				break;
			default:
				dprintf("arch_elf_relocate_rel: unhandled relocation type %d\n",
					type);
				return B_BAD_DATA;
		}
	}

	return B_OK;
}
#endif
