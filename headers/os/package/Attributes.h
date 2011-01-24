/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HAIKU__PACKAGE__ATTRIBUTES_H_
#define _HAIKU__PACKAGE__ATTRIBUTES_H_


namespace Haiku {

namespace Package {


// attributes used in package and as file attribute, too
extern const char* kNameAttribute;
extern const char* kVendorAttribute;
extern const char* kVersionAttribute;

// attributes kept local to packages
extern const char* kCopyrightAttribute;
extern const char* kLicenseAttribute;
extern const char* kProvidesAttribute;
extern const char* kRequiresAttribute;


}	// namespace Package

}	// namespace Haiku


#endif // _HAIKU__PACKAGE__ATTRIBUTES_H_
