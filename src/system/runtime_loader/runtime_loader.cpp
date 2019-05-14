/*
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002, Manuel J. Petit. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "runtime_loader_private.h"

#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <algorithm>

#include <ByteOrder.h>

#include <directories.h>
#include <find_directory_private.h>
#include <image_defs.h>
#include <syscalls.h>
#include <user_runtime.h>
#include <vm_defs.h>

#include "elf_symbol_lookup.h"
#include "pe.h"


struct user_space_program_args *gProgramArgs;
void *__gCommPageAddress;
void *__dso_handle;

int32 __gCPUCount = 1;

const directory_which kLibraryDirectories[] = {
	B_SYSTEM_LIB_DIRECTORY,
	B_SYSTEM_NONPACKAGED_LIB_DIRECTORY,
	B_USER_LIB_DIRECTORY,
	B_USER_NONPACKAGED_LIB_DIRECTORY
};


static const char *
search_path_for_type(image_type type)
{
	const char *path = NULL;

	// If "user add-ons" are disabled via safemode settings, we bypass the
	// environment and defaults and return a different set of paths without
	// the user or non-packaged ones.
	if (gProgramArgs->disable_user_addons) {
		switch (type) {
			case B_APP_IMAGE:
				return kGlobalBinDirectory
					":" kSystemAppsDirectory
					":" kSystemPreferencesDirectory;

			case B_LIBRARY_IMAGE:
				return kAppLocalLibDirectory
					":" kSystemLibDirectory;

			case B_ADD_ON_IMAGE:
				return kAppLocalAddonsDirectory
					":" kSystemAddonsDirectory;

			default:
				return NULL;
		}
	}

	// TODO: The *PATH variables should not include the standard system paths.
	// Instead those paths should always be used after the directories specified
	// via the variables.
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
			return kSystemNonpackagedBinDirectory
				":" kGlobalBinDirectory
				":" kSystemAppsDirectory
				":" kSystemPreferencesDirectory;

		case B_LIBRARY_IMAGE:
			return kAppLocalLibDirectory
				":" kSystemNonpackagedLibDirectory
				":" kSystemLibDirectory;

		case B_ADD_ON_IMAGE:
			return kAppLocalAddonsDirectory
				":" kSystemNonpackagedAddonsDirectory
				":" kSystemAddonsDirectory;

		default:
			return NULL;
	}
}


static bool
replace_executable_path_placeholder(const char*& dir, int& dirLength,
	const char* placeholder, size_t placeholderLength,
	const char* replacementSubPath, char*& buffer, size_t& bufferSize,
	status_t& _error)
{
	if (dirLength < (int)placeholderLength
		|| strncmp(dir, placeholder, placeholderLength) != 0) {
		return false;
	}

	if (replacementSubPath == NULL) {
		_error = B_ENTRY_NOT_FOUND;
		return true;
	}

	char* lastSlash = strrchr(replacementSubPath, '/');

	// Copy replacementSubPath without the last component (the application file
	// name, respectively the requesting executable file name).
	size_t toCopy;
	if (lastSlash != NULL) {
		toCopy = lastSlash - replacementSubPath;
		strlcpy(buffer, replacementSubPath,
			std::min((ssize_t)bufferSize, lastSlash + 1 - replacementSubPath));
	} else {
		replacementSubPath = ".";
		toCopy = 1;
		strlcpy(buffer, ".", bufferSize);
	}

	if (toCopy >= bufferSize) {
		_error = B_NAME_TOO_LONG;
		return true;
	}

	memcpy(buffer, replacementSubPath, toCopy);
	buffer[toCopy] = '\0';

	buffer += toCopy;
	bufferSize -= toCopy;
	dir += placeholderLength;
	dirLength -= placeholderLength;

	_error = B_OK;
	return true;
}


static int
try_open_executable(const char *dir, int dirLength, const char *name,
	const char *programPath, const char *requestingObjectPath,
	const char *abiSpecificSubDir, char *path, size_t pathLength)
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

		if (replace_executable_path_placeholder(dir, dirLength, "%A", 2,
				programPath, buffer, pathLength, status)
			|| replace_executable_path_placeholder(dir, dirLength, "$ORIGIN", 7,
				requestingObjectPath, buffer, pathLength, status)) {
			if (status != B_OK)
				return status;
		} else if (abiSpecificSubDir != NULL) {
			// We're looking for a library or an add-on and the executable has
			// not been compiled with a compiler using the same ABI as the one
			// the OS has been built with. Thus we only look in subdirs
			// specific to that ABI.
			// However, only if it's a known library location
			for (int i = 0; i < 4; ++i) {
				char buffer[PATH_MAX];
				status_t result = __find_directory(kLibraryDirectories[i], -1,
					false, buffer, PATH_MAX);
				if (result == B_OK && strncmp(dir, buffer, dirLength) == 0) {
					subDirLen = strlen(abiSpecificSubDir) + 1;
					break;
				}
			}
		}

		if (dirLength + 1 + subDirLen + nameLength >= pathLength)
			return B_NAME_TOO_LONG;

		memcpy(buffer, dir, dirLength);
		buffer[dirLength] = '/';
		if (subDirLen > 0) {
			memcpy(buffer + dirLength + 1, abiSpecificSubDir, subDirLen - 1);
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
	int pathListLen, const char *programPath, const char *requestingObjectPath,
	const char *abiSpecificSubDir, char *pathBuffer, size_t pathBufferLength)
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
			programPath, requestingObjectPath, abiSpecificSubDir, pathBuffer,
			pathBufferLength);
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
	const char *programPath, const char *requestingObjectPath,
	const char *abiSpecificSubDir)
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
				semicolon - firstList, programPath, requestingObjectPath, NULL,
				buffer, sizeof(buffer));
		}
		if (fd < 0) {
			fd = search_executable_in_path_list(name, secondList,
				strlen(secondList), programPath, requestingObjectPath, NULL,
				buffer, sizeof(buffer));
		}
	}

	// If not found yet, let's evaluate the system path variables to find the
	// shared object.
	if (fd < 0) {
		if (const char *paths = search_path_for_type(type)) {
			fd = search_executable_in_path_list(name, paths, strlen(paths),
				programPath, NULL, abiSpecificSubDir, buffer, sizeof(buffer));
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
	Applies haiku-specific fixes to a shebang line.
*/
static void
fixup_shebang(char *invoker)
{
	char *current = invoker;
	while (*current == ' ' || *current == '\t') {
		++current;
	}

	char *commandStart = current;
	while (*current != ' ' && *current != '\t' && *current != '\0') {
		++current;
	}

	// replace /usr/bin/ with /bin/
	if (memcmp(commandStart, "/usr/bin/", strlen("/usr/bin/")) == 0)
		memmove(commandStart, commandStart + 4, strlen(commandStart + 4) + 1);
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

	fd = open_executable(path, B_APP_IMAGE, NULL, NULL, NULL, NULL);
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
#ifdef _COMPAT_MODE
#ifdef __x86_64__
	if (status == B_NOT_AN_EXECUTABLE)
		status = elf32_verify_header(buffer, length);
#else
	if (status == B_NOT_AN_EXECUTABLE)
		status = elf64_verify_header(buffer, length);
#endif	// __x86_64__
#endif	// _COMPAT_MODE
	if (status == B_NOT_AN_EXECUTABLE) {
		if (!strncmp(buffer, "#!", 2)) {
			// test for shell scripts
			char *end;
			buffer[min_c((size_t)length, sizeof(buffer) - 1)] = '\0';

			end = strchr(buffer, '\n');
			if (end == NULL) {
				status = E2BIG;
				goto out;
			} else
				end[0] = '\0';

			if (invoker) {
				strcpy(invoker, buffer + 2);
				fixup_shebang(invoker);
			}

			status = B_OK;
		} else {
			// Something odd like a PE?
			status = pe_verify_header(buffer, length);

			// It is a PE, throw B_UNKNOWN_EXECUTABLE
			// likely win32 at this point
			if (status == B_OK)
				status = B_UNKNOWN_EXECUTABLE;
		}
	} else if (status == B_OK) {
		elf_ehdr *elfHeader = (elf_ehdr *)buffer;
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


static bool
determine_x86_abi(int fd, const Elf32_Ehdr& elfHeader, bool& _isGcc2)
{
	// Unless we're a little-endian CPU, don't bother. We're not x86, so it
	// doesn't matter all that much whether we can determine the correct gcc
	// ABI. This saves the code below from having to deal with endianess
	// conversion.
#if B_HOST_IS_LENDIAN

	// Since we don't want to load the complete image, we can't use the
	// functions that normally determine the Haiku version and ABI. Instead
	// we'll load the symbol and string tables and resolve the ABI symbol
	// manually.

	// map the file into memory
	struct stat st;
	if (_kern_read_stat(fd, NULL, true, &st, sizeof(st)) != B_OK)
		return false;

	void* fileBaseAddress;
	area_id area = _kern_map_file("mapped file", &fileBaseAddress,
		B_ANY_ADDRESS, st.st_size, B_READ_AREA, REGION_NO_PRIVATE_MAP, false,
		fd, 0);
	if (area < 0)
		return false;

	struct AreaDeleter {
		AreaDeleter(area_id area)
			:
			fArea(area)
		{
		}

		~AreaDeleter()
		{
			_kern_delete_area(fArea);
		}

	private:
		area_id	fArea;
	} areaDeleter(area);

	// get the section headers
	if (elfHeader.e_shoff == 0 || elfHeader.e_shentsize < sizeof(Elf32_Shdr))
		return false;

	size_t sectionHeadersSize = elfHeader.e_shentsize * elfHeader.e_shnum;
	if (elfHeader.e_shoff + (off_t)sectionHeadersSize > st.st_size)
		return false;

	void* sectionHeaders = (uint8*)fileBaseAddress + elfHeader.e_shoff;

	// find the sections we need
	uint32* symbolHash = NULL;
	uint32 symbolHashSize = 0;
	uint32 symbolHashChainSize = 0;
	Elf32_Sym* symbolTable = NULL;
	uint32 symbolTableSize = 0;
	const char* stringTable = NULL;
	off_t stringTableSize = 0;

	for (int32 i = 0; i < elfHeader.e_shnum; i++) {
		Elf32_Shdr* sectionHeader
			= (Elf32_Shdr*)((uint8*)sectionHeaders + i * elfHeader.e_shentsize);
		if ((off_t)sectionHeader->sh_offset + (off_t)sectionHeader->sh_size
				> st.st_size) {
			continue;
		}

		void* sectionAddress = (uint8*)fileBaseAddress
			+ sectionHeader->sh_offset;

		switch (sectionHeader->sh_type) {
			case SHT_HASH:
				symbolHash = (uint32*)sectionAddress;
				if (sectionHeader->sh_size < (off_t)sizeof(symbolHash[0]))
					return false;
				symbolHashSize = symbolHash[0];
				symbolHashChainSize
					= sectionHeader->sh_size / sizeof(symbolHash[0]);
				if (symbolHashChainSize < symbolHashSize + 2)
					return false;
				symbolHashChainSize -= symbolHashSize + 2;
				break;
			case SHT_DYNSYM:
				symbolTable = (Elf32_Sym*)sectionAddress;
				symbolTableSize = sectionHeader->sh_size;
				break;
			case SHT_STRTAB:
				// .shstrtab has the same type as .dynstr, but it isn't loaded
				// into memory.
				if (sectionHeader->sh_addr == 0)
					continue;
				stringTable = (const char*)sectionAddress;
				stringTableSize = (off_t)sectionHeader->sh_size;
				break;
			default:
				continue;
		}
	}

	if (symbolHash == NULL || symbolTable == NULL || stringTable == NULL)
		return false;
	uint32 symbolCount
		= std::min(symbolTableSize / (uint32)sizeof(Elf32_Sym),
			symbolHashChainSize);
	if (symbolCount < symbolHashSize)
		return false;

	// look up the ABI symbol
	const char* name = B_SHARED_OBJECT_HAIKU_ABI_VARIABLE_NAME;
	size_t nameLength = strlen(name);
	uint32 bucket = elf_hash(name) % symbolHashSize;

	for (uint32 i = symbolHash[bucket + 2]; i < symbolCount && i != STN_UNDEF;
		i = symbolHash[2 + symbolHashSize + i]) {
		Elf32_Sym* symbol = symbolTable + i;
		if (symbol->st_shndx != SHN_UNDEF
			&& ((symbol->Bind() == STB_GLOBAL) || (symbol->Bind() == STB_WEAK))
			&& symbol->Type() == STT_OBJECT
			&& (off_t)symbol->st_name + (off_t)nameLength < stringTableSize
			&& strcmp(stringTable + symbol->st_name, name) == 0) {
			if (symbol->st_value > 0 && symbol->st_size >= sizeof(uint32)
				&& symbol->st_shndx < elfHeader.e_shnum) {
				Elf32_Shdr* sectionHeader = (Elf32_Shdr*)((uint8*)sectionHeaders
					+ symbol->st_shndx * elfHeader.e_shentsize);
				if (symbol->st_value >= sectionHeader->sh_addr
					&& symbol->st_value
						<= sectionHeader->sh_addr + sectionHeader->sh_size) {
					off_t fileOffset = symbol->st_value - sectionHeader->sh_addr
						+ sectionHeader->sh_offset;
					if (fileOffset + (off_t)sizeof(uint32) <= st.st_size) {
						uint32 abi
							= *(uint32*)((uint8*)fileBaseAddress + fileOffset);
						_isGcc2 = (abi & B_HAIKU_ABI_MAJOR)
							== B_HAIKU_ABI_GCC_2;
						return true;
					}
				}
			}

			return false;
		}
	}

	// ABI symbol not found. That means the object pre-dates its introduction
	// in Haiku. So this is most likely gcc 2. We don't fall back to reading
	// the comment sections to verify.
	_isGcc2 = true;
	return true;
#else	// not little endian
	return false;
#endif
}


static status_t
get_executable_architecture(int fd, const char** _architecture)
{
	// Read the ELF header. We read the 32 bit header. Generally the e_machine
	// field is the last one that interests us and the 64 bit header is still
	// identical at that point.
	Elf32_Ehdr elfHeader;
	ssize_t bytesRead = _kern_read(fd, 0, &elfHeader, sizeof(elfHeader));
	if (bytesRead < 0)
		return bytesRead;
	if ((size_t)bytesRead != sizeof(elfHeader))
		return B_NOT_AN_EXECUTABLE;

	// check whether this is indeed an ELF file
	if (memcmp(elfHeader.e_ident, ELFMAG, 4) != 0)
		return B_NOT_AN_EXECUTABLE;

	// check the architecture
	uint16 machine = elfHeader.e_machine;
	if ((elfHeader.e_ident[EI_DATA] == ELFDATA2LSB) != (B_HOST_IS_LENDIAN != 0))
		machine = (machine >> 8) | (machine << 8);

	const char* architecture = NULL;
	switch (machine) {
		case EM_386:
		case EM_486:
		{
			bool isGcc2;
			if (determine_x86_abi(fd, elfHeader, isGcc2) && isGcc2)
				architecture = "x86_gcc2";
			else
				architecture = "x86";
			break;
		}
		case EM_68K:
			architecture = "m68k";
			break;
		case EM_PPC:
			architecture = "ppc";
			break;
		case EM_ARM:
			architecture = "arm";
			break;
		case EM_ARM64:
			architecture = "arm64";
			break;
		case EM_X86_64:
			architecture = "x86_64";
			break;
		case EM_RISCV:
			architecture = "riscv";
			break;
	}

	if (architecture == NULL)
		return B_NOT_SUPPORTED;

	*_architecture = architecture;
	return B_OK;
}


status_t
get_executable_architecture(const char* path, const char** _architecture)
{
	int fd = _kern_open(-1, path, O_RDONLY, 0);
	if (fd < 0)
		return fd;

	status_t error = get_executable_architecture(fd, _architecture);

	_kern_close(fd);
	return error;
}


/*!
	This is the main entry point of the runtime loader as
	specified by its ld-script.
*/
int
runtime_loader(void* _args, void* commpage)
{
	void *entry = NULL;
	int returnCode;

	gProgramArgs = (struct user_space_program_args *)_args;
	__gCommPageAddress = commpage;

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
