/* 
** Copyright 2003, Daniel Reinhold, danielre@users.sf.net. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <SupportDefs.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/param.h>


/* the root directory in /dev where all tty devices reside */
#define DEV_TTY  "/dev/tt/"


/** return the full pathname of the tty device
 *	(NULL if not a tty or an error occurs)
 */

char *
ttyname(int fd)
{
	static char pathname[MAXPATHLEN];
	char  *stub = pathname + sizeof(DEV_TTY) - 1;
	int   stubLen = sizeof(pathname) - sizeof(DEV_TTY);
	struct stat fdStat;
	DIR   *dir;
	bool  found = false;

	// first, some sanity checks:
	if (!isatty(fd))
		return NULL;

	if (fstat(fd, &fdStat) < 0)
		return NULL;

	if (!S_ISCHR(fdStat.st_mode))
		return NULL;

	// start with the root tty directory at /dev
	strcpy(pathname, DEV_TTY);

	if ((dir = opendir(pathname)) != NULL) {
		// find a matching entry in the directory:
		//   we must match both the inode for the file 
		//   and the device that the file resides on
		struct dirent *e;
		struct stat entryStat;

		while ((e = readdir(dir)) != NULL) {
			// try to match the file's inode
			if (e->d_ino != fdStat.st_ino)
				continue;

			// construct the entry's full filename and
			// call stat() to retrieve the inode info
			strncpy(stub, e->d_name, stubLen);

			if (stat(pathname, &entryStat) < 0)
				continue;

			if (entryStat.st_ino == fdStat.st_ino &&
			    entryStat.st_dev == fdStat.st_dev) {
			    found = true;
			    break;
			}
		}

		closedir(dir);
	}

	return found ? pathname : NULL;
}
