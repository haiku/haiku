/*
 * Copyright 2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "package_support.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <AutoDeleter.h>
#include <boot/vfs.h>
#include <package/PackagesDirectoryDefs.h>


#define TRACE_PACKAGE_SUPPORT
#ifdef TRACE_PACKAGE_SUPPORT
#	define TRACE(...) dprintf(__VA_ARGS__)
#else
#	define TRACE(...) do {} while (false)
#endif

static const char* const kAdministrativeDirectory
	= PACKAGES_DIRECTORY_ADMIN_DIRECTORY;
static const char* const kActivatedPackagesFile
	= PACKAGES_DIRECTORY_ACTIVATION_FILE;


static inline bool
is_system_package(const char* name)
{
	// The name must end with ".hpkg".
	size_t nameLength = strlen(name);
	if (nameLength < 6 || strcmp(name + nameLength - 5, ".hpkg") != 0)
		return false;

	// The name must either be "haiku.hpkg" or start with "haiku-".
	return strcmp(name, "haiku.hpkg") == 0 || strncmp(name, "haiku-", 6) == 0;
}


// #pragma mark - PackageVolumeState


PackageVolumeState::PackageVolumeState()
	:
	fName(NULL),
	fDisplayName(NULL),
	fSystemPackage(NULL)
{
}


PackageVolumeState::~PackageVolumeState()
{
	Unset();
}


status_t
PackageVolumeState::SetTo(const char* stateName)
{
	Unset();

	if (stateName != NULL) {
		fName = strdup(stateName);
		if (fName == NULL)
			return B_NO_MEMORY;

		// Derive the display name from the directory name: Chop off the leading
		// "state_" and replace underscores by spaces.
		fDisplayName = strncmp(stateName, "state_", 6) == 0
			? strdup(stateName + 6) : strdup(stateName);
		if (fDisplayName == NULL)
			return B_NO_MEMORY;

		char* remainder = fDisplayName;
		while (char* underscore = strchr(remainder, '_')) {
			*underscore = ' ';
			remainder = underscore + 1;
		}
	}

	return B_OK;
}


void
PackageVolumeState::Unset()
{
	free(fName);
	fName = NULL;

	free(fDisplayName);
	fDisplayName = NULL;

	free(fSystemPackage);
	fSystemPackage = NULL;
}


const char*
PackageVolumeState::DisplayName() const
{
	return fDisplayName != NULL ? fDisplayName : "Latest state";
}


status_t
PackageVolumeState::SetSystemPackage(const char* package)
{
	if (fSystemPackage != NULL)
		free(fSystemPackage);

	fSystemPackage = strdup(package);
	return fSystemPackage != NULL ? B_OK : B_NO_MEMORY;
}


void
PackageVolumeState::GetPackagePath(const char* name, char* path,
	size_t pathSize)
{
	if (fName == NULL) {
		// the current state -- packages are directly in the packages directory
		strlcpy(path, name, pathSize);
	} else {
		// an old state
		snprintf(path, pathSize, "%s/%s/%s", kAdministrativeDirectory, fName,
			name);
	}
}


/*static*/ bool
PackageVolumeState::IsNewer(const PackageVolumeState* a,
	const PackageVolumeState* b)
{
	if (b->fName == NULL)
		return false;
	if (a->fName == NULL)
		return true;
	return strcmp(a->fName, b->fName) > 0;
}


// #pragma mark - PackageVolumeInfo


PackageVolumeInfo::PackageVolumeInfo()
	:
	BReferenceable(),
	fStates(),
	fPackagesDir(NULL)
{
}


PackageVolumeInfo::~PackageVolumeInfo()
{
	while (PackageVolumeState* state = fStates.RemoveHead())
		delete state;

	if (fPackagesDir != NULL)
		closedir(fPackagesDir);
}


status_t
PackageVolumeInfo::SetTo(Directory* baseDirectory, const char* packagesPath)
{
	TRACE("PackageVolumeInfo::SetTo()\n");

	if (fPackagesDir != NULL)
		closedir(fPackagesDir);

	// get the packages directory
	fPackagesDir = open_directory(baseDirectory, packagesPath);
	if (fPackagesDir == NULL) {
		TRACE("PackageVolumeInfo::SetTo(): failed to open packages directory: "
			"%s\n", strerror(errno));
		return errno;
	}

	Directory* packagesDirectory = directory_from(fPackagesDir);
	packagesDirectory->Acquire();

	// add the current state
	PackageVolumeState* state = _AddState(NULL);
	if (state == NULL)
		return B_NO_MEMORY;
	status_t error = _InitState(packagesDirectory, fPackagesDir, state);
	if (error != B_OK) {
		TRACE("PackageVolumeInfo::SetTo(): failed to init current state: "
			"%s\n", strerror(error));
		return error;
	}

	return B_OK;
}


status_t
PackageVolumeInfo::LoadOldStates()
{
	if (fPackagesDir == NULL) {
		TRACE("PackageVolumeInfo::LoadOldStates(): package directory is NULL");
		return B_ERROR;
	}

	Directory* packagesDirectory = directory_from(fPackagesDir);
	packagesDirectory->Acquire();

	if (DIR* administrativeDir = open_directory(packagesDirectory,
			kAdministrativeDirectory)) {
		while (dirent* entry = readdir(administrativeDir)) {
			if (strncmp(entry->d_name, "state_", 6) == 0) {
				TRACE("  old state directory \"%s\"\n", entry->d_name);
				_AddState(entry->d_name);
			}
		}

		closedir(administrativeDir);

		fStates.Sort(&PackageVolumeState::IsNewer);

		// initialize the old states
		PackageVolumeState* state = fStates.Head();
		status_t error;
		for (state = fStates.GetNext(state); state != NULL;) {
			PackageVolumeState* nextState = fStates.GetNext(state);
			if (state->Name()) {
				error = _InitState(packagesDirectory, fPackagesDir, state);
				if (error != B_OK) {
					TRACE("PackageVolumeInfo::LoadOldStates(): failed to "
						"init state \"%s\": %s\n", state->Name(),
						strerror(error));
					fStates.Remove(state);
					delete state;
				}
			}
			state = nextState;
		}
	} else {
		TRACE("PackageVolumeInfo::LoadOldStates(): failed to open "
			"administrative directory: %s\n", strerror(errno));
	}

	return B_OK;
}


PackageVolumeState*
PackageVolumeInfo::_AddState(const char* stateName)
{
	PackageVolumeState* state = new(std::nothrow) PackageVolumeState;
	if (state == NULL)
		return NULL;

	if (state->SetTo(stateName) != B_OK) {
		delete state;
		return NULL;
	}

	fStates.Add(state);
	return state;
}


status_t
PackageVolumeInfo::_InitState(Directory* packagesDirectory, DIR* dir,
	PackageVolumeState* state)
{
	// find the system package
	char systemPackageName[B_FILE_NAME_LENGTH];
	status_t error = _ParseActivatedPackagesFile(packagesDirectory, state,
		systemPackageName, sizeof(systemPackageName));
	if (error == B_OK) {
		// check, if package exists
		for (PackageVolumeState* otherState = state; otherState != NULL;
				otherState = fStates.GetPrevious(otherState)) {
			char packagePath[B_PATH_NAME_LENGTH];
			otherState->GetPackagePath(systemPackageName, packagePath,
				sizeof(packagePath));
			struct stat st;
			if (get_stat(packagesDirectory, packagePath, st) == B_OK
				&& S_ISREG(st.st_mode)) {
				state->SetSystemPackage(packagePath);
				break;
			}
		}
	} else {
		TRACE("PackageVolumeInfo::_InitState(): failed to parse "
			"activated-packages: %s\n", strerror(error));

		// No or invalid activated-packages file. That is OK for the current
		// state. We'll iterate through the packages directory to find the
		// system package. We don't do that for old states, though.
		if (state->Name() != NULL)
			return B_ENTRY_NOT_FOUND;

		while (dirent* entry = readdir(dir)) {
			// The name must end with ".hpkg".
			if (is_system_package(entry->d_name)) {
				state->SetSystemPackage(entry->d_name);
				break;
			}
		}
	}

	if (state->SystemPackage() == NULL)
		return B_ENTRY_NOT_FOUND;

	return B_OK;
}


status_t
PackageVolumeInfo::_ParseActivatedPackagesFile(Directory* packagesDirectory,
	PackageVolumeState* state, char* packageName, size_t packageNameSize)
{
	// open the activated-packages file
	char path[3 * B_FILE_NAME_LENGTH + 2];
	snprintf(path, sizeof(path), "%s/%s/%s",
		kAdministrativeDirectory, state->Name() != NULL ? state->Name() : "",
		kActivatedPackagesFile);
	int fd = open_from(packagesDirectory, path, O_RDONLY);
	if (fd < 0)
		return fd;
	FileDescriptorCloser fdCloser(fd);

	struct stat st;
	if (fstat(fd, &st) != 0)
		return errno;
	if (!S_ISREG(st.st_mode))
		return B_ENTRY_NOT_FOUND;

	// read the file until we find the system package line
	size_t remainingBytes = 0;
	for (;;) {
		ssize_t bytesRead = read(fd, path + remainingBytes,
			sizeof(path) - remainingBytes - 1);
		if (bytesRead <= 0)
			return B_ENTRY_NOT_FOUND;

		remainingBytes += bytesRead;
		path[remainingBytes] = '\0';

		char* line = path;
		while (char* lineEnd = strchr(line, '\n')) {
			*lineEnd = '\0';
			if (is_system_package(line)) {
				return strlcpy(packageName, line, packageNameSize)
						< packageNameSize
					?  B_OK : B_NAME_TOO_LONG;
			}

			line = lineEnd + 1;
		}

		// move the remainder to the start of the buffer
		if (line < path + remainingBytes) {
			size_t left = path + remainingBytes - line;
			memmove(path, line, left);
			remainingBytes = left;
		} else
			remainingBytes = 0;
	}

	return B_ENTRY_NOT_FOUND;
}
