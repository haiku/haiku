/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ELF_HAIKU_VERSION_H
#define ELF_HAIKU_VERSION_H

#include "runtime_loader_private.h"


void	analyze_image_haiku_version_and_abi(int fd, image_t* image,
			elf_ehdr& eheader, int32 sheaderSize, char* buffer,
			size_t bufferSize);


#endif	// ELF_HAIKU_VERSION_H
