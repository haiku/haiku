/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__PACKAGE_ARCHITECTURE_H_
#define _PACKAGE__PACKAGE_ARCHITECTURE_H_


namespace BPackageKit {


enum BPackageArchitecture {
	B_PACKAGE_ARCHITECTURE_ANY		= 0,
	B_PACKAGE_ARCHITECTURE_X86		= 1,
	B_PACKAGE_ARCHITECTURE_X86_GCC2	= 2,
	//
	B_PACKAGE_ARCHITECTURE_ENUM_COUNT,
};


}	// namespace BPackageKit


#endif	// _PACKAGE__PACKAGE_ARCHITECTURE_H_
