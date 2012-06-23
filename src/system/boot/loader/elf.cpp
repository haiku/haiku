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


// #pragma mark - Generic ELF loader


template<typename Class>
class ELFLoader {
private:
	typedef typename Class::ImageType	ImageType;
	typedef typename Class::RegionType	RegionType;
	typedef typename Class::EhdrType	EhdrType;
	typedef typename Class::PhdrType	PhdrType;
	typedef typename Class::ShdrType	ShdrType;
	typedef typename Class::DynType		DynType;
	typedef typename Class::SymType		SymType;
	typedef typename Class::RelType		RelType;
	typedef typename Class::RelaType	RelaType;

public:
	static	status_t	Create(int fd, preloaded_image** _image);
	static	status_t	Load(int fd, preloaded_image* image);
	static	status_t	Relocate(preloaded_image* image);

private:
	static	status_t	_LoadSymbolTable(int fd, ImageType* image);
	static	status_t	_ParseDynamicSection(ImageType* image);
};


struct ELF32Class {
	static const uint8 kIdentClass = ELFCLASS32;

	typedef preloaded_elf32_image	ImageType;
	typedef elf32_region			RegionType;
	typedef Elf32_Ehdr				EhdrType;
	typedef Elf32_Phdr				PhdrType;
	typedef Elf32_Shdr				ShdrType;
	typedef Elf32_Dyn				DynType;
	typedef Elf32_Sym				SymType;
	typedef Elf32_Rel				RelType;
	typedef Elf32_Rela				RelaType;
};

typedef ELFLoader<ELF32Class> ELF32Loader;


#ifdef BOOT_SUPPORT_ELF64
struct ELF64Class {
	static const uint8 kIdentClass = ELFCLASS64;

	typedef preloaded_elf64_image	ImageType;
	typedef elf64_region			RegionType;
	typedef Elf64_Ehdr				EhdrType;
	typedef Elf64_Phdr				PhdrType;
	typedef Elf64_Shdr				ShdrType;
	typedef Elf64_Dyn				DynType;
	typedef Elf64_Sym				SymType;
	typedef Elf64_Rel				RelType;
	typedef Elf64_Rela				RelaType;
};

typedef ELFLoader<ELF64Class> ELF64Loader;
#endif


template<typename Class>
/*static*/ status_t
ELFLoader<Class>::Create(int fd, preloaded_image** _image)
{
	ImageType* image = (ImageType*)kernel_args_malloc(sizeof(ImageType));
	if (image == NULL)
		return B_NO_MEMORY;

	EhdrType& elfHeader = image->elf_header;

	ssize_t length = read_pos(fd, 0, &elfHeader, sizeof(EhdrType));
	if (length < (ssize_t)sizeof(EhdrType)) {
		kernel_args_free(image);
		return B_BAD_TYPE;
	}

	if (memcmp(elfHeader.e_ident, ELF_MAGIC, 4) != 0
		|| elfHeader.e_ident[4] != Class::kIdentClass
		|| elfHeader.e_phoff == 0
		|| !elfHeader.IsHostEndian()
		|| elfHeader.e_phentsize != sizeof(PhdrType)) {
		kernel_args_free(image);
		return B_BAD_TYPE;
	}

	image->elf_class = elfHeader.e_ident[EI_CLASS];

	*_image = image;
	return B_OK;
}


template<typename Class>
/*static*/ status_t
ELFLoader<Class>::Load(int fd, preloaded_image* _image)
{
	size_t totalSize;
	ssize_t length;
	status_t status;

	ImageType* image = static_cast<ImageType*>(_image);
	EhdrType& elfHeader = image->elf_header;

	ssize_t size = elfHeader.e_phnum * elfHeader.e_phentsize;
	PhdrType* programHeaders = (PhdrType*)malloc(size);
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
		PhdrType& header = programHeaders[i];

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

		RegionType* region;
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
	RegionType* firstRegion;
	RegionType* secondRegion;
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

	for (int32 i = 0; i < elfHeader.e_phnum; i++) {
		PhdrType& header = programHeaders[i];

		if (header.p_type != PT_LOAD)
			continue;

		RegionType* region;
		if (header.IsReadWrite())
			region = &image->data_region;
		else if (header.IsExecutable())
			region = &image->text_region;
		else
			continue;

		TRACE(("load segment %d (%ld bytes)...\n", i, header.p_filesz));

		length = read_pos(fd, header.p_offset,
			(void*)(region->start + (header.p_vaddr % B_PAGE_SIZE)),
			header.p_filesz);
		if (length < (ssize_t)header.p_filesz) {
			status = B_BAD_DATA;
			dprintf("error reading in seg %ld\n", i);
			goto error2;
		}

		// Clear anything above the file size (that may also contain the BSS
		// area)

		uint32 offset = (header.p_vaddr % B_PAGE_SIZE) + header.p_filesz;
		if (offset < region->size)
			memset((void*)(region->start + offset), 0, region->size - offset);
	}

	// offset dynamic section, and program entry addresses by the delta of the
	// regions
	image->dynamic_section.start += image->text_region.delta;
	image->elf_header.e_entry += image->text_region.delta;

	image->num_debug_symbols = 0;
	image->debug_symbols = NULL;
	image->debug_string_table = NULL;

	if (sLoadElfSymbols)
		_LoadSymbolTable(fd, image);

	free(programHeaders);

	return B_OK;

error2:
	if (image->text_region.start != 0)
		platform_free_region((void*)image->text_region.start, totalSize);
error1:
	free(programHeaders);
	kernel_args_free(image);

	return status;
}


template<typename Class>
/*static*/ status_t
ELFLoader<Class>::Relocate(preloaded_image* _image)
{
	ImageType* image = static_cast<ImageType*>(_image);

	status_t status = _ParseDynamicSection(image);
	if (status != B_OK)
		return status;

	// deal with the rels first
	if (image->rel) {
		TRACE(("total %i relocs\n", image->rel_len / (int)sizeof(RelType)));

		status = boot_arch_elf_relocate_rel(image, image->rel, image->rel_len);
		if (status < B_OK)
			return status;
	}

	if (image->pltrel) {
		TRACE(("total %i plt-relocs\n",
			image->pltrel_len / (int)sizeof(RelType)));

		RelType* pltrel = image->pltrel;
		if (image->pltrel_type == DT_REL) {
			status = boot_arch_elf_relocate_rel(image, pltrel,
				image->pltrel_len);
		} else {
			status = boot_arch_elf_relocate_rela(image, (RelaType *)pltrel,
				image->pltrel_len);
		}
		if (status < B_OK)
			return status;
	}

	if (image->rela) {
		TRACE(("total %i rela relocs\n",
			image->rela_len / (int)sizeof(RelaType)));
		status = boot_arch_elf_relocate_rela(image, image->rela,
			image->rela_len);
		if (status < B_OK)
			return status;
	}

	return B_OK;
}


template<typename Class>
/*static*/ status_t
ELFLoader<Class>::_LoadSymbolTable(int fd, ImageType* image)
{
	EhdrType& elfHeader = image->elf_header;
	SymType* symbolTable = NULL;
	ShdrType* stringHeader = NULL;
	uint32 numSymbols = 0;
	char* stringTable;
	status_t status;

	// get section headers

	ssize_t size = elfHeader.e_shnum * elfHeader.e_shentsize;
	ShdrType* sectionHeaders = (ShdrType*)malloc(size);
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
			size = sectionHeaders[i].sh_size;
			symbolTable = (SymType*)kernel_args_malloc(size);
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

			numSymbols = size / sizeof(SymType);
			break;
		}
	}

	if (symbolTable == NULL) {
		TRACE(("no symbol table\n"));
		status = B_BAD_VALUE;
		goto error1;
	}

	// read in string table

	size = stringHeader->sh_size;
	stringTable = (char*)kernel_args_malloc(size);
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


template<typename Class>
/*static*/ status_t
ELFLoader<Class>::_ParseDynamicSection(ImageType* image)
{
	image->syms = 0;
	image->rel = 0;
	image->rel_len = 0;
	image->rela = 0;
	image->rela_len = 0;
	image->pltrel = 0;
	image->pltrel_len = 0;
	image->pltrel_type = 0;

	DynType* d = (DynType*)image->dynamic_section.start;
	if (!d)
		return B_ERROR;

	for (int i = 0; d[i].d_tag != DT_NULL; i++) {
		switch (d[i].d_tag) {
			case DT_HASH:
			case DT_STRTAB:
				break;
			case DT_SYMTAB:
				image->syms = (SymType*)(d[i].d_un.d_ptr
					+ image->text_region.delta);
				break;
			case DT_REL:
				image->rel = (RelType*)(d[i].d_un.d_ptr
					+ image->text_region.delta);
				break;
			case DT_RELSZ:
				image->rel_len = d[i].d_un.d_val;
				break;
			case DT_RELA:
				image->rela = (RelaType*)(d[i].d_un.d_ptr
					+ image->text_region.delta);
				break;
			case DT_RELASZ:
				image->rela_len = d[i].d_un.d_val;
				break;
			case DT_JMPREL:
				image->pltrel = (RelType*)(d[i].d_un.d_ptr
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


// #pragma mark -


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


status_t
elf_load_image(int fd, preloaded_image** _image)
{
	status_t status;

	TRACE(("elf_load_image(fd = %d, _image = %p)\n", fd, _image));

#if BOOT_SUPPORT_ELF64
	status = ELF64Loader::Create(fd, _image);
	if (status == B_OK) {
		return ELF64Loader::Load(fd, *_image);
	} else if (status == B_BAD_TYPE) {
#endif
		status = ELF32Loader::Create(fd, _image);
		if (status == B_OK)
			return ELF32Loader::Load(fd, *_image);
#if BOOT_SUPPORT_ELF64
	}
#endif

	return status;
}


status_t
elf_load_image(Directory* directory, const char* path)
{
	preloaded_image* image;

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
elf_relocate_image(preloaded_image* image)
{
#ifdef BOOT_SUPPORT_ELF64
	if (image->elf_class == ELFCLASS64)
		return ELF64Loader::Relocate(image);
	else
#endif
		return ELF32Loader::Relocate(image);
}


template<typename ImageType, typename SymType, typename AddrType>
inline status_t
resolve_symbol(ImageType* image, SymType* symbol, AddrType* symbolAddress)
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


status_t
boot_elf_resolve_symbol(preloaded_elf32_image* image, struct Elf32_Sym* symbol,
	Elf32_Addr* symbolAddress)
{
	return resolve_symbol(image, symbol, symbolAddress);
}


#ifdef BOOT_SUPPORT_ELF64
status_t
boot_elf_resolve_symbol(preloaded_elf64_image* image, struct Elf64_Sym* symbol,
	Elf64_Addr* symbolAddress)
{
	return resolve_symbol(image, symbol, symbolAddress);
}
#endif
