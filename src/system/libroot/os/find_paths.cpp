/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
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
#include <directories.h>
#include <syscalls.h>

#include "PathBuffer.h"


static size_t kHomeInstallationLocationIndex = 1;

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


struct InstallationLocations {
public:
	static const size_t	kCount = 4;

public:
	InstallationLocations()
		:
		fReferenceCount(1)
	{
		fLocations[0] = kUserNonpackagedDirectory;
		fLocations[1] = kUserConfigDirectory;
		fLocations[2] = kSystemNonpackagedDirectory;
		fLocations[3] = kSystemDirectory;
	}

	InstallationLocations(const char* home)
		:
		fReferenceCount(1)
	{
		static const char* const kNonPackagedSuffix = "/non-packaged";
		char* homeNonPackaged
			= (char*)malloc(strlen(home) + strlen(kNonPackagedSuffix) + 1);
		fLocations[0] = homeNonPackaged;
		if (homeNonPackaged != NULL) {
			strcpy(homeNonPackaged, home);
			strcat(homeNonPackaged, kNonPackagedSuffix);
		}

		fLocations[1] = strdup(home);

		fLocations[2] = kSystemNonpackagedDirectory;
		fLocations[3] = kSystemDirectory;
	}

	~InstallationLocations()
	{
		free(const_cast<char*>(fLocations[0]));
		free(const_cast<char*>(fLocations[1]));
	}

	bool IsValid() const
	{
		return fLocations[0] != NULL && fLocations[1] != NULL;
	}

	bool IsUserIndex(size_t index) const
	{
		return index==0 || index==1;
	}

	bool IsSystemIndex(size_t index) const
	{
		return index==2 || index==3;
	}

	static InstallationLocations* Default()
	{
		static char sBuffer[sizeof(InstallationLocations)];
		static InstallationLocations* sDefaultLocations
			= new(&sBuffer) InstallationLocations;
		return sDefaultLocations;
	}

	static InstallationLocations* Get()
	{
		InstallationLocations* defaultLocations = Default();

		// Get the home config installation location and create a new object,
		// if it differs from the default.
		char homeInstallationLocation[B_PATH_NAME_LENGTH];
		if (__find_directory(B_USER_CONFIG_DIRECTORY, -1, false,
					homeInstallationLocation, sizeof(homeInstallationLocation))
				== B_OK) {
			_kern_normalize_path(homeInstallationLocation, true,
				homeInstallationLocation);
				// failure is OK
			if (strcmp(homeInstallationLocation,
						defaultLocations->At(kHomeInstallationLocationIndex))
					!= 0) {
				InstallationLocations* locations
					= new(std::nothrow) InstallationLocations(
						homeInstallationLocation);
				if (locations != NULL && locations->IsValid())
					return locations;
				delete locations;
			}
		}

		atomic_add(&defaultLocations->fReferenceCount, 1);
		return defaultLocations;
	}

	void Put()
	{
		if (atomic_add(&fReferenceCount, -1) == 1)
			delete this;
	}

	const char* At(size_t index) const
	{
		return fLocations[index];
	}

	const char* LocationFor(const char* path, size_t& _index)
	{
		for (size_t i = 0; i < kCount; i++) {
			size_t length = strlen(fLocations[i]);
			if (strncmp(path, fLocations[i], length) == 0
				&& (path[length] == '/' || path[length] == '\0')) {
				_index = i;
				return fLocations[i];
			}
		}

		return NULL;
	}

private:
	int32		fReferenceCount;
	const char*	fLocations[kCount];
};


}	// unnamed namespace


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
			return "/lib%";
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
		case B_FIND_PATH_PACKAGE_PATH:
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


static status_t
get_file_attribute(const char* path, const char* attribute, char* nameBuffer,
	size_t bufferSize)
{
	int fd = fs_open_attr(path, attribute, B_STRING_TYPE,
		O_RDONLY | O_NOTRAVERSE);
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

	// resolve dependency
	char packageName[B_FILE_NAME_LENGTH];
		// Temporarily used here, permanently used below where
		// B_FIND_PATH_PACKAGE_PATH is handled.
	if (dependency != NULL) {
		// get the versioned package name
		status_t error = get_file_attribute(referencePath, "SYS:PACKAGE",
			packageName, sizeof(packageName));
		if (error != B_OK)
			return error;

		// normalize the dependency name
		char normalizedDependency[B_FILE_NAME_LENGTH];
		error = normalize_dependency(dependency, normalizedDependency,
			sizeof(normalizedDependency));
		if (error != B_OK)
			return error;

		// Compute the path of the dependency symlink. This will yield the
		// installation location path when normalized.
		if (snprintf(referencePath, referencePathSize,
					kSystemPackageLinksDirectory "/%s/%s", packageName,
					normalizedDependency)
				>= (ssize_t)referencePathSize) {
			return B_BUFFER_OVERFLOW;
		}
	}

	// handle B_FIND_PATH_IMAGE_PATH
	if (baseDirectory == B_FIND_PATH_IMAGE_PATH)
		return copy_path(referencePath, pathBuffer, bufferSize);

	// Handle B_FIND_PATH_PACKAGE_PATH: get the package file name and
	// simply adjust our arguments to look the package file up in the packages
	// directory.
	if (baseDirectory == B_FIND_PATH_PACKAGE_PATH) {
		status_t error = get_file_attribute(referencePath, "SYS:PACKAGE_FILE",
			packageName, sizeof(packageName));
		if (error != B_OK)
			return error;

		dependency = NULL;
		subPath = packageName;
		baseDirectory = B_FIND_PATH_PACKAGES_DIRECTORY;
		flags = B_FIND_PATH_EXISTING_ONLY;
	}

	// normalize
	status_t error = normalize_path(referencePath, referencePath,
		referencePathSize);
	if (error != B_OK)
		return error;

	// get the installation location
	InstallationLocations* installationLocations = InstallationLocations::Get();
	MethodDeleter<InstallationLocations, void, &InstallationLocations::Put>
		installationLocationsDeleter(installationLocations);

	size_t installationLocationIndex;
	const char* installationLocation = installationLocations->LocationFor(
		referencePath, installationLocationIndex);
	if (installationLocation == NULL)
		return B_ENTRY_NOT_FOUND;

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
	if (baseDirectory == B_FIND_PATH_IMAGE_PATH)
		return B_BAD_VALUE;

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

	// get the installation locations
	InstallationLocations* installationLocations = InstallationLocations::Get();
	MethodDeleter<InstallationLocations, void, &InstallationLocations::Put>
		installationLocationsDeleter(installationLocations);

	// Get the relative paths and compute the total size to allocate.
	const char* relativePaths[InstallationLocations::kCount];
	size_t totalSize = 0;

	for (size_t i = 0; i < InstallationLocations::kCount; i++) {
		if (((flags & B_FIND_PATHS_USER_ONLY) != 0
				&& !installationLocations->IsUserIndex(i))
			|| ((flags & B_FIND_PATHS_SYSTEM_ONLY) != 0
				&& !installationLocations->IsSystemIndex(i)))
			continue;

		relativePaths[i] = get_relative_directory_path(i, baseDirectory);
		if (relativePaths[i] == NULL)
			return B_BAD_VALUE;

		totalSize += strlen(installationLocations->At(i))
			+ strlen(relativePaths[i]) + subPathLength + 1;
		if (strchr(relativePaths[i], '%') != NULL)
			totalSize += architectureSize - 1;
	}

	// allocate storage
	char** paths = (char**)malloc(sizeof(char*) * InstallationLocations::kCount
		+ totalSize);
	if (paths == NULL)
		return B_NO_MEMORY;
	MemoryDeleter pathsDeleter(paths);

	// construct and process the paths
	size_t count = 0;
	char* pathBuffer = (char*)(paths + InstallationLocations::kCount);
	const char* pathBufferEnd = pathBuffer + totalSize;
	for (size_t i = 0; i < InstallationLocations::kCount; i++) {
		if (((flags & B_FIND_PATHS_USER_ONLY) != 0
				&& !installationLocations->IsUserIndex(i))
			|| ((flags & B_FIND_PATHS_SYSTEM_ONLY) != 0
				&& !installationLocations->IsSystemIndex(i)))
			continue;

		ssize_t pathSize = process_path(installationLocations->At(i),
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
	InstallationLocations* installationLocations = InstallationLocations::Get();
	MethodDeleter<InstallationLocations, void, &InstallationLocations::Put>
		installationLocationsDeleter(installationLocations);

	size_t installationLocationIndex;
	const char* installationLocation = installationLocations->LocationFor(
		prefix, installationLocationIndex);
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
