/*
 * find_directory() replacement
 * (c) 2004, François Revol.
 */

#include <FindDirectory.h>
#include <StorageDefs.h>
#include <fs_info.h>
#include <errno.h>
#include <pwd.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>

/* use R5 undocumented syscalls to find mountpoints */
//#define USE_R5_SYSCALL_HACK

/* use pwents to find home */
#define USE_PWENTS

#ifdef COMPILE_FOR_R5
#define USE_R5_SYSCALL_HACK
#undef USE_PWENTS
#endif


#ifdef USE_R5_SYSCALL_HACK
// un* are unknown params
extern int _kopen_vn_(dev_t d, ino_t i, long un1, long un2, long un3);
extern int _kopendir_(int fromfd, long un1, long un2);
extern status_t _kreaddir_(int dir, struct dirent *dent, size_t dentlen, long count);
extern status_t _kclosedir_(int dir);
#endif

/* /bin/strings rox */
static const char *beos_dirs[] = {
"beos",
"beos/system",
"beos/system/add-ons",
"beos/system/boot",
"beos/etc/fonts",
"beos/system/lib",
"beos/system/servers",
"beos/apps",
"beos/bin",
"beos/etc",
"beos/documentation",
"beos/preferences",
"beos/system/add-ons/Translators",
"beos/system/add-ons/media",
"beos/etc/sounds",
};

/* R5 uses that, but it's *really* wrong, 
 * we don't want "common" directories to depend on user's home ! */
//#define HOME "$h"
#define HOME "home"

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

/* make dir and parents if needed */
static int mkdir_p(const char *path, mode_t mode)
{
	char buffer[B_PATH_NAME_LENGTH+1];
	int slash = 0;
	int plen = 0;
	char *p;
	struct stat st;

//return 0;
	if (!path || ((plen=strlen(path)) > B_PATH_NAME_LENGTH))
		return EINVAL;
	memset(buffer, 0, B_PATH_NAME_LENGTH+1);
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
//printf("path[0:%d] = %s\n", slash, buffer);
		if (stat(buffer, &st) < 0) {
//perror("stat");
			errno = 0;
			if (mkdir(buffer, mode) < 0) {
//perror("mkdir");
				return errno;
			}
		}
	}
	return 0;
	
}

status_t find_directory (directory_which which, dev_t device, bool create_it, char *returned_path, int32 path_length)
{
	status_t err;
	dev_t bootdev = -1;
	struct fs_info finfo;
	struct stat st;
	char *buffer;
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
#ifdef USE_R5_SYSCALL_HACK
			int fd;
			int dir;
			/* do weird stuff to find the path to the mountpoint of the device
			 * basically:
			 * - open the mountpoint,
			 * - get a DIR* from it,
			 * - read the first entry (.),
			 * - and get the path from the dirent.
			 */
			fd = _kopen_vn_(device, finfo.root, 0, 0, 1);
			if (fd >= 0) {
				dir = _kopendir_(fd, 0, 1);
				close(fd);
				if (dir) {
					status_t err;
					struct {
						struct dirent d;
						char n[B_FILE_NAME_LENGTH];
					} dent;
					// NOT threadsafe readdir(dir);
					err = _kreaddir_(dir, &dent.d, sizeof(dent), 1);
					_kclosedir_(dir);
					if (err>0) {
						err = get_path_for_dirent(&dent, buffer, path_length);
						if (err >= 0) {
							int i = strlen(buffer);
							if ((i > 2) && !strcmp(&buffer[i-2], "/."))
								buffer[i-2] = '\0';
							err = 0;
						}
					} else
						err = ENODEV;
				} else
					err = ENODEV;
			} else
				err = ENODEV;
#else
			strncat(buffer, "/wincrash", 9); // XXX: TODO
#endif
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
		template = beos_dirs[which - B_BEOS_DIRECTORY];
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
	
	//printf("template = '%s'\nbuffer = '%s'\n", template, buffer);
	
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
				if (pw) {
					home = pw->pw_dir;
//printf("getpwuid %d %s\n", getuid(), home);
				}
#endif
				if (!home) {
					/* use env var */
					home = getenv("HOME");
//printf("getenv %s\n", home);
				}
				if (!home)
					home = "/boot/home";
				strncpy(buffer, home, path_length);
			}
			template += 2;
		} else
			strcat(buffer, "/");
		if (!err && strlen(buffer)+2+strlen(template) < path_length) {
			//strcat(buffer, "/");
			strcat(buffer, template);
		} else
			err = err?err:E2BIG;
	} else
		err = err?err:ENOENT;
	if (!err && create_it && (stat(buffer, &st) < 0))
		err = mkdir_p(buffer, 0755);
	if (!err)
		strncpy(returned_path, buffer, path_length);
	free(buffer);
	return err;
}

#if 0
int main(int argc, char **argv)
{
	printf("mkdir %d\n", mkdir_p(argv[1], 0755));
	perror("mkdir");
}
#endif
