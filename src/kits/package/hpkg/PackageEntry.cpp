/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/PackageEntry.h>


namespace BPackageKit {

namespace BHPKG {


BPackageEntry::BPackageEntry(BPackageEntry* parent, const char* name)
	:
	fParent(parent),
	fName(name),
	fUserToken(NULL),
	fMode(S_IFREG | S_IRUSR | S_IRGRP | S_IROTH),
	fSymlinkPath(NULL)
{
	fAccessTime.tv_sec = 0;
	fAccessTime.tv_nsec = 0;
	fModifiedTime.tv_sec = 0;
	fModifiedTime.tv_nsec = 0;
	fCreationTime.tv_sec = 0;
	fCreationTime.tv_nsec = 0;
}


}	// namespace BHPKG

}	// namespace BPackageKit
