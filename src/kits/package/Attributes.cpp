/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <package/Attributes.h>


namespace Haiku {

namespace Package {


// attributes used in package and as file attribute, too
const char* kNameAttribute			= "PKG:name";
const char* kPlatformAttribute		= "PKG:platform";
const char* kVendorAttribute		= "PKG:vendor";
const char* kVersionAttribute		= "PKG:version";

// attributes kept local to packages
const char* kCopyrightAttribute		= "PKG:copyright";
const char* kLicenseAttribute		= "PKG:license";
const char* kProvidesAttribute		= "PKG:provides";
const char* kRequiresAttribute		= "PKG:requires";


}	// namespace Package

}	// namespace Haiku
