/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <boot/vfs.h>
#include <boot/stdio.h>
#include <util/kernel_cpp.h>

#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include <algorithm>


//#undef stdout
//#undef stdin
//extern FILE *stdout;
//extern FILE *stdin;


#undef errno
int errno;


int*
_errnop(void)
{
	return &errno;
}


int
vfprintf(FILE *file, const char *format, va_list list)
{
	ConsoleNode *node = (ConsoleNode *)file;
	char buffer[512];
		// the buffer handling could (or should) be done better...

	int length = vsnprintf(buffer, sizeof(buffer), format, list);
	length = std::min(length, (int)sizeof(buffer) - 1);
	if (length > 0)
		node->Write(buffer, length);

	return length;
}


int
vprintf(const char *format, va_list args)
{
	return vfprintf(stdout, format, args);
}


int
printf(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	int status = vfprintf(stdout, format, args);
	va_end(args);

	return status;
}


int
fprintf(FILE *file, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	int status = vfprintf(file, format, args);
	va_end(args);

	return status;
}


int
fputc(int c, FILE *file)
{
	if (file == NULL)
		return B_FILE_ERROR;

    status_t status;
	char character = (char)c;

	// we only support direct console output right now...
	status = ((ConsoleNode *)file)->Write(&character, 1);

	if (status > 0)
		return character;

	return status;
}


int
fputs(const char *string, FILE *file)
{
	if (file == NULL)
		return B_FILE_ERROR;

	status_t status = ((ConsoleNode *)file)->Write(string, strlen(string));
	fputc('\n', file);

	return status;
}


int
putc(int character)
{
	return fputc(character, stdout);
}


int
putchar(int character)
{
	return fputc(character, stdout);
}


int
puts(const char *string)
{
	return fputs(string, stdout);
}

