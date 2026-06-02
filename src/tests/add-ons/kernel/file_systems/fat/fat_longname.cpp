/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

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

#include "dosfs.h"
#include "encodings.h"
#include "support.h"


struct iconv_functions* msdosfs_iconv;


status_t
populate_winentries(const char* utf8String, std::vector<winentry>* entries, msdosfsmount* fatVolume)
{
	const u_char* unsignedString = reinterpret_cast<const u_char*>(utf8String);
	size_t utf8Len = strlen(utf8String);

	// need short name to calculate checksum
	u_char nshort[11];
	int conversion = unix2dosfn(unsignedString, nshort, utf8Len, 0, fatVolume);
	if (conversion == 0) {
		fprintf(stderr, "unix2dosfn returned 0.\n");
		return B_ERROR;
	}

	int entryCount = 1;
	bool unfinished = true;
	uint16 carryoverUnit = 0;
	size_t bytesParsed = 0;
	int checkSum = winChksum(nshort);
	int entriesNeeded = winSlotCnt(unsignedString, utf8Len, fatVolume);
	if (entriesNeeded == 0) {
		fprintf(stderr, "File name %s is too long for the FAT file system.\n", utf8String);
		return B_NAME_TOO_LONG;
	}

	// populate winentries
	winentry entry;
	for (uint32 i = entriesNeeded - 1; i >= 0 && unfinished == true; --i) {
		unfinished = utf8_to_winentry(unsignedString, utf8Len, &entry, entryCount++, checkSum,
			fatVolume, &carryoverUnit, &bytesParsed);
		// arrange entries in reverse order, as on disk
		try {
			entries->insert(entries->begin(), entry);
		} catch (std::exception& e) {
			fprintf(stderr, "vector::insert failure (%s).\n", e.what());
			return B_ERROR;
		}
	}

	int32 entriesCreated = entryCount - 1;
	if (entriesCreated != entriesNeeded) {
		fprintf(stderr, "Wrote %d entries but expected to need %d.\n", entriesCreated,
			entriesNeeded);
		return B_ERROR;
	}

	return B_OK;
}


status_t
populate_dirent_name(std::vector<winentry>* entries, dirent* vfsDirent)
{
	mbnambuf longName;
		// filename after extraction and conversion from winentries
	mbnambuf_init(&longName);

	uint16 carryoverUnit = 0;
	int checkSum = -1;
	size_t index;
	for (index = 0; index < entries->size() && (index == 0 || checkSum != -1); ++index)
		checkSum = winentry_to_utf8(&longName, &(*entries)[index], checkSum, &carryoverUnit);

	if (checkSum == -1) {
		fprintf(stderr, "winentry_to_utf8 returned -1 for entry at index %" B_PRIuSIZE ".\n",
			index - 1);
		return B_ERROR;
	}

	mbnambuf_flush(&longName, vfsDirent);

	return B_OK;
}


/*!	Mimics the conversion of a UTF-8 string to the UTF-16 arrays that store long file names
	in the FAT filesystem.
*/
int
main(int argc, char** argv)
{
	if (argc == 1 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
		printf("Tests the translation of a UTF-8 filename to the UTF-16 name stored in a series "
			   "of FAT long file name entries.\n"
			   "Usage:\n\t%s [-r] [name]\n\t%s [-r] [byte1] [byte2]...\n"
			   "If only one input argument is provided, it is treated as a UTF-8 string. "
			   "If more than one input argument is provided, each argument is treated "
			   "as one hex byte of a UTF-8 encoding. The -r option sets roundtrip mode, "
			   "in which input is converted to long file name form, and then converted "
			   "back to a UTF-8 string that should match the input.",
			argv[0], argv[0]);
		return 0;
	}

	struct msdosfsmount fatVolume;
	fatVolume.pm_flags = 0;

	std::vector<winentry> entries;

	char* utf8Str;
	bool roundtrip = false;
	int32 argcAdjusted = argc;
	uint32 inputArgIndex = 1;
	if (strcmp(argv[1], "-r") == 0) {
		roundtrip = true;
		argcAdjusted -= 1;
		inputArgIndex = 2;
	}

	if (argcAdjusted == 2) {
		utf8Str = argv[inputArgIndex];
	} else if (argcAdjusted - 1 <= B_FILE_NAME_LENGTH) {
		// build a string from the individual input bytes
		utf8Str = reinterpret_cast<char*>(alloca(argcAdjusted));
		for (int32 i = inputArgIndex; i < argc; ++i) {
			uint64 byte = strtoul(argv[i], NULL, 16);
			if (byte > UCHAR_MAX) {
				fprintf(stderr, "Invalid byte %" B_PRIu64 ".\n", byte);
				return 1;
			}
			utf8Str[i - inputArgIndex] = byte;
		}
		utf8Str[argcAdjusted - 1] = 0;
	} else {
		fprintf(stderr, "Too many input bytes.\n");
		return 1;
	}

	status_t status = populate_winentries(utf8Str, &entries, &fatVolume);
	if (status != B_OK)
		return 1;

	dirent* vfsDirent
		= reinterpret_cast<dirent*>(alloca(sizeof(struct dirent) + B_FILE_NAME_LENGTH + 1));
	memset(vfsDirent, 0, sizeof(struct dirent) + B_FILE_NAME_LENGTH + 1);

	if (roundtrip)
		status = populate_dirent_name(&entries, vfsDirent);

	if (!roundtrip || status != B_OK) {
		for (size_t i = 0; i < entries.size(); ++i) {
			uint32 index = entries[i].weCnt & ~WIN_LAST;
			printf("winentry %u\n", index);
			printf("\t\twePart1:  ");
			for (int j = 0; j < WEPART1_BYTES; ++j)
				printf("%02hX ", entries[i].wePart1[j]);
			printf("\n\t\twePart2:  ");
			for (int j = 0; j < WEPART2_BYTES; ++j)
				printf("%02hX ", entries[i].wePart2[j]);
			printf("\n\t\twePart3:  ");
			for (int j = 0; j < WEPART3_BYTES; ++j)
				printf("%02hX ", entries[i].wePart3[j]);
			printf("\n");
		}
	} else {
		printf("%s", vfsDirent->d_name);
	}

	return status == B_OK ? 0 : 1;
}
