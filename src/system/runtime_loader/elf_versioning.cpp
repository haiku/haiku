/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "elf_versioning.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "images.h"


static status_t
assert_defined_image_version(image_t* dependentImage, image_t* image,
	const elf_version_info& neededVersion, bool weak)
{
	// If the image doesn't have version definitions, we print a warning and
	// succeed. Weird, but that's how glibc does it. Not unlikely we'll fail
	// later when resolving versioned symbols.
	if (image->version_definitions == NULL) {
		FATAL("%s: No version information available (required by %s)\n",
			image->path, dependentImage->path);
		return B_OK;
	}

	// iterate through the defined versions to find the given one
	elf_verdef* definition = image->version_definitions;
	for (uint32 i = 0; i < image->num_version_definitions; i++) {
		uint32 versionIndex = VER_NDX(definition->vd_ndx);
		elf_version_info& info = image->versions[versionIndex];

		if (neededVersion.hash == info.hash
			&& strcmp(neededVersion.name, info.name) == 0) {
			return B_OK;
		}

		definition = (elf_verdef*)((uint8*)definition + definition->vd_next);
	}

	// version not found -- fail, if not weak
	if (!weak) {
		FATAL("%s: version \"%s\" not found (required by %s)\n", image->path,
			neededVersion.name, dependentImage->name);
		return B_MISSING_SYMBOL;
	}

	return B_OK;
}


// #pragma mark -


status_t
init_image_version_infos(image_t* image)
{
	// First find out how many version infos we need -- i.e. get the greatest
	// version index from the defined and needed versions (they use the same
	// index namespace).
	uint32 maxIndex = 0;

	if (image->version_definitions != NULL) {
		elf_verdef* definition = image->version_definitions;
		for (uint32 i = 0; i < image->num_version_definitions; i++) {
			if (definition->vd_version != 1) {
				FATAL("%s: Unsupported version definition revision: %u\n",
					image->path, definition->vd_version);
				return B_BAD_VALUE;
			}

			uint32 versionIndex = VER_NDX(definition->vd_ndx);
			if (versionIndex > maxIndex)
				maxIndex = versionIndex;

			definition = (elf_verdef*)
				((uint8*)definition + definition->vd_next);
		}
	}

	if (image->needed_versions != NULL) {
		elf_verneed* needed = image->needed_versions;
		for (uint32 i = 0; i < image->num_needed_versions; i++) {
			if (needed->vn_version != 1) {
				FATAL("%s: Unsupported version needed revision: %u\n",
					image->path, needed->vn_version);
				return B_BAD_VALUE;
			}

			elf_vernaux* vernaux
				= (elf_vernaux*)((uint8*)needed + needed->vn_aux);
			for (uint32 k = 0; k < needed->vn_cnt; k++) {
				uint32 versionIndex = VER_NDX(vernaux->vna_other);
				if (versionIndex > maxIndex)
					maxIndex = versionIndex;

				vernaux = (elf_vernaux*)((uint8*)vernaux + vernaux->vna_next);
			}

			needed = (elf_verneed*)((uint8*)needed + needed->vn_next);
		}
	}

	if (maxIndex == 0)
		return B_OK;

	// allocate the version infos
	size_t size = sizeof(elf_version_info) * (maxIndex + 1);
	image->versions
		= (elf_version_info*)malloc(size);
	if (image->versions == NULL) {
		FATAL("Memory shortage in init_image_version_infos()");
		return B_NO_MEMORY;
	}
	memset(image->versions, 0, size);
	image->num_versions = maxIndex + 1;

	// init the version infos

	// version definitions
	if (image->version_definitions != NULL) {
		elf_verdef* definition = image->version_definitions;
		for (uint32 i = 0; i < image->num_version_definitions; i++) {
			if (definition->vd_cnt > 0
				&& (definition->vd_flags & VER_FLG_BASE) == 0) {
				elf_verdaux* verdaux
					= (elf_verdaux*)((uint8*)definition + definition->vd_aux);

				uint32 versionIndex = VER_NDX(definition->vd_ndx);
				elf_version_info& info = image->versions[versionIndex];
				info.hash = definition->vd_hash;
				info.name = STRING(image, verdaux->vda_name);
				info.file_name = NULL;
			}

			definition = (elf_verdef*)
				((uint8*)definition + definition->vd_next);
		}
	}

	// needed versions
	if (image->needed_versions != NULL) {
		elf_verneed* needed = image->needed_versions;
		for (uint32 i = 0; i < image->num_needed_versions; i++) {
			const char* fileName = STRING(image, needed->vn_file);

			elf_vernaux* vernaux
				= (elf_vernaux*)((uint8*)needed + needed->vn_aux);
			for (uint32 k = 0; k < needed->vn_cnt; k++) {
				uint32 versionIndex = VER_NDX(vernaux->vna_other);
				elf_version_info& info = image->versions[versionIndex];
				info.hash = vernaux->vna_hash;
				info.name = STRING(image, vernaux->vna_name);
				info.file_name = fileName;

				vernaux = (elf_vernaux*)((uint8*)vernaux + vernaux->vna_next);
			}

			needed = (elf_verneed*)((uint8*)needed + needed->vn_next);
		}
	}

	return B_OK;
}


status_t
check_needed_image_versions(image_t* image)
{
	if (image->needed_versions == NULL)
		return B_OK;

	elf_verneed* needed = image->needed_versions;
	for (uint32 i = 0; i < image->num_needed_versions; i++) {
		const char* fileName = STRING(image, needed->vn_file);
		image_t* dependency = find_loaded_image_by_name(fileName,
			ALL_IMAGE_TYPES);
		if (dependency == NULL) {
			// This can't really happen, unless the object file is broken, since
			// the file should also appear in DT_NEEDED.
			FATAL("%s: Version dependency \"%s\" not found", image->path,
				fileName);
			return B_FILE_NOT_FOUND;
		}

		elf_vernaux* vernaux
			= (elf_vernaux*)((uint8*)needed + needed->vn_aux);
		for (uint32 k = 0; k < needed->vn_cnt; k++) {
			uint32 versionIndex = VER_NDX(vernaux->vna_other);
			elf_version_info& info = image->versions[versionIndex];

			status_t error = assert_defined_image_version(image, dependency,
				info, (vernaux->vna_flags & VER_FLG_WEAK) != 0);
			if (error != B_OK)
				return error;

			vernaux = (elf_vernaux*)((uint8*)vernaux + vernaux->vna_next);
		}

		needed = (elf_verneed*)((uint8*)needed + needed->vn_next);
	}

	return B_OK;
}
