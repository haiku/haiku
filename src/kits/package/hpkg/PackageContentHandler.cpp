/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/PackageContentHandler.h>


namespace BPackageKit {

namespace BHPKG {


// #pragma mark - BLowLevelPackageContentHandler


static const char* kAttributeNames[B_HPKG_ATTRIBUTE_ID_ENUM_COUNT + 1] = {
	#define B_DEFINE_HPKG_ATTRIBUTE(id, type, name, constant)	\
		name,
	#include <package/hpkg/PackageAttributes.h>
	#undef B_DEFINE_HPKG_ATTRIBUTE
	//
	NULL	// B_HPKG_ATTRIBUTE_ID_ENUM_COUNT
};


BLowLevelPackageContentHandler::~BLowLevelPackageContentHandler()
{
}


/*static*/ const char*
BLowLevelPackageContentHandler::AttributeNameForID(uint8 id)
{
	if (id >= B_HPKG_ATTRIBUTE_ID_ENUM_COUNT)
		return NULL;

	return kAttributeNames[id];
}


// #pragma mark - BPackageContentHandler


BPackageContentHandler::~BPackageContentHandler()
{
}


}	// namespace BHPKG

}	// namespace BPackageKit
