/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include <OS.h>

#include <errno_private.h>
#include <libroot_private.h>
#include <locks.h>
#include <runtime_loader.h>
#include <syscall_utils.h>
#include <user_runtime.h>


static const char* const kEnvLockName = "env lock";

static mutex sEnvLock = MUTEX_INITIALIZER(kEnvLockName);
static char **sManagedEnviron;

char **environ = NULL;


static inline void
lock_variables(void)
{
	mutex_lock(&sEnvLock);
}


static inline void
unlock_variables(void)
{
	mutex_unlock(&sEnvLock);
}


static void
free_variables(void)
{
	int32 i;

	if (sManagedEnviron == NULL)
		return;

	for (i = 0; sManagedEnviron[i] != NULL; i++) {
		free(sManagedEnviron[i]);
	}

	free(sManagedEnviron);
	sManagedEnviron = NULL;
}


static int32
count_variables(void)
{
	int32 i = 0;

	if (environ == NULL)
		return 0;

	while (environ[i])
		i++;

	return i;
}


static int32
add_variable(void)
{
	int32 count = count_variables() + 1;
	char **newEnviron = (char**)realloc(environ, (count + 1) * sizeof(char *));
	if (newEnviron == NULL)
		return B_NO_MEMORY;

	newEnviron[count] = NULL;
		// null terminate the array

	environ = sManagedEnviron = newEnviron;

	return count - 1;
}


static char *
find_variable(const char *name, int32 length, int32 *_index)
{
	int32 i;

	if (environ == NULL)
		return NULL;

	for (i = 0; environ[i] != NULL; i++) {
		if (!strncmp(name, environ[i], length) && environ[i][length] == '=') {
			if (_index != NULL)
				*_index = i;
			return environ[i];
		}
	}

	return NULL;
}


/*!	Copies the environment from its current location into a heap managed
	environment, if it's not already there.

	This is needed whenever the environment is changed, that is, when one
	of the POSIX *env() functions is called, and we either used the environment
	provided by the kernel, or by an application that changed \c environ
	directly.
*/
static status_t
copy_environ_to_heap_if_needed(void)
{
	int32 i = 0;

	if (environ == sManagedEnviron)
		return B_OK;

	// free previously used "environ" if it has been changed by an application
	free_variables();

	sManagedEnviron = (char**)malloc((count_variables() + 1) * sizeof(char *));
	if (sManagedEnviron == NULL)
		return B_NO_MEMORY;

	if (environ != NULL) {
		// copy from previous
		for (; environ[i]; i++) {
			sManagedEnviron[i] = strdup(environ[i]);
		}
	}

	sManagedEnviron[i] = NULL;
		// null terminate the array

	environ = sManagedEnviron;
	return B_OK;
}


static status_t
update_variable(const char *name, int32 length, const char *value,
	bool overwrite)
{
	bool update = false;
	int32 index;
	char *env;

	copy_environ_to_heap_if_needed();

	env = find_variable(name, length, &index);
	if (env != NULL && overwrite) {
		// change variable
		free(environ[index]);
		update = true;
	} else if (env == NULL) {
		// add variable
		index = add_variable();
		if (index < 0)
			return B_NO_MEMORY;

		update = true;
	}

	if (update) {
		environ[index] = (char*)malloc(length + 2 + strlen(value));
		if (environ[index] == NULL)
			return B_NO_MEMORY;

		memcpy(environ[index], name, length);
		environ[index][length] = '=';
		strcpy(environ[index] + length + 1, value);
	}

	return B_OK;
}


static void
environ_fork_hook(void)
{
	mutex_init(&sEnvLock, kEnvLockName);
}


//	#pragma mark - libroot initializer


void
__init_env(const struct user_space_program_args *args)
{
	// Following POSIX, there is no need to make any of the environment
	// functions thread-safe - but we do it anyway as much as possible to
	// protect our implementation
	environ = args->env;
	sManagedEnviron = NULL;

	atfork(environ_fork_hook);
}


//	#pragma mark - public API


int
clearenv(void)
{
	lock_variables();

	free_variables();
	environ = NULL;

	unlock_variables();

	return 0;
}


char *
getenv(const char *name)
{
	int32 length = strlen(name);
	char *value;

	lock_variables();

	value = find_variable(name, length, NULL);
	unlock_variables();

	if (value == NULL)
		return NULL;

	return value + length + 1;
}


int
setenv(const char *name, const char *value, int overwrite)
{
	status_t status;

	if (name == NULL || name[0] == '\0' || strchr(name, '=') != NULL) {
		__set_errno(B_BAD_VALUE);
		return -1;
	}

	lock_variables();
	status = update_variable(name, strlen(name), value, overwrite);
	unlock_variables();

	RETURN_AND_SET_ERRNO(status);
}


int
unsetenv(const char *name)
{
	int32 index, length;
	char *env;

	if (name == NULL || name[0] == '\0' || strchr(name, '=') != NULL) {
		__set_errno(B_BAD_VALUE);
		return -1;
	}

	length = strlen(name);

	lock_variables();

	copy_environ_to_heap_if_needed();

	env = find_variable(name, length, &index);
	while (env != NULL) {
		// we don't free the memory for the slot, we just move the array
		// contents
		free(env);
		memmove(environ + index, environ + index + 1,
			sizeof(char *) * (count_variables() - index));

		// search possible another occurence, introduced via putenv()
		// and renamed since
		env = find_variable(name, length, &index);
	}

	unlock_variables();
	return 0;
}


int
putenv(const char *string)
{
	char *value = strchr(string, '=');
	status_t status;

	if (value == NULL) {
		__set_errno(B_BAD_VALUE);
		return -1;
	}

	lock_variables();
	status = update_variable(string, value - string, value + 1, true);
	unlock_variables();

	RETURN_AND_SET_ERRNO(status);
}

