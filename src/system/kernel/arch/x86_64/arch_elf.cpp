/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <elf.h>


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
