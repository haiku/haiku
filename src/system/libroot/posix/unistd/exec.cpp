/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2004-2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <libroot_private.h>
#include <syscalls.h>

#include <alloca.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <umask.h>
#include <unistd.h>

#include <errno_private.h>


static int
count_arguments(va_list list, const char* arg, char*** _env)
{
	int count = 0;

	while (arg != NULL) {
		count++;
		arg = va_arg(list, const char*);
	}

	if (_env)
		*_env = va_arg(list, char**);

	return count;
}


static void
copy_arguments(va_list list, const char** args, const char* arg)
{
	int count = 0;

	while (arg != NULL) {
		args[count++] = arg;
		arg = va_arg(list, const char*);
	}

	args[count] = NULL;
		// terminate list
}


static int
do_exec(const char* path, char* const args[], char* const environment[],
	bool useDefaultInterpreter)
{
	if (path == NULL || args == NULL) {
		__set_errno(B_BAD_VALUE);
		return -1;
	}

	// Count argument/environment list entries here, we don't want
	// to do this in the kernel
	int32 argCount = 0;
	while (args[argCount] != NULL) {
		argCount++;
	}

	int32 envCount = 0;
	if (environment != NULL) {
		while (environment[envCount] != NULL) {
			envCount++;
		}
	}

	if (argCount == 0) {
		// we need some more info on what to do...
		__set_errno(B_BAD_VALUE);
		return -1;
	}

	// Test validity of executable + support for scripts
	char invoker[B_FILE_NAME_LENGTH];
	status_t status = __test_executable(path, invoker);
	if (status < B_OK) {
		if (status == B_NOT_AN_EXECUTABLE && useDefaultInterpreter) {
			strcpy(invoker, "/bin/sh");
			status = B_OK;
		} else {
			__set_errno(status);
			return -1;
		}
	}

	char** newArgs = NULL;
	if (invoker[0] != '\0') {
		status = __parse_invoke_line(invoker, &newArgs, &args, &argCount, path);
		if (status < B_OK) {
			__set_errno(status);
			return -1;
		}

		path = newArgs[0];
	}

	char** flatArgs = NULL;
	size_t flatArgsSize;
	status = __flatten_process_args(newArgs ? newArgs : args, argCount,
		environment, &envCount, path, &flatArgs, &flatArgsSize);

	if (status == B_OK) {
		__set_errno(_kern_exec(path, flatArgs, flatArgsSize, argCount, envCount,
			__gUmask));
			// if this call returns, something definitely went wrong

		free(flatArgs);
	} else
		__set_errno(status);

	free(newArgs);
	return -1;
}


//	#pragma mark -


int
execve(const char* path, char* const args[], char* const environment[])
{
	return do_exec(path, args, environment, false);
}


int
execv(const char* path, char* const argv[])
{
	return do_exec(path, argv, environ, false);
}


int
execvp(const char* file, char* const argv[])
{
	return execvpe(file, argv, environ);
}


int
execvpe(const char* file, char* const argv[], char* const environment[])
{
	// let do_exec() handle cases where file is a path (or invalid)
	if (file == NULL || strchr(file, '/') != NULL)
		return do_exec(file, argv, environment, true);

	// file is just a leaf name, so we have to look it up in the path

	char path[B_PATH_NAME_LENGTH];
	status_t status = __look_up_in_path(file, path);
	if (status != B_OK) {
		__set_errno(status);
		return -1;
	}

	return do_exec(path, argv, environment, true);
}


int
execl(const char* path, const char* arg, ...)
{
	const char** args;
	va_list list;
	int count;

	// count arguments

	va_start(list, arg);
	count = count_arguments(list, arg, NULL);
	va_end(list);

	// copy arguments

	args = (const char**)alloca((count + 1) * sizeof(char*));
	va_start(list, arg);
	copy_arguments(list, args, arg);
	va_end(list);

	return do_exec(path, (char* const*)args, environ, false);
}


int
execlp(const char* file, const char* arg, ...)
{
	const char** args;
	va_list list;
	int count;

	// count arguments

	va_start(list, arg);
	count = count_arguments(list, arg, NULL);
	va_end(list);

	// copy arguments

	args = (const char**)alloca((count + 1) * sizeof(char*));
	va_start(list, arg);
	copy_arguments(list, args, arg);
	va_end(list);

	return execvp(file, (char* const*)args);
}


int
execle(const char* path, const char* arg, ... /*, char** env */)
{
	const char** args;
	char** env;
	va_list list;
	int count;

	// count arguments

	va_start(list, arg);
	count = count_arguments(list, arg, &env);
	va_end(list);

	// copy arguments

	args = (const char**)alloca((count + 1) * sizeof(char*));
	va_start(list, arg);
	copy_arguments(list, args, arg);
	va_end(list);

	return do_exec(path, (char* const*)args, env, false);
}

// TODO: remove this again if possible
extern int exect(const char *path, char *const *argv);

int
exect(const char* path, char* const* argv)
{
	return execv(path, argv);
}
