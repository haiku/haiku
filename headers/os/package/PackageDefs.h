/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__PACKAGE_DEFS_H_
#define _PACKAGE__PACKAGE_DEFS_H_


namespace BPackageKit {


enum BPackageInstallationLocation {
	B_PACKAGE_INSTALLATION_LOCATION_SYSTEM,
	B_PACKAGE_INSTALLATION_LOCATION_COMMON,
	B_PACKAGE_INSTALLATION_LOCATION_HOME,

	B_PACKAGE_INSTALLATION_LOCATION_ENUM_COUNT
};


}	// namespace BPackageKit


#endif // _PACKAGE__PACKAGE_DEFS_H_
