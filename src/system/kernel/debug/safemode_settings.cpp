/*
 * Copyright 2004-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <safemode.h>

#include <ctype.h>
#include <string.h>
#include <strings.h>

#include <algorithm>

#include <KernelExport.h>

#include <boot/kernel_args.h>
#include <kernel.h>
#include <syscalls.h>


#ifndef _BOOT_MODE


static status_t
get_option_from_kernel_args(kernel_args* args, const char* settingsName,
	const char* parameter, size_t parameterLength, char* buffer,
	size_t* _bufferSize)
{
	// find the settings in the kernel args
	const char* settings = NULL;
	for (driver_settings_file* file = args->driver_settings;
			file != NULL; file = file->next) {
		if (strcmp(settingsName, file->name) == 0) {
			settings = file->buffer;
			break;
		}
	}

	if (settings == NULL)
		return B_ENTRY_NOT_FOUND;

	// Unfortunately we can't just use parse_driver_settings_string(), since
	// we might not have a working heap yet. So we do very limited parsing
	// ourselves.
	const char* settingsEnd = settings + strlen(settings);
	int32 parameterLevel = 0;

	while (*settings != '\0') {
		// find end of line
		const char* lineEnd = strchr(settings, '\n');
		const char* nextLine;
		if (lineEnd != NULL)
			nextLine = lineEnd + 1;
		else
			nextLine = lineEnd = settingsEnd;

		// ignore any trailing comments
		lineEnd = std::find(settings, lineEnd, '#');

		const char* nameStart = NULL;
		const char* nameEnd = NULL;
		const char* valueStart = NULL;
		const char* valueEnd = NULL;
		const char** elementEnd = NULL;
		bool sawSeparator = true;

		for (; settings < lineEnd; settings++) {
			switch (*settings) {
				case '{':
					parameterLevel++;
					sawSeparator = true;
					break;

				case '}':
					parameterLevel--;
					sawSeparator = true;
					break;

				case ';':
					// TODO: That's not correct. There should be another loop.
					sawSeparator = true;
					break;

				default:
					if (parameterLevel != 0)
						break;

					if (isspace(*settings)) {
						sawSeparator = true;
						break;
					}

					if (!sawSeparator)
						break;

					sawSeparator = false;

					if (nameStart == NULL) {
						nameStart = settings;
						elementEnd = &nameEnd;
					} else if (valueStart == NULL) {
						valueStart = settings;
						elementEnd = &valueEnd;
					}
					break;
			}

			if (sawSeparator && elementEnd != NULL) {
				*elementEnd = settings;
				elementEnd = NULL;
			}
		}

		if (elementEnd != NULL)
			*elementEnd = settings;

		if (nameStart != NULL && size_t(nameEnd - nameStart) == parameterLength
			&& strncmp(parameter, nameStart, parameterLength) == 0) {
			if (valueStart == NULL)
				return B_NAME_NOT_FOUND;

			size_t length = valueEnd - valueStart;
			if (*_bufferSize > 0) {
				size_t toCopy = std::min(length, *_bufferSize - 1);
				memcpy(buffer, valueStart, toCopy);
				buffer[toCopy] = '\0';
			}

			*_bufferSize = length;
			return B_OK;
		}

		settings = nextLine;
	}

	return B_NAME_NOT_FOUND;
}


#endif	// !_BOOT_MODE


static status_t
get_option(kernel_args* args, const char* settingsName, const char* parameter,
	size_t parameterLength, char* buffer, size_t* _bufferSize)
{
#ifndef _BOOT_MODE
	if (args != NULL) {
		return get_option_from_kernel_args(args, settingsName, parameter,
			parameterLength, buffer, _bufferSize);
	}
#endif

	void* handle = load_driver_settings(settingsName);
	if (handle == NULL)
		return B_ENTRY_NOT_FOUND;

	status_t status = B_NAME_NOT_FOUND;

	const char* value = get_driver_parameter(handle, parameter, NULL, NULL);
	if (value != NULL) {
		*_bufferSize = strlcpy(buffer, value, *_bufferSize);
		status = B_OK;
	}

	unload_driver_settings(handle);
	return status;
}


static status_t
get_option(kernel_args* args, const char* parameter, char* buffer,
	size_t* _bufferSize)
{
	size_t parameterLength = strlen(parameter);
	status_t status = get_option(args, B_SAFEMODE_DRIVER_SETTINGS, parameter,
		parameterLength, buffer, _bufferSize);
	if (status != B_OK) {
		// Try kernel settings file as a fall back
		status = get_option(args, "kernel", parameter, parameterLength, buffer,
			_bufferSize);
	}

	return status;
}


static bool
get_boolean(kernel_args* args, const char* parameter, bool defaultValue)
{
	char value[16];
	size_t length = sizeof(value);

	if (get_option(args, parameter, value, &length) != B_OK)
		return defaultValue;

	return !strcasecmp(value, "on") || !strcasecmp(value, "true")
		|| !strcmp(value, "1") || !strcasecmp(value, "yes")
		|| !strcasecmp(value, "enabled");
}


// #pragma mark -


status_t
get_safemode_option(const char* parameter, char* buffer, size_t* _bufferSize)
{
	return get_option(NULL, parameter, buffer, _bufferSize);
}


bool
get_safemode_boolean(const char* parameter, bool defaultValue)
{
	return get_boolean(NULL, parameter, defaultValue);
}


#ifndef _BOOT_MODE


status_t
get_safemode_option_early(kernel_args* args, const char* parameter,
	char* buffer, size_t* _bufferSize)
{
	return get_option(args, parameter, buffer, _bufferSize);
}


bool
get_safemode_boolean_early(kernel_args* args, const char* parameter,
	bool defaultValue)
{
	return get_boolean(args, parameter, defaultValue);
}


#endif	// _BOOT_MODE


//	#pragma mark - syscalls


#ifndef _BOOT_MODE


extern "C" status_t
_user_get_safemode_option(const char* userParameter, char* userBuffer,
	size_t* _userBufferSize)
{
	char parameter[B_FILE_NAME_LENGTH];
	char buffer[B_PATH_NAME_LENGTH];
	size_t bufferSize, originalBufferSize;

	if (!IS_USER_ADDRESS(userParameter) || !IS_USER_ADDRESS(userBuffer)
		|| !IS_USER_ADDRESS(_userBufferSize)
		|| user_memcpy(&bufferSize, _userBufferSize, sizeof(size_t)) != B_OK
		|| user_strlcpy(parameter, userParameter, B_FILE_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	if (bufferSize > B_PATH_NAME_LENGTH)
		bufferSize = B_PATH_NAME_LENGTH;

	originalBufferSize = bufferSize;
	status_t status = get_safemode_option(parameter, buffer, &bufferSize);

	if (status == B_OK
		&& (user_strlcpy(userBuffer, buffer, originalBufferSize) < B_OK
			|| user_memcpy(_userBufferSize, &bufferSize, sizeof(size_t))
				!= B_OK))
		return B_BAD_ADDRESS;

	return status;
}


#endif	// !_BOOT_MODE
