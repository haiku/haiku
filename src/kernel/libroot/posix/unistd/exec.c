/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <syscalls.h>

#include <unistd.h>
#include <errno.h>
#include <alloca.h>
#include <stdio.h>
#include <stdarg.h>


static int
count_arguments(va_list list, const char *arg, char ***_env)
{
	int count = 0;

	while (arg != NULL) {
		count++;
		arg = va_arg(list, const char *);
	}

	if (_env)
		*_env = va_arg(list, char **);

	return count;
}


static void
copy_arguments(va_list list, const char **args, const char *arg)
{
	int count = 0;

	while (arg != NULL) {
		arg = va_arg(list, const char *);
		args[count++] = arg;
	}

	args[count] = NULL;
		// terminate list
}


//	#pragma mark -


int
execve(const char *path, char * const argv[], char * const envp[])
{
	// ToDo: implement me for real!
	fprintf(stderr, "execve(): NOT IMPLEMENTED\n");
	return -1;
}


int
execv(const char *path, char * const *argv)
{
	return execve(path, argv, environ);
}


int
execvp(const char *file, char * const *argv)
{
	// ToDo: do the "p" thing
	return execve(file, argv, environ);
}


int
execl(const char *path, const char *arg, ...)
{
	const char **args;
	va_list list;
	int count;

	// count arguments

	va_start(list, arg);
	count = count_arguments(list, arg, NULL);
	va_end(list);

	// copy arguments

	args = alloca((count + 1) * sizeof(char *));
	va_start(list, arg);
	copy_arguments(list, args, arg);
	va_end(list);

	return execve(path, (char * const *)args, environ);
}


int
execlp(const char *file, const char *arg, ...)
{
	const char **args;
	va_list list;
	int count;

	// count arguments

	va_start(list, arg);
	count = count_arguments(list, arg, NULL);
	va_end(list);

	// copy arguments

	args = alloca((count + 1) * sizeof(char *));
	va_start(list, arg);
	copy_arguments(list, args, arg);
	va_end(list);

	return execvp(file, (char * const *)args);
}


int
execle(const char *path, const char *arg, ... /*, char **env */)
{
	const char **args;
	char **env;
	va_list list;
	int count;

	// count arguments

	va_start(list, arg);
	count = count_arguments(list, arg, &env);
	va_end(list);

	// copy arguments

	args = alloca((count + 1) * sizeof(char *));
	va_start(list, arg);
	copy_arguments(list, args, arg);
	va_end(list);

	return execve(path, (char * const *)args, env);
}


int
exect(const char *path, char * const *argv)
{
	// ToDo: is this any different?
	return execv(path, argv);
}

