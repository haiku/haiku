/*
 * Copyright 2017-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "TarArchiveService.h"

#include <AutoDeleter.h>
#include <Directory.h>
#include <File.h>
#include <StringList.h>

#include "DataIOUtils.h"
#include "Logger.h"
#include "StorageUtils.h"


#define OFFSET_FILENAME 0
#define OFFSET_LENGTH 124
#define OFFSET_CHECKSUM 148
#define OFFSET_FILETYPE 156

#define LENGTH_FILENAME 100
#define LENGTH_LENGTH 12
#define LENGTH_CHECKSUM 8
#define LENGTH_BLOCK 512


/*!	This method will parse the header that is located at the current position of
	the supplied tarIo.  Upon success, the tarIo will point to the start of the
	data for the parsed entry.
*/

/*static*/	status_t
TarArchiveService::GetEntry(BPositionIO& tarIo, TarArchiveHeader& header)
{
	uint8 buffer[LENGTH_BLOCK];
	status_t result = tarIo.ReadExactly(buffer, LENGTH_BLOCK);
	if (result == B_OK)
		result = _ReadHeader(buffer, header);
	return result;
}


/*!	For each entry in the tar file, this method will call the listener.  If the
	listener does not return B_OK then the process will stop.  This method is
	useful for obtaining a catalog of items in the tar file for example.
*/

/*status*/	status_t
TarArchiveService::ForEachEntry(BPositionIO& tarIo, TarEntryListener* listener)
{
	uint8 buffer[LENGTH_BLOCK];
	uint8 zero_buffer[LENGTH_BLOCK];
	status_t result = B_OK;
	uint32_t countItemsRead = 0;
	off_t offset = 0;

	memset(zero_buffer, 0, sizeof zero_buffer);
	tarIo.Seek(offset, SEEK_SET);

	while (result == B_OK
			&& B_OK == (result = tarIo.ReadExactly(
				buffer, LENGTH_BLOCK))) {

		if (memcmp(zero_buffer, buffer, sizeof zero_buffer) == 0) {
			HDDEBUG("detected end of tar-ball after %" B_PRIu32 " items",
				countItemsRead);
			return B_OK; // end of tar-ball.
		}

		TarArchiveHeader header;
		result = _ReadHeader(buffer, header);

		HDTRACE("did read tar entry header for [%s]",
			header.FileName().String());

		if (result == B_OK) {
			countItemsRead++;

			// call out to the listener to read the data from the entry
			// and/or just process the header information.

			if (listener != NULL) {
				BDataIO *entryData = new ConstraintedDataIO(&tarIo,
					header.Length());
				ObjectDeleter<BDataIO> entryDataDeleter(entryData);
				result = listener->Handle(header, offset, entryData);
			}
		}

		offset += LENGTH_BLOCK;
		offset += _BytesRoundedToBlocks(header.Length());

		if (result == B_OK)
			tarIo.Seek(offset, SEEK_SET);
	}

	if (result == B_OK || result == B_CANCELED)
		HDINFO("did list %" B_PRIu32 " tar items", countItemsRead);
	else
		HDERROR("error occurred listing tar items; %s", strerror(result));

	return result;
}


tar_file_type
TarArchiveService::_ReadHeaderFileType(unsigned char data) {
	switch (data) {
		case 0:
		case '0':
			return TAR_FILE_TYPE_NORMAL;
		default:
			return TAR_FILE_TYPE_OTHER;
	}
}


/*static*/ int32
TarArchiveService::_ReadHeaderStringLength(const uint8* data,
	size_t maxStringLength)
{
	int32 actualLength = 0;
	while (actualLength < (int32) maxStringLength && data[actualLength] != 0)
		actualLength++;
	return actualLength;
}


void
TarArchiveService::_ReadHeaderString(const uint8 *data, size_t maxStringLength,
	BString& result)
{
	result.SetTo((const char *) data,
		_ReadHeaderStringLength(data, maxStringLength));
}


/*!	This function will return true if the character supplied is a valid
	character in an number expressed in octal.
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


/*static*/ uint32
TarArchiveService::_ReadHeaderNumeric(const uint8 *data, size_t dataLength)
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


/*static*/ uint32
TarArchiveService::_CalculateBlockChecksum(const uint8* block)
{
	uint32 result = 0;

	for (uint32 i = 0; i < LENGTH_BLOCK; i++) {
		if (i >= OFFSET_CHECKSUM && i < OFFSET_CHECKSUM + LENGTH_CHECKSUM)
			result += 32;
		else
			result += (uint32) block[i];
	}

	return result;
}


/*static*/ status_t
TarArchiveService::_ReadHeader(const uint8* block, TarArchiveHeader& header)
{
	uint32 actualChecksum = _CalculateBlockChecksum(block);
	uint32 expectedChecksum = _ReadHeaderNumeric(&block[OFFSET_CHECKSUM],
		LENGTH_CHECKSUM);

	if(actualChecksum != expectedChecksum) {
		HDERROR("tar archive header has bad checksum;"
			"expected %" B_PRIu32 " actual %" B_PRIu32,
			expectedChecksum, actualChecksum);
		return B_BAD_DATA;
	}

	BString fileName;
	_ReadHeaderString(&block[OFFSET_FILENAME], LENGTH_FILENAME, fileName);

	header.SetFileName(fileName);
	header.SetLength(
		_ReadHeaderNumeric(&block[OFFSET_LENGTH], LENGTH_LENGTH));
	header.SetFileType(
		_ReadHeaderFileType(block[OFFSET_FILETYPE]));

	return B_OK;
}


/*static*/ off_t
TarArchiveService::_BytesRoundedToBlocks(off_t value)
{
	if (0 != value % LENGTH_BLOCK)
		return ((value / LENGTH_BLOCK) + 1) * LENGTH_BLOCK;
	return (value / LENGTH_BLOCK) * LENGTH_BLOCK;
}
