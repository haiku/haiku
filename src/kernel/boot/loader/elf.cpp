/* 
** Copyright 2002-2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "elf.h"

#include <boot/platform.h>
#include <boot/stage2.h>
#include <elf32.h>
#include <kernel.h>

#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define TRACE_ELF 0
#if TRACE_ELF
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


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


status_t
elf_load_image(int fd, preloaded_image *image)
{
	size_t totalSize;
	status_t status;

	TRACE(("elf_load_image(fd = %d, image = %p)\n", fd, image));

	struct Elf32_Ehdr &elfHeader = image->elf_header;

	ssize_t length = read_pos(fd, 0, &elfHeader, sizeof(Elf32_Ehdr));
	if (length < (ssize_t)sizeof(Elf32_Ehdr))
		return B_BAD_TYPE;

	status = verify_elf_header(elfHeader);
	if (status < B_OK)
		return status;

	ssize_t size = elfHeader.e_phnum * elfHeader.e_phentsize;
	Elf32_Phdr *programHeaders = (struct Elf32_Phdr *)malloc(size);
	if (programHeaders == NULL) {
		dprintf("error allocating space for program headers\n");
		return B_NO_MEMORY;
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

	for (int i = 0; i < elfHeader.e_phnum; i++) {
		switch (programHeaders[i].p_type) {
			case PT_LOAD:
				break;
			case PT_DYNAMIC:
				image->dynamic_section = programHeaders[i].p_vaddr;
				continue;
			default:
				dprintf("unhandled pheader type 0x%lx\n", programHeaders[i].p_type);
				continue;
		}

		elf_region *region;
		if (programHeaders[i].IsReadWrite()) {
			if (image->data_region.size != 0) {
				dprintf("elf: rw already handled!\n");
				continue;
			}
			region = &image->data_region;
		} else if (programHeaders[i].IsExecutable()) {
			if (image->text_region.size != 0) {
				dprintf("elf: ro already handled!\n");
				continue;
			}
			region = &image->text_region;
		} else
			continue;

		region->start = ROUNDOWN(programHeaders[i].p_vaddr, B_PAGE_SIZE);
		region->size = ROUNDUP(programHeaders[i].p_memsz + (programHeaders[i].p_vaddr % B_PAGE_SIZE), PAGE_SIZE);
		region->delta = -region->start;
	}

	// if image->text_region.start == NULL (image is relocatable), 
	// platform_allocate_region() automatically allocates an address
	totalSize = image->text_region.size + image->data_region.size;
	if (platform_allocate_region((void **)&image->text_region.start, totalSize,
			B_READ_AREA | B_WRITE_AREA) < B_OK) {
		status = B_NO_MEMORY;
		goto error1;
	}

	// initialize the region pointers to the allocated region
	// (text region comes first)
	image->data_region.start = image->text_region.start + image->text_region.size;

	image->data_region.delta += image->data_region.start;
	image->text_region.delta += image->text_region.start;

	// load program data

	for (int i = 0; i < elfHeader.e_phnum; i++) {
		if (programHeaders[i].p_type != PT_LOAD)
			continue;

		elf_region *region;
		if (programHeaders[i].IsReadWrite())
			region = &image->data_region;
		else if (programHeaders[i].IsExecutable())
			region = &image->text_region;
		else
			continue;

		length = read_pos(fd, programHeaders[i].p_offset,
			(void *)(region->start + (programHeaders[i].p_vaddr % B_PAGE_SIZE)),
			programHeaders[i].p_filesz);
		if (length < (ssize_t)programHeaders[i].p_filesz) {
			status = B_BAD_DATA;
			dprintf("error reading in seg %d\n", i);
			goto error2;
		}
	}

	// modify the dynamic section by the delta of the regions
	image->dynamic_section += image->text_region.delta;

	free(programHeaders);

	return B_OK;

error2:
	if (image->text_region.start != NULL)
		platform_free_region((void *)image->text_region.start, totalSize);
error1:
	free(programHeaders);

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

	image = (preloaded_image *)malloc(sizeof(preloaded_image));
	if (image == NULL) {
		close(fd);
		return B_NO_MEMORY;
	}

	status_t status = elf_load_image(fd, image);
	if (status == B_OK) {
		image->name = strdup(path);

		// insert to kernel args
		image->next = gKernelArgs.preloaded_images;
		gKernelArgs.preloaded_images = image;
	} else
		free(image);

	close(fd);
	return B_OK;
}

