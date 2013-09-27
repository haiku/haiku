/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/PackageData.h>

#include <string.h>

#include <package/hpkg/HPKGDefsPrivate.h>


namespace BPackageKit {

namespace BHPKG {


using namespace BPrivate;


BPackageData::BPackageData()
	:
	fSize(0),
	fEncodedInline(true)
{
}


void
BPackageData::SetData(uint64 size, uint64 offset)
{
	fSize = size;
	fOffset = offset;
	fEncodedInline = false;
}


void
BPackageData::SetData(uint8 size, const void* data)
{
	fSize = size;
	if (size > 0)
		memcpy(fInlineData, data, size);
	fEncodedInline = true;
}


}	// namespace BHPKG

}	// namespace BPackageKit
