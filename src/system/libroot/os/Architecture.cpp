/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <architecture_private.h>

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <OS.h>

#include <directories.h>
#include <find_directory_private.h>
#include <runtime_loader.h>


static const char* const kArchitecture = B_HAIKU_ABI_NAME;
static const char* const kPrimaryArchitecture = __HAIKU_PRIMARY_PACKAGING_ARCH;

#ifdef __HAIKU_ARCH_X86
	static const char* const kSiblingArchitectures[] = {"x86_gcc2", "x86"};
#else
	static const char* const kSiblingArchitectures[] = {};
#endif

static const size_t kSiblingArchitectureCount
	= sizeof(kSiblingArchitectures) / sizeof(const char*);


static bool
has_secondary_architecture(const char* architecture)
{
	if (strcmp(architecture, kPrimaryArchitecture) == 0)
		return false;

	char path[B_PATH_NAME_LENGTH];
	snprintf(path, sizeof(path), kSystemLibDirectory "/%s/libroot.so",
		architecture);

	struct stat st;
	return lstat(path, &st) == 0;
}


// #pragma mark -


const char*
__get_architecture()
{
	return kArchitecture;
}


const char*
__get_primary_architecture()
{
	return kPrimaryArchitecture;
}


size_t
__get_secondary_architectures(const char** architectures, size_t count)
{
	size_t index = 0;

	// If this is an architecture that could be a primary or secondary
	// architecture, check for which architectures a libroot.so is present.
	if (kSiblingArchitectureCount > 0) {
		for (size_t i = 0; i < kSiblingArchitectureCount; i++) {
			const char* architecture = kSiblingArchitectures[i];
			if (!has_secondary_architecture(architecture))
				continue;

			if (index < count)
				architectures[index] = architecture;
			index++;
		}
	}

	return index;
}


size_t
__get_architectures(const char** architectures, size_t count)
{
	if (count == 0)
		return __get_secondary_architectures(NULL, 0) + 1;

	architectures[0] = __get_primary_architecture();
	return __get_secondary_architectures(architectures + 1, count -1) + 1;
}


const char*
__guess_architecture_for_path(const char* path)
{
	if (kSiblingArchitectureCount == 0)
		return kPrimaryArchitecture;

	// ask the runtime loader
	const char* architecture;
	if (__gRuntimeLoader->get_executable_architecture(path, &architecture)
			== B_OK) {
		// verify that it is one of the sibling architectures
		for (size_t i = 0; i < kSiblingArchitectureCount; i++) {
			if (strcmp(architecture, kSiblingArchitectures[i]) == 0)
				return kSiblingArchitectures[i];
		}
	}

	// guess from the given path
	architecture = __guess_secondary_architecture_from_path(path,
		kSiblingArchitectures, kSiblingArchitectureCount);

	return architecture != NULL && has_secondary_architecture(architecture)
		? architecture : kPrimaryArchitecture;
}


B_DEFINE_WEAK_ALIAS(__get_architecture, get_architecture);
B_DEFINE_WEAK_ALIAS(__get_primary_architecture, get_primary_architecture);
B_DEFINE_WEAK_ALIAS(__get_secondary_architectures, get_secondary_architectures);
B_DEFINE_WEAK_ALIAS(__get_architectures, get_architectures);
B_DEFINE_WEAK_ALIAS(__guess_architecture_for_path, guess_architecture_for_path);
