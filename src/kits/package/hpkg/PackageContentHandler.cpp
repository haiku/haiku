/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/PackageContentHandler.h>


namespace BPackageKit {

namespace BHPKG {


// #pragma mark - BLowLevelPackageContentHandler


static const char* kAttributeNames[B_HPKG_ATTRIBUTE_ID_ENUM_COUNT + 1] = {
	"dir:entry",
	"file:type",
	"file:permissions",
	"file:user",
	"file:group",
	"file:atime",
	"file:mtime",
	"file:crtime",
	"file:atime:nanos",
	"file:mtime:nanos",
	"file:crtime:nanos",
	"file:attribute",
	"file:attribute:type",
	"data",
	"data:compression",
	"data:size",
	"data:chunk_size",
	"symlink:path",
	"package:name",
	"package:summary",
	"package:description",
	"package:vendor",
	"package:packager",
	"package:flags",
	"package:architecture",
	"package:version.major",
	"package:version.minor",
	"package:version.micro",
	"package:version.release",
	"package:copyright",
	"package:license",
	"package:provides",
	"package:provides.type",
	"package:requires",
	"package:supplements",
	"package:conflicts",
	"package:freshens",
	"package:replaces",
	"package:resolvable.operator",
	"package:checksum",
	"package:version.prerelease",
	"package:provides.compatible",
	"package:url",
	"package:source-url",
	"package:install-path",
	NULL
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
