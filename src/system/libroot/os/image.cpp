/*
 * Copyright 2003-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <image.h>
#include <image_private.h>

#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <new>

#include <fs_attr.h>

#include <AutoDeleter.h>

#include <libroot_private.h>
#include <runtime_loader.h>
#include <syscalls.h>
#include <syscall_load_image.h>
#include <user_runtime.h>


struct EnvironmentFilter {
	EnvironmentFilter()
		:
		fBuffer(NULL),
		fEntries(NULL),
		fBufferSize(0),
		fEntryCount(0),
		fAdditionalEnvCount(0),
		fNextEntryIndex(0)
	{
	}

	~EnvironmentFilter()
	{
		free(fBuffer);
		delete[] fEntries;
	}

	void Init(const char* path, const char* const* env, size_t envCount)
	{
		int fd = open(path, O_RDONLY);
		if (fd < 0)
			return;
		FileDescriptorCloser fdCloser(fd);

		static const char* const kEnvAttribute = "SYS:ENV";
		attr_info info;
		if (fs_stat_attr(fd, kEnvAttribute, &info) < 0)
			return;

		_Init(fd, kEnvAttribute, info.size, env, envCount);
	}

	size_t AdditionalSlotsNeeded() const
	{
		return fAdditionalEnvCount;
	}

	size_t AdditionalSizeNeeded() const
	{
		return fBufferSize + fAdditionalEnvCount * sizeof(char*);
	}

	size_t PrepareSlot(const char* env, int32 index, char* buffer)
	{
		if (fNextEntryIndex < fEntryCount
			&& fEntries[fNextEntryIndex].index == index) {
			env = fEntries[fNextEntryIndex].replacement;
			fNextEntryIndex++;
		}

		return _FillSlot(env, buffer);
	}

	void PrepareAdditionalSlots(char**& slot, char*& buffer)
	{
		for (size_t i = 0; i < fAdditionalEnvCount; i++) {
			size_t envSize = _FillSlot(fEntries[i].replacement, buffer);
			*slot++ = buffer;
			buffer += envSize;
		}
	}

private:
	void _Init(int fd, const char* attribute, size_t size,
		const char* const* env, size_t envCount)
	{
		if (size == 0)
			return;

		// read the attribute
		char* buffer = (char*)malloc(size + 1);
		if (buffer == NULL)
			return;
		MemoryDeleter bufferDeleter(buffer);

		ssize_t bytesRead = fs_read_attr(fd, attribute, B_STRING_TYPE, 0,
			buffer, size);
		if (bytesRead < 0 || (size_t)bytesRead != size)
			return;
		buffer[size] = '\0';

		// deescape the buffer and count the entries
		size_t entryCount = 1;
		char* out = buffer;
		for (const char* c = buffer; *c != '\0'; c++) {
			if (*c == '\\') {
				c++;
				if (*c == '\0')
					break;
				if (*c == '0') {
					*out++ = '\0';
					entryCount++;
				} else
					*out++ = *c;
			} else
				*out++ = *c;
		}
		*out++ = '\0';
		size = out - buffer + 1;

		// create an entry array
		fEntries = new(std::nothrow) Entry[entryCount];
		if (fEntries == NULL)
			return;

		bufferDeleter.Detach();
		fBuffer = buffer;
		fBufferSize = size;

		// init the entries
		out = buffer;
		for (size_t i = 0; i < entryCount; i++) {
			const char* separator = strchr(out, '=');
			if (separator != NULL && separator != out) {
				fEntries[fEntryCount].replacement = out;
				fEntries[fEntryCount].index = _FindEnvEntry(env, envCount, out,
					separator - out);
				if (fEntries[fEntryCount].index < 0)
					fAdditionalEnvCount++;
				fEntryCount++;
			}
			out += strlen(out) + 1;
		}

		if (fEntryCount > 1)
			std::sort(fEntries, fEntries + fEntryCount);

		// Advance fNextEntryIndex to the first entry pointing to an existing
		// env variable.
		while (fNextEntryIndex < fEntryCount
			&& fEntries[fNextEntryIndex].index < 0) {
			fNextEntryIndex++;
		}
	}

	int32 _FindEnvEntry(const char* const* env, size_t envCount,
		const char* variable, size_t variableLength)
	{
		for (size_t i = 0; i < envCount; i++) {
			if (strncmp(env[i], variable, variableLength) == 0
				&& env[i][variableLength] == '=') {
				return i;
			}
		}

		return -1;
	}

	size_t _FillSlot(const char* env, char* buffer)
	{
		size_t envSize = strlen(env) + 1;
		memcpy(buffer, env, envSize);
		return envSize;
	}

private:
	struct Entry {
		char*	replacement;
		int32	index;

		bool operator<(const Entry& other) const
		{
			return index < other.index;
		}
	};

private:
	char*	fBuffer;
	Entry*	fEntries;
	size_t	fBufferSize;
	size_t	fEntryCount;
	size_t	fAdditionalEnvCount;
	size_t	fNextEntryIndex;
};


thread_id
load_image(int32 argCount, const char **args, const char **environ)
{
	char invoker[B_FILE_NAME_LENGTH];
	char **newArgs = NULL;
	int32 envCount = 0;
	thread_id thread;

	if (argCount < 1 || environ == NULL)
		return B_BAD_VALUE;

	// test validity of executable + support for scripts
	{
		status_t status = __test_executable(args[0], invoker);
		if (status < B_OK)
			return status;

		if (invoker[0]) {
			status = __parse_invoke_line(invoker, &newArgs,
				(char * const **)&args, &argCount, args[0]);
			if (status < B_OK)
				return status;
		}
	}

	// count environment variables
	while (environ[envCount] != NULL)
		envCount++;

	char** flatArgs = NULL;
	size_t flatArgsSize;
	status_t status = __flatten_process_args(args, argCount, environ,
		&envCount, args[0], &flatArgs, &flatArgsSize);

	if (status == B_OK) {
		thread = _kern_load_image(flatArgs, flatArgsSize, argCount, envCount,
			B_NORMAL_PRIORITY, B_WAIT_TILL_LOADED, -1, 0);

		free(flatArgs);
	} else
		thread = status;

	free(newArgs);
	return thread;
}


image_id
load_add_on(char const *name)
{
	if (name == NULL)
		return B_BAD_VALUE;

	return __gRuntimeLoader->load_add_on(name, 0);
}


status_t
unload_add_on(image_id id)
{
	return __gRuntimeLoader->unload_add_on(id);
}


status_t
get_image_symbol(image_id id, char const *symbolName, int32 symbolType,
	void **_location)
{
	return __gRuntimeLoader->get_image_symbol(id, symbolName, symbolType,
		false, NULL, _location);
}


status_t
get_image_symbol_etc(image_id id, char const *symbolName, int32 symbolType,
	bool recursive, image_id *_inImage, void **_location)
{
	return __gRuntimeLoader->get_image_symbol(id, symbolName, symbolType,
		recursive, _inImage, _location);
}


status_t
get_nth_image_symbol(image_id id, int32 num, char *nameBuffer, int32 *_nameLength,
	int32 *_symbolType, void **_location)
{
	return __gRuntimeLoader->get_nth_image_symbol(id, num, nameBuffer, _nameLength, _symbolType, _location);
}


status_t
_get_image_info(image_id id, image_info *info, size_t infoSize)
{
	return _kern_get_image_info(id, info, infoSize);
}


status_t
_get_next_image_info(team_id team, int32 *cookie, image_info *info, size_t infoSize)
{
	return _kern_get_next_image_info(team, cookie, info, infoSize);
}


void
clear_caches(void *address, size_t length, uint32 flags)
{
	_kern_clear_caches(address, length, flags);
}


//	#pragma mark -


static char *
next_argument(char **_start, bool separate)
{
	char *line = *_start;
	char quote = 0;
	int32 i;

	// eliminate leading spaces
	while (line[0] == ' ')
		line++;

	if (line[0] == '"' || line[0] == '\'') {
		quote = line[0];
		line++;
	}

	if (!line[0])
		return NULL;

	for (i = 0;; i++) {
		if (line[i] == '\\' && line[i + 1] != '\0')
			continue;

		if (line[i] == '\0') {
			*_start = &line[i];
			return line;
		}
		if ((!quote && line[i] == ' ') || line[i] == quote) {
			// argument separator!
			if (separate)
				line[i] = '\0';
			*_start = &line[i + 1];
			return line;
		}
	}

	return NULL;
}


status_t
__parse_invoke_line(char *invoker, char ***_newArgs,
	char * const **_oldArgs, int32 *_argCount, const char *arg0)
{
	int32 i, count = 0;
	char *arg = invoker;
	char **newArgs;

	// count arguments in the line

	while (next_argument(&arg, false)) {
		count++;
	}

	// this is a shell script and requires special treatment
	newArgs = (char**)malloc((*_argCount + count + 1) * sizeof(void *));
	if (newArgs == NULL)
		return B_NO_MEMORY;

	// copy invoker and old arguments and to newArgs

	for (i = 0; (arg = next_argument(&invoker, true)) != NULL; i++) {
		newArgs[i] = arg;
	}
	for (i = 0; i < *_argCount; i++) {
		if (i == 0)
			newArgs[i + count] = (char*)arg0;
		else
			newArgs[i + count] = (char *)(*_oldArgs)[i];
	}

	newArgs[i + count] = NULL;

	*_newArgs = newArgs;
	*_oldArgs = (char * const *)newArgs;
	*_argCount += count;

	return B_OK;
}


status_t
__get_next_image_dependency(image_id id, uint32 *cookie, const char **_name)
{
	return __gRuntimeLoader->get_next_image_dependency(id, cookie, _name);
}


status_t
__test_executable(const char *path, char *invoker)
{
	return __gRuntimeLoader->test_executable(path, invoker);
}


/*!	Allocates a flat buffer and copies the argument and environment strings
	into it. The buffer starts with a char* array which contains pointers to
	the strings of the arguments and environment, followed by the strings. Both
	arguments and environment arrays are NULL-terminated.

	If executablePath is non-NULL, it should refer to the executable to be
	executed. If the executable file specifies changes to environment variable
	values, those will be performed.
*/
status_t
__flatten_process_args(const char* const* args, int32 argCount,
	const char* const* env, int32* _envCount, const char* executablePath,
	char*** _flatArgs, size_t* _flatSize)
{
	if (args == NULL || _envCount == NULL || (env == NULL && *_envCount != 0))
		return B_BAD_VALUE;

	int32 envCount = *_envCount;

	// determine total needed size
	int32 argSize = 0;
	for (int32 i = 0; i < argCount; i++) {
		if (args[i] == NULL)
			return B_BAD_VALUE;
		argSize += strlen(args[i]) + 1;
	}

	int32 envSize = 0;
	for (int32 i = 0; i < envCount; i++) {
		if (env[i] == NULL)
			return B_BAD_VALUE;
		envSize += strlen(env[i]) + 1;
	}

	EnvironmentFilter envFilter;
	if (executablePath != NULL)
		envFilter.Init(executablePath, env, envCount);

	int32 totalSlotCount = argCount + envCount + 2
		+ envFilter.AdditionalSlotsNeeded();
	int32 size = totalSlotCount * sizeof(char*) + argSize + envSize
		+ envFilter.AdditionalSizeNeeded();
	if (size > MAX_PROCESS_ARGS_SIZE)
		return B_TOO_MANY_ARGS;

	// allocate space
	char** flatArgs = (char**)malloc(size);
	if (flatArgs == NULL)
		return B_NO_MEMORY;

	char** slot = flatArgs;
	char* stringSpace = (char*)(flatArgs + totalSlotCount);

	// copy arguments and environment
	for (int32 i = 0; i < argCount; i++) {
		int32 argSize = strlen(args[i]) + 1;
		memcpy(stringSpace, args[i], argSize);
		*slot++ = stringSpace;
		stringSpace += argSize;
	}

	*slot++ = NULL;

	for (int32 i = 0; i < envCount; i++) {
		size_t envSize = envFilter.PrepareSlot(env[i], i, stringSpace);
		*slot++ = stringSpace;
		stringSpace += envSize;
	}

	envFilter.PrepareAdditionalSlots(slot, stringSpace);

	*slot++ = NULL;

	*_envCount = envCount + envFilter.AdditionalSlotsNeeded();
	*_flatArgs = flatArgs;
	*_flatSize = stringSpace - (char*)flatArgs;
	return B_OK;
}


extern "C" void _call_init_routines_(void);
void
_call_init_routines_(void)
{
	// This is called by the original BeOS startup code.
	// We don't need it, because our loader already does the job, right?
}

