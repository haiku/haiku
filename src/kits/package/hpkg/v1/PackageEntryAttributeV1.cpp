/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/v1/PackageEntryAttribute.h>


namespace BPackageKit {

namespace BHPKG {

namespace V1 {


BPackageEntryAttribute::BPackageEntryAttribute(const char* name)
	:
	fName(name),
	fType(0)
{
}


}	// namespace V1

}	// namespace BHPKG

}	// namespace BPackageKit
