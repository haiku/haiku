/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <find_directory_private.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <algorithm>

#include <fs_attr.h>

#include <architecture_private.h>
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

static const path_base_directory kArchitectureSpecificBaseDirectories[] = {
	B_FIND_PATH_ADD_ONS_DIRECTORY,
	B_FIND_PATH_BIN_DIRECTORY,
	B_FIND_PATH_DEVELOP_LIB_DIRECTORY,
	B_FIND_PATH_HEADERS_DIRECTORY,
};

static size_t kArchitectureSpecificBaseDirectoryCount =
	sizeof(kArchitectureSpecificBaseDirectories)
		/ sizeof(kArchitectureSpecificBaseDirectories[0]);


namespace {

struct PathBuffer {
	PathBuffer(char* buffer, size_t size)
		:
		fBuffer(buffer),
		fSize(size),
		fLength(0)
	{
		if (fSize > 0)
			fBuffer[0] = '\0';
	}

	bool Append(const char* toAppend, size_t length)
	{
		if (fLength < fSize) {
			size_t toCopy = std::min(length, fSize - fLength);
			if (toCopy > 0) {
				memcpy(fBuffer + fLength, toAppend, toCopy);
				fBuffer[fLength + toCopy] = '\0';
			}
		}

		fLength += length;
		return fLength < fSize;
	}

	bool Append(const char* toAppend)
	{
		return Append(toAppend, strlen(toAppend));
	}

	size_t Length() const
	{
		return fLength;
	}

private:
	char*	fBuffer;
	size_t	fSize;
	size_t	fLength;
};

}



/*!	Returns the installation location relative path for the given base directory
	constant and installation location index. A '%' in the returned path must be
	replaced by "" for the primary architecture and by "/<arch>" for a secondary
	architecture.
 */
static const char*
get_relative_directory_path(size_t installationLocationIndex,
	path_base_directory baseDirectory)
{
	switch (baseDirectory) {
		case B_FIND_PATH_INSTALLATION_LOCATION_DIRECTORY:
			return "";
		case B_FIND_PATH_ADD_ONS_DIRECTORY:
			return "/add-ons%";
		case B_FIND_PATH_APPS_DIRECTORY:
			return "/apps";
		case B_FIND_PATH_BIN_DIRECTORY:
			return "/bin%";
		case B_FIND_PATH_BOOT_DIRECTORY:
			return "/boot";
		case B_FIND_PATH_CACHE_DIRECTORY:
			return "/cache";
		case B_FIND_PATH_DATA_DIRECTORY:
			return "/data";
		case B_FIND_PATH_DEVELOP_DIRECTORY:
			return "/develop";
		case B_FIND_PATH_DEVELOP_LIB_DIRECTORY:
			return "/develop/lib%";
		case B_FIND_PATH_DOCUMENTATION_DIRECTORY:
			return "/documentation";
		case B_FIND_PATH_ETC_DIRECTORY:
			return "/settings/etc";
		case B_FIND_PATH_FONTS_DIRECTORY:
			return "/data/fonts";
		case B_FIND_PATH_HEADERS_DIRECTORY:
			return "/develop/headers%";
		case B_FIND_PATH_LIB_DIRECTORY:
			return "/lib";
		case B_FIND_PATH_LOG_DIRECTORY:
			return "/log";
		case B_FIND_PATH_MEDIA_NODES_DIRECTORY:
			return "/add-ons%/media";
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
			return "/add-ons%/Translators";
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


static status_t
normalize_longest_existing_path_prefix(const char* path, char* buffer,
	size_t bufferSize)
{
	if (strlcpy(buffer, path, bufferSize) >= bufferSize)
		return B_NAME_TOO_LONG;

	// Until we have an existing path, chop off leaf components.
	for (;;) {
		struct stat st;
		if (lstat(buffer, &st) == 0)
			break;

		// Chop off the leaf, but fail, it it's "..", since then we'd actually
		// construct a subpath.
		char* lastSlash = strrchr(buffer, '/');
		if (lastSlash == NULL || strcmp(lastSlash + 1, "..") == 0)
			return B_ENTRY_NOT_FOUND;

		*lastSlash = '\0';
	}

	// normalize the existing prefix path
	size_t prefixLength = strlen(buffer);
	status_t error = normalize_path(buffer, buffer, bufferSize);
	if (error != B_OK)
		return error;

	// Re-append the non-existent suffix. Remove duplicate slashes and "."
	// components.
	const char* bufferEnd = buffer + bufferSize;
	char* end = buffer + strlen(buffer);
	const char* remainder = path + prefixLength + 1;
	while (*remainder != '\0') {
		// find component start
		if (*remainder == '/') {
			remainder++;
			continue;
		}

		// find component end
		const char* componentEnd = strchr(remainder, '/');
		if (componentEnd == NULL)
			componentEnd = remainder + strlen(remainder);

		// skip "." components
		size_t componentLength = componentEnd - remainder;
		if (componentLength == 1 && *remainder == '.') {
			remainder++;
			continue;
		}

		// append the component
		if (end + 1 + componentLength >= bufferEnd)
			return B_BUFFER_OVERFLOW;

		*end++ = '/';
		memcpy(end, remainder, componentLength);
		end += componentLength;
		remainder += componentLength;
	}

	*end = '\0';
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
process_path(const char* installationLocation, const char* architecture,
	const char* relativePath, const char* subPath, uint32 flags,
	char* pathBuffer, size_t bufferSize)
{
	// copy the installation location
	PathBuffer buffer(pathBuffer, bufferSize);
	buffer.Append(installationLocation);

	// append the relative path, expanding the architecture placeholder
	if (const char* placeholder = strchr(relativePath, '%')) {
		buffer.Append(relativePath, placeholder - relativePath);

		if (architecture != NULL) {
			buffer.Append("/", 1);
			buffer.Append(architecture);
		}

		buffer.Append(placeholder + 1);
	} else
		buffer.Append(relativePath);

	// append subpath, if given
	if (subPath != NULL) {
		buffer.Append("/", 1);
		buffer.Append(subPath);
	}

	size_t totalLength = buffer.Length();
	if (totalLength >= bufferSize)
		return B_BUFFER_OVERFLOW;

	// handle the flags
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
	const char* dependency, const char* architecture,
	path_base_directory baseDirectory, const char* subPath, uint32 flags,
	char* pathBuffer, size_t bufferSize)
{
	if (strcmp(architecture, __get_primary_architecture()) == 0)
		architecture = NULL;

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

	ssize_t pathSize = process_path(installationLocation, architecture,
		relativePath, subPath, flags, pathBuffer, bufferSize);
	if (pathSize <= 0)
		return pathSize == 0 ? B_ENTRY_NOT_FOUND : pathSize;
	return B_OK;
}


// #pragma mark -


status_t
__find_path(const void* codePointer, path_base_directory baseDirectory,
	const char* subPath, char* pathBuffer, size_t bufferSize)
{
	return __find_path_etc(codePointer, NULL, NULL, baseDirectory, subPath, 0,
		pathBuffer, bufferSize);
}


status_t
__find_path_etc(const void* codePointer, const char* dependency,
	const char* architecture, path_base_directory baseDirectory,
	const char* subPath, uint32 flags, char* pathBuffer, size_t bufferSize)
{
	if (pathBuffer == NULL)
		return B_BAD_VALUE;

	// resolve codePointer to image info
	image_info imageInfo;
	status_t error = find_image(codePointer, imageInfo);
	if (error != B_OK)
		return error;

	if (architecture == NULL)
		architecture = __get_architecture();

	return internal_path_for_path(imageInfo.name, sizeof(imageInfo.name),
		dependency, architecture, baseDirectory, subPath, flags, pathBuffer,
		bufferSize);
}


status_t
__find_path_for_path(const char* path, path_base_directory baseDirectory,
	const char* subPath, char* pathBuffer, size_t bufferSize)
{
	return __find_path_for_path_etc(path, NULL, NULL, baseDirectory, subPath, 0,
		pathBuffer, bufferSize);
}


status_t
__find_path_for_path_etc(const char* path, const char* dependency,
	const char* architecture, path_base_directory baseDirectory,
	const char* subPath, uint32 flags, char* pathBuffer, size_t bufferSize)
{
	char referencePath[B_PATH_NAME_LENGTH];
	if (strlcpy(referencePath, path, sizeof(referencePath))
			>= sizeof(referencePath)) {
		return B_NAME_TOO_LONG;
	}

	if (architecture == NULL)
		architecture = __guess_architecture_for_path(path);

	return internal_path_for_path(referencePath, sizeof(referencePath),
		dependency, architecture, baseDirectory, subPath, flags, pathBuffer,
		bufferSize);
}


status_t
__find_paths(path_base_directory baseDirectory, const char* subPath,
	char*** _paths, size_t* _pathCount)
{
	return __find_paths_etc(NULL, baseDirectory, subPath, 0, _paths,
		_pathCount);
}


status_t
__find_paths_etc(const char* architecture, path_base_directory baseDirectory,
	const char* subPath, uint32 flags, char*** _paths, size_t* _pathCount)
{
	if (_paths == NULL || _pathCount == NULL)
		return B_BAD_VALUE;

	// Analyze architecture. If NULL, use the caller's architecture. If the
	// effective architecture is the primary one, set architecture to NULL to
	// indicate that we don't need to insert an architecture subdirectory
	// component.
	if (architecture == NULL)
		architecture = __get_architecture();
	if (strcmp(architecture, __get_primary_architecture()) == 0)
		architecture = NULL;
	size_t architectureSize = architecture != NULL
		? strlen(architecture) + 1 : 0;

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
		if (strchr(relativePaths[i], '%') != NULL)
			totalSize += architectureSize - 1;
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
			architecture, relativePaths[i], subPath, flags, pathBuffer,
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


const char*
__guess_secondary_architecture_from_path(const char* path,
	const char* const* secondaryArchitectures,
	size_t secondaryArchitectureCount)
{
	// Get the longest existing prefix path and normalize it.
	char prefix[B_PATH_NAME_LENGTH];
	if (normalize_longest_existing_path_prefix(path, prefix, sizeof(prefix))
			!= B_OK) {
		return NULL;
	}

	// get an installation location relative path
	size_t installationLocationIndex;
	const char* installationLocation = get_installation_location(prefix,
		installationLocationIndex);
	if (installationLocation == NULL)
		return NULL;

	const char* relativePath = prefix + strlen(installationLocation);
	if (relativePath[0] != '/')
		return NULL;

	// Iterate through the known paths that would indicate a secondary
	// architecture and try to match them with our given path.
	for (size_t i = 0; i < kArchitectureSpecificBaseDirectoryCount; i++) {
		const char* basePath = get_relative_directory_path(
			installationLocationIndex, kArchitectureSpecificBaseDirectories[i]);
		const char* placeholder = strchr(basePath, '%');
		if (placeholder == NULL)
			continue;

		// match the part up to the architecture placeholder
		size_t prefixLength = placeholder - basePath;
		if (strncmp(relativePath, basePath, prefixLength) != 0
			|| relativePath[prefixLength] != '/') {
			continue;
		}

		// match the architecture
		const char* architecturePart = relativePath + prefixLength + 1;
		for (size_t k = 0; k < secondaryArchitectureCount; k++) {
			const char* architecture = secondaryArchitectures[k];
			size_t architectureLength = strlen(architecture);
			if (strncmp(architecturePart, architecture, architectureLength) == 0
				&& (architecturePart[architectureLength] == '/'
					|| architecturePart[architectureLength] == '\0')) {
				return architecture;
			}
		}
	}

	return NULL;
}


B_DEFINE_WEAK_ALIAS(__find_path, find_path);
B_DEFINE_WEAK_ALIAS(__find_path_etc, find_path_etc);
B_DEFINE_WEAK_ALIAS(__find_path_for_path, find_path_for_path);
B_DEFINE_WEAK_ALIAS(__find_path_for_path_etc, find_path_for_path_etc);
B_DEFINE_WEAK_ALIAS(__find_paths, find_paths);
B_DEFINE_WEAK_ALIAS(__find_paths_etc, find_paths_etc);
