//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file FindDirectory.cpp
	find_directory() implementations.	
*/

#include <FindDirectory.h>

#include <errno.h>

#include <Directory.h>
#include <Entry.h>
#include <fs_info.h>
#include <Path.h>
#include <Volume.h>

#ifdef USE_OPENBEOS_NAMESPACE
namespace OpenBeOS {
#endif

enum {
	NOT_IMPLEMENTED	= B_ERROR,
};

// find_directory
/*!	\brief Internal find_directory() helper function, that does the real work.
	\param which the directory_which constant specifying the directory
	\param path a BPath object to be initialized to the directory's path
	\param createIt \c true, if the directory shall be created, if it doesn't
		   already exist, \c false otherwise.
	\param device the volume on which the directory is located
	\return \c B_OK if everything went fine, an error code otherwise.
*/
static
status_t
find_directory(directory_which which, BPath &path, bool createIt, dev_t device)
{
	status_t error = B_BAD_VALUE;
	switch (which) {
		// volume relative dirs
		case B_DESKTOP_DIRECTORY:
		{
			if (device < 0)
				device = dev_for_path("/boot");
			fs_info info;
			if (fs_stat_dev(device, &info) == 0) {
				if (!strcmp(info.fsh_name, "bfs")) {
					entry_ref ref(device, info.root, "home");
					BPath homePath(&ref);
					error = homePath.InitCheck();
					if (error == B_OK)
						path.SetTo(homePath.Path(), "Desktop");
				} else
					error = B_ENTRY_NOT_FOUND;
			} else
				error = errno;
			break;
		}
		case B_TRASH_DIRECTORY:
		{
			if (device < 0)
				device = dev_for_path("/boot");
			fs_info info;
			if (fs_stat_dev(device, &info) == 0) {
				if (!strcmp(info.fsh_name, "bfs")) {
					entry_ref ref(device, info.root, "home");
					BPath homePath(&ref);
					error = homePath.InitCheck();
					if (error == B_OK)
						path.SetTo(homePath.Path(), "Desktop/Trash");
				} else if (!strcmp(info.fsh_name, "dos")) {
					entry_ref ref(device, info.root, "RECYCLED");
					BPath recycledPath(&ref);
					error = recycledPath.InitCheck();
					if (error == B_OK)
						path.SetTo(recycledPath.Path(), "_BEOS_");
				} else
					error = B_ENTRY_NOT_FOUND;
			} else
				error = errno;
			break;
		}
		// BeOS directories.  These are mostly accessed read-only.
		case B_BEOS_DIRECTORY:
			error = path.SetTo("/boot/beos");
			break;
		case B_BEOS_SYSTEM_DIRECTORY:
			error = path.SetTo("/boot/beos/system");
			break;
		case B_BEOS_ADDONS_DIRECTORY:
			error = path.SetTo("/boot/beos/system/add-ons");
			break;
		case B_BEOS_BOOT_DIRECTORY:
			error = path.SetTo("/boot/beos/system/boot");
			break;
		case B_BEOS_FONTS_DIRECTORY:
			error = path.SetTo("/boot/beos/etc/fonts");
			break;
		case B_BEOS_LIB_DIRECTORY:
			error = path.SetTo("/boot/beos/system/lib");
			break;
 		case B_BEOS_SERVERS_DIRECTORY:
			error = path.SetTo("/boot/beos/system/servers");
			break;
		case B_BEOS_APPS_DIRECTORY:
			error = path.SetTo("/boot/beos/apps");
			break;
		case B_BEOS_BIN_DIRECTORY:
			error = path.SetTo("/boot/beos/bin");
			break;
		case B_BEOS_ETC_DIRECTORY:
			error = path.SetTo("/boot/beos/etc");
			break;
		case B_BEOS_DOCUMENTATION_DIRECTORY:
			error = path.SetTo("/boot/beos/documentation");
			break;
		case B_BEOS_PREFERENCES_DIRECTORY:
			error = path.SetTo("/boot/beos/preferences");
			break;
		case B_BEOS_TRANSLATORS_DIRECTORY:
			error = path.SetTo("/boot/beos/system/add-ons/Translators");
			break;
		case B_BEOS_MEDIA_NODES_DIRECTORY:
			error = path.SetTo("/boot/beos/system/add-ons/media");
			break;
		case B_BEOS_SOUNDS_DIRECTORY:
			error = path.SetTo("/boot/beos/etc/sounds");
			break;
		// Common directories, shared among all users.
		case B_COMMON_DIRECTORY:
			error = path.SetTo("/boot/home");
			break;
		case B_COMMON_SYSTEM_DIRECTORY:
			error = path.SetTo("/boot/home/config");
			break;
		case B_COMMON_ADDONS_DIRECTORY:
			error = path.SetTo("/boot/home/config/add-ons");
			break;
		case B_COMMON_BOOT_DIRECTORY:
			error = path.SetTo("/boot/home/config/boot");
			break;
		case B_COMMON_FONTS_DIRECTORY:
			error = path.SetTo("/boot/home/config/fonts");
			break;
		case B_COMMON_LIB_DIRECTORY:
			error = path.SetTo("/boot/home/config/lib");
			break;
		case B_COMMON_SERVERS_DIRECTORY:
			error = path.SetTo("/boot/home/config/servers");
			break;
		case B_COMMON_BIN_DIRECTORY:
			error = path.SetTo("/boot/home/config/bin");
			break;
		case B_COMMON_ETC_DIRECTORY:
			error = path.SetTo("/boot/home/config/etc");
			break;
		case B_COMMON_DOCUMENTATION_DIRECTORY:
			error = path.SetTo("/boot/home/config/documentation");
			break;
		case B_COMMON_SETTINGS_DIRECTORY:
			error = path.SetTo("/boot/home/config/settings");
			break;
		case B_COMMON_DEVELOP_DIRECTORY:
			error = path.SetTo("/boot/develop");
			break;
		case B_COMMON_LOG_DIRECTORY:
			error = path.SetTo("/boot/var/log");
			break;
		case B_COMMON_SPOOL_DIRECTORY:
			error = path.SetTo("/boot/var/spool");
			break;
		case B_COMMON_TEMP_DIRECTORY:
			error = path.SetTo("/boot/var/tmp");
			break;
		case B_COMMON_VAR_DIRECTORY:
			error = path.SetTo("/boot/var");
			break;
		case B_COMMON_TRANSLATORS_DIRECTORY:
			error = path.SetTo("/boot/home/config/add-ons/Translators");
			break;
		case B_COMMON_MEDIA_NODES_DIRECTORY:
			error = path.SetTo("/boot/home/config/add-ons/media");
			break;
		case B_COMMON_SOUNDS_DIRECTORY:
			error = path.SetTo("/boot/home/config/sounds");
			break;
		// User directories.  These are interpreted in the context
		// of the user making the find_directory call.
		case B_USER_DIRECTORY:
			error = path.SetTo("/boot/home");
			break;
		case B_USER_CONFIG_DIRECTORY:
			error = path.SetTo("/boot/home/config");
			break;
		case B_USER_ADDONS_DIRECTORY:
			error = path.SetTo("/boot/home/config/add-ons");
			break;
		case B_USER_BOOT_DIRECTORY:
			error = path.SetTo("/boot/home/config/boot");
			break;
		case B_USER_FONTS_DIRECTORY:
			error = path.SetTo("/boot/home/config/fonts");
			break;
		case B_USER_LIB_DIRECTORY:
			error = path.SetTo("/boot/home/config/lib");
			break;
		case B_USER_SETTINGS_DIRECTORY:
			error = path.SetTo("/boot/home/config/settings");
			break;
		case B_USER_DESKBAR_DIRECTORY:
			error = path.SetTo("/boot/home/config/be");
			break;
		case B_USER_PRINTERS_DIRECTORY:
			error = path.SetTo("/boot/home/config/settings/printers");
			break;
		case B_USER_TRANSLATORS_DIRECTORY:
			error = path.SetTo("/boot/home/config/add-ons/Translators");
			break;
		case B_USER_MEDIA_NODES_DIRECTORY:
			error = path.SetTo("/boot/home/config/add-ons/media");
			break;
		case B_USER_SOUNDS_DIRECTORY:
			error = path.SetTo("/boot/home/config/sounds");
			break;
		// Global directories.
		case B_APPS_DIRECTORY:
			error = path.SetTo("/boot/apps");
			break;
		case B_PREFERENCES_DIRECTORY:
			error = path.SetTo("/boot/preferences");
			break;
		case B_UTILITIES_DIRECTORY:
			error = path.SetTo("/boot/utilities");
			break;
	}
	// create the directory, if desired
	if (error == B_OK && createIt)
		create_directory(path.Path(), S_IRWXU | S_IRWXG | S_IRWXO);
	return error;
}


// find_directory
//!	Returns a path of a directory specified by a directory_which constant.
/*!	If the supplied volume ID is 
	\param which the directory_which constant specifying the directory
	\param volume the volume on which the directory is located
	\param createIt \c true, if the directory shall be created, if it doesn't
		   already exist, \c false otherwise.
	\param pathString a pointer to a buffer into which the directory path
		   shall be written.
	\param length the size of the buffer
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a pathString.
	- \c E2BIG: Buffer is too small for path.
	- another error code
*/
status_t
find_directory(directory_which which, dev_t volume, bool createIt,
			   char *pathString, int32 length)
{
	status_t error = (pathString ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		BPath path;
		error = find_directory(which, path, createIt, volume);
		if (error == B_OK && (int32)strlen(path.Path()) >= length)
			error = E2BIG;
		if (error == B_OK)
			strcpy(pathString, path.Path());
	}
	return error;
}

// find_directory
//!	Returns a path of a directory specified by a directory_which constant.
/*!	\param which the directory_which constant specifying the directory
	\param path a BPath object to be initialized to the directory's path
	\param createIt \c true, if the directory shall be created, if it doesn't
		   already exist, \c false otherwise.
	\param volume the volume on which the directory is located
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a path.
	- another error code
*/
status_t
find_directory(directory_which which, BPath *path, bool createIt,
			   BVolume *volume)
{
	status_t error = (path ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		dev_t device = -1;
		if (volume && volume->InitCheck() == B_OK)
			device = volume->Device();
		error = find_directory(which, *path, createIt, device);
	}
	return error;
}


#ifdef USE_OPENBEOS_NAMESPACE
};		// namespace OpenBeOS
#endif

