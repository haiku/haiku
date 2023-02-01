/*
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Trung Nguyen, trungnt282910@gmail.com
 */


#include <kernel/image.h>
#include <link.h>
#include <stddef.h>


#if B_HAIKU_32_BIT
	typedef Elf32_Ehdr Elf_Ehdr;
#elif B_HAIKU_64_BIT
	typedef Elf64_Ehdr Elf_Ehdr;
#endif


// While get_next_image_info does not return the address of
// the ELF header, it does return the first page of the image's
// text segment (defined by runtime_loader as the first loaded page
// with read-only protection). For most images produced by
// normal compilers, including Haiku ELF files, the file header
// is always loaded at the beginning of the text segment.
//
// We can therefore take advantage of this fact and populate
// struct dl_phdr_info.
int
dl_iterate_phdr(int (*callback)(struct dl_phdr_info* info, size_t size, void* data), void* data)
{
	image_info info;
	int32 cookie = 0;
	int status;

	struct dl_phdr_info phdr_info;

	while (get_next_image_info(0, &cookie, &info) == B_OK) {
		const Elf_Ehdr* header = (const Elf_Ehdr*)info.text;

		// Check for the special commpage info (which is not an ELF image),
		// and also guard against any maliciously crafted file which
		// does not load its header in memory.
		if (!IS_ELF(*header))
			continue;

		phdr_info.dlpi_addr = (Elf_Addr)info.text;
		phdr_info.dlpi_name = info.name;
		phdr_info.dlpi_phnum = header->e_phnum;
		phdr_info.dlpi_phdr = (const Elf_Phdr*)((const char*)info.text + header->e_phoff);

		status = callback(&phdr_info, sizeof(phdr_info), data);

		if (status != 0)
			return status;
	}

	return 0;
}
