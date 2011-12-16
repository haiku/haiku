/*
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002, Manuel J. Petit. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "runtime_loader_private.h"

#include <elf32.h>
#include <syscalls.h>
#include <user_runtime.h>

#include <directories.h>

#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <algorithm>


struct user_space_program_args *gProgramArgs;


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

	if (path != NULL)
		return path;

	// The environment variables may not have been set yet - in that case,
	// we're returning some useful defaults.
	// Since the kernel does not set any variables, this is also needed
	// to start the root shell.

	switch (type) {
		case B_APP_IMAGE:
			return kUserBinDirectory
						// TODO: Remove!
				":" kCommonBinDirectory
				":" kGlobalBinDirectory
				":" kAppsDirectory
				":" kPreferencesDirectory
				":" kSystemAppsDirectory
				":" kSystemPreferencesDirectory
				":" kCommonDevelopToolsBinDirectory;

		case B_LIBRARY_IMAGE:
			return kAppLocalLibDirectory
				":" kUserLibDirectory
					// TODO: Remove!
				":" kCommonLibDirectory
				":" kSystemLibDirectory;

		case B_ADD_ON_IMAGE:
			return kAppLocalAddonsDirectory
				":" kUserAddonsDirectory
					// TODO: Remove!
				":" kSystemAddonsDirectory;

		default:
			return NULL;
	}
}


static int
try_open_executable(const char *dir, int dirLength, const char *name,
	const char *programPath, const char *compatibilitySubDir, char *path,
	size_t pathLength)
{
	size_t nameLength = strlen(name);
	struct stat stat;
	status_t status;

	// construct the path
	if (dirLength > 0) {
		char *buffer = path;
		size_t subDirLen = 0;

		if (programPath == NULL)
			programPath = gProgramArgs->program_path;

		if (dirLength >= 2 && strncmp(dir, "%A", 2) == 0) {
			// Replace %A with current app folder path (of course,
			// this must be the first part of the path)
			char *lastSlash = strrchr(programPath, '/');
			int bytesCopied;

			// copy what's left (when the application name is removed)
			if (lastSlash != NULL) {
				strlcpy(buffer, programPath,
					std::min((long)pathLength, lastSlash + 1 - programPath));
			} else
				strlcpy(buffer, ".", pathLength);

			bytesCopied = strlen(buffer);
			buffer += bytesCopied;
			pathLength -= bytesCopied;
			dir += 2;
			dirLength -= 2;
		} else if (compatibilitySubDir != NULL) {
			// We're looking for a library or an add-on and the executable has
			// not been compiled with a compiler compatible with the one the
			// OS has been built with. Thus we only look in specific subdirs.
			subDirLen = strlen(compatibilitySubDir) + 1;
		}

		if (dirLength + 1 + subDirLen + nameLength >= pathLength)
			return B_NAME_TOO_LONG;

		memcpy(buffer, dir, dirLength);
		buffer[dirLength] = '/';
		if (subDirLen > 0) {
			memcpy(buffer + dirLength + 1, compatibilitySubDir, subDirLen - 1);
			buffer[dirLength + subDirLen] = '/';
		}
		strcpy(buffer + dirLength + 1 + subDirLen, name);
	} else {
		if (nameLength >= pathLength)
			return B_NAME_TOO_LONG;

		strcpy(path + dirLength + 1, name);
	}

	TRACE(("runtime_loader: try_open_container(): %s\n", path));

	// Test if the target is a symbolic link, and correct the path in this case

	status = _kern_read_stat(-1, path, false, &stat, sizeof(struct stat));
	if (status < B_OK)
		return status;

	if (S_ISLNK(stat.st_mode)) {
		char buffer[PATH_MAX];
		size_t length = PATH_MAX - 1;
		char *lastSlash;

		// it's a link, indeed
		status = _kern_read_link(-1, path, buffer, &length);
		if (status < B_OK)
			return status;
		buffer[length] = '\0';

		lastSlash = strrchr(path, '/');
		if (buffer[0] != '/' && lastSlash != NULL) {
			// relative path
			strlcpy(lastSlash + 1, buffer, lastSlash + 1 - path + pathLength);
		} else
			strlcpy(path, buffer, pathLength);
	}

	return _kern_open(-1, path, O_RDONLY, 0);
}


static int
search_executable_in_path_list(const char *name, const char *pathList,
	int pathListLen, const char *programPath, const char *compatibilitySubDir,
	char *pathBuffer, size_t pathBufferLength)
{
	const char *pathListEnd = pathList + pathListLen;
	status_t status = B_ENTRY_NOT_FOUND;

	TRACE(("runtime_loader: search_container_in_path_list() %s in %.*s\n", name,
		pathListLen, pathList));

	while (pathListLen > 0) {
		const char *pathEnd = pathList;
		int fd;

		// find the next ':' or run till the end of the string
		while (pathEnd < pathListEnd && *pathEnd != ':')
			pathEnd++;

		fd = try_open_executable(pathList, pathEnd - pathList, name,
			programPath, compatibilitySubDir, pathBuffer, pathBufferLength);
		if (fd >= 0) {
			// see if it's a dir
			struct stat stat;
			status = _kern_read_stat(fd, NULL, true, &stat, sizeof(struct stat));
			if (status == B_OK) {
				if (!S_ISDIR(stat.st_mode))
					return fd;
				status = B_IS_A_DIRECTORY;
			}
			_kern_close(fd);
		}

		pathListLen = pathListEnd - pathEnd - 1;
		pathList = pathEnd + 1;
	}

	return status;
}


int
open_executable(char *name, image_type type, const char *rpath,
	const char *programPath, const char *compatibilitySubDir)
{
	char buffer[PATH_MAX];
	int fd = B_ENTRY_NOT_FOUND;

	if (strchr(name, '/')) {
		// the name already contains a path, we don't have to search for it
		fd = _kern_open(-1, name, O_RDONLY, 0);
		if (fd >= 0 || type == B_APP_IMAGE)
			return fd;

		// can't search harder an absolute path add-on name!
		if (type == B_ADD_ON_IMAGE && name[0] == '/')
			return fd;

		// Even though ELF specs don't say this, we give shared libraries
		// and relative path based add-ons another chance and look
		// them up in the usual search paths - at
		// least that seems to be what BeOS does, and since it doesn't hurt...
		if (type == B_LIBRARY_IMAGE) {
			// For library (but not add-on), strip any path from name.
			// Relative path of add-on is kept.
			const char* paths = strrchr(name, '/') + 1;
			memmove(name, paths, strlen(paths) + 1);
		}
	}

	// try rpath (DT_RPATH)
	if (rpath != NULL) {
		// It consists of a colon-separated search path list. Optionally a
		// second search path list follows, separated from the first by a
		// semicolon.
		const char *semicolon = strchr(rpath, ';');
		const char *firstList = (semicolon ? rpath : NULL);
		const char *secondList = (semicolon ? semicolon + 1 : rpath);
			// If there is no ';', we set only secondList to simplify things.
		if (firstList) {
			fd = search_executable_in_path_list(name, firstList,
				semicolon - firstList, programPath, NULL, buffer,
				sizeof(buffer));
		}
		if (fd < 0) {
			fd = search_executable_in_path_list(name, secondList,
				strlen(secondList), programPath, NULL, buffer, sizeof(buffer));
		}
	}

	// If not found yet, let's evaluate the system path variables to find the
	// shared object.
	if (fd < 0) {
		if (const char *paths = search_path_for_type(type)) {
			fd = search_executable_in_path_list(name, paths, strlen(paths),
				programPath, compatibilitySubDir, buffer, sizeof(buffer));

			// If not found and a compatibility sub directory has been
			// specified, look again in the standard search paths.
			if (fd == B_ENTRY_NOT_FOUND && compatibilitySubDir != NULL) {
				fd = search_executable_in_path_list(name, paths, strlen(paths),
					programPath, NULL, buffer, sizeof(buffer));
			}
		}
	}

	if (fd >= 0) {
		// we found it, copy path!
		TRACE(("runtime_loader: open_executable(%s): found at %s\n", name, buffer));
		strlcpy(name, buffer, PATH_MAX);
	}

	return fd;
}


/*!
	Tests if there is an executable file at the provided path. It will
	also test if the file has a valid ELF header or is a shell script.
	Even if the runtime loader does not need to be able to deal with
	both types, the caller will give scripts a proper treatment.
*/
status_t
test_executable(const char *name, char *invoker)
{
	char path[B_PATH_NAME_LENGTH];
	char buffer[B_FILE_NAME_LENGTH];
		// must be large enough to hold the ELF header
	status_t status;
	ssize_t length;
	int fd;

	if (name == NULL)
		return B_BAD_VALUE;

	strlcpy(path, name, sizeof(path));

	fd = open_executable(path, B_APP_IMAGE, NULL, NULL, NULL);
	if (fd < B_OK)
		return fd;

	// see if it's executable at all
	status = _kern_access(-1, path, X_OK, false);
	if (status != B_OK)
		goto out;

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
			buffer[min_c((size_t)length, sizeof(buffer) - 1)] = '\0';

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
	} else if (status == B_OK) {
		struct Elf32_Ehdr *elfHeader = (struct Elf32_Ehdr *)buffer;
		if (elfHeader->e_entry == 0) {
			// we don't like to open shared libraries
			status = B_NOT_AN_EXECUTABLE;
		} else if (invoker)
			invoker[0] = '\0';
	}

out:
	_kern_close(fd);
	return status;
}


/*!
	This is the main entry point of the runtime loader as
	specified by its ld-script.
*/
int
runtime_loader(void *_args)
{
	void *entry = NULL;
	int returnCode;

	gProgramArgs = (struct user_space_program_args *)_args;

	// Relocate the args and env arrays -- they are organized in a contiguous
	// buffer which the kernel just copied into user space without adjusting the
	// pointers.
	{
		int32 i;
		addr_t relocationOffset = 0;

		if (gProgramArgs->arg_count > 0)
			relocationOffset = (addr_t)gProgramArgs->args[0];
		else if (gProgramArgs->env_count > 0)
			relocationOffset = (addr_t)gProgramArgs->env[0];

		// That's basically: <new buffer address> - <old buffer address>.
		// It looks a little complicated, since we don't have the latter one at
		// hand and thus need to reconstruct it (<first string pointer> -
		// <arguments + environment array sizes>).
		relocationOffset = (addr_t)gProgramArgs->args - relocationOffset
			+ (gProgramArgs->arg_count + gProgramArgs->env_count + 2)
				* sizeof(char*);

		for (i = 0; i < gProgramArgs->arg_count; i++)
			gProgramArgs->args[i] += relocationOffset;

		for (i = 0; i < gProgramArgs->env_count; i++)
			gProgramArgs->env[i] += relocationOffset;
	}

#if DEBUG_RLD
	close(0); open("/dev/console", 0); /* stdin   */
	close(1); open("/dev/console", 0); /* stdout  */
	close(2); open("/dev/console", 0); /* stderr  */
#endif

	if (heap_init() < B_OK)
		return 1;

	rldexport_init();
	rldelf_init();

	load_program(gProgramArgs->program_path, &entry);

	if (entry == NULL)
		return -1;

	// call the program entry point (usually _start())
	returnCode = ((int (*)(int, void *, void *))entry)(gProgramArgs->arg_count,
		gProgramArgs->args, gProgramArgs->env);

	terminate_program();

	return returnCode;
}
