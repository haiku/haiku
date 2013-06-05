/*
 * Copyright 2013, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__WRITABLE_FILE_UPDATE_TYPE_H_
#define _PACKAGE__WRITABLE_FILE_UPDATE_TYPE_H_


#include <String.h>


namespace BPackageKit {


// global writable file update types -- specifies behavior in case the previous
// version of a writable file provided by a package has been changed by the
// user.
enum BWritableFileUpdateType {
	B_WRITABLE_FILE_UPDATE_TYPE_KEEP_OLD	= 0,
		// the old writable file can be kept
	B_WRITABLE_FILE_UPDATE_TYPE_MANUAL		= 1,
		// the old writable file needs to be updated manually
	B_WRITABLE_FILE_UPDATE_TYPE_AUTO_MERGE	= 2,
		// try a three-way merge

	B_WRITABLE_FILE_UPDATE_TYPE_ENUM_COUNT,

	B_WRITABLE_FILE_UPDATE_TYPE_DEFAULT = B_WRITABLE_FILE_UPDATE_TYPE_KEEP_OLD
};


}	// namespace BPackageKit


#endif	// _PACKAGE__WRITABLE_FILE_UPDATE_TYPE_H_
