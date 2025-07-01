/*
 * Copyright 2002-2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Copyright 2002, Travis Geiselbrecht. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "elf_priv.h"

#include <elf.h>
#include <kernel.h>
#include <kimage.h>
#include <safemode.h>
#include <user_runtime.h>
#include <vm/vm.h>
#include <vm/VMAddressSpace.h>

#include <arch/elf.h>
#include <boot/stage2.h>
#include <debug.h>
#include <elf_parser.h>
#include <runtime_loader.h>
#include <string.h>
#include <sys/cdefs.h>
#include <syscall_numbers.h>
#include <team.h>
#include <thread.h>
#include <util/OpenHashTable.h>
#include <util/Vector.h>
#include <vm/vm_priv.h>
#include <vfs.h> // for fs_read_vnode_page

#include <new>


//#define TRACE_ELF
#ifdef TRACE_ELF
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


struct elf_region {
	area_id		id;
	addr_t		start;
	addr_t		size;
	int32		delta;
	uint32		protection;
};


#define ELF_MAX_REGIONS			8
	// that should be enough for everyone (text, data, bss, shared text,
	// shared data, shared bss, runtime_loader text, runtime_loader data)

#define IS_KERNEL_ADDRESS(x)	(((addr_t)(x)) >= KERNEL_BASE)

typedef ElfParser<BKernel::Team, ElfClassKernel> KernelElfParser;

// The kernel's image. We can't use image_id for it, since that clashes with
// the userland libroot definition.
static image_t sKernelImage;

static image_t* sPreloadedImages;
static uint32 sPreloadedImageCount;

static image_t* sLoadedUserImages = NULL;
static mutex sLoadedUserImagesLock;

static struct ElfImageHashTableLink {
	ElfImageHashTableLink* Next() const { return fNext; }
	image_t* Previous() const { return fPrevious; }
	image_t* Value() const { return fImage; }

	static uint32 Hash(image_t* image)
	{
		return image->id;
	}

	static ElfImageHashTableLink*& GetLink(image_t* image)
	{
		return image->hash_link;
	}

	static bool Compare(image_t* image, image_t* otherImage)
	{
		return image->id == otherImage->id;
	}

	image_t*				fImage;
	ElfImageHashTableLink*	fNext;
	image_t*				fPrevious;
};

typedef BOpenHashTable<ElfImageHashTableLink, image_t*, image_t*,
	ElfImageHashTableLink> ElfImageHashTable;

static ElfImageHashTable* sUserImageHashTable = NULL;


static status_t elf_verify_header(const Elf_Ehdr* eheader);
static status_t elf_verify_program_headers(const Elf_Ehdr* eheader,
	const Elf_Phdr* programHeaders, size_t programHeadersSize,
	size_t* _imageSize, size_t* _textSize, size_t* _dataSize,
	size_t* _bssSize);
static status_t load_elf_segments(int fd, const Elf_Ehdr* eheader,
	const Elf_Phdr* programHeaders, size_t programHeadersSize,
	bool kernel, addr_t* _loadAddress, addr_t* _entryAddress,
	addr_t* _dynamicSection);
static status_t parse_program_headers(const Elf_Phdr* programHeaders,
	int programHeaderCount, elf_region* regions, int* _regionCount,
	addr_t* _loadOffset, addr_t* _entryAddress, addr_t* _dynamicSection,
	addr_t* _textSize, addr_t* _dataSize, addr_t* _bssSize,
	size_t* _imageSize);
static status_t map_elf_segments(int fd, const Elf_Phdr* programHeaders,
	int programHeaderCount, elf_region* regions, int regionCount,
	addr_t loadOffset, bool mapIntoKernelSpace, const char* name,
	uint32 wiring, uint32 protection, uint32 addressSpec, addr_t* _baseAddress,
	addr_t* _entryAddress, addr_t* _dynamicSection);
static status_t elf_parse_dynamic_section(elf_image_info *image);
static status_t elf_relocate_image(elf_image_info *image);


// #pragma mark -


/*!	Verifies an ELF header.
	Currently, this function only checks the type of the ELF header.
	It may be extended to do more checks on other fields in the future.
*/
static status_t
elf_verify_header(const Elf_Ehdr* eheader)
{
	if (memcmp(eheader->e_ident, ELF_MAGIC, 4) != 0)
		return B_NOT_AN_EXECUTABLE;

	if (eheader->e_ident[EI_CLASS] != ELF_CLASS)
		return B_NOT_AN_EXECUTABLE;

	if (eheader->e_ident[EI_DATA] != ELF_DATA_ENCODING)
		return B_NOT_AN_EXECUTABLE;

	if (eheader->e_ident[EI_VERSION] != EV_CURRENT)
		return B_NOT_AN_EXECUTABLE;

	if (eheader->e_type != ET_EXEC && eheader->e_type != ET_DYN)
		return B_NOT_AN_EXECUTABLE;

	if (eheader->e_machine != ELF_MACHINE_ARCH)
		return B_NOT_AN_EXECUTABLE;

	if (eheader->e_phentsize != sizeof(Elf_Phdr))
		return B_NOT_AN_EXECUTABLE;

	if (eheader->e_shentsize != sizeof(Elf_Shdr) && eheader->e_shentsize != 0)
		return B_NOT_AN_EXECUTABLE;

	return B_OK;
}


/*!	Verifies the program headers of an ELF image.
	\param eheader The ELF header.
	\param programHeaders The program headers.
	\param programHeadersSize The total size of the program headers in bytes.
	\param _imageSize On success, set to the total size of the image in memory.
	\param _textSize On success, set to the total size of the text segment.
	\param _dataSize On success, set to the total size of the data segment.
	\param _bssSize On success, set to the total size of the bss segment.
	\return \c B_OK on success, another error code otherwise.
*/
static status_t
elf_verify_program_headers(const Elf_Ehdr* eheader,
	const Elf_Phdr* programHeaders, size_t programHeadersSize,
	size_t* _imageSize, size_t* _textSize, size_t* _dataSize,
	size_t* _bssSize)
{
	*_imageSize = 0;
	*_textSize = 0;
	*_dataSize = 0;
	*_bssSize = 0;

	if (programHeadersSize < eheader->e_phnum * sizeof(Elf_Phdr))
		return B_BAD_DATA;

	size_t imageSize = 0;
	size_t textSize = 0;
	size_t dataSize = 0;
	size_t bssSize = 0;

	for (int i = 0; i < eheader->e_phnum; i++) {
		const Elf_Phdr* phdr = &programHeaders[i];

		if (phdr->p_type != PT_LOAD)
			continue;

		if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr)
			return B_BAD_DATA; // overflow

		if (phdr->p_vaddr + phdr->p_memsz > imageSize)
			imageSize = phdr->p_vaddr + phdr->p_memsz;

		if ((phdr->p_flags & PF_X) != 0)
			textSize += phdr->p_memsz;
		else if ((phdr->p_flags & PF_W) != 0) {
			dataSize += phdr->p_filesz;
			bssSize += phdr->p_memsz - phdr->p_filesz;
		}
	}

	*_imageSize = imageSize;
	*_textSize = textSize;
	*_dataSize = dataSize;
	*_bssSize = bssSize;

	return B_OK;
}


static elf_image_info*
create_image_struct()
{
	elf_image_info* image = new(std::nothrow) elf_image_info;
	if (image == NULL)
		return NULL;

	image->name = NULL;
	image->text_region.id = B_ERROR;
	image->data_region.id = B_ERROR;
	image->text_region.start = 0;
	image->text_region.size = 0;
	image->data_region.start = 0;
	image->data_region.size = 0;
	image->text_delta = 0;
	image->num_symbols = 0;
	image->needed = NULL;
	image->symbol_table = NULL;
	image->string_table = NULL;
	image->dynamic_ptr = 0;
	image->rel = NULL;
	image->rel_len = 0;
	image->rela = NULL;
	image->rela_len = 0;
	image->pltrel = NULL;
	image->pltrel_len = 0;
	image->pltrel_type = 0;
	image->jmprel = NULL; // another name for pltrel
	image->pltgot = NULL;
	image->debug_info = NULL;
	image->elf_header = NULL;
	image->program_headers = NULL;
	image->dynamic_section = 0;
	image->dynamic_section_size = 0; // Replaced dynamic_section_memsz
	image->symbol_hash_table = NULL;
	image->plt_got_offset = 0;
	image->string_table_size = 0; // Added string_table_size
	image->soname = NULL;
	image->path = NULL;
	image->next = NULL;
	image->ref_count = 0;
	image->api_version = 0;
	image->abi = 0;
	image->type = 0;
	image->active = false;
	image->find_image_next = NULL;
	image->find_image_previous = NULL;
	image->debug_info_image = NULL;
	image->is_driver = false;
	image->no_init_fini = false;
	image->delete_self = false;
	image->hash_link = NULL;

	return image;
}


static void
delete_image_struct(elf_image_info *image)
{
	if (image->name)
		free(image->name);
	if (image->needed) {
		for (int i = 0; image->needed[i]; i++)
			free(image->needed[i]);
		free(image->needed);
	}
	if (image->symbol_table)
		free(image->symbol_table);
	if (image->string_table)
		free(image->string_table);
	if (image->symbol_hash_table)
		free(image->symbol_hash_table);
	if (image->rel)
		free(image->rel);
	if (image->rela)
		free(image->rela);
	if (image->jmprel) // Same as pltrel
		free(image->jmprel);
	if (image->elf_header)
		free(image->elf_header);
	if (image->program_headers)
		free(image->program_headers);
	if (image->soname)
		free(image->soname);
	if (image->path)
		free(image->path);

	delete image;
}


/*!	If \a fd is a normal file, this function reads from it at the current
	position. Otherwise, it's assumed to be a vnode, and vm_node_read()
	is used.
*/
static status_t
read_from_file_or_vnode(int fd, void* buffer, size_t size)
{
	if (IS_KERNEL_ADDRESS(buffer)) {
		// kernel space
		if (fd < 0) {
			// no file, just zero the buffer
			memset(buffer, 0, size);
			return B_OK;
		}

		// The fd is a vnode pointer.
		struct vnode* vnode = (struct vnode*)fd;
		off_t pos = 0; // Assuming we read from the beginning for kernel images
		return vfs_read_vnode_page(vnode, NULL, pos, (addr_t)buffer, &size);
	}

	// user space
	if (fd < 0) {
		// no file, just zero the buffer
		memset(buffer, 0, size);
		return B_OK;
	}

	ssize_t bytesRead = read(fd, buffer, size);
	if (bytesRead < 0)
		return bytesRead;
	if ((size_t)bytesRead != size)
		return B_BAD_DATA; // short read

	return B_OK;
}


static status_t
insert_preloaded_image(preloaded_elf_image* preloadedImage, bool kernel)
{
	elf_image_info* image = create_image_struct();
	if (image == NULL)
		return B_NO_MEMORY;

	image->id = next_image_id(true);
	image->name = strdup(preloadedImage->name);
	if (image->name == NULL) {
		delete_image_struct(image);
		return B_NO_MEMORY;
	}

	image->text_region.start = preloadedImage->text_region.start;
	image->text_region.size = preloadedImage->text_region.size;
	image->text_delta = preloadedImage->text_delta;
	image->data_region.start = preloadedImage->data_region.start;
	image->data_region.size = preloadedImage->data_region.size;
	image->dynamic_ptr = preloadedImage->dynamic_ptr;
	image->is_driver = preloadedImage->is_driver;
	image->no_init_fini = preloadedImage->no_init_fini;
	image->type = preloadedImage->type;

	// TODO: This is ugly! The preloaded_elf_image should store an Elf_Ehdr,
	// not an elf_ehdr!
	status_t status = verify_eheader((Elf_Ehdr*)&preloadedImage->elf_header);
	if (status != B_OK) {
		delete_image_struct(image);
		return status;
	}

	image->elf_header = (Elf_Ehdr*)malloc(sizeof(Elf_Ehdr));
	if (image->elf_header == NULL) {
		delete_image_struct(image);
		return B_NO_MEMORY;
	}
	memcpy(image->elf_header, &preloadedImage->elf_header, sizeof(Elf_Ehdr));

	// If it's the kernel, we can't access the file system yet to read the
	// program headers. But it's not a problem, since we don't need them for
	// the kernel anyway.
	if (!kernel) {
		size_t programHeadersSize = image->elf_header->e_phnum
			* image->elf_header->e_phentsize;
		image->program_headers = (Elf_Phdr*)malloc(programHeadersSize);
		if (image->program_headers == NULL) {
			delete_image_struct(image);
			return B_NO_MEMORY;
		}

		int fd = open(preloadedImage->path, O_RDONLY);
		if (fd < 0) {
			delete_image_struct(image);
			return fd;
		}
		lseek(fd, image->elf_header->e_phoff, SEEK_SET);
		status = read_from_file_or_vnode(fd, image->program_headers,
			programHeadersSize);
		close(fd);
		if (status != B_OK) {
			delete_image_struct(image);
			return status;
		}
	}

	if (kernel) {
		sKernelImage = image;
		image->ref_count = 1;
			// the kernel image will never be removed
		image->active = true;
	} else {
		image->next = sPreloadedImages;
		sPreloadedImages = image;
		sPreloadedImageCount++;
	}

	return B_OK;
}


static status_t
load_elf_symbol_table(int fd, elf_image_info* image)
{
	Elf_Ehdr* eheader = image->elf_header;
	Elf_Shdr* symTableHeader = NULL;
	Elf_Shdr* stringHeader = NULL;
	Elf_Shdr* sectionHeaders = NULL;
	status_t status = B_OK;

	if (eheader->e_shoff == 0) {
		// No section headers, so no symbol table either.
		TRACE("load_elf_symbol_table: image %p has no section headers\n", image);
		return B_OK;
	}

	// read section headers
	size_t sectionHeadersSize = eheader->e_shnum * eheader->e_shentsize;
	if (sectionHeadersSize == 0) {
		TRACE("load_elf_symbol_table: image %p has no section headers (size 0)\n", image);
		return B_OK; // No sections, so no symbols
	}

	sectionHeaders = (Elf_Shdr*)malloc(sectionHeadersSize);
	if (sectionHeaders == NULL) {
		status = B_NO_MEMORY;
		goto error0;
	}

	if (lseek(fd, eheader->e_shoff, SEEK_SET) != (off_t)eheader->e_shoff) {
		status = B_IO_ERROR;
		goto error1;
	}
	if (read_from_file_or_vnode(fd, sectionHeaders, sectionHeadersSize)
			!= (ssize_t)sectionHeadersSize) {
		status = B_IO_ERROR;
		goto error1;
	}

	// find symbol table and string sections
	for (int i = 0; i < eheader->e_shnum; i++) {
		if (sectionHeaders[i].sh_type == SHT_SYMTAB) {
			symTableHeader = &sectionHeaders[i];
		} else if (sectionHeaders[i].sh_type == SHT_STRTAB
			&& i == (int)symTableHeader->sh_link) {
				// Check if this is the string table linked by the symbol table
			stringHeader = &sectionHeaders[i];
		}
	}

	if (symTableHeader == NULL) {
		TRACE("load_elf_symbol_table: image %p has no symbol table section\n", image);
		status = B_OK; // No symbol table is not an error
		goto success_no_symbols;
	}
	if (stringHeader == NULL) {
		// Symbol table without a string table is unusual but possible.
		// We can't do much with it though.
		TRACE("load_elf_symbol_table: image %p symbol table has no string table\n", image);
		status = B_OK;
		goto success_no_symbols;
	}

	// load symbol table
	image->num_symbols = symTableHeader->sh_size / symTableHeader->sh_entsize;
	image->symbol_table = (Elf_Sym*)malloc(symTableHeader->sh_size);
	if (image->symbol_table == NULL) {
		status = B_NO_MEMORY;
		goto error2;
	}

	if (lseek(fd, symTableHeader->sh_offset, SEEK_SET) != (off_t)symTableHeader->sh_offset) {
		status = B_IO_ERROR;
		goto error2;
	}
	if (read_from_file_or_vnode(fd, image->symbol_table, symTableHeader->sh_size)
			!= (ssize_t)symTableHeader->sh_size) {
		status = B_IO_ERROR;
		goto error2;
	}

	// load string table
	{ // Define strtab_size in a new scope
		size_t strtab_size = stringHeader->sh_size;
		image->string_table = (char*)malloc(strtab_size);
		if (image->string_table == NULL) {
			status = B_NO_MEMORY;
			goto error2;
		}
		image->string_table_size = strtab_size; // Store the size

		if (lseek(fd, stringHeader->sh_offset, SEEK_SET) != (off_t)stringHeader->sh_offset) {
			status = B_IO_ERROR;
			goto error2;
		}
		if (read_from_file_or_vnode(fd, image->string_table, strtab_size)
				!= (ssize_t)strtab_size) {
			status = B_IO_ERROR;
			goto error2;
		}
	} // strtab_size scope ends

success_no_symbols:
	// This label is for successful exit when no symbols are found or needed.
	// Make sure resources allocated before this point are handled.
	// If sectionHeaders was allocated, it should be freed if not erroring out.
	if (sectionHeaders != NULL)
		free(sectionHeaders);
	return status; // This will be B_OK if we jumped here.

error2:
	if (image->string_table) {
		free(image->string_table);
		image->string_table = NULL;
		image->string_table_size = 0;
	}
	if (image->symbol_table) {
		free(image->symbol_table);
		image->symbol_table = NULL;
		image->num_symbols = 0;
	}
error1:
	if (sectionHeaders)
		free(sectionHeaders);
error0: // Added error0 label
	return status;
}


static status_t
verify_eheader(const Elf_Ehdr* eheader)
{
	if (memcmp(eheader->e_ident, ELF_MAGIC, 4) != 0)
		return B_NOT_AN_EXECUTABLE;

	if (eheader->e_ident[EI_CLASS] != ELF_CLASS)
		return B_NOT_AN_EXECUTABLE;

	if (eheader->e_ident[EI_DATA] != ELF_DATA_ENCODING)
		return B_NOT_AN_EXECUTABLE;

	if (eheader->e_ident[EI_VERSION] != EV_CURRENT)
		return B_NOT_AN_EXECUTABLE;

	if (eheader->e_type != ET_EXEC && eheader->e_type != ET_DYN)
		return B_NOT_AN_EXECUTABLE;

	if (eheader->e_machine != ELF_MACHINE_ARCH)
		return B_NOT_AN_EXECUTABLE;

	if (eheader->e_phentsize != sizeof(Elf_Phdr))
		return B_NOT_AN_EXECUTABLE;

	if (eheader->e_shentsize != sizeof(Elf_Shdr) && eheader->e_shentsize != 0)
		return B_NOT_AN_EXECUTABLE;

	return B_OK;
}


/*!	Loads the ELF image specified by \a fd.
	The file must be seekable. This function will close the file descriptor
	when it's no longer needed.
	\param fd The file descriptor for the ELF image.
	\param image On success, filled in with information about the loaded image.
	\param kernel If \c true, the image is loaded into kernel space.
	\param _entryAddress On success, set to the entry address of the image.
	\return \c B_OK on success, another error code otherwise.
*/
static status_t
load_elf_image(int fd, elf_image_info* image, bool kernel,
	addr_t* _entryAddress)
{
	image->elf_header = (Elf_Ehdr*)malloc(sizeof(Elf_Ehdr));
	if (image->elf_header == NULL)
		return B_NO_MEMORY;

	status_t status = read_from_file_or_vnode(fd, image->elf_header,
		sizeof(Elf_Ehdr));
	if (status != B_OK)
		goto error1;

	status = verify_eheader(image->elf_header);
	if (status != B_OK)
		goto error1;

	size_t programHeadersSize = image->elf_header->e_phnum
		* image->elf_header->e_phentsize;
	if (programHeadersSize == 0) {
		status = B_NOT_AN_EXECUTABLE;
		goto error1;
	}
	image->program_headers = (Elf_Phdr*)malloc(programHeadersSize);
	if (image->program_headers == NULL) {
		status = B_NO_MEMORY;
		goto error1;
	}

	if (lseek(fd, image->elf_header->e_phoff, SEEK_SET)
			!= (off_t)image->elf_header->e_phoff) {
		status = B_IO_ERROR;
		goto error2;
	}
	status = read_from_file_or_vnode(fd, image->program_headers,
		programHeadersSize);
	if (status != B_OK)
		goto error2;

	status = elf_verify_program_headers(image->elf_header,
		image->program_headers, programHeadersSize, &image->text_region.size,
		&image->text_region.size, &image->data_region.size,
		&image->data_region.size); // TODO: Fix size parameters
	if (status != B_OK)
		goto error2;

	status = load_elf_segments(fd, image->elf_header, image->program_headers,
		programHeadersSize, kernel, &image->text_region.start, _entryAddress,
		&image->dynamic_ptr);
	if (status != B_OK)
		goto error2;

	image->text_delta = image->text_region.start
		- image->program_headers[0].p_vaddr;
		// TODO: This is not correct for all images!

	// The data region start is usually the text region end, but might not be.
	// Find the PT_LOAD segment with write permissions.
	for (int i = 0; i < image->elf_header->e_phnum; i++) {
		if (image->program_headers[i].p_type == PT_LOAD
			&& (image->program_headers[i].p_flags & PF_W) != 0) {
			image->data_region.start = image->text_delta
				+ image->program_headers[i].p_vaddr;
			// data_region.size was set by elf_verify_program_headers
			break;
		}
	}

	// load symbol table and string table
	status = load_elf_symbol_table(fd, image);
	if (status != B_OK)
		goto error2;

	close(fd);
	return B_OK;

error2:
	free(image->program_headers);
	image->program_headers = NULL;
error1:
	free(image->elf_header);
	image->elf_header = NULL;
	close(fd);
	return status;
}


static status_t
load_elf_segments(int fd, const Elf_Ehdr* eheader,
	const Elf_Phdr* programHeaders, size_t programHeadersSize,
	bool kernel, addr_t* _loadAddress, addr_t* _entryAddress,
	addr_t* _dynamicSection)
{
	elf_region regions[ELF_MAX_REGIONS];
	int regionCount = 0;
	addr_t loadOffset = 0;
	addr_t textSize = 0;
	addr_t dataSize = 0;
	addr_t bssSize = 0;
	size_t imageSize = 0;

	status_t status = parse_program_headers(programHeaders, eheader->e_phnum,
		regions, &regionCount, &loadOffset, _entryAddress, _dynamicSection,
		&textSize, &dataSize, &bssSize, &imageSize);
	if (status != B_OK)
		return status;

	// TODO: This is a hack!
	// We need to find a better way to determine the load address.
	// For now, we just assume that the first PT_LOAD segment is the text segment
	// and its p_vaddr is the base address.
	if (eheader->e_type == ET_DYN) {
		// For shared objects, we need to find a base address.
		// For now, we just use the first available address.
		// TODO: This is not correct!
		loadOffset = vm_allocate_any_address(imageSize, B_RANDOMIZED_ANY_ADDRESS,
			0, 0);
		if (loadOffset == 0)
			return B_NO_MEMORY;
	} else {
		// For executables, the load address is specified in the program headers.
		loadOffset = regions[0].start - programHeaders[0].p_vaddr;
	}

	*_loadAddress = loadOffset;

	return map_elf_segments(fd, programHeaders, eheader->e_phnum, regions,
		regionCount, loadOffset, kernel, "elf_segments", B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, B_ANY_ADDRESS,
		_loadAddress, _entryAddress, _dynamicSection);
}


static status_t
parse_program_headers(const Elf_Phdr* programHeaders, int programHeaderCount,
	elf_region* regions, int* _regionCount, addr_t* _loadOffset,
	addr_t* _entryAddress, addr_t* _dynamicSection, addr_t* _textSize,
	addr_t* _dataSize, addr_t* _bssSize, size_t* _imageSize)
{
	int regionCount = 0;
	addr_t loadOffset = 0;
	addr_t entryAddress = 0;
	addr_t dynamicSection = 0;
	addr_t textSize = 0;
	addr_t dataSize = 0;
	addr_t bssSize = 0;
	size_t imageSize = 0;

	for (int i = 0; i < programHeaderCount; i++) {
		const Elf_Phdr* phdr = &programHeaders[i];

		if (phdr->p_type == PT_LOAD) {
			if (regionCount >= ELF_MAX_REGIONS)
				return B_BAD_DATA; // too many regions

			regions[regionCount].start = phdr->p_vaddr;
			regions[regionCount].size = phdr->p_memsz;
			regions[regionCount].delta = phdr->p_offset - phdr->p_vaddr;
			regions[regionCount].protection = 0;
			if ((phdr->p_flags & PF_R) != 0)
				regions[regionCount].protection |= B_READ_AREA;
			if ((phdr->p_flags & PF_W) != 0)
				regions[regionCount].protection |= B_WRITE_AREA;
			if ((phdr->p_flags & PF_X) != 0)
				regions[regionCount].protection |= B_EXECUTE_AREA;

			if (loadOffset == 0)
				loadOffset = regions[regionCount].start;

			if (phdr->p_vaddr + phdr->p_memsz > imageSize)
				imageSize = phdr->p_vaddr + phdr->p_memsz;

			if ((phdr->p_flags & PF_X) != 0)
				textSize += phdr->p_memsz;
			else if ((phdr->p_flags & PF_W) != 0) {
				dataSize += phdr->p_filesz;
				bssSize += phdr->p_memsz - phdr->p_filesz;
			}

			regionCount++;
		} else if (phdr->p_type == PT_DYNAMIC) {
			dynamicSection = phdr->p_vaddr;
		}
	}

	if (_loadOffset != NULL)
		*_loadOffset = loadOffset;
	if (_entryAddress != NULL)
		*_entryAddress = entryAddress; // This should be set from e_entry
	if (_dynamicSection != NULL)
		*_dynamicSection = dynamicSection;
	if (_textSize != NULL)
		*_textSize = textSize;
	if (_dataSize != NULL)
		*_dataSize = dataSize;
	if (_bssSize != NULL)
		*_bssSize = bssSize;
	if (_imageSize != NULL)
		*_imageSize = imageSize;
	if (_regionCount != NULL)
		*_regionCount = regionCount;

	return B_OK;
}


static status_t
map_elf_segments(int fd, const Elf_Phdr* programHeaders, int programHeaderCount,
	elf_region* regions, int regionCount, addr_t loadOffset,
	bool mapIntoKernelSpace, const char* name, uint32 wiring,
	uint32 protection, uint32 addressSpec, addr_t* _baseAddress,
	addr_t* _entryAddress, addr_t* _dynamicSection)
{
	addr_t baseAddress = 0;
	status_t status = B_OK;

	for (int i = 0; i < regionCount; i++) {
		addr_t start = regions[i].start + loadOffset;
		addr_t size = regions[i].size;
		uint32 regionProtection = regions[i].protection;

		if (mapIntoKernelSpace)
			regionProtection |= B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA;

		// TODO: This is a hack!
		// We need to find a better way to determine the area protection.
		// For now, we just use the protection from the program header.
		if ((regionProtection & B_WRITE_AREA) != 0)
			wiring = B_LAZY_LOCK; // data segments are not fully locked

		regions[i].id = create_area(name, (void**)&start, addressSpec, size,
			wiring, regionProtection);
		if (regions[i].id < 0) {
			status = regions[i].id;
			goto error;
		}

		regions[i].start = start; // update with actual mapped address

		if (baseAddress == 0)
			baseAddress = start;

		// Load the segment from the file.
		const Elf_Phdr* phdr = NULL;
		for (int j = 0; j < programHeaderCount; j++) {
			if (programHeaders[j].p_type == PT_LOAD
				&& programHeaders[j].p_vaddr == regions[i].start - loadOffset) {
				phdr = &programHeaders[j];
				break;
			}
		}

		if (phdr == NULL) {
			// This should not happen!
			status = B_BAD_DATA;
			goto error;
		}

		if (phdr->p_filesz > 0) {
			if (lseek(fd, phdr->p_offset, SEEK_SET) != (off_t)phdr->p_offset) {
				status = B_IO_ERROR;
				goto error;
			}
			status = read_from_file_or_vnode(fd, (void*)start, phdr->p_filesz);
			if (status != B_OK)
				goto error;
		}

		// Zero the BSS part of the segment.
		if (phdr->p_memsz > phdr->p_filesz) {
			memset((void*)(start + phdr->p_filesz), 0,
				phdr->p_memsz - phdr->p_filesz);
		}
	}

	if (_baseAddress != NULL)
		*_baseAddress = baseAddress;
	// _entryAddress and _dynamicSection should be adjusted by loadOffset
	// This is typically done by the caller (e.g. load_elf_image)

	return B_OK;

error:
	for (int i = 0; i < regionCount; i++) {
		if (regions[i].id >= 0)
			delete_area(regions[i].id);
	}
	return status;
}


//	#pragma mark - Kernel specific


status_t
elf_init(kernel_args *args)
{
	status_t status;

	mutex_init(&sLoadedUserImagesLock, "elf user images");

	sUserImageHashTable = new(std::nothrow) ElfImageHashTable;
	if (sUserImageHashTable == NULL)
		return B_NO_MEMORY;
	status = sUserImageHashTable->Init();
	if (status != B_OK) {
		delete sUserImageHashTable;
		sUserImageHashTable = NULL;
		return status;
	}

	// Insert all preloaded images into the list.
	// The kernel is the first one.
	preloaded_elf_image* preloaded = args->preloaded_images;
	if (preloaded == NULL) {
		panic("elf_init: No preloaded images!\n");
		return B_BAD_DATA;
	}

	status = insert_preloaded_image(preloaded, true);
	if (status != B_OK) {
		panic("elf_init: Failed to insert kernel image: %s\n",
			strerror(status));
		return status;
	}

	preloaded = preloaded->next;
	while (preloaded != NULL) {
		status = insert_preloaded_image(preloaded, false);
		if (status != B_OK) {
			dprintf("elf_init: Failed to insert preloaded image %s: %s\n",
				preloaded->name, strerror(status));
			// We can live with that (unless it's the runtime_loader).
		}
		preloaded = preloaded->next;
	}

	return B_OK;
}


/*!	Loads the kernel image from the specified ELF image.
	This function is called by the boot loader.
	\param fd The file descriptor for the kernel image.
	\param args The kernel arguments.
	\return \c B_OK on success, another error code otherwise.
*/
status_t
elf_load_kernel(int fd, kernel_args* args)
{
	sKernelImage = create_image_struct();
	if (sKernelImage == NULL)
		return B_NO_MEMORY;

	sKernelImage->name = strdup("kernel_" ELF_MACHINE_ARCH_ supplément);
	if (sKernelImage->name == NULL) {
		delete_image_struct(sKernelImage);
		sKernelImage = NULL;
		return B_NO_MEMORY;
	}

	sKernelImage->id = 0; // The kernel always has image ID 0.
	sKernelImage->ref_count = 1; // Never unload the kernel.
	sKernelImage->active = true;
	sKernelImage->type = ET_EXEC; // Kernel is an executable.

	addr_t entryAddress;
	status_t status = load_elf_image(fd, sKernelImage, true, &entryAddress);
	if (status != B_OK) {
		delete_image_struct(sKernelImage);
		sKernelImage = NULL;
		return status;
	}

	args->kernel_image = sKernelImage;
	args->kernel_args_size = sizeof(kernel_args);
	args->kernel_image_size = sKernelImage->text_region.size
		+ sKernelImage->data_region.size;
	args->kernel_image_text_base = sKernelImage->text_region.start;
	args->kernel_image_data_base = sKernelImage->data_region.start;
	args->entry_point = entryAddress;

	// The kernel doesn't have a dynamic section in the traditional sense,
	// but we need to parse it to find the symbol table and string table.
	// TODO: This is not correct! The kernel's dynamic section is not used
	// for relocation.
	if (sKernelImage->dynamic_ptr != 0) {
		status = elf_parse_dynamic_section(sKernelImage);
		if (status != B_OK) {
			dprintf("elf_load_kernel: Failed to parse dynamic section: %s\n",
				strerror(status));
			// Not fatal, but symbols might be missing.
		}
	}

	// The kernel is already relocated by the boot loader.
	// We don't need to do it again.

	return B_OK;
}


/*!	Loads a kernel add-on from the specified ELF image.
	\param path The path to the add-on image.
	\return The image ID of the loaded add-on on success, or an error code
		otherwise.
*/
image_id
load_kernel_add_on(const char* path)
{
	int fd = open(path, O_RDONLY);
	if (fd < 0)
		return fd;

	elf_image_info* image = create_image_struct();
	if (image == NULL) {
		close(fd);
		return B_NO_MEMORY;
	}

	image->name = strdup(path);
	if (image->name == NULL) {
		delete_image_struct(image);
		close(fd);
		return B_NO_MEMORY;
	}
	image->path = strdup(path); // Store path for debugging/info

	image->id = next_image_id(true);
	image->type = ET_DYN; // Kernel add-ons are shared objects.
	image->is_driver = true; // Assume it's a driver for now.

	addr_t entryAddress;
	status_t status = load_elf_image(fd, image, true, &entryAddress);
		// fd is closed by load_elf_image
	if (status != B_OK) {
		delete_image_struct(image);
		return status;
	}

	// Parse dynamic section for dependencies and relocations.
	if (image->dynamic_ptr != 0) {
		status = elf_parse_dynamic_section(image);
		if (status != B_OK) {
			dprintf("load_kernel_add_on: Failed to parse dynamic section "
				"for %s: %s\n", path, strerror(status));
			// This might be fatal if relocations are needed.
			// For now, continue and see.
		}
	}

	// Relocate the image.
	status = elf_relocate_image(image);
	if (status != B_OK) {
		dprintf("load_kernel_add_on: Failed to relocate image %s: %s\n",
			path, strerror(status));
		// TODO: Unload the image if relocation fails.
		delete_image_struct(image); // Simplistic cleanup for now
		return status;
	}

	// Add to loaded images list (if we had one for kernel add-ons).
	// For now, just return the ID.
	// TODO: Manage kernel add-on images properly.

	return image->id;
}


//	#pragma mark - Userland specific


/*!	Loads a userland ELF image.
	\param path The path to the image file.
	\param team The team into which to load the image.
	\param flags Image loading flags (see \c image.h).
	\param _entryAddress On success, set to the entry address of the image.
	\return The image ID of the loaded image on success, or an error code
		otherwise.
*/
image_id
elf_load_user_image(const char* path, BKernel::Team* team, uint32 flags,
	addr_t* _entryAddress)
{
	int fd = vfs_open_vnode_from_path(path, O_RDONLY, 0, false, true);
	if (fd < 0)
		return fd;

	elf_image_info* image = create_image_struct();
	if (image == NULL) {
		close(fd);
		return B_NO_MEMORY;
	}

	image->name = strdup(path);
	if (image->name == NULL) {
		delete_image_struct(image);
		close(fd);
		return B_NO_MEMORY;
	}
	image->path = strdup(path);

	image->id = next_image_id(false);
	// Type will be determined by elf_header->e_type

	addr_t entryAddress;
	status_t status = load_elf_image(fd, image, false, &entryAddress);
		// fd is closed by load_elf_image
	if (status != B_OK) {
		delete_image_struct(image);
		return status;
	}

	image->type = image->elf_header->e_type;

	// Parse dynamic section
	if (image->dynamic_ptr != 0) {
		status = elf_parse_dynamic_section(image);
		if (status != B_OK) {
			dprintf("elf_load_user_image: Failed to parse dynamic section for %s: %s\n",
				path, strerror(status));
			// TODO: Proper cleanup
			delete_image_struct(image);
			return status;
		}
	}

	// Relocate image
	status = elf_relocate_image(image);
	if (status != B_OK) {
		dprintf("elf_load_user_image: Failed to relocate image %s: %s\n",
			path, strerror(status));
		// TODO: Proper cleanup
		delete_image_struct(image);
		return status;
	}

	// Add to team's image list and global hash table
	mutex_lock(&sLoadedUserImagesLock);
	image->next = sLoadedUserImages;
	sLoadedUserImages = image;
	sUserImageHashTable->Insert(image);
	mutex_unlock(&sLoadedUserImagesLock);

	team->AddImage(image);
	image->ref_count = 1;
	image->active = true;

	if (_entryAddress != NULL)
		*_entryAddress = entryAddress;

	return image->id;
}


static status_t
elf_parse_dynamic_section(elf_image_info* image)
{
	Elf_Dyn* d = (Elf_Dyn*)(image->dynamic_ptr + image->text_delta);
	int i;
	int needed_offset = 0;

	if (d == NULL) {
		// No dynamic section, nothing to do.
		// This is valid for statically linked executables.
		return B_OK;
	}

	// First pass: Count DT_NEEDED entries to allocate image->needed
	int neededCount = 0;
	for (i = 0; d[i].d_tag != DT_NULL; i++) {
		if (d[i].d_tag == DT_NEEDED)
			neededCount++;
	}

	if (neededCount > 0) {
		image->needed = (char**)calloc(neededCount + 1, sizeof(char*));
		if (image->needed == NULL)
			return B_NO_MEMORY;
	}

	// Second pass: Process all dynamic tags
	for (i = 0; d[i].d_tag != DT_NULL; i++) {
		switch (d[i].d_tag) {
			case DT_NEEDED:
				// Value is an offset into the string table for the library name
				// This should have been handled by runtime_loader for user images,
				// but for kernel add-ons, we might need to handle it here.
				// For now, assume string table is loaded if symbols are.
				if (image->string_table != NULL && d[i].d_un.d_val < image->string_table_size) {
					image->needed[needed_offset++] = strdup(image->string_table + d[i].d_un.d_val);
					if (image->needed[needed_offset-1] == NULL) return B_NO_MEMORY; // strdup failed
				} else {
					dprintf("elf_parse_dynamic_section: DT_NEEDED points outside string table or no string table for image %s\n", image->name);
					// This is problematic.
				}
				break;
			case DT_PLTRELSZ:
				image->pltrel_len = d[i].d_un.d_val;
				break;
			case DT_PLTGOT:
				image->pltgot = (addr_t*)(d[i].d_un.d_ptr + image->text_delta);
				break;
			case DT_HASH:
				// This is the old SysV hash table. We prefer DT_GNU_HASH if available.
				// For now, just store the pointer.
				image->symbol_hash_table = (uint32*)(d[i].d_un.d_ptr + image->text_delta);
				break;
			case DT_STRTAB:
				// This is the dynamic string table, different from section header strtab.
				// If image->string_table is already set from section headers, this might
				// be redundant or a different table. ELF can be complex.
				// For now, assume image->string_table from load_elf_symbol_table is sufficient.
				// image->string_table = (char *)(d[i].d_un.d_ptr + image->text_delta);
				break;
			case DT_SYMTAB:
				// Similar to DT_STRTAB, dynamic symbol table.
				// image->symbol_table = (Elf_Sym *)(d[i].d_un.d_ptr + image->text_delta);
				break;
			case DT_RELA:
				image->rela = (Elf_Rela*)(d[i].d_un.d_ptr + image->text_delta);
				break;
			case DT_RELASZ:
				image->rela_len = d[i].d_un.d_val;
				break;
			case DT_RELAENT:
				if (d[i].d_un.d_val != sizeof(Elf_Rela))
					return B_BAD_DATA; // Mismatched RELA entry size
				break;
			case DT_STRSZ:
				// image->string_table_size = d[i].d_un.d_val; // Size of dynamic string table
				break;
			case DT_SYMENT:
				if (d[i].d_un.d_val != sizeof(Elf_Sym))
					return B_BAD_DATA; // Mismatched symbol entry size
				break;
			case DT_INIT:
				// image->init_routine = d[i].d_un.d_ptr + image->text_delta;
				break;
			case DT_FINI:
				// image->term_routine = d[i].d_un.d_ptr + image->text_delta;
				break;
			case DT_SONAME:
				if (image->string_table != NULL && d[i].d_un.d_val < image->string_table_size) {
					image->soname = strdup(image->string_table + d[i].d_un.d_val);
					if (image->soname == NULL) return B_NO_MEMORY;
				}
				break;
			case DT_REL:
				image->rel = (Elf_Rel*)(d[i].d_un.d_ptr + image->text_delta);
				break;
			case DT_RELSZ:
				image->rel_len = d[i].d_un.d_val;
				break;
			case DT_RELENT:
				if (d[i].d_un.d_val != sizeof(Elf_Rel))
					return B_BAD_DATA; // Mismatched REL entry size
				break;
			case DT_PLTREL:
				image->pltrel_type = d[i].d_un.d_val;
				break;
			case DT_DEBUG:
				// Contains pointer to r_debug structure for GDB.
				// image->debug_info = (struct r_debug*)(d[i].d_un.d_ptr + image->text_delta);
				break;
			case DT_JMPREL:
				// Pointer to PLT relocations (REL or RELA).
				image->jmprel = (void*)(d[i].d_un.d_ptr + image->text_delta);
				break;
			// TODO: Handle other dynamic tags like DT_BIND_NOW, DT_FLAGS, DT_RPATH, DT_RUNPATH, etc.
			// DT_GNU_HASH for faster symbol lookups.
			default:
				continue;
		}
	}
	return B_OK;
}


static status_t
elf_relocate_image(elf_image_info* image)
{
	// Perform relocations. This is a complex part of ELF loading.
	// We need to handle different relocation types (R_X86_64_GLOB_DAT, R_X86_64_JUMP_SLOT, R_X86_64_RELATIVE, etc.)

	// Handle RELA relocations (if any)
	if (image->rela != NULL && image->rela_len > 0) {
		intپردازنده = image->rela_len / sizeof(Elf_Rela);
		for (int i = 0; i <پردازنده; i++) {
			Elf_Rela* rela = &image->rela[i];
			addr_t* resolve_addr = (addr_t*)(rela->r_offset + image->text_delta);
			Elf_Sym* sym = NULL;
			addr_t sym_val = 0;

			if (ELF_R_SYM(rela->r_info) != 0) {
				// Find symbol definition. This is tricky: needs to search self and dependencies.
				// For now, a placeholder.
				// sym = find_symbol(image, ELF_R_SYM(rela->r_info));
				// if (sym) sym_val = sym->st_value + image_of_symbol->text_delta;
				// else { dprintf("Relocation error: symbol not found\n"); return B_MISSING_SYMBOL; }
			}

			switch (ELF_R_TYPE(rela->r_info)) {
				case R_X86_64_NONE:
					break;
				case R_X86_64_64:
					*resolve_addr = sym_val + rela->r_addend;
					break;
				case R_X86_64_PC32:
					*resolve_addr = sym_val + rela->r_addend - (addr_t)resolve_addr;
					break;
				case R_X86_64_GLOB_DAT:
				case R_X86_64_JUMP_SLOT:
					*resolve_addr = sym_val + rela->r_addend; // Or just sym_val for JUMP_SLOT if PLT is involved
					break;
				case R_X86_64_RELATIVE:
					*resolve_addr = image->text_delta + rela->r_addend;
					break;
				// Add more relocation types as needed
				default:
					dprintf("elf_relocate_image: unknown RELA relocation type %lld for image %s\n",
						(long long)ELF_R_TYPE(rela->r_info), image->name);
					return B_NOT_AN_EXECUTABLE;
			}
		}
	}

	// Handle REL relocations (if any) - similar logic to RELA but r_addend is implicit
	if (image->rel != NULL && image->rel_len > 0) {
		intپردازنده = image->rel_len / sizeof(Elf_Rel);
		for (int i = 0; i <پردازنده; i++) {
			Elf_Rel* rel = &image->rel[i];
			addr_t* resolve_addr = (addr_t*)(rel->r_offset + image->text_delta);
			Elf_Sym* sym = NULL;
			addr_t sym_val = 0;
			Elf_Sxword addend = *resolve_addr; // Implicit addend for REL

			if (ELF_R_SYM(rel->r_info) != 0) {
				// sym = find_symbol(image, ELF_R_SYM(rel->r_info));
				// if (sym) sym_val = sym->st_value + image_of_symbol->text_delta;
				// else { dprintf("Relocation error: symbol not found\n"); return B_MISSING_SYMBOL; }
			}

			switch (ELF_R_TYPE(rel->r_info)) {
				case R_X86_64_NONE:
					break;
				case R_X86_64_64: // S + A
					*resolve_addr = sym_val + addend;
					break;
				case R_X86_64_PC32: // S + A - P
					*resolve_addr = sym_val + addend - (addr_t)resolve_addr;
					break;
				case R_X86_64_GLOB_DAT: // S
				case R_X86_64_JUMP_SLOT: // S
					*resolve_addr = sym_val;
					break;
				case R_X86_64_RELATIVE: // B + A
					*resolve_addr = image->text_delta + addend;
					break;
				default:
					dprintf("elf_relocate_image: unknown REL relocation type %lld for image %s\n",
						(long long)ELF_R_TYPE(rel->r_info), image->name);
					return B_NOT_AN_EXECUTABLE;
			}
		}
	}

	// Handle JMPREL (PLT) relocations
	if (image->jmprel != NULL && image->pltrel_len > 0) {
		if (image->pltrel_type == DT_REL) {
			intپردازنده = image->pltrel_len / sizeof(Elf_Rel);
			Elf_Rel* plt_rel = (Elf_Rel*)image->jmprel;
			for (int i = 0; i <پردازنده; i++) {
				// Similar to DT_REL processing, but specifically for JUMP_SLOTs
				// Often, the PLTGOT also needs to be initialized.
			}
		} else if (image->pltrel_type == DT_RELA) {
			intپردازنده = image->pltrel_len / sizeof(Elf_Rela);
			Elf_Rela* plt_rela = (Elf_Rela*)image->jmprel;
			for (int i = 0; i <پردازنده; i++) {
				// Similar to DT_RELA processing for JUMP_SLOTs
			}
		}
	}

	return B_OK;
}

// Placeholder for resolving symbols. A real implementation would search
// the current image and its dependencies (DT_NEEDED).
static Elf_Sym*
find_symbol(elf_image_info* image, const char* name, int32 type)
{
	if (image->symbol_table == NULL || image->string_table == NULL)
		return NULL;

	// Basic linear search for now. A hash table (DT_HASH or DT_GNU_HASH) is much better.
	for (uint32 i = 0; i < image->num_symbols; i++) {
		Elf_Sym* sym = &image->symbol_table[i];
		if (sym->st_name != 0 && sym->st_name < image->string_table_size) {
			const char* symName = image->string_table + sym->st_name;
			if (strcmp(symName, name) == 0) {
				// TODO: Check symbol type if provided (ELF_ST_TYPE(sym->st_info))
				// TODO: Check symbol binding (ELF_ST_BIND(sym->st_info)) - global, weak, local
				// TODO: Handle versioned symbols if DT_VERSYM/DT_VERDEF/DT_VERNEED are present
				if (sym->st_shndx != SHN_UNDEF) { // Symbol is defined in this image
					return sym;
				}
			}
		}
	}

	// TODO: If not found in current image, search dependencies (image->needed)
	// This requires loading those dependencies first.

	return NULL;
}


// #pragma mark - Debugger commands


static int
dump_image_info(int argc, char **argv)
{
	image_id id;
	elf_image_info *image;
	bool found = false;

	if (argc < 2) {
		kprintf("usage: %s <image_id|image_name>\n", argv[0]);
		return 0;
	}

	if (IS_KERNEL_ADDRESS(argv[1])) {
		// interpret as address
		image = (elf_image_info*)strtoul(argv[1], NULL, 0);
		// TODO: validate this pointer!
		id = image->id;
		found = true;
	} else if (is_kernel_image(argv[1])) {
		image = sKernelImage;
		id = image->id;
		found = true;
	} else if (string_to_image_id(argv[1], &id) == B_OK) {
		// image ID
		image = elf_find_image(id, false);
		if (image != NULL)
			found = true;
	} else {
		// image name
		image = elf_find_image_by_name(argv[1], false, false);
		if (image != NULL) {
			id = image->id;
			found = true;
		}
	}

	if (!found) {
		kprintf("invalid image id\n");
		return 0;
	}

	kprintf("IMAGE %" B_PRId32 " (%s):\n", id, image->name);
	kprintf(" id: %" B_PRId32 "\n", image->id);
	kprintf(" type: %s\n",
		image->type == ET_EXEC ? "ET_EXEC" : (image->type == ET_DYN ? "ET_DYN" : "OTHER"));
	kprintf(" name: '%s'\n", image->name);
	kprintf(" path: '%s'\n", image->path ? image->path : "(null)");
	kprintf(" text: %p - %p (size %#" B_PRIxADDR ")\n",
		(void*)image->text_region.start,
		(void*)(image->text_region.start + image->text_region.size),
		image->text_region.size);
	kprintf(" data: %p - %p (size %#" B_PRIxADDR ")\n",
		(void*)image->data_region.start,
		(void*)(image->data_region.start + image->data_region.size),
		image->data_region.size);
	kprintf(" text_delta: %#" B_PRIx32 "\n", image->text_delta);
	kprintf(" dynamic section: %p\n", (void*)image->dynamic_ptr);
	kprintf(" entry: %p\n", (void*)(image->elf_header ? image->elf_header->e_entry + image->text_delta : 0));
	kprintf(" ref_count: %" B_PRId32 "\n", image->ref_count);
	kprintf(" active: %s\n", image->active ? "true" : "false");
	kprintf(" is_driver: %s\n", image->is_driver ? "true" : "false");
	kprintf(" no_init_fini: %s\n", image->no_init_fini ? "true" : "false");

	if (image->needed) {
		kprintf(" needed libraries:\n");
		for (int i = 0; image->needed[i]; i++)
			kprintf("  '%s'\n", image->needed[i]);
	}

	if (image->num_symbols > 0 && image->symbol_table && image->string_table) {
		kprintf(" %" B_PRIu32 " symbols:\n", image->num_symbols);
		// Optionally dump some symbols
		for (uint32 i = 0; i < min_c(image->num_symbols, 10u); i++) {
			Elf_Sym* sym = &image->symbol_table[i];
			if (sym->st_name < image->string_table_size) {
				kprintf("  %s (value: %p, size: %#" B_PRIx64 ", info: %#x, shndx: %#x)\n",
					image->string_table + sym->st_name,
					(void*)(sym->st_value + (sym->st_shndx != SHN_ABS ? image->text_delta : 0)),
					(uint64)sym->st_size, sym->st_info, sym->st_shndx);
			}
		}
		if (image->num_symbols > 10)
			kprintf("  ... (more symbols)\n");
	} else {
		kprintf(" no symbol information.\n");
	}

	return 0;
}


// #pragma mark - Private C++ API


elf_image_info*
elf_find_image(image_id id, bool kernel)
{
	if (kernel) {
		if (id == 0 && sKernelImage != NULL)
			return sKernelImage;
		// TODO: Search kernel add-ons if we manage them in a list.
		return NULL;
	}

	MutexLocker locker(sLoadedUserImagesLock);
	return sUserImageHashTable->Lookup(id);
}


elf_image_info*
elf_find_image_by_name(const char* name, bool kernel, bool searchDependencies)
{
	if (kernel) {
		if (sKernelImage != NULL && strcmp(sKernelImage->name, name) == 0)
			return sKernelImage;
		// TODO: Search kernel add-ons.
		return NULL;
	}

	MutexLocker locker(sLoadedUserImagesLock);
	for (elf_image_info* image = sLoadedUserImages; image != NULL; image = image->next) {
		if (strcmp(image->name, name) == 0)
			return image;
		if (image->path != NULL && strcmp(image->path, name) == 0) // Also check by path
			return image;
	}
	// TODO: Implement searchDependencies if needed.
	return NULL;
}


bool
is_kernel_image(const char* name)
{
	return sKernelImage != NULL && strcmp(sKernelImage->name, name) == 0;
}


// #pragma mark - Kernel private C API (used by other parts of the kernel)


image_id
get_kernel_image_id()
{
	return sKernelImage != NULL ? sKernelImage->id : -1;
}


elf_image_info*
get_kernel_image()
{
	return sKernelImage;
}


status_t
elf_get_image_info(image_id id, image_info* userInfo)
{
	elf_image_info* image = elf_find_image(id, IS_KERNEL_ADDRESS(userInfo));
		// Determine kernel/user based on userInfo pointer
	if (image == NULL)
		return B_BAD_IMAGE_ID;

	userInfo->id = image->id;
	userInfo->type = image->type;
	userInfo->sequence = 0; // Not used by kernel images
	userInfo->init_order = 0; // Not used
	userInfo->init_routine = 0; // TODO: Expose if needed
	userInfo->term_routine = 0; // TODO: Expose if needed
	userInfo->device = -1; // Not applicable
	userInfo->node = -1;   // Not applicable
	strlcpy(userInfo->name, image->name, B_PATH_NAME_LENGTH);
	userInfo->text = (void*)image->text_region.start;
	userInfo->data = (void*)image->data_region.start;
	userInfo->text_size = image->text_region.size;
	userInfo->data_size = image->data_region.size;

	// TODO: If this is for userland, copy out to user space.
	// For now, assume kernel usage.

	return B_OK;
}


status_t
elf_get_next_image_info(team_id teamID, int32* cookie, image_info* userInfo)
{
	// This function is typically for iterating userland images of a team.
	// Kernel images are not usually iterated this way by userland.
	// If teamID is current_team, iterate sLoadedUserImages.
	// The cookie would be an index or pointer into the list.

	// Simplified for now:
	if (teamID != team_get_current_team_id()) // Placeholder for team check
		return B_BAD_TEAM_ID;

	MutexLocker locker(sLoadedUserImagesLock);
	elf_image_info* image = NULL;

	if (*cookie == 0) {
		// Start iteration
		image = sLoadedUserImages;
	} else {
		// Continue iteration - find image matching cookie
		// This assumes cookie is image_id for simplicity here.
		// A real implementation might use a pointer or index.
		elf_image_info* current = sLoadedUserImages;
		while (current != NULL) {
			if (current->id == *cookie) {
				image = current->next; // Get the next one
				break;
			}
			current = current->next;
		}
	}

	if (image == NULL) {
		*cookie = 0; // Reset cookie, end of list
		return B_BAD_INDEX;
	}

	// Found an image, populate userInfo
	status_t status = elf_get_image_info(image->id, userInfo);
	if (status == B_OK) {
		*cookie = image->id; // Update cookie to current image's ID for next call
	} else {
		*cookie = 0; // Error, reset cookie
	}
	return status;
}


status_t
elf_get_image_symbol(image_id id, const char* symbolName, int32 symbolType,
	void** _symbolLocation, size_t* _symbolSize, int32* _symbolType)
{
	elf_image_info* image = elf_find_image(id, IS_KERNEL_ADDRESS(_symbolLocation));
	if (image == NULL)
		return B_BAD_IMAGE_ID;

	Elf_Sym* sym = find_symbol(image, symbolName, symbolType);
	if (sym == NULL)
		return B_MISSING_SYMBOL;

	if (_symbolLocation != NULL) {
		*_symbolLocation = (void*)(sym->st_value
			+ (sym->st_shndx != SHN_ABS ? image->text_delta : 0));
	}
	if (_symbolSize != NULL)
		*_symbolSize = sym->st_size;
	if (_symbolType != NULL)
		*_symbolType = ELF_ST_TYPE(sym->st_info); // Return specific type (func, obj, etc.)

	return B_OK;
}


/*!	Looks up a symbol by address in the specified image.
	\param id The ID of the image to search.
	\param address The address of the symbol.
	\param _symbolName On success, set to the name of the symbol. The caller is
		responsible for freeing this string.
	\param _symbolAddress On success, set to the address of the symbol.
	\param _symbolSize On success, set to the size of the symbol.
	\param _symbolType On success, set to the type of the symbol.
	\return \c B_OK on success, another error code otherwise.
*/
status_t
elf_lookup_symbol(image_id id, addr_t address, char** _symbolName,
	addr_t* _symbolAddress, size_t* _symbolSize, int32* _symbolType)
{
	elf_image_info* image = elf_find_image(id, IS_KERNEL_ADDRESS((void*)address));
	if (image == NULL)
		return B_BAD_IMAGE_ID;

	if (image->symbol_table == NULL || image->string_table == NULL)
		return B_MISSING_SYMBOL; // No symbol info

	Elf_Sym* bestMatch = NULL;
	addr_t bestMatchAddress = 0;

	for (uint32 i = 0; i < image->num_symbols; i++) {
		Elf_Sym* sym = &image->symbol_table[i];
		if (sym->st_shndx == SHN_UNDEF || sym->st_shndx >= SHN_LORESERVE)
			continue; // Skip undefined or special section symbols

		addr_t symbolLoadAddress = sym->st_value
			+ (sym->st_shndx != SHN_ABS ? image->text_delta : 0);

		if (address >= symbolLoadAddress
			&& address < symbolLoadAddress + sym->st_size) {
			// Exact match or address is within this symbol's range
			bestMatch = sym;
			bestMatchAddress = symbolLoadAddress;
			break; // Found a good match
		} else if (symbolLoadAddress <= address) {
			// Symbol starts before or at the address.
			// If it's the closest one so far, keep it.
			if (bestMatch == NULL || symbolLoadAddress > bestMatchAddress) {
				bestMatch = sym;
				bestMatchAddress = symbolLoadAddress;
			}
		}
	}

	if (bestMatch == NULL)
		return B_MISSING_SYMBOL;

	if (_symbolName != NULL) {
		if (bestMatch->st_name < image->string_table_size) {
			*_symbolName = strdup(image->string_table + bestMatch->st_name);
			if (*_symbolName == NULL) return B_NO_MEMORY;
		} else {
			*_symbolName = strdup("<bad_st_name_offset>");
		}
	}
	if (_symbolAddress != NULL)
		*_symbolAddress = bestMatchAddress;
	if (_symbolSize != NULL)
		*_symbolSize = bestMatch->st_size;
	if (_symbolType != NULL)
		*_symbolType = ELF_ST_TYPE(bestMatch->st_info);

	return B_OK;
}


status_t
elf_unmap_image(image_id id)
{
	// This function would be responsible for unmapping the memory regions
	// associated with an image and cleaning up its resources.
	// For kernel add-ons, this is complex due to potential in-use code/data.
	// For user images, it's part of team cleanup.

	elf_image_info* image = elf_find_image(id, false); // Assume user image for now
	if (image == NULL)
		return B_BAD_IMAGE_ID;

	// TODO: Proper ref counting. If ref_count > 0, don't unmap yet.
	// This is a simplified version.

	if (image->text_region.id >= 0)
		delete_area(image->text_region.id);
	if (image->data_region.id >= 0 && image->data_region.id != image->text_region.id)
		delete_area(image->data_region.id);

	// Remove from global list
	mutex_lock(&sLoadedUserImagesLock);
	if (sLoadedUserImages == image) {
		sLoadedUserImages = image->next;
	} else {
		elf_image_info* current = sLoadedUserImages;
		while (current != NULL && current->next != image) {
			current = current->next;
		}
		if (current != NULL)
			current->next = image->next;
	}
	sUserImageHashTable->Remove(image);
	mutex_unlock(&sLoadedUserImagesLock);

	// TODO: Inform team that image is gone.
	// BKernel::Team* team = team_get_team_struct_locked(image->team_id);
	// if (team) team->RemoveImage(image);

	delete_image_struct(image);
	return B_OK;
}
