/*
 * Copyright 2003-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <image.h>
#include <image_private.h>

#include <stdlib.h>
#include <string.h>

#include <OS.h>

#include <libroot_private.h>
#include <runtime_loader.h>
#include <syscalls.h>
#include <user_runtime.h>


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
	status_t status = __flatten_process_args(args, argCount, environ, envCount,
		&flatArgs, &flatArgsSize);

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
*/
status_t
__flatten_process_args(const char* const* args, int32 argCount,
	const char* const* env, int32 envCount, char*** _flatArgs,
	size_t* _flatSize)
{
	if (args == NULL || env == NULL)
		return B_BAD_VALUE;

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

	int32 size = (argCount + envCount + 2) * sizeof(char*) + argSize + envSize;
	if (size > MAX_PROCESS_ARGS_SIZE)
		return B_TOO_MANY_ARGS;

	// allocate space
	char** flatArgs = (char**)malloc(size);
	if (flatArgs == NULL)
		return B_NO_MEMORY;

	char** slot = flatArgs;
	char* stringSpace = (char*)(flatArgs + argCount + envCount + 2);

	// copy arguments and environment
	for (int32 i = 0; i < argCount; i++) {
		int32 argSize = strlen(args[i]) + 1;
		memcpy(stringSpace, args[i], argSize);
		*slot++ = stringSpace;
		stringSpace += argSize;
	}

	*slot++ = NULL;

	for (int32 i = 0; i < envCount; i++) {
		int32 envSize = strlen(env[i]) + 1;
		memcpy(stringSpace, env[i], envSize);
		*slot++ = stringSpace;
		stringSpace += envSize;
	}

	*slot++ = NULL;

	*_flatArgs = flatArgs;
	*_flatSize = size;
	return B_OK;
}


extern "C" void _call_init_routines_(void);
void
_call_init_routines_(void)
{
	// This is called by the original BeOS startup code.
	// We don't need it, because our loader already does the job, right?
}

