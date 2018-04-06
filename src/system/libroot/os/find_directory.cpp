/*
 * Copyright 2004, François Revol.
 * Copyright 2007-2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de.
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 *
 * Distributed under the terms of the MIT license.
 */

// TODO: this call is currently compiled for the kernel and libroot separately;
//		they may not always return the same directory right now!

#ifdef _KERNEL_MODE
#	include <vfs.h>
#else
#	include <syscalls.h>
#endif

#include <directories.h>
#include <FindDirectory.h>
#include <fs_info.h>
#include <StackOrHeapArray.h>

#include <errno.h>
#include <pwd.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

#include <architecture_private.h>
#include <errno_private.h>
#include <find_directory_private.h>
#include <stdlib_private.h>
#include <symbol_versioning.h>
#include <user_group.h>

#include <AutoDeleter.h>

#include "PathBuffer.h"


/* use pwents to find home */
#define USE_PWENTS


/*
 * If you change any of the directories below, please have a look at
 * headers/private/libroot/directories.h and adjust that accordingly!
 */

#define SYSTEM "system"
#define COMMON "system/data/empty"
#define NON_PACKAGED "/non-packaged"

enum {
	// obsolete common directories
	B_COMMON_DIRECTORY					= 2000,
	B_COMMON_SYSTEM_DIRECTORY,
	B_COMMON_ADDONS_DIRECTORY,
	B_COMMON_BOOT_DIRECTORY,
	B_COMMON_FONTS_DIRECTORY,
	B_COMMON_LIB_DIRECTORY,
	B_COMMON_SERVERS_DIRECTORY,
	B_COMMON_BIN_DIRECTORY,
	_B_COMMON_ETC_DIRECTORY,
	B_COMMON_DOCUMENTATION_DIRECTORY,
	_B_COMMON_SETTINGS_DIRECTORY,
	B_COMMON_DEVELOP_DIRECTORY,
	_B_COMMON_LOG_DIRECTORY,
	_B_COMMON_SPOOL_DIRECTORY,
	_B_COMMON_TEMP_DIRECTORY,
	_B_COMMON_VAR_DIRECTORY,
	B_COMMON_TRANSLATORS_DIRECTORY,
	B_COMMON_MEDIA_NODES_DIRECTORY,
	B_COMMON_SOUNDS_DIRECTORY,
	B_COMMON_DATA_DIRECTORY,
	_B_COMMON_CACHE_DIRECTORY,
	B_COMMON_PACKAGES_DIRECTORY,
	B_COMMON_HEADERS_DIRECTORY,
};


/* Haiku system directories */

static const char *kSystemDirectories[] = {
	SYSTEM,										// B_SYSTEM_DIRECTORY
	SYSTEM,										// B_BEOS_SYSTEM_DIRECTORY
	SYSTEM "/add-ons$a",
	SYSTEM "/boot",
	SYSTEM "/data/fonts",
	SYSTEM "/lib$a",
	SYSTEM "/servers",
	SYSTEM "/apps",
	SYSTEM "/bin$a",
	SYSTEM "/settings/etc",
	SYSTEM "/documentation",
	SYSTEM "/preferences",
	SYSTEM "/add-ons$a/Translators",
	SYSTEM "/add-ons$a/media",
	SYSTEM "/data/sounds",
	SYSTEM "/data",
	SYSTEM "/develop",
	SYSTEM "/packages",
	SYSTEM "/develop/headers$a",
};

/* Common directories, shared among users */

static const char *kCommonDirectories[] = {
	COMMON,									// B_COMMON_DIRECTORY
	COMMON,									// B_COMMON_SYSTEM_DIRECTORY
	COMMON "/add-ons$a",
	COMMON "/boot",
	COMMON "/data/fonts",
	COMMON "/lib$a",
	COMMON "/servers",
	COMMON "/bin$a",
	SYSTEM "/settings/etc",					// B_SYSTEM_ETC_DIRECTORY
	COMMON "/documentation",
	SYSTEM "/settings",						// B_SYSTEM_SETTINGS_DIRECTORY
	COMMON "/develop",
	SYSTEM "/var/log",						// B_SYSTEM_LOG_DIRECTORY
	SYSTEM "/var/spool",					// B_SYSTEM_SPOOL_DIRECTORY
	SYSTEM "/cache/tmp",					// B_SYSTEM_TEMP_DIRECTORY
	SYSTEM "/var",							// B_SYSTEM_VAR_DIRECTORY
	COMMON "/add-ons$a/Translators",
	COMMON "/add-ons$a/media",
	COMMON "/data/sounds",
	COMMON "/data",
	SYSTEM "/cache",						// B_SYSTEM_CACHE_DIRECTORY
	COMMON "/packages",
	COMMON "/develop/headers$a",
	SYSTEM NON_PACKAGED,
	SYSTEM NON_PACKAGED "/add-ons$a",
	SYSTEM NON_PACKAGED "/add-ons$a/Translators",
	SYSTEM NON_PACKAGED "/add-ons$a/media",
	SYSTEM NON_PACKAGED "/bin$a",
	SYSTEM NON_PACKAGED "/data",
	SYSTEM NON_PACKAGED "/data/fonts",
	SYSTEM NON_PACKAGED "/data/sounds",
	SYSTEM NON_PACKAGED "/documentation",
	SYSTEM NON_PACKAGED "/lib$a",
	SYSTEM NON_PACKAGED "/develop/headers$a",
	SYSTEM NON_PACKAGED "/develop",
};

/* User directories */

#define HOME "$h"
#define CONFIG "/config"

static const char *kUserDirectories[] = {
	HOME,									// B_USER_DIRECTORY
	HOME CONFIG,							// B_USER_CONFIG_DIRECTORY
	HOME CONFIG "/add-ons$a",
	HOME CONFIG "/settings/boot",
	HOME CONFIG "/data/fonts",
	HOME CONFIG "/lib$a",
	HOME CONFIG "/settings",
	HOME CONFIG "/settings/deskbar/menu",
	HOME CONFIG "/settings/printers",
	HOME CONFIG "/add-ons$a/Translators",
	HOME CONFIG "/add-ons$a/media",
	HOME CONFIG "/data/sounds",
	HOME CONFIG "/data",
	HOME CONFIG "/cache",
	HOME CONFIG "/packages",
	HOME CONFIG "/develop/headers$a",
	HOME CONFIG NON_PACKAGED,
	HOME CONFIG NON_PACKAGED "/add-ons$a",
	HOME CONFIG NON_PACKAGED "/add-ons$a/Translators",
	HOME CONFIG NON_PACKAGED "/add-ons$a/media",
	HOME CONFIG NON_PACKAGED "/bin$a",
	HOME CONFIG NON_PACKAGED "/data",
	HOME CONFIG NON_PACKAGED "/data/fonts",
	HOME CONFIG NON_PACKAGED "/data/sounds",
	HOME CONFIG NON_PACKAGED "/documentation",
	HOME CONFIG NON_PACKAGED "/lib$a",
	HOME CONFIG NON_PACKAGED "/develop/headers$a",
	HOME CONFIG NON_PACKAGED "/develop",
	HOME CONFIG "/develop",
	HOME CONFIG "/documentation",
	HOME CONFIG "/servers",
	HOME CONFIG "/apps",
	HOME CONFIG "/bin$a",
	HOME CONFIG "/preferences",
	HOME CONFIG "/settings/etc",
	HOME CONFIG "/var/log",
	HOME CONFIG "/var/spool",
	HOME CONFIG "/var",
};

#ifndef _LOADER_MODE
/*! make dir and its parents if needed */
static int
create_path(const char *path, mode_t mode)
{
	int pathLength;
	int i = 0;

	if (path == NULL || ((pathLength = strlen(path)) > B_PATH_NAME_LENGTH))
		return EINVAL;

	BStackOrHeapArray<char, 128> buffer(pathLength + 1);

	while (++i < pathLength) {
		char *slash = strchr(&path[i], '/');
		struct stat st;

		if (slash == NULL)
			i = pathLength;
		else if (i != slash - path)
			i = slash - path;
		else
			continue;

		strlcpy(buffer, path, i + 1);
		if (stat(buffer, &st) < 0) {
			__set_errno(0);
			if (mkdir(buffer, mode) < 0)
				return errno;
		}
	}

	return 0;
}


static size_t
get_user_home_path(char* buffer, size_t bufferSize)
{
	const char* home = NULL;
#ifndef _KERNEL_MODE
#ifdef USE_PWENTS
	uid_t user = geteuid();
	if (user == 0) {
		// TODO: this is a work-around as the launch_daemon, and the registrar
		// must not call getpwuid_r().
		return strlcpy(buffer, kUserDirectory, bufferSize);
	}

	struct passwd pwBuffer;
	char pwStringBuffer[MAX_PASSWD_BUFFER_SIZE];
	struct passwd* pw;

	if (getpwuid_r(user, &pwBuffer, pwStringBuffer,
			sizeof(pwStringBuffer), &pw) == 0
		&& pw != NULL) {
		home = pw->pw_dir;
	}
#endif	// USE_PWENTS
	if (home == NULL) {
		/* use env var */
		ssize_t result = __getenv_reentrant("HOME", buffer, bufferSize);
		if (result >= 0)
			return result;
	}
#endif	// !_KERNEL_MODE
	if (home == NULL)
		home = kUserDirectory;

	return strlcpy(buffer, home, bufferSize);
}


//	#pragma mark -


status_t
__find_directory(directory_which which, dev_t device, bool createIt,
	char *returnedPath, int32 _pathLength)
{
	if (_pathLength <= 0)
		return E2BIG;
	size_t pathLength = _pathLength;

	status_t err = B_OK;
	dev_t bootDevice = -1;
	struct fs_info fsInfo;
	struct stat st;
	const char *templatePath = NULL;

	/* as with the R5 version, no on-stack buffer */
	char *buffer = (char*)malloc(pathLength);
	if (buffer == NULL)
		return B_NO_MEMORY;
	MemoryDeleter bufferDeleter(buffer);

	memset(buffer, 0, pathLength);

	/* fiddle with non-boot volume for items that need it */
	switch (which) {
		case B_DESKTOP_DIRECTORY:
		case B_TRASH_DIRECTORY:
			bootDevice = dev_for_path("/boot");
			if (device <= 0)
				device = bootDevice;
			if (fs_stat_dev(device, &fsInfo) != B_OK)
				return ENODEV;
			if (device != bootDevice) {
#ifdef _KERNEL_MODE
				err = _user_entry_ref_to_path(device, fsInfo.root, /*"."*/
					NULL, buffer, pathLength);
#else
				err = _kern_entry_ref_to_path(device, fsInfo.root, /*"."*/
					NULL, buffer, pathLength);
#endif
				if (err != B_OK)
					return err;
			} else {
				/* use the user id to find the home folder */
				/* done later */
				strlcat(buffer, "/boot", pathLength);
			}
			break;
		case B_PACKAGE_LINKS_DIRECTORY:
			// this is a directory living in rootfs
			break;
		default:
			strlcat(buffer, "/boot", pathLength);
			break;
	}

	switch ((int)which) {
		/* Per volume directories */
		case B_DESKTOP_DIRECTORY:
			if (device == bootDevice || !strcmp(fsInfo.fsh_name, "bfs"))
				templatePath = "$h/Desktop";
			break;
		case B_TRASH_DIRECTORY:
			// TODO: eventually put that into the file system API?
			if (device == bootDevice || !strcmp(fsInfo.fsh_name, "bfs"))
				templatePath = "trash"; // TODO: add suffix for current user
			else if (!strcmp(fsInfo.fsh_name, "fat"))
				templatePath = "RECYCLED/_BEOS_";
			break;

		/* Haiku system directories */
		case B_SYSTEM_DIRECTORY:
		case B_BEOS_SYSTEM_DIRECTORY:
		case B_SYSTEM_ADDONS_DIRECTORY:
		case B_SYSTEM_BOOT_DIRECTORY:
		case B_SYSTEM_FONTS_DIRECTORY:
		case B_SYSTEM_LIB_DIRECTORY:
		case B_SYSTEM_SERVERS_DIRECTORY:
		case B_SYSTEM_APPS_DIRECTORY:
		case B_SYSTEM_BIN_DIRECTORY:
		case B_BEOS_ETC_DIRECTORY:
		case B_SYSTEM_DOCUMENTATION_DIRECTORY:
		case B_SYSTEM_PREFERENCES_DIRECTORY:
		case B_SYSTEM_TRANSLATORS_DIRECTORY:
		case B_SYSTEM_MEDIA_NODES_DIRECTORY:
		case B_SYSTEM_SOUNDS_DIRECTORY:
		case B_SYSTEM_DATA_DIRECTORY:
		case B_SYSTEM_DEVELOP_DIRECTORY:
		case B_SYSTEM_PACKAGES_DIRECTORY:
		case B_SYSTEM_HEADERS_DIRECTORY:
			templatePath = kSystemDirectories[which - B_SYSTEM_DIRECTORY];
			break;

		/* Obsolete common directories and writable system directories */
		case B_COMMON_DIRECTORY:
		case B_COMMON_SYSTEM_DIRECTORY:
		case B_COMMON_ADDONS_DIRECTORY:
		case B_COMMON_BOOT_DIRECTORY:
		case B_COMMON_FONTS_DIRECTORY:
		case B_COMMON_LIB_DIRECTORY:
		case B_COMMON_SERVERS_DIRECTORY:
		case B_COMMON_BIN_DIRECTORY:
		case B_SYSTEM_ETC_DIRECTORY:
		case B_COMMON_DOCUMENTATION_DIRECTORY:
		case B_SYSTEM_SETTINGS_DIRECTORY:
		case B_COMMON_DEVELOP_DIRECTORY:
		case B_SYSTEM_LOG_DIRECTORY:
		case B_SYSTEM_SPOOL_DIRECTORY:
		case B_SYSTEM_TEMP_DIRECTORY:
		case B_SYSTEM_VAR_DIRECTORY:
		case B_COMMON_TRANSLATORS_DIRECTORY:
		case B_COMMON_MEDIA_NODES_DIRECTORY:
		case B_COMMON_SOUNDS_DIRECTORY:
		case B_COMMON_DATA_DIRECTORY:
		case B_SYSTEM_CACHE_DIRECTORY:
		case B_COMMON_PACKAGES_DIRECTORY:
		case B_COMMON_HEADERS_DIRECTORY:
		case B_SYSTEM_NONPACKAGED_DIRECTORY:
		case B_SYSTEM_NONPACKAGED_ADDONS_DIRECTORY:
		case B_SYSTEM_NONPACKAGED_TRANSLATORS_DIRECTORY:
		case B_SYSTEM_NONPACKAGED_MEDIA_NODES_DIRECTORY:
		case B_SYSTEM_NONPACKAGED_BIN_DIRECTORY:
		case B_SYSTEM_NONPACKAGED_DATA_DIRECTORY:
		case B_SYSTEM_NONPACKAGED_FONTS_DIRECTORY:
		case B_SYSTEM_NONPACKAGED_SOUNDS_DIRECTORY:
		case B_SYSTEM_NONPACKAGED_DOCUMENTATION_DIRECTORY:
		case B_SYSTEM_NONPACKAGED_LIB_DIRECTORY:
		case B_SYSTEM_NONPACKAGED_HEADERS_DIRECTORY:
		case B_SYSTEM_NONPACKAGED_DEVELOP_DIRECTORY:
			templatePath = kCommonDirectories[which - B_COMMON_DIRECTORY];
			break;

		/* User directories */
		case B_USER_DIRECTORY:
		case B_USER_CONFIG_DIRECTORY:
		case B_USER_ADDONS_DIRECTORY:
		case B_USER_BOOT_DIRECTORY:
		case B_USER_FONTS_DIRECTORY:
		case B_USER_LIB_DIRECTORY:
		case B_USER_SETTINGS_DIRECTORY:
		case B_USER_DESKBAR_DIRECTORY:
		case B_USER_PRINTERS_DIRECTORY:
		case B_USER_TRANSLATORS_DIRECTORY:
		case B_USER_MEDIA_NODES_DIRECTORY:
		case B_USER_SOUNDS_DIRECTORY:
		case B_USER_DATA_DIRECTORY:
		case B_USER_CACHE_DIRECTORY:
		case B_USER_PACKAGES_DIRECTORY:
		case B_USER_HEADERS_DIRECTORY:
		case B_USER_DEVELOP_DIRECTORY:
		case B_USER_DOCUMENTATION_DIRECTORY:
		case B_USER_NONPACKAGED_DIRECTORY:
		case B_USER_NONPACKAGED_ADDONS_DIRECTORY:
		case B_USER_NONPACKAGED_TRANSLATORS_DIRECTORY:
		case B_USER_NONPACKAGED_MEDIA_NODES_DIRECTORY:
		case B_USER_NONPACKAGED_BIN_DIRECTORY:
		case B_USER_NONPACKAGED_DATA_DIRECTORY:
		case B_USER_NONPACKAGED_FONTS_DIRECTORY:
		case B_USER_NONPACKAGED_SOUNDS_DIRECTORY:
		case B_USER_NONPACKAGED_DOCUMENTATION_DIRECTORY:
		case B_USER_NONPACKAGED_LIB_DIRECTORY:
		case B_USER_NONPACKAGED_HEADERS_DIRECTORY:
		case B_USER_NONPACKAGED_DEVELOP_DIRECTORY:
		case B_USER_SERVERS_DIRECTORY:
		case B_USER_APPS_DIRECTORY:
		case B_USER_BIN_DIRECTORY:
		case B_USER_PREFERENCES_DIRECTORY:
		case B_USER_ETC_DIRECTORY:
		case B_USER_LOG_DIRECTORY:
		case B_USER_SPOOL_DIRECTORY:
		case B_USER_VAR_DIRECTORY:
			templatePath = kUserDirectories[which - B_USER_DIRECTORY];
			break;

		/* Global directories */
		case B_APPS_DIRECTORY:
		case B_UTILITIES_DIRECTORY:
			templatePath = SYSTEM "/apps";
			break;
		case B_PREFERENCES_DIRECTORY:
			templatePath = SYSTEM "/preferences";
			break;
		case B_PACKAGE_LINKS_DIRECTORY:
			templatePath = "packages";
			break;

		default:
			return EINVAL;
	}

	if (templatePath == NULL)
		return ENOENT;

	PathBuffer pathBuffer(buffer, pathLength, strlen(buffer));

	// resolve "$h" placeholder to the user's home directory
	if (!strncmp(templatePath, "$h", 2)) {
		if (bootDevice > -1 && device != bootDevice) {
			pathBuffer.Append("/home");
		} else {
			size_t length = get_user_home_path(buffer, pathLength);
			if (length >= pathLength)
				return E2BIG;
			pathBuffer.SetTo(buffer, pathLength, length);
		}
		templatePath += 2;
	} else if (templatePath[0] != '\0')
		pathBuffer.Append('/');

	// resolve "$a" placeholder to the architecture subdirectory, if not
	// primary
	if (char* dollar = strchr(templatePath, '$')) {
		if (dollar[1] == 'a') {
			pathBuffer.Append(templatePath, dollar - templatePath);
#ifndef _KERNEL_MODE
			const char* architecture = __get_architecture();
			if (strcmp(architecture, __get_primary_architecture()) != 0) {
				pathBuffer.Append('/');
				pathBuffer.Append(architecture);
			}
#endif
			templatePath = dollar + 2;
		}
	}

	// append (remainder of) template path
	pathBuffer.Append(templatePath);

	if (pathBuffer.Length() >= pathLength)
		return E2BIG;

	if (createIt && stat(buffer, &st) < 0) {
		err = create_path(buffer, 0755);
		if (err != B_OK)
			return err;
	}

	strlcpy(returnedPath, buffer, pathLength);
	return B_OK;
}


extern "C" status_t
__find_directory_alpha4(directory_which which, dev_t device, bool createIt,
	char *returnedPath, int32 pathLength)
{
	return __find_directory(which, device, createIt, returnedPath, pathLength);
}


DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__find_directory_alpha4",
	"find_directory@", "BASE");

DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("__find_directory", "find_directory@@",
	"1_ALPHA5");
#else // _LOADER_MODE
status_t
__find_directory(directory_which which, dev_t device, bool createIt,
	char *returnedPath, int32 _pathLength)
{
	if (_pathLength <= 0)
		return E2BIG;
	size_t pathLength = _pathLength;

	const char *templatePath = NULL;

	/* as with the R5 version, no on-stack buffer */
	char *buffer = (char*)malloc(pathLength);
	if (buffer == NULL)
		return B_NO_MEMORY;
	MemoryDeleter bufferDeleter(buffer);

	memset(buffer, 0, pathLength);

	strlcat(buffer, "/boot", pathLength);

	switch ((int)which) {
		/* Haiku system directories */
		case B_SYSTEM_DIRECTORY:
		case B_BEOS_SYSTEM_DIRECTORY:
		case B_SYSTEM_ADDONS_DIRECTORY:
		case B_SYSTEM_BOOT_DIRECTORY:
		case B_SYSTEM_FONTS_DIRECTORY:
		case B_SYSTEM_LIB_DIRECTORY:
		case B_SYSTEM_SERVERS_DIRECTORY:
		case B_SYSTEM_APPS_DIRECTORY:
		case B_SYSTEM_BIN_DIRECTORY:
		case B_BEOS_ETC_DIRECTORY:
		case B_SYSTEM_DOCUMENTATION_DIRECTORY:
		case B_SYSTEM_PREFERENCES_DIRECTORY:
		case B_SYSTEM_TRANSLATORS_DIRECTORY:
		case B_SYSTEM_MEDIA_NODES_DIRECTORY:
		case B_SYSTEM_SOUNDS_DIRECTORY:
		case B_SYSTEM_DATA_DIRECTORY:
		case B_SYSTEM_DEVELOP_DIRECTORY:
		case B_SYSTEM_PACKAGES_DIRECTORY:
		case B_SYSTEM_HEADERS_DIRECTORY:
			templatePath = kSystemDirectories[which - B_SYSTEM_DIRECTORY];
			break;

		/* Obsolete common directories and writable system directories */
		case B_COMMON_DIRECTORY:
		case B_COMMON_SYSTEM_DIRECTORY:
		case B_COMMON_ADDONS_DIRECTORY:
		case B_COMMON_BOOT_DIRECTORY:
		case B_COMMON_FONTS_DIRECTORY:
		case B_COMMON_LIB_DIRECTORY:
		case B_COMMON_SERVERS_DIRECTORY:
		case B_COMMON_BIN_DIRECTORY:
		case B_SYSTEM_ETC_DIRECTORY:
		case B_COMMON_DOCUMENTATION_DIRECTORY:
		case B_SYSTEM_SETTINGS_DIRECTORY:
		case B_COMMON_DEVELOP_DIRECTORY:
		case B_SYSTEM_LOG_DIRECTORY:
		case B_SYSTEM_SPOOL_DIRECTORY:
		case B_SYSTEM_TEMP_DIRECTORY:
		case B_SYSTEM_VAR_DIRECTORY:
		case B_COMMON_TRANSLATORS_DIRECTORY:
		case B_COMMON_MEDIA_NODES_DIRECTORY:
		case B_COMMON_SOUNDS_DIRECTORY:
		case B_COMMON_DATA_DIRECTORY:
		case B_SYSTEM_CACHE_DIRECTORY:
		case B_COMMON_PACKAGES_DIRECTORY:
		case B_COMMON_HEADERS_DIRECTORY:
		case B_SYSTEM_NONPACKAGED_DIRECTORY:
		case B_SYSTEM_NONPACKAGED_ADDONS_DIRECTORY:
		case B_SYSTEM_NONPACKAGED_TRANSLATORS_DIRECTORY:
		case B_SYSTEM_NONPACKAGED_MEDIA_NODES_DIRECTORY:
		case B_SYSTEM_NONPACKAGED_BIN_DIRECTORY:
		case B_SYSTEM_NONPACKAGED_DATA_DIRECTORY:
		case B_SYSTEM_NONPACKAGED_FONTS_DIRECTORY:
		case B_SYSTEM_NONPACKAGED_SOUNDS_DIRECTORY:
		case B_SYSTEM_NONPACKAGED_DOCUMENTATION_DIRECTORY:
		case B_SYSTEM_NONPACKAGED_LIB_DIRECTORY:
		case B_SYSTEM_NONPACKAGED_HEADERS_DIRECTORY:
		case B_SYSTEM_NONPACKAGED_DEVELOP_DIRECTORY:
			templatePath = kCommonDirectories[which - B_COMMON_DIRECTORY];
			break;

		/* User directories */
		case B_USER_DIRECTORY:
		case B_USER_CONFIG_DIRECTORY:
		case B_USER_ADDONS_DIRECTORY:
		case B_USER_BOOT_DIRECTORY:
		case B_USER_FONTS_DIRECTORY:
		case B_USER_LIB_DIRECTORY:
		case B_USER_SETTINGS_DIRECTORY:
		case B_USER_DESKBAR_DIRECTORY:
		case B_USER_PRINTERS_DIRECTORY:
		case B_USER_TRANSLATORS_DIRECTORY:
		case B_USER_MEDIA_NODES_DIRECTORY:
		case B_USER_SOUNDS_DIRECTORY:
		case B_USER_DATA_DIRECTORY:
		case B_USER_CACHE_DIRECTORY:
		case B_USER_PACKAGES_DIRECTORY:
		case B_USER_HEADERS_DIRECTORY:
		case B_USER_DEVELOP_DIRECTORY:
		case B_USER_DOCUMENTATION_DIRECTORY:
		case B_USER_NONPACKAGED_DIRECTORY:
		case B_USER_NONPACKAGED_ADDONS_DIRECTORY:
		case B_USER_NONPACKAGED_TRANSLATORS_DIRECTORY:
		case B_USER_NONPACKAGED_MEDIA_NODES_DIRECTORY:
		case B_USER_NONPACKAGED_BIN_DIRECTORY:
		case B_USER_NONPACKAGED_DATA_DIRECTORY:
		case B_USER_NONPACKAGED_FONTS_DIRECTORY:
		case B_USER_NONPACKAGED_SOUNDS_DIRECTORY:
		case B_USER_NONPACKAGED_DOCUMENTATION_DIRECTORY:
		case B_USER_NONPACKAGED_LIB_DIRECTORY:
		case B_USER_NONPACKAGED_HEADERS_DIRECTORY:
		case B_USER_NONPACKAGED_DEVELOP_DIRECTORY:
		case B_USER_SERVERS_DIRECTORY:
		case B_USER_APPS_DIRECTORY:
		case B_USER_BIN_DIRECTORY:
		case B_USER_PREFERENCES_DIRECTORY:
		case B_USER_ETC_DIRECTORY:
		case B_USER_LOG_DIRECTORY:
		case B_USER_SPOOL_DIRECTORY:
		case B_USER_VAR_DIRECTORY:
			templatePath = kUserDirectories[which - B_USER_DIRECTORY];
			break;

		default:
			return EINVAL;
	}

	if (templatePath == NULL)
		return ENOENT;

	PathBuffer pathBuffer(buffer, pathLength, strlen(buffer));

	// resolve "$h" placeholder to the user's home directory
	if (!strncmp(templatePath, "$h", 2)) {
		pathBuffer.Append("/home");
		templatePath += 2;
	} else if (templatePath[0] != '\0')
		pathBuffer.Append('/');

	// resolve "$a" placeholder to the architecture subdirectory, if not
	// primary
	if (char* dollar = strchr(templatePath, '$')) {
		if (dollar[1] == 'a') {
			pathBuffer.Append(templatePath, dollar - templatePath);
			templatePath = dollar + 2;
		}
	}

	// append (remainder of) template path
	pathBuffer.Append(templatePath);

	if (pathBuffer.Length() >= pathLength)
		return E2BIG;

	strlcpy(returnedPath, buffer, pathLength);
	return B_OK;
}
#endif // _LOADER_MODE
