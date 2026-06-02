/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <vector>
#include <wchar.h>

#include <Path.h>
#include <fs_info.h>
#include <syscalls.h>

#include <AutoDeleter.h>
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

#include "dosfs.h"
#include "encodings.h"


struct iconv_functions* msdosfs_iconv;


struct PathMap {
	BPath	oldPath;
	BPath	newPath;
};


class Recoder {
public:
			status_t			Initialize(int nameCount, char** names);
			status_t			Recode();

			size_t				PathCount() const
									{ return fPathMaps.size(); }

private:
			status_t			CheckDiskName(const BPath path, bool& namesMatch);
			status_t			CreateTempLeaf(const char* newLeaf, char* tempLeaf,
									size_t tempSize);

private:
			std::vector<PathMap>	fPathMaps;
};


status_t
Recoder::Initialize(int nameCount, char** names)
{
	char cwd[B_PATH_NAME_LENGTH + 1];
	status_t status = _kern_getcwd(cwd, B_PATH_NAME_LENGTH + 1);
	if (status != B_OK) {
		fprintf(stderr, "Can't get current working directory (%s).\n", strerror(status));
		return B_ERROR;
	}

	for (int32 nameIndex = 0; nameIndex < nameCount; ++nameIndex) {
		struct stat nameStat;
		status = stat(names[nameIndex], &nameStat);
		if (status != 0) {
			fprintf(stderr, "Can't stat %s.\n", names[nameIndex]);
			return B_ERROR;
		}

		fs_info info;
		status = _kern_read_fs_info(nameStat.st_dev, &info);
		if (strcmp(info.fsh_name, "fat") != 0) {
			fprintf(stderr, "%s is not on a FAT volume.\n", names[nameIndex]);
			return B_ERROR;
		}

		BPath oldPath;
		status = oldPath.SetTo(cwd, names[nameIndex]);
		if (status != B_OK) {
			fprintf(stderr, "Failed to initialize BPath with %s (%s).\n", names[nameIndex],
				strerror(status));
			return B_ERROR;
		}

		BPath newPath;
		status = oldPath.GetParent(&newPath);
		if (status != B_OK) {
			fprintf(stderr, "Failed to get parent of %s.\n", names[nameIndex]);
			return B_ERROR;
		}

		const char* oldLeaf = oldPath.Leaf();
		char newLeaf[B_FILE_NAME_LENGTH + 1];
		strlcpy(newLeaf, oldLeaf, B_FILE_NAME_LENGTH + 1);

		status = fix_miscoded_longname(newLeaf, strlen(newLeaf), B_FILE_NAME_LENGTH + 1);
		if (status != B_OK) {
			if (status != B_CANCELED) {
				fprintf(stderr, "Could not fix %s.\n", oldLeaf);
				return B_ERROR;
			} else {
				// B_CANCELED indicates the filename presented by the driver is not miscoded;
				// either it was auto-corrected in dosfs_readdir or is coded correctly on disk
				bool diskNameMiscoded;
				status = CheckDiskName(oldPath, diskNameMiscoded);
				if (status != B_OK) {
					fprintf(stderr, "Can't compare %s to disk name.\n", newLeaf);
					return B_ERROR;
				}

				if (!diskNameMiscoded) {
					printf("%s is already coded correctly, no changes needed.\n", oldLeaf);
					continue;
				}
			}
		}

		status = newPath.Append(newLeaf, false);
		if (status != B_OK) {
			fprintf(stderr, "Failed to append new name %s to path %s.\n", newLeaf, newPath.Path());
			return B_ERROR;
		}

		PathMap map = {oldPath, newPath};

		try {
			fPathMaps.push_back(map);
		} catch (std::exception& e) {
			fprintf(stderr, "vector::push_back failure (%s).\n", e.what());
			return B_ERROR;
		}
	}

	return B_OK;
}


status_t
Recoder::Recode()
{
	printf("The following files will be renamed:\n");
	for (uint32 i = 0; i < fPathMaps.size(); ++i)
		printf("%s --> %s\n\n", fPathMaps[i].oldPath.Leaf(), fPathMaps[i].newPath.Leaf());
	printf("Proceed? [yes/no] (no) ");
	char confirmed[4] = "no";
	fgets(confirmed, sizeof(confirmed), stdin);
	if (strcmp(confirmed, "yes") != 0) {
		printf("Exiting with no filename changes.\n");
		return 0;
	}

	for (size_t i = 0; i < fPathMaps.size(); ++i) {
		BPath& oldPath = fPathMaps[i].oldPath;
		BPath& newPath = fPathMaps[i].newPath;
		// in case the driver has auto-corrected a miscoded name, the UTF-8 of the old and new
		// names will be identical, which may cause the rename call to return early; do a 2-step
		// rename to avoid this

		// create a temporary name with -TMP appended
		char tempLeaf[B_FILE_NAME_LENGTH + 1];
		status_t status = CreateTempLeaf(newPath.Leaf(), tempLeaf, B_FILE_NAME_LENGTH + 1);
		if (status != B_OK) {
			fprintf(stderr, "Failed to create temp filename for %s (%s).\n", newPath.Leaf(),
				strerror(status));
			return B_ERROR;
		}

		// create the temp BPath
		BPath tempPath;
		status = newPath.GetParent(&tempPath);
		if (status != B_OK) {
			fprintf(stderr, "Failed to get parent of %s for temp name.\n", newPath.Path());
			return B_ERROR;
		}
		status = tempPath.Append(tempLeaf, false);
		if (status != B_OK) {
			fprintf(stderr, "Failed to append temp leaf %s (%s).\n", tempLeaf, strerror(status));
			return B_ERROR;
		}

		status = _kern_rename(-1, oldPath.Path(), -1, tempPath.Path());
		if (status != B_OK) {
			fprintf(stderr, "Step 1, renaming %s to %s, failed (%s).\n", oldPath.Path(),
				tempPath.Path(), strerror(status));
			return B_ERROR;
		}

		status = _kern_rename(-1, tempPath.Path(), -1, newPath.Path());
		if (status == B_OK) {
			printf("Renamed %s to %s\n\n", oldPath.Leaf(), newPath.Leaf());
		} else {
			fprintf(stderr, "Step 2, renaming %s to %s, failed (%s)\n", tempPath.Path(),
				newPath.Path(), strerror(status));
			return B_ERROR;
		}
	}

	return B_OK;
}


/*!	Check to see whether an apparently valid file name is really encoded correctly.
	@param path Absolute path for the file of interest.
	@post If B_OK was returned, \a miscodedOnDisk is set to true if \a path does not literally
	represent the UTF-16 on disk (indicating it was autocorrected in dosfs_readdir).
*/
status_t
Recoder::CheckDiskName(const BPath path, bool& miscodedOnDisk)
{
	FileDescriptorCloser fdc(open(path.Path(), O_RDONLY));
	if (!fdc.IsSet()) {
		fprintf(stderr, "Can't open %s (%s).\n", path.Path(), strerror(fdc.Get()));
		return B_ERROR;
	}

	const char* fileNameUtf8 = path.Leaf();
	uint16 diskName[WIN_MAXLEN + 1];
	strlcpy(reinterpret_cast<char*>(diskName), fileNameUtf8, WIN_MAXLEN + 1);
	status_t status = ioctl(fdc.Get(), FAT_IOCTL_GET_UTF16_LONGNAME, &diskName, sizeof(diskName));
	if (status != B_OK) {
		fprintf(stderr, "ioctl failure (%s).\n", strerror(status));
		return B_ERROR;
	}

	// files that have no long name can be ignored
	if (diskName[0] == 0) {
		miscodedOnDisk = false;
		return B_OK;
	}

	uint16 fileNameUtf16[WIN_MAXLEN + 1];
	size_t convertedLength = strlen(fileNameUtf8);
	ssize_t conversionUnits = UTF8_TO_UTF16HE(fileNameUtf8, &convertedLength, fileNameUtf16,
		WIN_MAXLEN + 1, NULL, true);
	if (conversionUnits < 0) {
		fprintf(stderr, "Name conversion failure (%s).\n", strerror(conversionUnits));
		return B_ERROR;
	}
	if (convertedLength < strlen(fileNameUtf8)) {
		fprintf(stderr, "Name conversion %s ran out of buffer, converted %" B_PRIuSIZE " bytes.\n",
			fileNameUtf8, convertedLength);
		return B_ERROR;
	}

	// if the name was auto-corrected by dosfs_readdir, we will proceed because the encoding
	// on disk is wrong; if the name was not auto-corrected we will skip it
	size_t diskNameUnits = 0;
	while (diskName[diskNameUnits] != 0)
		++diskNameUnits;
	if (possibly_miscoded_longname(reinterpret_cast<uint8*>(diskName), diskNameUnits) == true
		&& memcmp(diskName, fileNameUtf16, conversionUnits * 2) != 0) {
		miscodedOnDisk = true;
	} else {
		miscodedOnDisk = false;
	}

	if (miscodedOnDisk) {
		// Since the UTF-8 translation of a miscoded name may exceed B_FILE_NAME_LENGTH, conversion
		// to UTF-8 has to be done in 2 steps to avoid the conversion function's check for
		// overlong names.
		char diskNameUtf8[MISCODED_UTF8_SIZE];

		size_t convertUnitCount1 = diskNameUnits / 2;
		ssize_t utf8Len
			= UTF16HE_TO_UTF8(diskName, convertUnitCount1, diskNameUtf8, sizeof(diskNameUtf8));

		size_t convertUnitCount2 = diskNameUnits - convertUnitCount1;
		if (utf8Len > 0) {
			utf8Len = UTF16HE_TO_UTF8(diskName + convertUnitCount1, convertUnitCount2,
				diskNameUtf8 + utf8Len, sizeof(diskNameUtf8) - utf8Len);
		}

		if (utf8Len > 0) {
			printf("Note: name displayed in Haiku (%s) differs from literal name (%s). "
				   "Proceed with renaming to correct the literal name.\n\n",
				fileNameUtf8, diskNameUtf8);
		} else {
			fprintf(stderr, "Unable to generate UTF-8 disk name (%s).\n", strerror(utf8Len));
		}
	}

	return status;
}


status_t
Recoder::CreateTempLeaf(const char* newLeaf, char* tempLeaf, size_t tempSize)
{
	size_t leafLength = strlcpy(tempLeaf, newLeaf, tempSize);
	if (leafLength >= tempSize)
		return B_ERROR;

	// if the file name is too long to append to, the temp name will overwrite the last
	// few bytes in the name; these bytes will be restored in step 2
	size_t freeSpace = B_FILE_NAME_LENGTH - 1 - strlen(tempLeaf);
	bool alternateSuffix = false;
	if (freeSpace < 4) {
		ASSERT(strlen(tempLeaf) >= 4);
		size_t truncateIndex = strlen(tempLeaf) - 4 + freeSpace;

		// don't truncate in the middle of a character
		while (!BEGINS_UTF8CHAR(tempLeaf[truncateIndex]) && truncateIndex > 0)
			--truncateIndex;

		if (strcmp(tempLeaf + truncateIndex, "-TMP") == 0)
			alternateSuffix = true;
		tempLeaf[truncateIndex] = '\0';
	}

	// append the suffix
	size_t tempLength;
	const char* suffix;
	if (alternateSuffix == false)
		suffix = "-TMP";
	else
		suffix = "^TMP";
	tempLength = strlcat(tempLeaf, suffix, tempSize);
	if (tempLength >= tempSize)
		return B_ERROR;

	return B_OK;
}


/*!	Recode a filename on a FAT volume in correct UTF-16. Repairs names created by an earlier
	version of the Haiku FAT driver that do not display correctly due to a past bug.
*/
int
main(int argc, char** argv)
{
	printf("Use this tool to repair files created in Haiku on FAT partitions, that either:\n"
		   "\tdisplay an incorrect filename in other operating systems, or\n"
		   "\tdisplay an incorrect filename in both Haiku and other operating systems.\n\n"
		   "Do not use it on other files. Doing so may corrupt the file name.\n\n");

	if (argc < 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
		printf("usage: %s <file or directory name>\n", argv[0]);
		return 0;
	}

	Recoder recoder;
	status_t status = recoder.Initialize(argc - 1, argv + 1);
	if (status != B_OK)
		return 1;

	if (recoder.PathCount() == 0) {
		printf("Exiting with no filename changes.\n");
		return 0;
	}

	status = recoder.Recode();

	return (status == B_OK) ? 0 : 1;
}
