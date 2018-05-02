/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ELF_LOAD_IMAGE_H
#define ELF_LOAD_IMAGE_H

#include "runtime_loader_private.h"


status_t	parse_elf_header(elf_ehdr* eheader, int32* _pheaderSize,
				int32* _sheaderSize);
#if defined(_COMPAT_MODE)
	#if defined(__x86_64__)
status_t	parse_elf32_header(Elf32_Ehdr* eheader, int32* _pheaderSize,
	int32* _sheaderSize);
	#else
status_t	parse_elf64_header(Elf64_Ehdr* eheader, int32* _pheaderSize,
	int32* _sheaderSize);
	#endif
#endif
status_t	load_image(char const* name, image_type type, const char* rpath,
				const char* requestingObjectPath, image_t** _image);


#endif	// ELF_LOAD_IMAGE_H
