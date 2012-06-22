/*
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include "elf.h"

#include <boot/arch.h>
#include <boot/platform.h>
#include <boot/stage2.h>
#include <driver_settings.h>
#include <elf32.h>
#include <kernel.h>

#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

//#define TRACE_ELF
#ifdef TRACE_ELF
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


static bool sLoadElfSymbols = true;


void
elf_init()
{
// TODO: This cannot work, since the driver settings are loaded *after* the
// kernel has been loaded successfully.
#if 0
	void *settings = load_driver_settings("kernel");
	if (settings == NULL)
		return;

	sLoadElfSymbols = !get_driver_boolean_parameter(settings, "load_symbols",
		false, false);
	unload_driver_settings(settings);
#endif
}


static status_t
verify_elf_header(struct Elf32_Ehdr &header)
{
	if (memcmp(header.e_ident, ELF_MAGIC, 4) != 0
		|| header.e_ident[4] != ELFCLASS32
		|| header.e_phoff == 0
		|| !header.IsHostEndian()
		|| header.e_phentsize != sizeof(struct Elf32_Phdr))
		return B_BAD_TYPE;

	return B_OK;
}


static status_t
elf_parse_dynamic_section(preloaded_elf32_image *image)
{
	image->syms = 0;
	image->rel = 0;
	image->rel_len = 0;
	image->rela = 0;
	image->rela_len = 0;
	image->pltrel = 0;
	image->pltrel_len = 0;
	image->pltrel_type = 0;

	struct Elf32_Dyn *d = (struct Elf32_Dyn *)image->dynamic_section.start;
	if (!d)
		return B_ERROR;

	for (int i = 0; d[i].d_tag != DT_NULL; i++) {
		switch (d[i].d_tag) {
			case DT_HASH:
			case DT_STRTAB:
				break;
			case DT_SYMTAB:
				image->syms = (struct Elf32_Sym *)(d[i].d_un.d_ptr
					+ image->text_region.delta);
				break;
			case DT_REL:
				image->rel = (struct Elf32_Rel *)(d[i].d_un.d_ptr
					+ image->text_region.delta);
				break;
			case DT_RELSZ:
				image->rel_len = d[i].d_un.d_val;
				break;
			case DT_RELA:
				image->rela = (struct Elf32_Rela *)(d[i].d_un.d_ptr
					+ image->text_region.delta);
				break;
			case DT_RELASZ:
				image->rela_len = d[i].d_un.d_val;
				break;
			case DT_JMPREL:
				image->pltrel = (struct Elf32_Rel *)(d[i].d_un.d_ptr
					+ image->text_region.delta);
				break;
			case DT_PLTRELSZ:
				image->pltrel_len = d[i].d_un.d_val;
				break;
			case DT_PLTREL:
				image->pltrel_type = d[i].d_un.d_val;
				break;

			default:
				continue;
		}
	}

	// lets make sure we found all the required sections
	if (image->syms == NULL)
		return B_ERROR;

	return B_OK;
}


static status_t
load_elf_symbol_table(int fd, preloaded_elf32_image *image)
{
	struct Elf32_Ehdr &elfHeader = image->elf_header;
	Elf32_Sym *symbolTable = NULL;
	Elf32_Shdr *stringHeader = NULL;
	uint32 numSymbols = 0;
	char *stringTable;
	status_t status;

	// get section headers

	ssize_t size = elfHeader.e_shnum * elfHeader.e_shentsize;
	Elf32_Shdr *sectionHeaders = (struct Elf32_Shdr *)malloc(size);
	if (sectionHeaders == NULL) {
		dprintf("error allocating space for section headers\n");
		return B_NO_MEMORY;
	}

	ssize_t length = read_pos(fd, elfHeader.e_shoff, sectionHeaders, size);
	if (length < size) {
		TRACE(("error reading in program headers\n"));
		status = B_ERROR;
		goto error1;
	}

	// find symbol table in section headers

	for (int32 i = 0; i < elfHeader.e_shnum; i++) {
		if (sectionHeaders[i].sh_type == SHT_SYMTAB) {
			stringHeader = &sectionHeaders[sectionHeaders[i].sh_link];

			if (stringHeader->sh_type != SHT_STRTAB) {
				TRACE(("doesn't link to string table\n"));
				status = B_BAD_DATA;
				goto error1;
			}

			// read in symbol table
			symbolTable = (Elf32_Sym *)kernel_args_malloc(
				size = sectionHeaders[i].sh_size);
			if (symbolTable == NULL) {
				status = B_NO_MEMORY;
				goto error1;
			}

			length = read_pos(fd, sectionHeaders[i].sh_offset, symbolTable,
				size);
			if (length < size) {
				TRACE(("error reading in symbol table\n"));
				status = B_ERROR;
				goto error1;
			}

			numSymbols = size / sizeof(Elf32_Sym);
			break;
		}
	}

	if (symbolTable == NULL) {
		TRACE(("no symbol table\n"));
		status = B_BAD_VALUE;
		goto error1;
	}

	// read in string table

	stringTable = (char *)kernel_args_malloc(size = stringHeader->sh_size);
	if (stringTable == NULL) {
		status = B_NO_MEMORY;
		goto error2;
	}

	length = read_pos(fd, stringHeader->sh_offset, stringTable, size);
	if (length < size) {
		TRACE(("error reading in string table\n"));
		status = B_ERROR;
		goto error3;
	}

	TRACE(("loaded %ld debug symbols\n", numSymbols));

	// insert tables into image
	image->debug_symbols = symbolTable;
	image->num_debug_symbols = numSymbols;
	image->debug_string_table = stringTable;
	image->debug_string_table_size = size;

	free(sectionHeaders);
	return B_OK;

error3:
	kernel_args_free(stringTable);
error2:
	kernel_args_free(symbolTable);
error1:
	free(sectionHeaders);

	return status;
}


status_t
elf_load_image(int fd, preloaded_image **_image)
{
	size_t totalSize;
	status_t status;

	TRACE(("elf_load_image(fd = %d, _image = %p)\n", fd, _image));

	preloaded_elf32_image *image = (preloaded_elf32_image *)kernel_args_malloc(
		sizeof(preloaded_elf32_image));
	if (image == NULL)
		return B_NO_MEMORY;

	struct Elf32_Ehdr &elfHeader = image->elf_header;

	ssize_t length = read_pos(fd, 0, &elfHeader, sizeof(Elf32_Ehdr));
	if (length < (ssize_t)sizeof(Elf32_Ehdr)) {
		kernel_args_free(image);
		return B_BAD_TYPE;
	}

	status = verify_elf_header(elfHeader);
	if (status < B_OK) {
		kernel_args_free(image);
		return status;
	}

	image->elf_class = elfHeader.e_ident[EI_CLASS];

	ssize_t size = elfHeader.e_phnum * elfHeader.e_phentsize;
	Elf32_Phdr *programHeaders = (struct Elf32_Phdr *)malloc(size);
	if (programHeaders == NULL) {
		dprintf("error allocating space for program headers\n");
		status = B_NO_MEMORY;
		goto error1;
	}

	length = read_pos(fd, elfHeader.e_phoff, programHeaders, size);
	if (length < size) {
		TRACE(("error reading in program headers\n"));
		status = B_ERROR;
		goto error1;
	}

	// create an area large enough to hold the image

	image->data_region.size = 0;
	image->text_region.size = 0;

	for (int32 i = 0; i < elfHeader.e_phnum; i++) {
		Elf32_Phdr &header = programHeaders[i];

		switch (header.p_type) {
			case PT_LOAD:
				break;
			case PT_DYNAMIC:
				image->dynamic_section.start = header.p_vaddr;
				image->dynamic_section.size = header.p_memsz;
				continue;
			case PT_INTERP:
			case PT_PHDR:
				// known but unused type
				continue;
			default:
				dprintf("unhandled pheader type 0x%lx\n", header.p_type);
				continue;
		}

		elf32_region *region;
		if (header.IsReadWrite()) {
			if (image->data_region.size != 0) {
				dprintf("elf: rw already handled!\n");
				continue;
			}
			region = &image->data_region;
		} else if (header.IsExecutable()) {
			if (image->text_region.size != 0) {
				dprintf("elf: ro already handled!\n");
				continue;
			}
			region = &image->text_region;
		} else
			continue;

		region->start = ROUNDDOWN(header.p_vaddr, B_PAGE_SIZE);
		region->size = ROUNDUP(header.p_memsz + (header.p_vaddr % B_PAGE_SIZE),
			B_PAGE_SIZE);
		region->delta = -region->start;

		TRACE(("segment %ld: start = 0x%lx, size = %lu, delta = %lx\n", i,
			region->start, region->size, region->delta));
	}

	// found both, text and data?
	if (image->data_region.size == 0 || image->text_region.size == 0) {
		dprintf("Couldn't find both text and data segment!\n");
		status = B_BAD_DATA;
		goto error1;
	}

	// get the segment order
	elf32_region *firstRegion;
	elf32_region *secondRegion;
	if (image->text_region.start < image->data_region.start) {
		firstRegion = &image->text_region;
		secondRegion = &image->data_region;
	} else {
		firstRegion = &image->data_region;
		secondRegion = &image->text_region;
	}

	// Check whether the segments have an unreasonable amount of unused space
	// inbetween.
	totalSize = secondRegion->start + secondRegion->size - firstRegion->start;
	if (totalSize > image->text_region.size + image->data_region.size
		+ 8 * 1024) {
		status = B_BAD_DATA;
		goto error1;
	}

	// The kernel and the modules are relocatable, thus
	// platform_allocate_region() can automatically allocate an address,
	// but shall prefer the specified base address.
	if (platform_allocate_region((void **)&firstRegion->start, totalSize,
			B_READ_AREA | B_WRITE_AREA, false) < B_OK) {
		status = B_NO_MEMORY;
		goto error1;
	}

	// initialize the region pointers to the allocated region
	secondRegion->start += firstRegion->start + firstRegion->delta;

	image->data_region.delta += image->data_region.start;
	image->text_region.delta += image->text_region.start;

	// load program data

	for (int i = 0; i < elfHeader.e_phnum; i++) {
		Elf32_Phdr &header = programHeaders[i];

		if (header.p_type != PT_LOAD)
			continue;

		elf32_region *region;
		if (header.IsReadWrite())
			region = &image->data_region;
		else if (header.IsExecutable())
			region = &image->text_region;
		else
			continue;

		TRACE(("load segment %d (%ld bytes)...\n", i, header.p_filesz));

		length = read_pos(fd, header.p_offset,
			(void *)(region->start + (header.p_vaddr % B_PAGE_SIZE)),
			header.p_filesz);
		if (length < (ssize_t)header.p_filesz) {
			status = B_BAD_DATA;
			dprintf("error reading in seg %d\n", i);
			goto error2;
		}

		// Clear anything above the file size (that may also contain the BSS
		// area)

		uint32 offset = (header.p_vaddr % B_PAGE_SIZE) + header.p_filesz;
		if (offset < region->size)
			memset((void *)(region->start + offset), 0, region->size - offset);
	}

	// offset dynamic section, and program entry addresses by the delta of the
	// regions
	image->dynamic_section.start += image->text_region.delta;
	image->elf_header.e_entry += image->text_region.delta;

	image->num_debug_symbols = 0;
	image->debug_symbols = NULL;
	image->debug_string_table = NULL;

	if (sLoadElfSymbols)
		load_elf_symbol_table(fd, image);

	free(programHeaders);

	*_image = image;
	return B_OK;

error2:
	if (image->text_region.start != 0)
		platform_free_region((void *)image->text_region.start, totalSize);
error1:
	free(programHeaders);
	kernel_args_free(image);

	return status;
}


status_t
elf_load_image(Directory *directory, const char *path)
{
	preloaded_image *image;

	TRACE(("elf_load_image(directory = %p, \"%s\")\n", directory, path));

	int fd = open_from(directory, path, O_RDONLY);
	if (fd < 0)
		return fd;

	// check if this file has already been loaded

	struct stat stat;
	if (fstat(fd, &stat) < 0)
		return errno;

	image = gKernelArgs.preloaded_images;
	for (; image != NULL; image = image->next) {
		if (image->inode == stat.st_ino) {
			// file has already been loaded, no need to load it twice!
			close(fd);
			return B_OK;
		}
	}

	// we still need to load it, so do it

	status_t status = elf_load_image(fd, &image);
	if (status == B_OK) {
		image->name = kernel_args_strdup(path);
		image->inode = stat.st_ino;

		// insert to kernel args
		image->next = gKernelArgs.preloaded_images;
		gKernelArgs.preloaded_images = image;
	} else
		kernel_args_free(image);

	close(fd);
	return status;
}


status_t
elf_relocate_image(preloaded_image *_image)
{
	preloaded_elf32_image *image = static_cast<preloaded_elf32_image *>(_image);

	status_t status = elf_parse_dynamic_section(image);
	if (status != B_OK)
		return status;

	// deal with the rels first
	if (image->rel) {
		TRACE(("total %i relocs\n",
			image->rel_len / (int)sizeof(struct Elf32_Rel)));

		status = boot_arch_elf_relocate_rel(image, image->rel, image->rel_len);
		if (status < B_OK)
			return status;
	}

	if (image->pltrel) {
		TRACE(("total %i plt-relocs\n",
			image->pltrel_len / (int)sizeof(struct Elf32_Rel)));

		struct Elf32_Rel *pltrel = image->pltrel;
		if (image->pltrel_type == DT_REL) {
			status = boot_arch_elf_relocate_rel(image, pltrel,
				image->pltrel_len);
		} else {
			status = boot_arch_elf_relocate_rela(image,
				(struct Elf32_Rela *)pltrel, image->pltrel_len);
		}
		if (status < B_OK)
			return status;
	}

	if (image->rela) {
		TRACE(("total %i rela relocs\n",
			image->rela_len / (int)sizeof(struct Elf32_Rela)));
		status = boot_arch_elf_relocate_rela(image, image->rela,
			image->rela_len);
		if (status < B_OK)
			return status;
	}

	return B_OK;
}


status_t
boot_elf_resolve_symbol(struct preloaded_elf32_image *image,
	struct Elf32_Sym *symbol, addr_t *symbolAddress)
{
	switch (symbol->st_shndx) {
		case SHN_UNDEF:
			// Since we do that only for the kernel, there shouldn't be
			// undefined symbols.
			return B_MISSING_SYMBOL;
		case SHN_ABS:
			*symbolAddress = symbol->st_value;
			return B_NO_ERROR;
		case SHN_COMMON:
			// ToDo: finish this
			TRACE(("elf_resolve_symbol: COMMON symbol, finish me!\n"));
			return B_ERROR;
		default:
			// standard symbol
			*symbolAddress = symbol->st_value + image->text_region.delta;
			return B_NO_ERROR;
	}
}
