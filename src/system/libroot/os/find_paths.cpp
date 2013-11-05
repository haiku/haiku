/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <find_directory_private.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <fs_attr.h>

#include <AutoDeleter.h>
#include <syscalls.h>


static const char* const kInstallationLocations[] = {
	"/boot/home/config/non-packaged",
	"/boot/home/config",
	"/boot/system/non-packaged",
	"/boot/system",
};

static size_t kHomeInstallationLocationIndex = 1;
static size_t kInstallationLocationCount
	= sizeof(kInstallationLocations) / sizeof(kInstallationLocations[0]);


static const char*
get_relative_directory_path(size_t installationLocationIndex,
	path_base_directory baseDirectory)
{
	switch (baseDirectory) {
		case B_FIND_PATH_INSTALLATION_LOCATION_DIRECTORY:
			return "";
		case B_FIND_PATH_ADD_ONS_DIRECTORY:
			return "/add-ons";
		case B_FIND_PATH_APPS_DIRECTORY:
			return "/apps";
		case B_FIND_PATH_BIN_DIRECTORY:
			return "/bin";
		case B_FIND_PATH_BOOT_DIRECTORY:
			return "/boot";
		case B_FIND_PATH_CACHE_DIRECTORY:
			return "/cache";
		case B_FIND_PATH_DATA_DIRECTORY:
			return "/data";
		case B_FIND_PATH_DEVELOP_DIRECTORY:
			return "/develop";
		case B_FIND_PATH_DEVELOP_LIB_DIRECTORY:
			return "/develop/lib";
		case B_FIND_PATH_DOCUMENTATION_DIRECTORY:
			return "/documentation";
		case B_FIND_PATH_ETC_DIRECTORY:
			return "/settings/etc";
		case B_FIND_PATH_FONTS_DIRECTORY:
			return "/data/fonts";
		case B_FIND_PATH_HEADERS_DIRECTORY:
			return "/develop/headers";
		case B_FIND_PATH_LIB_DIRECTORY:
			return "/lib";
		case B_FIND_PATH_LOG_DIRECTORY:
			return "/log";
		case B_FIND_PATH_MEDIA_NODES_DIRECTORY:
			return "/add-ons/media";
		case B_FIND_PATH_PACKAGES_DIRECTORY:
			return "/packages";
		case B_FIND_PATH_PREFERENCES_DIRECTORY:
			return "/preferences";
		case B_FIND_PATH_SERVERS_DIRECTORY:
			return "/servers";
		case B_FIND_PATH_SETTINGS_DIRECTORY:
			return installationLocationIndex == kHomeInstallationLocationIndex
				? "/settings/global" : "/settings";
		case B_FIND_PATH_SOUNDS_DIRECTORY:
			return "/data/sounds";
		case B_FIND_PATH_SPOOL_DIRECTORY:
			return "/var/spool";
		case B_FIND_PATH_TRANSLATORS_DIRECTORY:
			return "/add-ons/Translators";
		case B_FIND_PATH_VAR_DIRECTORY:
			return "/var";

		case B_FIND_PATH_IMAGE_PATH:
		case B_FIND_PATH_IMAGE_PACKAGE_PATH:
		default:
			return NULL;
	}
}


static status_t
create_directory(char* path)
{
	// find the first directory that doesn't exist
	char* slash = path;
	bool found = false;
	while (!found && (slash = strchr(slash + 1, '/')) != NULL) {
		*slash = '\0';
		struct stat st;
		if (lstat(path, &st) != 0)
			break;
		*slash = '/';
	}

	if (found)
		return B_OK;

	// create directories
	while (slash != NULL) {
		*slash = '\0';
		bool created = mkdir(path, 0755);
		*slash = '/';

		if (!created)
			return errno;

		slash = strchr(slash + 1, '/');
	}

	return B_OK;
}


static bool
is_in_range(const void* pointer, const void* base, size_t size)
{
	return pointer >= base && (addr_t)pointer < (addr_t)base + size;
}


static status_t
find_image(const void* codePointer, image_info& _info)
{
	int32 cookie = 0;

	while (get_next_image_info(B_CURRENT_TEAM, &cookie, &_info) == B_OK) {
		if (codePointer == NULL ? _info.type == B_APP_IMAGE
				: (is_in_range(codePointer, _info.text, _info.text_size)
					|| is_in_range(codePointer, _info.data, _info.data_size))) {
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


static status_t
copy_path(const char* path, char* buffer, size_t bufferSize)
{
	if (strlcpy(buffer, path, bufferSize) >= bufferSize)
		return B_BUFFER_OVERFLOW;
	return B_OK;
}


static status_t
normalize_path(const char* path, char* buffer, size_t bufferSize)
{
	status_t error;
	if (bufferSize >= B_PATH_NAME_LENGTH) {
		error = _kern_normalize_path(path, true, buffer);
	} else {
		char normalizedPath[B_PATH_NAME_LENGTH];
		error = _kern_normalize_path(path, true, normalizedPath);
		if (error == B_OK)
			error = copy_path(path, buffer, bufferSize);
	}

	if (error != B_OK)
		return error;

	// make sure the path exists
	struct stat st;
	if (lstat(buffer, &st) != 0)
		return errno;

	return B_OK;
}


static const char*
get_installation_location(const char* path, size_t& _index)
{
	for (size_t i = 0; i < kInstallationLocationCount; i++) {
		size_t length = strlen(kInstallationLocations[i]);
		if (strncmp(path, kInstallationLocations[i], length) == 0
			&& (path[length] == '/' || path[length] == '\0')) {
			_index = i;
			return kInstallationLocations[i];
		}
	}

	return NULL;
}


static status_t
get_file_attribute(const char* path, const char* attribute, char* nameBuffer,
	size_t bufferSize)
{
	int fd = fs_open_attr(path, attribute, B_STRING_TYPE, O_RDONLY);
	if (fd < 0)
		return errno;

	status_t error = B_OK;
	ssize_t bytesRead = read(fd, nameBuffer, bufferSize - 1);
	if (bytesRead < 0)
		error = bytesRead;
	else if (bytesRead == 0)
		error = B_ENTRY_NOT_FOUND;
	else
		nameBuffer[bytesRead] = '\0';

	fs_close_attr(fd);

	return error;
}


static status_t
normalize_dependency(const char* dependency, char* buffer, size_t bufferSize)
{
	if (strlcpy(buffer, dependency, bufferSize) >= bufferSize)
		return B_NAME_TOO_LONG;

	// replace all ':' with '~'
	char* colon = buffer - 1;
	while ((colon = strchr(colon + 1, ':')) != NULL)
		*colon = '~';

	return B_OK;
}


static ssize_t
process_path(const char* installationLocation, const char* relativePath,
	const char* subPath, uint32 flags, char* pathBuffer, size_t bufferSize)
{
	size_t totalLength;
	if (subPath != NULL) {
		totalLength = snprintf(pathBuffer, bufferSize, "%s%s/%s",
			installationLocation, relativePath, subPath);
	} else {
		totalLength = snprintf(pathBuffer, bufferSize, "%s%s",
			installationLocation, relativePath);
	}

	if (totalLength >= bufferSize)
		return B_BUFFER_OVERFLOW;

	char* path = pathBuffer;

	status_t error = B_OK;
	if ((flags & B_FIND_PATH_CREATE_DIRECTORY) != 0) {
		// create the directory
		error = create_directory(path);
	} else if ((flags & B_FIND_PATH_CREATE_PARENT_DIRECTORY) != 0) {
		// create the parent directory
		char* lastSlash = strrchr(path, '/');
		*lastSlash = '\0';
		error = create_directory(path);
		*lastSlash = '/';
	}

	if (error != B_OK)
		return error;

	if ((flags & B_FIND_PATH_EXISTING_ONLY) != 0) {
		// check if the entry exists
		struct stat st;
		if (lstat(path, &st) != 0)
			return 0;
	}

	return totalLength + 1;
}


status_t
internal_path_for_path(char* referencePath, size_t referencePathSize,
	const char* dependency, path_base_directory baseDirectory,
	const char* subPath, uint32 flags, char* pathBuffer, size_t bufferSize)
{
	// normalize
	status_t error = normalize_path(referencePath, referencePath,
		referencePathSize);
	if (error != B_OK)
		return error;

	// handle B_FIND_PATH_IMAGE_PATH
	if (baseDirectory == B_FIND_PATH_IMAGE_PATH)
		return copy_path(referencePath, pathBuffer, bufferSize);

	// get the installation location
	size_t installationLocationIndex;
	const char* installationLocation = get_installation_location(referencePath,
		installationLocationIndex);
	if (installationLocation == NULL)
		return B_ENTRY_NOT_FOUND;

	// Handle B_FIND_PATH_IMAGE_PACKAGE_PATH: get the package file name and
	// simply adjust our arguments to look the package file up in the packages
	// directory.
	char packageName[B_FILE_NAME_LENGTH];
	if (baseDirectory == B_FIND_PATH_IMAGE_PACKAGE_PATH) {
		error = get_file_attribute(referencePath, "SYS:PACKAGE_FILE",
			packageName, sizeof(packageName));
		if (error != B_OK)
			return error;

		dependency = NULL;
		subPath = packageName;
		baseDirectory = B_FIND_PATH_PACKAGES_DIRECTORY;
		flags = B_FIND_PATH_EXISTING_ONLY;
	}

	// resolve dependency
	if (dependency != NULL) {
		// get the versioned package name
		error = get_file_attribute(referencePath, "SYS:PACKAGE",
			packageName, sizeof(packageName));
		if (error != B_OK)
			return error;

		// normalize the dependency name
		char normalizedDependency[B_FILE_NAME_LENGTH];
		error = normalize_dependency(dependency, normalizedDependency,
			sizeof(normalizedDependency));
		if (error != B_OK)
			return error;

		// Compute the path of the dependency symlink and normalize it. This
		// should yield the installation location path.
		if (snprintf(referencePath, referencePathSize, "/packages/%s/%s",
					packageName, normalizedDependency)
				>= (ssize_t)referencePathSize) {
			return B_BUFFER_OVERFLOW;
		}

		error = normalize_path(referencePath, referencePath, referencePathSize);
		if (error != B_OK)
			return error;

		// get the installation location
		installationLocation = get_installation_location(referencePath,
			installationLocationIndex);
		if (installationLocation == NULL)
			return B_ENTRY_NOT_FOUND;
	}

	// get base dir and process the path
	const char* relativePath = get_relative_directory_path(
		installationLocationIndex, baseDirectory);
	if (relativePath == NULL)
		return B_BAD_VALUE;

	ssize_t pathSize = process_path(installationLocation, relativePath, subPath,
		flags, pathBuffer, bufferSize);
	if (pathSize <= 0)
		return pathSize == 0 ? B_ENTRY_NOT_FOUND : pathSize;
	return B_OK;
}


// #pragma mark -


status_t
__find_path(const void* codePointer, const char* dependency,
	path_base_directory baseDirectory, const char* subPath, uint32 flags,
	char* pathBuffer, size_t bufferSize)
{
	if (pathBuffer == NULL)
		return B_BAD_VALUE;

	// resolve codePointer to image info
	image_info imageInfo;
	status_t error = find_image(codePointer, imageInfo);
	if (error != B_OK)
		return error;

	return internal_path_for_path(imageInfo.name, sizeof(imageInfo.name),
		dependency, baseDirectory, subPath, flags, pathBuffer, bufferSize);
}


status_t
__find_path_for_path(const char* path, const char* dependency,
	path_base_directory baseDirectory, const char* subPath, uint32 flags,
	char* pathBuffer, size_t bufferSize)
{
	char referencePath[B_PATH_NAME_LENGTH];
	if (strlcpy(referencePath, path, sizeof(referencePath))
			>= sizeof(referencePath)) {
		return B_NAME_TOO_LONG;
	}

	return internal_path_for_path(referencePath, sizeof(referencePath),
		dependency, baseDirectory, subPath, flags, pathBuffer, bufferSize);
}


status_t
__find_paths(path_base_directory baseDirectory, const char* subPath,
	uint32 flags, char*** _paths, size_t* _pathCount)
{
	if (_paths == NULL || _pathCount == NULL)
		return B_BAD_VALUE;

	size_t subPathLength = subPath != NULL ? strlen(subPath) + 1 : 0;

	// Get the relative paths and compute the total size to allocate.
	const char* relativePaths[kInstallationLocationCount];
	size_t totalSize = 0;

	for (size_t i = 0; i < kInstallationLocationCount; i++) {
		relativePaths[i] = get_relative_directory_path(i, baseDirectory);
		if (relativePaths[i] == NULL)
			return B_BAD_VALUE;

		totalSize += strlen(kInstallationLocations[i])
			+ strlen(relativePaths[i]) + subPathLength + 1;
	}

	// allocate storage
	char** paths = (char**)malloc(sizeof(char*) * kInstallationLocationCount
		+ totalSize);
	if (paths == NULL)
		return B_NO_MEMORY;
	MemoryDeleter pathsDeleter(paths);

	// construct and process the paths
	size_t count = 0;
	char* pathBuffer = (char*)(paths + kInstallationLocationCount);
	const char* pathBufferEnd = pathBuffer + totalSize;
	for (size_t i = 0; i < kInstallationLocationCount; i++) {
		ssize_t pathSize = process_path(kInstallationLocations[i],
			relativePaths[i], subPath, flags, pathBuffer,
			pathBufferEnd - pathBuffer);
		if (pathSize < 0)
			return pathSize;
		if (pathSize > 0) {
			paths[count++] = pathBuffer;
			pathBuffer += pathSize;
		}
	}

	if (count == 0)
		return B_ENTRY_NOT_FOUND;

	*_paths = paths;
	*_pathCount = count;
	pathsDeleter.Detach();

	return B_OK;
}


B_DEFINE_WEAK_ALIAS(__find_path, find_path);
B_DEFINE_WEAK_ALIAS(__find_path_for_path, find_path_for_path);
B_DEFINE_WEAK_ALIAS(__find_paths, find_paths);
