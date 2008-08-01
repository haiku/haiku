/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>

#include <libroot_private.h>
#include <user_runtime.h>

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


#define RETURN_AND_SET_ERRNO(err) \
	if (err < 0) { \
		errno = err; \
		return -1; \
	} \
	return err;


// TODO: Use benaphore!
static sem_id sEnvLock;
static char **sManagedEnviron;
static bool sCopied;

char **environ = NULL;


static inline void
lock_variables(void)
{
	while (acquire_sem(sEnvLock) == B_INTERRUPTED)
		;
}


static inline void
unlock_variables(void)
{
	release_sem(sEnvLock);
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
	char **newEnv = realloc(environ, (count + 1) * sizeof(char *));
	if (newEnv == NULL)
		return B_NO_MEMORY;

	newEnv[count] = NULL;
		// null terminate the array

	environ = sManagedEnviron = newEnv;

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


static status_t
copy_environ_to_heap_if_needed(void)
{
	int32 i = 0;

	if (sCopied && environ == sManagedEnviron)
		return B_OK;

	if (sManagedEnviron != NULL) {
		// free previously used "environ"; it has been changed by an application
		free_variables();
	}

	sManagedEnviron = malloc((count_variables() + 1) * sizeof(char *));
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
	sCopied = true;
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
		environ[index] = malloc(length + 2 + strlen(value));
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
	sEnvLock = create_sem(1, "env lock");
}


//	#pragma mark - libroot initializer


void
__init_env(const struct user_space_program_args *args)
{
	// Following POSIX, there is no need to make any of the environment
	// functions thread-safe - but we do it anyway as much as possible to
	// protect our implementation
	sEnvLock = create_sem(1, "env lock");
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
		errno = B_BAD_VALUE;
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
		errno = B_BAD_VALUE;
		return -1;
	}

	length = strlen(name);

	lock_variables();

	copy_environ_to_heap_if_needed();

	env = find_variable(name, length, &index);
	if (env != NULL) {
		// we don't free the memory for the slot, we just move the array
		// contents
		free(env);
		memmove(environ + index, environ + index + 1,
			sizeof(char *) * (count_variables() - index));
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
		errno = B_BAD_VALUE;
		return -1;
	}

	lock_variables();
	status = update_variable(string, value - string, value + 1, true);
	unlock_variables();

	RETURN_AND_SET_ERRNO(status);
}

