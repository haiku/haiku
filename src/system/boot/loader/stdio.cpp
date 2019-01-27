/*
 * Copyright 2003-2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <boot/vfs.h>
#include <boot/stdio.h>
#include <boot/net/NetStack.h>
#include <boot/net/UDP.h>

#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include <algorithm>


//#undef stdout
//#undef stdin
//extern FILE *stdout;
//extern FILE *stdin;


//#define ENABLE_SYSLOG


#undef errno
int errno;


int*
_errnop(void)
{
	return &errno;
}


#ifdef ENABLE_SYSLOG

static UDPSocket *sSyslogSocket = NULL;

static void
sendToSyslog(const char *message, int length)
{
	// Lazy-initialize the socket
	if (sSyslogSocket == NULL) {
		// Check if the network stack has been initialized yet
		if (NetStack::Default() != NULL) {
			sSyslogSocket = new(std::nothrow) UDPSocket;
			sSyslogSocket->Bind(INADDR_ANY, 60514);
		}
	}

	if (sSyslogSocket == NULL)
		return;

	// Strip trailing newlines
	while (length > 0) {
		if (message[length - 1] != '\n'
			&& message[length - 1] != '\r') {
			break;
		}
		length--;
	}
	if (length <= 0)
		return;

	char buffer[1500];
		// same comment as in vfprintf applies...
	const int facility = 0; // kernel
	int severity = 7; // debug
	int offset = snprintf(buffer, sizeof(buffer),
		"<%d>1 - - Haiku - - - \xEF\xBB\xBF",
		facility * 8 + severity);
	length = std::min(length, (int)sizeof(buffer) - offset);
	memcpy(buffer + offset, message, length);
	sSyslogSocket->Send(INADDR_BROADCAST, 514, buffer, offset + length);
}

#endif


int
vfprintf(FILE *file, const char *format, va_list list)
{
	ConsoleNode *node = (ConsoleNode *)file;
	char buffer[512];
		// the buffer handling could (or should) be done better...

	int length = vsnprintf(buffer, sizeof(buffer), format, list);
	length = std::min(length, (int)sizeof(buffer) - 1);
	if (length > 0) {
		node->Write(buffer, length);
#ifdef ENABLE_SYSLOG
		sendToSyslog(buffer, length);
#endif
	}

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

#ifdef ENABLE_SYSLOG
	sendToSyslog(&character, 1);
#endif

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

#ifdef ENABLE_SYSLOG
	sendToSyslog(string, strlen(string));
#endif

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

