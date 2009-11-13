/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageData.h"

#include <string.h>


PackageData::PackageData()
	:
	fCompressedSize(0),
	fUncompressedSize(0),
	fCompression(B_HPKG_COMPRESSION_NONE),
	fEncodedInline(true)
{
}


void
PackageData::SetData(uint64 size, uint64 offset)
{
	fUncompressedSize = fCompressedSize = size;
	fOffset = offset;
	fEncodedInline = false;
}


void
PackageData::SetData(uint8 size, const void* data)
{
	fUncompressedSize = fCompressedSize = size;
	if (size > 0)
		memcpy(fInlineData, data, size);
	fEncodedInline = true;
}
