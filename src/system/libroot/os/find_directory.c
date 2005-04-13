/*
 * Copyright (c) 2004, François Revol.
 * Distributed under the terms of the MIT license.
 */

// ToDo: that call should probably implemented by and in the kernel as well.

#include <FindDirectory.h>
#include <fs_info.h>
#include <syscalls.h>

#include <errno.h>
#include <pwd.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>


/* use pwents to find home */
#define USE_PWENTS

/* os root dir; just stick to 'beos' for now */
#define OSDIR "beos"
//#define OSDIR "haiku" // :)
//#define OSDIR "os"


/* /bin/strings rox */
static const char *os_dirs[] = {
OSDIR,
OSDIR"/system",
OSDIR"/system/add-ons",
OSDIR"/system/boot",
OSDIR"/etc/fonts",
OSDIR"/system/lib",
OSDIR"/system/servers",
OSDIR"/apps",
OSDIR"/bin",
OSDIR"/etc",
OSDIR"/documentation",
OSDIR"/preferences",
OSDIR"/system/add-ons/Translators",
OSDIR"/system/add-ons/media",
OSDIR"/etc/sounds",
};

/* R5 uses that, but it's *really* wrong, 
 * we don't want "common" directories to depend on user's home !
 * it should actually be something else than /boot/home even...
 */
//#define HOME "$h"
#define HOME "common"
	// ToDo: this is for now and might be changed back to "home"
	//	(or even something else) later

static const char *common_dirs[] = {
HOME"",
HOME"/config",
HOME"/config/add-ons",
HOME"/config/boot",
HOME"/config/fonts",
HOME"/config/lib",
HOME"/config/servers",
HOME"/config/bin",
HOME"/config/etc",
HOME"/config/documentation",
HOME"/config/settings",
"develop",
"var/log",
"var/spool",
"var/tmp",
"var",
HOME"/config/add-ons/Translators",
HOME"/config/add-ons/media",
HOME"/config/sounds",
};

#undef HOME
#define HOME "$h"

static const char *user_dirs[] = {
HOME"",
HOME"/config",
HOME"/config/add-ons",
HOME"/config/boot",
HOME"/config/fonts",
HOME"/config/lib",
HOME"/config/settings",
HOME"/config/be",
HOME"/config/settings/printers",
HOME"/config/add-ons/Translators",
HOME"/config/add-ons/media",
HOME"/config/sounds",
};

/*
utilities
preferences
apps
*/

/** make dir and parents if needed */

static int
mkdir_p(const char *path, mode_t mode)
{
	char buffer[B_PATH_NAME_LENGTH + 1];
	int slash = 0;
	int plen = 0;
	char *p;
	struct stat st;

	if (!path || ((plen = strlen(path)) > B_PATH_NAME_LENGTH))
		return EINVAL;

	memset(buffer, 0, B_PATH_NAME_LENGTH + 1);
	errno = 0;

	while (1) {
		slash++;
		if (slash > plen)
			return errno;
		p = strchr(&path[slash], '/');
		if (!p)
			slash = plen;
		else if (slash != p - path)
			slash = p - path;
		else
			continue;

		strncpy(buffer, path, slash);
		if (stat(buffer, &st) < 0) {
			errno = 0;
			if (mkdir(buffer, mode) < 0)
				return errno;
		}
	}
	return 0;
	
}


status_t
find_directory(directory_which which, dev_t device, bool create_it,
	char *returned_path, int32 path_length)
{
	status_t err = B_OK;
	dev_t bootdev = -1;
	struct fs_info finfo;
	struct stat st;
	char *buffer = NULL;
	char *home = NULL;
	const char *template = NULL;

	/* as with the R5 version, no on-stack buffer */
	buffer = (char *)malloc(path_length);
	memset(buffer, 0, path_length);

	/* fiddle with non-boot volume for items that need it */
	switch (which) {
		case B_DESKTOP_DIRECTORY:
		case B_TRASH_DIRECTORY:
			bootdev = dev_for_path("/boot");
			if (device <= 0)
				device = bootdev;
			if (fs_stat_dev(device, &finfo) < B_OK) {
				free(buffer);
				return ENODEV;
			}
			if (device != bootdev) {
				err = _kern_entry_ref_to_path(device, finfo.root, /*"."*/ NULL, buffer, path_length);
			} else {
				/* use the user id to find the home folder */
				/* done later */
				strncat(buffer, "/boot", path_length);
			}
			break;
		default:
			strncat(buffer, "/boot", path_length);
			break;
	}

	if (err < B_OK) {
		free(buffer);
		return err;
	}
	
	switch (which) {
		case B_DESKTOP_DIRECTORY:
			if (!strcmp(finfo.fsh_name, "bfs"))
				template = "$h/Desktop";
			break;
		case B_TRASH_DIRECTORY:
			if (!strcmp(finfo.fsh_name, "bfs"))
				template = "$h/Desktop/Trash";
			else if (!strcmp(finfo.fsh_name, "dos"))
				template = "RECYCLED/_BEOS_";
			break;
		case B_BEOS_DIRECTORY:
		case B_BEOS_SYSTEM_DIRECTORY:
		case B_BEOS_ADDONS_DIRECTORY:
		case B_BEOS_BOOT_DIRECTORY:
		case B_BEOS_FONTS_DIRECTORY:
		case B_BEOS_LIB_DIRECTORY:
		case B_BEOS_SERVERS_DIRECTORY:
		case B_BEOS_APPS_DIRECTORY:
		case B_BEOS_BIN_DIRECTORY:
		case B_BEOS_ETC_DIRECTORY:
		case B_BEOS_DOCUMENTATION_DIRECTORY:
		case B_BEOS_PREFERENCES_DIRECTORY:
		case B_BEOS_TRANSLATORS_DIRECTORY:
		case B_BEOS_MEDIA_NODES_DIRECTORY:
		case B_BEOS_SOUNDS_DIRECTORY:
			template = os_dirs[which - B_BEOS_DIRECTORY];
			break;
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
			template = common_dirs[which - B_COMMON_DIRECTORY];
			break;
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
			template = user_dirs[which - B_USER_DIRECTORY];
			break;
		case B_APPS_DIRECTORY:
			template = "apps";
			break;
		case B_PREFERENCES_DIRECTORY:
			template = "preferences";
			break;
		case B_UTILITIES_DIRECTORY:
			template = "utilities";
			break;
		default:
			free(buffer);
			return EINVAL;
	}
	
	err = B_OK;
	if (template) {
		if (!strncmp(template, "$h", 2)) {
			if ((bootdev > -1) && (device != bootdev)) {
				int l = path_length - strlen(buffer);
				if (l > 5)
					strncat(buffer, "/home", 5);
			} else {
#ifdef USE_PWENTS
				struct passwd *pw;
				// WARNING getpwuid() might not be threadsafe...
				pw = getpwuid(getuid());
				if (pw)
					home = pw->pw_dir;
#endif
				if (!home) {
					/* use env var */
					home = getenv("HOME");
				}
				if (!home)
					home = "/boot/home";
				strncpy(buffer, home, path_length);
			}
			template += 2;
		} else
			strcat(buffer, "/");

		if (!err && strlen(buffer) + 2 + strlen(template) < (uint32)path_length) {
			strcat(buffer, template);
		} else
			err = err ? err : E2BIG;
	} else
		err = err ? err : ENOENT;

	if (!err && create_it && stat(buffer, &st) < 0)
		err = mkdir_p(buffer, 0755);
	if (!err)
		strncpy(returned_path, buffer, path_length);

	free(buffer);
	return err;
}

