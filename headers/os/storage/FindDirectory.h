//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file FindDirectory.h
	Declarations of find_directory() functions and associated types.	
*/

#ifndef __sk_find_directory_h__
#define __sk_find_directory_h__

#include <SupportDefs.h>

#ifdef USE_OPENBEOS_NAMESPACE
namespace OpenBeOS {
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	/* ---
		Per volume directories.  When asking for these
		directories, a volume must be specified, or the call will assume
		the boot volume.
	--- */

	B_DESKTOP_DIRECTORY				= 0,
	B_TRASH_DIRECTORY,

	/* ---
		BeOS directories.  These are mostly accessed read-only.
	--- */

	B_BEOS_DIRECTORY				= 1000,
	B_BEOS_SYSTEM_DIRECTORY,
	B_BEOS_ADDONS_DIRECTORY,
	B_BEOS_BOOT_DIRECTORY,
	B_BEOS_FONTS_DIRECTORY,
	B_BEOS_LIB_DIRECTORY,
 	B_BEOS_SERVERS_DIRECTORY,
	B_BEOS_APPS_DIRECTORY,
	B_BEOS_BIN_DIRECTORY,
	B_BEOS_ETC_DIRECTORY,
	B_BEOS_DOCUMENTATION_DIRECTORY,
	B_BEOS_PREFERENCES_DIRECTORY,
	B_BEOS_TRANSLATORS_DIRECTORY,
	B_BEOS_MEDIA_NODES_DIRECTORY,
	B_BEOS_SOUNDS_DIRECTORY,

	/* ---
		Common directories, shared among all users.
	--- */

	B_COMMON_DIRECTORY				= 2000,
	B_COMMON_SYSTEM_DIRECTORY,
	B_COMMON_ADDONS_DIRECTORY,
	B_COMMON_BOOT_DIRECTORY,
	B_COMMON_FONTS_DIRECTORY,
	B_COMMON_LIB_DIRECTORY,
	B_COMMON_SERVERS_DIRECTORY,
	B_COMMON_BIN_DIRECTORY,
	B_COMMON_ETC_DIRECTORY,
	B_COMMON_DOCUMENTATION_DIRECTORY,
	B_COMMON_SETTINGS_DIRECTORY,
	B_COMMON_DEVELOP_DIRECTORY,
	B_COMMON_LOG_DIRECTORY,
	B_COMMON_SPOOL_DIRECTORY,
	B_COMMON_TEMP_DIRECTORY,
	B_COMMON_VAR_DIRECTORY,
	B_COMMON_TRANSLATORS_DIRECTORY,
	B_COMMON_MEDIA_NODES_DIRECTORY,
	B_COMMON_SOUNDS_DIRECTORY,


	/* ---
		User directories.  These are interpreted in the context
		of the user making the find_directory call.
	--- */

	B_USER_DIRECTORY				= 3000,
	B_USER_CONFIG_DIRECTORY,
	B_USER_ADDONS_DIRECTORY,
	B_USER_BOOT_DIRECTORY,
	B_USER_FONTS_DIRECTORY,
	B_USER_LIB_DIRECTORY,
	B_USER_SETTINGS_DIRECTORY,
	B_USER_DESKBAR_DIRECTORY,
	B_USER_PRINTERS_DIRECTORY,
	B_USER_TRANSLATORS_DIRECTORY,
	B_USER_MEDIA_NODES_DIRECTORY,
	B_USER_SOUNDS_DIRECTORY,

	/* ---
		Global directories.
	--- */

	B_APPS_DIRECTORY				= 4000,
	B_PREFERENCES_DIRECTORY,
	B_UTILITIES_DIRECTORY

} directory_which;

/* ---
	The C interface
--- */

status_t find_directory(directory_which which, dev_t volume, bool createIt,
						char *pathString, int32 length);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

class BVolume;
class BPath;

/* ---
	C++ interface
--- */

status_t find_directory(directory_which which, BPath *path,
						bool createIt = false, BVolume *volume = NULL);

#endif

#ifdef USE_OPENBEOS_NAMESPACE
};		// namespace OpenBeOS
#endif

#endif	// __sk_find_directory_h__
