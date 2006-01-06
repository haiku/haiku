/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <libroot_private.h>
#include <user_runtime.h>
#include <syscalls.h>

#include <OS.h>
#include <image.h>

#include <stdlib.h>


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
				(char * const **)&args, &argCount);
			if (status < B_OK)
				return status;
		}
	}

	// count environment variables
	while (environ[envCount] != NULL)
		envCount++;

	thread = _kern_load_image(argCount, args, envCount, environ,
		B_NORMAL_PRIORITY, B_WAIT_TILL_LOADED);

	free(newArgs);
	return thread;
}


image_id
load_add_on(char const *name)
{
	return __gRuntimeLoader->load_add_on(name, 0);
}


status_t
unload_add_on(image_id id)
{
	return __gRuntimeLoader->unload_add_on(id);
}


status_t
get_image_symbol(image_id id, char const *symbolName, int32 symbolType, void **_location)
{
	return __gRuntimeLoader->get_image_symbol(id, symbolName, symbolType, _location);
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
	char * const **_oldArgs, int32 *_argCount)
{
	int32 i, count = 0;
	char *arg = invoker;
	char **newArgs;

	// count arguments in the line

	while (next_argument(&arg, false)) {
		count++;
	}

	// this is a shell script and requires special treatment
	newArgs = malloc((*_argCount + count + 1) * sizeof(void *));
	if (newArgs == NULL)
		return B_NO_MEMORY;

	// copy invoker and old arguments and to newArgs

	for (i = 0; (arg = next_argument(&invoker, true)) != NULL; i++) {
		newArgs[i] = arg;
	}
	for (i = 0; i < *_argCount; i++) {
		newArgs[i + count] = (char *)(*_oldArgs)[i];
	}

	newArgs[i + count] = NULL;

	*_newArgs = newArgs;
	*_oldArgs = (char * const *)newArgs;
	*_argCount += count;

	return B_OK;
}


status_t
__test_executable(const char *path, char *invoker)
{
	return __gRuntimeLoader->test_executable(path, geteuid(), getegid(), invoker);
}


void _call_init_routines_(void);
void
_call_init_routines_(void)
{
	// This is called by the original BeOS startup code.
	// We don't need it, because our loader already does the job, right?
}

