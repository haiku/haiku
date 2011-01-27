/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <package/Attributes.h>


namespace BPackageKit {


// attributes used in package and as BFS-attribute
const char* kPackageNameAttribute		= "PKG:name";
const char* kPackagePlatformAttribute	= "PKG:platform";
const char* kPackageVendorAttribute		= "PKG:vendor";
const char* kPackageVersionAttribute	= "PKG:version";

// attributes kept local to packages
const char* kPackageCopyrightsAttribute	= "PKG:copyrights";
const char* kPackageLicensesAttribute	= "PKG:licenses";
const char* kPackagePackagerAttribute	= "PKG:packager";
const char* kPackageProvidesAttribute	= "PKG:provides";
const char* kPackageRequiresAttribute	= "PKG:requires";


}	// namespace BPackageKit
