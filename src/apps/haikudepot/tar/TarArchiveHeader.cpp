/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "TarArchiveHeader.h"

#include <stdio.h>


#define OFFSET_FILENAME 0
#define OFFSET_LENGTH 124
#define OFFSET_CHECKSUM 148
#define OFFSET_FILETYPE 156

#define LENGTH_FILENAME 100
#define LENGTH_LENGTH 12
#define LENGTH_CHECKSUM 8
#define LENGTH_BLOCK 512


TarArchiveHeader::TarArchiveHeader(const BString& fileName, uint64 length,
		tar_file_type fileType)
		:
		fFileName(fileName),
		fLength(length),
		fFileType(fileType)
{
}

tar_file_type
TarArchiveHeader::_ReadFileType(unsigned char data) {
	switch (data) {
		case 0:
		case '0':
			return TAR_FILE_TYPE_NORMAL;

		default:
			return TAR_FILE_TYPE_OTHER;
	}
}


const BString
TarArchiveHeader::_ReadString(const unsigned char *data, size_t dataLength)
{
	uint32 actualLength = 0;

	while (actualLength < dataLength && 0 != data[actualLength])
		actualLength++;

	return BString((const char *) data, actualLength);
}

/*
 * This is an octal value represented as an ASCII string.
 */

static bool tar_is_octal_digit(unsigned char c)
{
	switch (c) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
			return true;

		default:
			return false;
	}
}

uint32
TarArchiveHeader::_ReadNumeric(const unsigned char *data, size_t dataLength)
{
	uint32 actualLength = 0;

	while (actualLength < dataLength && tar_is_octal_digit(data[actualLength]))
		actualLength++;

	uint32 factor = 1;
	uint32 result = 0;

	for (uint32 i = 0; i < actualLength; i++) {
		result += (data[actualLength - (1 + i)] - '0') * factor;
		factor *= 8;
	}

	return result;
}

uint32
TarArchiveHeader::_CalculateChecksum(const unsigned char* block)
{
	uint32 result = 0;

	for (uint32 i = 0; i < LENGTH_BLOCK; i++) {
		if (i >= OFFSET_CHECKSUM && i < OFFSET_CHECKSUM + LENGTH_CHECKSUM)
			result += 32;
		else
			result += block[i];
	}

	return result;
}

TarArchiveHeader*
TarArchiveHeader::CreateFromBlock(const unsigned char* block)
{
	uint32 actualChecksum = _CalculateChecksum(block);
	uint32 expectedChecksum = _ReadNumeric(&block[OFFSET_CHECKSUM],
		LENGTH_CHECKSUM);

	if(actualChecksum != expectedChecksum) {
		fprintf(stderr, "tar archive header has bad checksum;"
			"expected %" B_PRIu32 " actual %" B_PRIu32 "\n",
			expectedChecksum, actualChecksum);
	} else {
		return new TarArchiveHeader(
			_ReadString(&block[OFFSET_FILENAME], LENGTH_FILENAME),
			_ReadNumeric(&block[OFFSET_LENGTH], LENGTH_LENGTH),
			_ReadFileType(block[OFFSET_FILETYPE]));
	}

	return NULL;
}

const BString&
TarArchiveHeader::GetFileName() const
{
	return fFileName;
}

size_t
TarArchiveHeader::GetLength() const
{
	return fLength;
}


tar_file_type
TarArchiveHeader::GetFileType() const
{
	return fFileType;
}
