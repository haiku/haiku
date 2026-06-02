/*
 * Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <util/convertutf.h>

#define _KERNEL
extern "C"
{
#include "sys/namei.h"
#include "sys/param.h"

#include "fs/msdosfs/bootsect.h"
#include "fs/msdosfs/bpb.h"
#include "fs/msdosfs/direntry.h"
#include "fs/msdosfs/fat.h"
#include "fs/msdosfs/msdosfsmount.h"
}

#include "encodings.h"


struct iconv_functions* msdosfs_iconv;


/*!	Returns true if a generation beyond 1 would ever be needed because a shortname collision with a
	different file name is possible. It's useful to check before calling unix2dosfn because
	that function will reject requests for unneeded tails and return a non-specific error code
	that leaves it unclear whether the call failed for some other reason.
*/
bool
requires_tail(const char* name, msdosfsmount* fatVolume)
{
	// driver policy is to always add a tail when encoding in SJIS
	if ((fatVolume->pm_flags & MSDOSFS_SJIS) != 0)
		return true;
	if ((fatVolume->pm_flags & MSDOSFS_CP850) == 0
		&& utf8_contains_japanese(reinterpret_cast<const u_char*>(name), strlen(name))) {
		return true;
	}

	// find the last period, ignoring trailing blanks and periods as unix2dosfn does
	char nameCopy[B_FILE_NAME_LENGTH + 1];
	strlcpy(nameCopy, name, B_FILE_NAME_LENGTH + 1);
	while ((nameCopy[strlen(nameCopy) - 1] == '.' || nameCopy[strlen(nameCopy) - 1] == ' ')
		&& strlen(nameCopy) > 0) {
		nameCopy[strlen(nameCopy) - 1] = '\0';
	}
	const char* period = strrchr(nameCopy, '.');

	// check for characters that will be converted to blanks or underscores, again ignoring
	// trailing blanks and periods
	for (size_t i = 0; i < strlen(nameCopy); ++i) {
		// unix2dos[] is not designed for UTF-8 but this check works because all elements with
		// a value of 1 are ASCII characters
		size_t arraySubscript = static_cast<size_t>(nameCopy[i]);
		if (unix2dos[arraySubscript] == 1 || unix2dos[arraySubscript] == 2)
			return true;
	}

	bool extension;
	if (period == nameCopy || period == NULL) {
		period = name + strlen(name);
			// not really a period, but it still marks one past the end of the base
		extension = false;
	}

	if (period - name > 8)
		return true;

	if (extension) {
		if (name + strlen(name) - period > 4)
			return true;
	}

	return false;
}


status_t
encode_short_name(struct msdosfsmount* fatVolume, u_char* src, size_t srcLen, u_char* dst,
	size_t dstLen, uint32 generation)
{

	int result = unix2dosfn(src, dst, srcLen, generation, fatVolume);
	if (result == 0) {
		printf("unix2dosfn failed.\n");
		return B_ERROR;
	}

	if (result == 1)
		printf("Note: unix2dosfn returned 1 (no long filename entry necessary).\n");

	printf("\tshort name as chars [");
	for (uint32 i = 0; i < 11; ++i) {
		unsigned char c = dst[i];
		if (i < 10)
			printf("%c ", c);
		else
			printf("%c", c);
	}
	printf("]\n");
	printf("\tshort name as hex bytes [");
	for (uint32 i = 0; i < 11; ++i) {
		uint8 c = dst[i];
		if (i < 10)
			printf("%02hX ", c);
		else
			printf("%02hX", c);
	}
	printf("]\n");

	return B_OK;
}


status_t
decode_short_name(struct msdosfsmount* fatVolume, u_char* src, u_char* dst)
{
	int result = dos2unixfn(src, dst, 0, fatVolume);

	printf("\tUTF-8 string [%s] of length %d\n", dst, result);

	return B_OK;
}


int
main(int argc, char** argv)
{
	if (argc != 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
		printf("Tests the translation of a UTF-8 file name to the OEM short name stored in a FAT "
			   "directory entry, using 3 methods corresponding to the kernel_code_page options.\n"
			   "Usage: %s [name]\n",
			argv[0]);
		return 0;
	}

	struct msdosfsmount fatVolume;
	fatVolume.pm_flags = 0;
	char* utf8Name = argv[1];

	// We test the creation of the baseline name (no tail or ~1) and ~1234.
	// The latter will be skipped if the name does not require truncation or substitution.
	for (int generation = 1; generation < 1235; generation += 1233) {
		u_char shortName[11];
		u_char recreatedName[B_FILE_NAME_LENGTH + 1];

		if (generation == 1) {
			printf("**Baseline file name (no collision with existing shortnames)**\n");
		} else if (!requires_tail(utf8Name, &fatVolume)) {
			printf("No numeric tail beyond ~1 needed for this file name.\n");
			return B_OK;
		} else {
			printf("**Generation %d**\n", generation);
		}

		printf("Encoding in CP 850\n");
		fatVolume.pm_flags |= MSDOSFS_CP850;
		status_t result = encode_short_name(&fatVolume, reinterpret_cast<u_char*>(utf8Name),
			strlen(utf8Name), shortName, 11, generation);
		if (result != B_OK) {
			fprintf(stderr, "Failed to encode\n");
			return 1;
		}
		// convert the created short name back to UTF-8, as dosfs_readdir would do if it had
		// no long file name entries to work with
		printf("Decoding from CP 850\n");
		decode_short_name(&fatVolume, shortName, recreatedName);

		fatVolume.pm_flags &= ~MSDOSFS_CP850;
		fatVolume.pm_flags |= MSDOSFS_SJIS;
		printf("Encoding in SJIS\n");
		result = encode_short_name(&fatVolume, reinterpret_cast<u_char*>(utf8Name),
			strlen(utf8Name), shortName, 11, generation);
		if (result != B_OK) {
			fprintf(stderr, "Failed to encode\n");
			return 1;
		}
		printf("Decoding from SJIS\n");
		decode_short_name(&fatVolume, shortName, recreatedName);

		fatVolume.pm_flags &= ~MSDOSFS_SJIS;
		printf("Encoding in CP 850 or SJIS, depending on detection of Japanese characters\n");
		result = encode_short_name(&fatVolume, reinterpret_cast<u_char*>(utf8Name),
			strlen(utf8Name), shortName, 11, generation);
		if (result != B_OK) {
			fprintf(stderr, "Failed to encode\n");
			return 1;
		}
		printf("Decoding from CP 850\n");
		decode_short_name(&fatVolume, shortName, recreatedName);
	}

	return 0;
}
