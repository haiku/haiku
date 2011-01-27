/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/PackageEntryAttribute.h>


namespace BPackageKit {

namespace BHaikuPackage {

namespace BPrivate {


PackageEntryAttribute::PackageEntryAttribute(const char* name)
	:
	fName(name),
	fType(0)
{
}


}	// namespace BPrivate

}	// namespace BHaikuPackage

}	// namespace BPackageKit
