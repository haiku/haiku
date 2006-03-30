/*
 * Copyright 2005-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002, Manuel J. Petit. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "runtime_loader_private.h"

#include <syscalls.h>
#include <user_runtime.h>

#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>


struct uspace_program_args *gProgramArgs;


static const char *
search_path_for_type(image_type type)
{
	const char *path = NULL;

	switch (type) {
		case B_APP_IMAGE:
			path = getenv("PATH");
			break;
		case B_LIBRARY_IMAGE:
			path = getenv("LIBRARY_PATH");
			break;
		case B_ADD_ON_IMAGE:
			path = getenv("ADDON_PATH");
			break;

		default:
			return NULL;
	}

	// ToDo: for now, if the variable was not set, return default paths
	if (path != NULL)
		return path;

	switch (type) {
		case B_APP_IMAGE:
			return "/boot/home/config/bin:"
				"/bin:"
				"/boot/apps:"
				"/boot/preferences:"
				"/boot/beos/apps:"
				"/boot/beos/preferences:"
				"/boot/develop/tools/gnupro/bin";

		case B_LIBRARY_IMAGE:
			return "%A/lib:/boot/home/config/lib:/boot/beos/system/lib";

		case B_ADD_ON_IMAGE:
			return "%A/add-ons"
				":/boot/home/config/add-ons"
				":/boot/beos/system/add-ons";

		default:
			return NULL;
	}
}


static int
try_open_executable(const char *dir, int dirLen, const char *name, char *path,
	int pathLen)
{
	int nameLen = strlen(name);

	// construct the path
	if (dirLen > 0) {
		char *buffer = path;

		if (dirLen >= 2 && strncmp(dir, "%A", 2) == 0) {
			// Replace %A with current app folder path (of course,
			// this must be the first part of the path)
			// ToDo: Maybe using first image info is better suited than
			// gProgamArgs->program_path here?
			char *lastSlash = strrchr(gProgramArgs->program_path, '/');
			int bytesCopied;

			// copy what's left (when the application name is removed)
			if (lastSlash != NULL) {
				strlcpy(buffer, gProgramArgs->program_path,
					min(pathLen, lastSlash + 1 - gProgramArgs->program_path));
			} else
				strlcpy(buffer, ".", pathLen);

			bytesCopied = strlen(buffer);
			buffer += bytesCopied;
			pathLen -= bytesCopied;
			dir += 2;
			dirLen -= 2;
		}

		if (dirLen + 1 + nameLen >= pathLen)
			return B_NAME_TOO_LONG;

		memcpy(buffer, dir, dirLen);
		buffer[dirLen] = '/';
		strcpy(buffer + dirLen + 1, name);
	} else {
		if (nameLen >= pathLen)
			return B_NAME_TOO_LONG;

		strcpy(path + dirLen + 1, name);
	}

	//TRACE(("rld.so: try_open_container(): %s\n", path));

	return _kern_open(-1, path, O_RDONLY, 0);
}


static int
search_executable_in_path_list(const char *name, const char *pathList,
	int pathListLen, char *pathBuffer, int pathBufferLen)
{
	const char *pathListEnd = pathList + pathListLen;

	//TRACE(("rld.so: search_container_in_path_list() %s in %.*s\n", name,
	//	pathListLen, pathList));

	while (pathListLen > 0) {
		const char *pathEnd = pathList;
		int fd;

		// find the next ':' or run till the end of the string
		while (pathEnd < pathListEnd && *pathEnd != ':')
			pathEnd++;

		fd = try_open_executable(pathList, pathEnd - pathList, name, pathBuffer,
			pathBufferLen);
		if (fd >= 0)
			return fd;

		pathListLen = pathListEnd - pathEnd - 1;
		pathList = pathEnd + 1;
	}

	return B_ENTRY_NOT_FOUND;
}


int
open_executable(char *name, image_type type, const char *rpath)
{
	const char *paths;
	char buffer[PATH_MAX];
	int fd = -1;

	if (strchr(name, '/')) {
		// the name already contains a path, we don't have to search for it
		return _kern_open(-1, name, O_RDONLY, 0);
	}

	// first try rpath (DT_RPATH)
	if (rpath) {
		// It consists of a colon-separated search path list. Optionally it
		// follows a second search path list, separated from the first by a
		// semicolon.
		const char *semicolon = strchr(rpath, ';');
		const char *firstList = (semicolon ? rpath : NULL);
		const char *secondList = (semicolon ? semicolon + 1 : rpath);
			// If there is no ';', we set only secondList to simplify things.
		if (firstList) {
			fd = search_executable_in_path_list(name, firstList,
				semicolon - firstList, buffer, sizeof(buffer));
		}
		if (fd < 0) {
			fd = search_executable_in_path_list(name, secondList,
				strlen(secondList), buffer, sizeof(buffer));
		}
	}

	// let's evaluate the system path variables to find the container
	if (fd < 0) {
		paths = search_path_for_type(type);
		if (paths) {
			fd = search_executable_in_path_list(name, paths, strlen(paths),
				buffer, sizeof(buffer));
		}
	}

	if (fd >= 0) {
		// we found it, copy path!
		//TRACE(("rld.so: open_container(%s): found at %s\n", name, buffer));
		strlcpy(name, buffer, PATH_MAX);
		return fd;
	}

	return B_ENTRY_NOT_FOUND;
}


/** Tests if there is an executable file at the provided path. It will
 *	also test if the file has a valid ELF header or is a shell script.
 *	Even if the runtime loader does not need to be able to deal with
 *	both types, the caller will give scripts a proper treatment.
 */

status_t
test_executable(const char *name, uid_t user, gid_t group, char *invoker)
{
	char path[B_PATH_NAME_LENGTH];
	char buffer[B_FILE_NAME_LENGTH];
		// must be large enough to hold the ELF header
	struct stat stat;
	status_t status;
	ssize_t length;
	int fd;

	strlcpy(path, name, sizeof(path));

	fd = open_executable(path, B_APP_IMAGE, NULL);
	if (fd < B_OK)
		return fd;

	// see if it's executable at all

	status = _kern_read_stat(fd, NULL, true, &stat, sizeof(struct stat));
	if (status != B_OK)
		goto out;

	// shift mode bits, to check directly against accessMode
	if (user == stat.st_uid)
		stat.st_mode >>= 6;
	else if (group == stat.st_gid)
		stat.st_mode >>= 3;

	if (~(stat.st_mode & S_IRWXO) & X_OK) {
		status = B_NOT_ALLOWED;
		goto out;
	}

	// read and verify the ELF header

	length = _kern_read(fd, 0, buffer, sizeof(buffer));
	if (length < 0) {
		status = length;
		goto out;
	}

	status = elf_verify_header(buffer, length);
	if (status == B_NOT_AN_EXECUTABLE) {
		// test for shell scripts
		if (!strncmp(buffer, "#!", 2)) {
			char *end;
			buffer[length - 1] = '\0';

			end = strchr(buffer, '\n');
			if (end == NULL) {
				status = E2BIG;
				goto out;
			} else
				end[0] = '\0';

			if (invoker)
				strcpy(invoker, buffer + 2);

			status = B_OK;
		}
	} else if (status == B_OK && invoker)
		invoker[0] = '\0';

out:
	_kern_close(fd);
	return status;
}


/** This is the main entry point of the runtime loader as
 *	specified by its ld-script.
 */

int
runtime_loader(void *_args)
{
	void *entry = NULL;
	int returnCode;

	gProgramArgs = (struct uspace_program_args *)_args;

#if DEBUG_RLD
	close(0); open("/dev/console", 0); /* stdin   */
	close(1); open("/dev/console", 0); /* stdout  */
	close(2); open("/dev/console", 0); /* stderr  */
#endif

	rldheap_init();
	rldexport_init();
	rldelf_init();

	load_program(gProgramArgs->program_path, &entry);

	if (entry == NULL)
		return -1;

	// call the program entry point (usually _start())
	returnCode = ((int (*)(int, void *, void *))entry)(gProgramArgs->argc,
		gProgramArgs->argv, gProgramArgs->envp);

	terminate_program();

	return returnCode;
}
