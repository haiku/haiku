/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__PACKAGE_FLAGS_H_
#define _PACKAGE__PACKAGE_FLAGS_H_


namespace BPackageKit {


enum {
	B_PACKAGE_FLAG_APPROVE_LICENSE	= 1ul << 0,
		// will trigger display and approval of license before installation
	B_PACKAGE_FLAG_SYSTEM_PACKAGE	= 1ul << 1,
		// marks package as system package (i.e. belonging into /boot/system)
};


}	// namespace BPackageKit


#endif	// _PACKAGE__PACKAGE_FLAGS_H_
