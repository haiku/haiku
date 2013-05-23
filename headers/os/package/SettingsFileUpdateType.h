/*
 * Copyright 2013, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__SETTINGS_FILE_UPDATE_TYPE_H_
#define _PACKAGE__SETTINGS_FILE_UPDATE_TYPE_H_


#include <String.h>


namespace BPackageKit {


// global settings file update types -- specifies behavior in case the previous
// version of a settings file provided by a package has been changed by the
// user.
enum BSettingsFileUpdateType {
	B_SETTINGS_FILE_UPDATE_TYPE_KEEP_OLD	= 0,
		// the old settings file can be kept
	B_SETTINGS_FILE_UPDATE_TYPE_MANUAL		= 1,
		// the old settings file needs to be updated manually
	B_SETTINGS_FILE_UPDATE_TYPE_AUTO_MERGE	= 2,
		// try a three-way merge

	B_SETTINGS_FILE_UPDATE_TYPE_ENUM_COUNT,

	B_SETTINGS_FILE_UPDATE_TYPE_DEFAULT = B_SETTINGS_FILE_UPDATE_TYPE_KEEP_OLD
};


}	// namespace BPackageKit


#endif	// _PACKAGE__SETTINGS_FILE_UPDATE_TYPE_H_
