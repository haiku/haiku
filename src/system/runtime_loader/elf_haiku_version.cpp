/*
 * Copyright 2008-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "elf_haiku_version.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <image_defs.h>
#include <syscalls.h>

#include "elf_symbol_lookup.h"


// interim Haiku API versions
#define HAIKU_VERSION_PRE_GLUE_CODE		0x00000010


static bool
analyze_object_gcc_version(int fd, image_t* image, elf_ehdr& eheader,
	int32 sheaderSize, char* buffer, size_t bufferSize)
{
	if (sheaderSize > (int)bufferSize) {
		FATAL("%s: Cannot handle section headers bigger than %lu bytes\n",
			image->path, bufferSize);
		return false;
	}

	// read section headers
	ssize_t length = _kern_read(fd, eheader.e_shoff, buffer, sheaderSize);
	if (length != sheaderSize) {
		FATAL("%s: Could not read section headers: %s\n", image->path,
			strerror(length));
		return false;
	}

	// load the string section
	elf_shdr* sectionHeader
		= (elf_shdr*)(buffer + eheader.e_shstrndx * eheader.e_shentsize);

	if (sheaderSize + sectionHeader->sh_size > bufferSize) {
		FATAL("%s: Buffer not big enough for section string section\n",
			image->path);
		return false;
	}

	char* sectionStrings = buffer + bufferSize - sectionHeader->sh_size;
	length = _kern_read(fd, sectionHeader->sh_offset, sectionStrings,
		sectionHeader->sh_size);
	if (length != (int)sectionHeader->sh_size) {
		FATAL("%s: Could not read section string section: %s\n", image->path,
			strerror(length));
		return false;
	}

	// find the .comment section
	off_t commentOffset = 0;
	size_t commentSize = 0;
	for (uint32 i = 0; i < eheader.e_shnum; i++) {
		sectionHeader = (elf_shdr*)(buffer + i * eheader.e_shentsize);
		const char* sectionName = sectionStrings + sectionHeader->sh_name;
		if (sectionHeader->sh_name != 0
			&& strcmp(sectionName, ".comment") == 0) {
			commentOffset = sectionHeader->sh_offset;
			commentSize = sectionHeader->sh_size;
			break;
		}
	}

	if (commentSize == 0) {
		FATAL("%s: Could not find .comment section\n", image->path);
		return false;
	}

	// read a part of the comment section
	if (commentSize > 512)
		commentSize = 512;

	length = _kern_read(fd, commentOffset, buffer, commentSize);
	if (length != (int)commentSize) {
		FATAL("%s: Could not read .comment section: %s\n", image->path,
			strerror(length));
		return false;
	}

	// the common prefix of the strings in the .comment section
	static const char* kGCCVersionPrefix = "GCC: (GNU) ";
	size_t gccVersionPrefixLen = strlen(kGCCVersionPrefix);

	size_t index = 0;
	int gccMajor = 0;
	int gccMiddle = 0;
	int gccMinor = 0;
	bool isHaiku = true;

	// Read up to 10 comments. The first three or four are usually from the
	// glue code.
	for (int i = 0; i < 10; i++) {
		// skip '\0'
		while (index < commentSize && buffer[index] == '\0')
			index++;
		char* stringStart = buffer + index;

		// find string end
		while (index < commentSize && buffer[index] != '\0')
			index++;

		// ignore the entry at the end of the buffer
		if (index == commentSize)
			break;

		// We have to analyze string like these:
		// GCC: (GNU) 2.9-beos-991026
		// GCC: (GNU) 2.95.3-haiku-080322
		// GCC: (GNU) 4.1.2

		// skip the common prefix
		if (strncmp(stringStart, kGCCVersionPrefix, gccVersionPrefixLen) != 0)
			continue;

		// the rest is the GCC version
		char* gccVersion = stringStart + gccVersionPrefixLen;
		char* gccPlatform = strchr(gccVersion, '-');
		char* patchLevel = NULL;
		if (gccPlatform != NULL) {
			*gccPlatform = '\0';
			gccPlatform++;
			patchLevel = strchr(gccPlatform, '-');
			if (patchLevel != NULL) {
				*patchLevel = '\0';
				patchLevel++;
			}
		}

		// split the gcc version into major, middle, and minor
		int version[3] = { 0, 0, 0 };

		for (int k = 0; gccVersion != NULL && k < 3; k++) {
			char* dot = strchr(gccVersion, '.');
			if (dot) {
				*dot = '\0';
				dot++;
			}
			version[k] = atoi(gccVersion);
			gccVersion = dot;
		}

		// got any version?
		if (version[0] == 0)
			continue;

		// Select the gcc version with the smallest major, but the greatest
		// middle/minor. This should usually ignore the glue code version as
		// well as cases where e.g. in a gcc 2 program a single C file has
		// been compiled with gcc 4.
		if (gccMajor == 0 || gccMajor > version[0]
		 	|| (gccMajor == version[0]
				&& (gccMiddle < version[1]
					|| (gccMiddle == version[1] && gccMinor < version[2])))) {
			gccMajor = version[0];
			gccMiddle = version[1];
			gccMinor = version[2];
		}

		if (gccMajor == 2 && gccPlatform != NULL
			&& strcmp(gccPlatform, "haiku")) {
			isHaiku = false;
		}
	}

	if (gccMajor == 0)
		return false;

	if (gccMajor == 2) {
		if (gccMiddle < 95)
			image->abi = B_HAIKU_ABI_GCC_2_ANCIENT;
		else if (isHaiku)
			image->abi = B_HAIKU_ABI_GCC_2_HAIKU;
		else
			image->abi = B_HAIKU_ABI_GCC_2_BEOS;
	} else
		image->abi = gccMajor << 16;

	return true;
}


void
analyze_image_haiku_version_and_abi(int fd, image_t* image, elf_ehdr& eheader,
	int32 sheaderSize, char* buffer, size_t bufferSize)
{
	// Haiku API version
	elf_sym* symbol = find_symbol(image,
		SymbolLookupInfo(B_SHARED_OBJECT_HAIKU_VERSION_VARIABLE_NAME,
			B_SYMBOL_TYPE_DATA));
	if (symbol != NULL && symbol->st_shndx != SHN_UNDEF
		&& symbol->st_value > 0
		&& symbol->Type() == STT_OBJECT
		&& symbol->st_size >= sizeof(uint32)) {
		image->api_version
			= *(uint32*)(symbol->st_value + image->regions[0].delta);
	} else
		image->api_version = 0;

	// Haiku ABI
	symbol = find_symbol(image,
		SymbolLookupInfo(B_SHARED_OBJECT_HAIKU_ABI_VARIABLE_NAME,
			B_SYMBOL_TYPE_DATA));
	if (symbol != NULL && symbol->st_shndx != SHN_UNDEF
		&& symbol->st_value > 0
		&& symbol->Type() == STT_OBJECT
		&& symbol->st_size >= sizeof(uint32)) {
		image->abi = *(uint32*)(symbol->st_value + image->regions[0].delta);
	} else
		image->abi = 0;

	if (image->abi == 0) {
		// No ABI version in the shared object, i.e. it has been built before
		// that was introduced in Haiku. We have to try and analyze the gcc
		// version.
		if (!analyze_object_gcc_version(fd, image, eheader, sheaderSize,
				buffer, bufferSize)) {
			FATAL("%s: Failed to get gcc version.\n", image->path);
				// not really fatal, actually

			// assume ancient BeOS
			image->abi = B_HAIKU_ABI_GCC_2_ANCIENT;
		}
	}

	// guess the API version, if we couldn't figure it out yet
	if (image->api_version == 0) {
		image->api_version = image->abi > B_HAIKU_ABI_GCC_2_BEOS
			? HAIKU_VERSION_PRE_GLUE_CODE : B_HAIKU_VERSION_BEOS;
	}
}
