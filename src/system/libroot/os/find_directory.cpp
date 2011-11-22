/*
 * Copyright 2004, François Revol.
 * Copyright 2007-2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
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

#include <errno.h>
#include <pwd.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

#include <errno_private.h>
#include <user_group.h>

/* use pwents to find home */
#define USE_PWENTS


/*
 * If you change any of the directories below, please have a look at
 * headers/private/libroot/directories.h and adjust that accordingly!
 */

#define SYSTEM "system"
#define COMMON "common"
#define NON_PACKAGED "/non-packaged"


/* Haiku system directories */

static const char *kSystemDirectories[] = {
	SYSTEM,										// B_SYSTEM_DIRECTORY
	SYSTEM,										// B_BEOS_SYSTEM_DIRECTORY
	SYSTEM "/add-ons",
	SYSTEM "/boot",
	SYSTEM "/data/fonts",
	SYSTEM "/lib",
	SYSTEM "/servers",
	SYSTEM "/apps",
	SYSTEM "/bin",
	COMMON "/etc",
	SYSTEM "/documentation",
	SYSTEM "/preferences",
	SYSTEM "/add-ons/Translators",
	SYSTEM "/add-ons/media",
	SYSTEM "/data/sounds",
	SYSTEM "/data",
	"develop",
	SYSTEM "/packages",
	"develop/headers",
};

/* Common directories, shared among users */

static const char *kCommonDirectories[] = {
	COMMON,									// B_COMMON_DIRECTORY
	COMMON,									// B_COMMON_SYSTEM_DIRECTORY
	COMMON "/add-ons",
	COMMON "/boot",
	COMMON "/data/fonts",
	COMMON "/lib",
	COMMON "/servers",
	COMMON "/bin",
	COMMON "/etc",
	COMMON "/documentation",
	COMMON "/settings",
	COMMON "/develop",						// B_COMMON_DEVELOP_DIRECTORY
	COMMON "/var/log",						// B_COMMON_LOG_DIRECTORY
	COMMON "/var/spool",					// B_COMMON_SPOOL_DIRECTORY
	COMMON "/cache/tmp",					// B_COMMON_TEMP_DIRECTORY
	COMMON "/var",							// B_COMMON_VAR_DIRECTORY
	COMMON "/add-ons/Translators",
	COMMON "/add-ons/media",
	COMMON "/data/sounds",
	COMMON "/data",
	COMMON "/cache",						// B_COMMON_CACHE_DIRECTORY
	COMMON "/packages",
	COMMON "/include",
	COMMON NON_PACKAGED,
	COMMON NON_PACKAGED "/add-ons",
	COMMON NON_PACKAGED "/add-ons/Translators",
	COMMON NON_PACKAGED "/add-ons/media",
	COMMON NON_PACKAGED "/bin",
	COMMON NON_PACKAGED "/data",
	COMMON NON_PACKAGED "/data/fonts",
	COMMON NON_PACKAGED "/data/sounds",
	COMMON NON_PACKAGED "/documentation",
	COMMON NON_PACKAGED "/lib",
	COMMON NON_PACKAGED "/develop/headers",
};

/* User directories */

#define HOME "$h"
#define CONFIG "/config"

static const char *kUserDirectories[] = {
	HOME,									// B_USER_DIRECTORY
	HOME CONFIG,							// B_USER_CONFIG_DIRECTORY
	HOME CONFIG "/add-ons",
	HOME CONFIG "/boot",
	HOME CONFIG "/data/fonts",
	HOME CONFIG "/lib",
	HOME CONFIG "/settings",
	HOME CONFIG "/settings/deskbar",
	HOME CONFIG "/settings/printers",
	HOME CONFIG "/add-ons/Translators",
	HOME CONFIG "/add-ons/media",
	HOME CONFIG "/data/sounds",
	HOME CONFIG "/data",
	HOME CONFIG "/cache",
	HOME CONFIG "/packages",
	HOME CONFIG "/include",
	HOME CONFIG NON_PACKAGED,
	HOME CONFIG NON_PACKAGED "/add-ons",
	HOME CONFIG NON_PACKAGED "/add-ons/Translators",
	HOME CONFIG NON_PACKAGED "/add-ons/media",
	HOME CONFIG NON_PACKAGED "/bin",
	HOME CONFIG NON_PACKAGED "/data",
	HOME CONFIG NON_PACKAGED "/data/fonts",
	HOME CONFIG NON_PACKAGED "/data/sounds",
	HOME CONFIG NON_PACKAGED "/documentation",
	HOME CONFIG NON_PACKAGED "/lib",
	HOME CONFIG NON_PACKAGED "/develop/headers",
};


/*! make dir and its parents if needed */
static int
create_path(const char *path, mode_t mode)
{
	char buffer[B_PATH_NAME_LENGTH + 1];
	int pathLength;
	int i = 0;

	if (path == NULL || ((pathLength = strlen(path)) > B_PATH_NAME_LENGTH))
		return EINVAL;

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


//	#pragma mark -


status_t
find_directory(directory_which which, dev_t device, bool createIt,
	char *returnedPath, int32 pathLength)
{
	status_t err = B_OK;
	dev_t bootDevice = -1;
	struct fs_info fsInfo;
	struct stat st;
	char *buffer = NULL;
	const char *home = NULL;
	const char *templatePath = NULL;

	/* as with the R5 version, no on-stack buffer */
	buffer = (char *)malloc(pathLength);
	memset(buffer, 0, pathLength);

	/* fiddle with non-boot volume for items that need it */
	switch (which) {
		case B_DESKTOP_DIRECTORY:
		case B_TRASH_DIRECTORY:
			bootDevice = dev_for_path("/boot");
			if (device <= 0)
				device = bootDevice;
			if (fs_stat_dev(device, &fsInfo) < B_OK) {
				free(buffer);
				return ENODEV;
			}
			if (device != bootDevice) {
#ifdef _KERNEL_MODE
				err = _user_entry_ref_to_path(device, fsInfo.root, /*"."*/
					NULL, buffer, pathLength);
#else
				err = _kern_entry_ref_to_path(device, fsInfo.root, /*"."*/
					NULL, buffer, pathLength);
#endif
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

	if (err < B_OK) {
		free(buffer);
		return err;
	}

	switch (which) {
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

		/* Common directories, shared among users */
		case B_COMMON_DIRECTORY:
		case B_COMMON_SYSTEM_DIRECTORY:
		case B_COMMON_ADDONS_DIRECTORY:
		case B_COMMON_BOOT_DIRECTORY:
		case B_COMMON_FONTS_DIRECTORY:
		case B_COMMON_LIB_DIRECTORY:
		case B_COMMON_SERVERS_DIRECTORY:
		case B_COMMON_BIN_DIRECTORY:
		case B_COMMON_ETC_DIRECTORY:
		case B_COMMON_DOCUMENTATION_DIRECTORY:
		case B_COMMON_SETTINGS_DIRECTORY:
		case B_COMMON_DEVELOP_DIRECTORY:
		case B_COMMON_LOG_DIRECTORY:
		case B_COMMON_SPOOL_DIRECTORY:
		case B_COMMON_TEMP_DIRECTORY:
		case B_COMMON_VAR_DIRECTORY:
		case B_COMMON_TRANSLATORS_DIRECTORY:
		case B_COMMON_MEDIA_NODES_DIRECTORY:
		case B_COMMON_SOUNDS_DIRECTORY:
		case B_COMMON_DATA_DIRECTORY:
		case B_COMMON_CACHE_DIRECTORY:
		case B_COMMON_PACKAGES_DIRECTORY:
		case B_COMMON_HEADERS_DIRECTORY:
		case B_COMMON_NONPACKAGED_DIRECTORY:
		case B_COMMON_NONPACKAGED_ADDONS_DIRECTORY:
		case B_COMMON_NONPACKAGED_TRANSLATORS_DIRECTORY:
		case B_COMMON_NONPACKAGED_MEDIA_NODES_DIRECTORY:
		case B_COMMON_NONPACKAGED_BIN_DIRECTORY:
		case B_COMMON_NONPACKAGED_DATA_DIRECTORY:
		case B_COMMON_NONPACKAGED_FONTS_DIRECTORY:
		case B_COMMON_NONPACKAGED_SOUNDS_DIRECTORY:
		case B_COMMON_NONPACKAGED_DOCUMENTATION_DIRECTORY:
		case B_COMMON_NONPACKAGED_LIB_DIRECTORY:
		case B_COMMON_NONPACKAGED_HEADERS_DIRECTORY:
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
			templatePath = kUserDirectories[which - B_USER_DIRECTORY];
			break;

		/* Global directories */
		case B_APPS_DIRECTORY:
			templatePath = "apps";
			break;
		case B_PREFERENCES_DIRECTORY:
			templatePath = "preferences";
			break;
		case B_UTILITIES_DIRECTORY:
			templatePath = "utilities";
			break;
		case B_PACKAGE_LINKS_DIRECTORY:
			templatePath = "package-links";
			break;

		default:
			free(buffer);
			return EINVAL;
	}

	err = B_OK;
	if (templatePath) {
		if (!strncmp(templatePath, "$h", 2)) {
			if (bootDevice > -1 && device != bootDevice) {
				int l = pathLength - strlen(buffer);
				if (l > 5)
					strncat(buffer, "/home", 5);
			} else {
#ifndef _KERNEL_MODE
#ifdef USE_PWENTS
				struct passwd pwBuffer;
				char pwStringBuffer[MAX_PASSWD_BUFFER_SIZE];
				struct passwd *pw;

				if (getpwuid_r(geteuid(), &pwBuffer, pwStringBuffer,
						sizeof(pwStringBuffer), &pw) == 0) {
					home = pw->pw_dir;
				}
#endif	// USE_PWENTS
				if (!home) {
					/* use env var */
					home = getenv("HOME");
				}
#endif	// !_KERNEL_MODE
				if (!home)
					home = kUserDirectory;
				strncpy(buffer, home, pathLength);
			}
			templatePath += 2;
		} else
			strlcat(buffer, "/", pathLength);

		if (!err && strlen(buffer) + 2 + strlen(templatePath)
				< (uint32)pathLength) {
			strcat(buffer, templatePath);
		} else
			err = err ? err : E2BIG;
	} else
		err = err ? err : ENOENT;

	if (!err && createIt && stat(buffer, &st) < 0)
		err = create_path(buffer, 0755);
	if (!err)
		strlcpy(returnedPath, buffer, pathLength);

	free(buffer);
	return err;
}

