/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <File.h>

#include <AutoDeleter.h>
#include <SHA256.h>

#include <package/ChecksumAccessors.h>


namespace BPackageKit {

namespace BPrivate {


#define NIBBLE_AS_HEX(nibble) \
	(nibble >= 10 ? 'a' + nibble - 10 : '0' + nibble)


ChecksumAccessor::~ChecksumAccessor()
{
}


ChecksumFileChecksumAccessor::ChecksumFileChecksumAccessor(
	const BEntry& checksumFileEntry)
	:
	fChecksumFileEntry(checksumFileEntry)
{
}


status_t
ChecksumFileChecksumAccessor::GetChecksum(BString& checksum) const
{
	BFile checksumFile(&fChecksumFileEntry, B_READ_ONLY);
	status_t result = checksumFile.InitCheck();
	if (result != B_OK)
		return result;

	const int kSHA256ChecksumHexDumpSize = 64;
	char* buffer = checksum.LockBuffer(kSHA256ChecksumHexDumpSize);
	if (buffer == NULL)
		return B_NO_MEMORY;

	ssize_t bytesRead = checksumFile.Read(buffer, kSHA256ChecksumHexDumpSize);
	buffer[kSHA256ChecksumHexDumpSize] = '\0';
	checksum.UnlockBuffer(kSHA256ChecksumHexDumpSize);
	if (bytesRead < 0)
		return bytesRead;
	if (bytesRead != kSHA256ChecksumHexDumpSize)
		return B_IO_ERROR;

	return B_OK;
}


GeneralFileChecksumAccessor::GeneralFileChecksumAccessor(
	const BEntry& fileEntry, bool skipMissingFile)
	:
	fFileEntry(fileEntry),
	fSkipMissingFile(skipMissingFile)
{
}


status_t
GeneralFileChecksumAccessor::GetChecksum(BString& checksum) const
{
	SHA256 sha;

	checksum.Truncate(0);

	{
		BFile file(&fFileEntry, B_READ_ONLY);
		status_t result = file.InitCheck();
		if (result != B_OK) {
			if (result == B_ENTRY_NOT_FOUND && fSkipMissingFile)
				return B_OK;
			return result;
		}

		off_t fileSize;
		if ((result = file.GetSize(&fileSize)) != B_OK)
			return result;

		const int kBlockSize = 64 * 1024;
		void* buffer = malloc(kBlockSize);
		if (buffer == NULL)
			return B_NO_MEMORY;
		MemoryDeleter memoryDeleter(buffer);

		off_t handledSize = 0;
		while (handledSize < fileSize) {
			ssize_t bytesRead = file.Read(buffer, kBlockSize);
			if (bytesRead < 0)
				return bytesRead;

			sha.Update(buffer, bytesRead);

			handledSize += bytesRead;
		}
	}

	const int kSHA256ChecksumSize = sha.DigestLength();
	char* buffer = checksum.LockBuffer(2 * kSHA256ChecksumSize);
	if (buffer == NULL)
		return B_NO_MEMORY;
	const uint8* digest = sha.Digest();
	for (int i = 0; i < kSHA256ChecksumSize; ++i) {
		uint8 highNibble = (digest[i] & 0xF0) >> 4;
		buffer[i * 2] = NIBBLE_AS_HEX(highNibble);
		uint8 lowNibble = digest[i] & 0x0F;
		buffer[1 + i * 2] = NIBBLE_AS_HEX(lowNibble);
	}
	buffer[2 * kSHA256ChecksumSize] = '\0';
	checksum.UnlockBuffer(2 * kSHA256ChecksumSize);

	return B_OK;
}


}	// namespace BPrivate

}	// namespace BPackageKit
