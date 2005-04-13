/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
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


static sem_id sEnvLock;
static bool sCopied;

char **environ = NULL;


static int32
count_variables(void)
{
	int32 i = 0;
	while (environ[i])
		i++;

	return i;
}


static int32
add_variable(void)
{
	int32 count = count_variables() + 1;
	char **newEnv = malloc((count + 1) * sizeof(char *));
	if (newEnv == NULL)
		return B_NO_MEMORY;

	memcpy(newEnv, environ, count * sizeof(char *));
	newEnv[count] = NULL;
		// null terminate the array

	return count - 1;
}


static char *
find_variable(const char *name, int32 length, int32 *_index)
{
	int32 i;

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
	char **newEnv;
	int32 i;

	if (sCopied)
		return B_OK;

	newEnv = malloc((count_variables() + 1) * sizeof(char *));
	if (newEnv == NULL)
		return B_NO_MEMORY;

	for (i = 0; environ[i]; i++) {
		newEnv[i] = strdup(environ[i]);
	}

	newEnv[i] = NULL;
		// null terminate the array

	sCopied = true;
	return B_OK;
}


static status_t
update_variable(const char *name, int32 length, const char *value, bool overwrite)
{
	status_t status = B_OK;
	bool update = false;
	int32 index;
	char *env;

	acquire_sem(sEnvLock);
	copy_environ_to_heap_if_needed();

	env = find_variable(name, length, &index);
	if (env != NULL && overwrite) {
		// change variable
		free(environ[index]);
		update = true;
	} else if (env == NULL) {
		// add variable
		index = add_variable();
		update = true;
	}

	if (update) {
		environ[index] = malloc(length + 2 + strlen(value));
		if (environ[index] != NULL) {
			memcpy(environ[index], name, length);
			environ[index][length] = '=';
			strcpy(environ[index] + length + 1, value);
		} else
			status = B_NO_MEMORY;
	}

	release_sem(sEnvLock);
	return status;
}


static void
environ_fork_hook(void)
{
	sEnvLock = create_sem(1, "env lock");
}


void 
__init_env(const struct uspace_program_args *args)
{
	// Following POSIX, there is no need to make any of the
	// environment functions thread-safe - but we do it anyway
	sEnvLock = create_sem(1, "env lock");
	environ = args->envp;

	atfork(environ_fork_hook);
}


//	#pragma mark -


char *
getenv(const char *name)
{
	int32 length = strlen(name);
	char *value;

	acquire_sem(sEnvLock);
	value = find_variable(name, length, NULL);
	release_sem(sEnvLock);

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

	status = update_variable(name, strlen(name), value, overwrite);
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

	acquire_sem(sEnvLock);
	copy_environ_to_heap_if_needed();

	env = find_variable(name, length, &index);
	if (env != NULL) {
		// we don't free the memory for the slot, we just move the array contents
		free(env);
		memmove(environ + index, environ + index + 1, sizeof(char *) * (count_variables() + 1));
	}

	release_sem(sEnvLock);
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

	status = update_variable(string, value - string, value + 1, true);
	RETURN_AND_SET_ERRNO(status);
}

