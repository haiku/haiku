/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__ATTRIBUTES_H_
#define _PACKAGE__ATTRIBUTES_H_


namespace BPackageKit {


// attributes used in package and as file attribute, too
extern const char* kPackageNameAttribute;
extern const char* kPackageVendorAttribute;
extern const char* kPackageVersionAttribute;

// attributes kept local to packages
extern const char* kPackageCopyrightsAttribute;
extern const char* kPackageLicensesAttribute;
extern const char* kPackagePackagerAttribute;
extern const char* kPackageProvidesAttribute;
extern const char* kPackageRequiresAttribute;


}	// namespace BPackageKit


#endif // _PACKAGE__ATTRIBUTES_H_
