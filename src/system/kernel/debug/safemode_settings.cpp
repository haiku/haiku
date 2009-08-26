/*
 * Copyright 2004-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>

#include <safemode.h>
#include <kernel.h>
#include <syscalls.h>

#include <string.h>


extern "C" status_t
get_safemode_option(const char* parameter, char* buffer, size_t* _bufferSize)
{
	status_t status = B_ENTRY_NOT_FOUND;

	void* handle = load_driver_settings(B_SAFEMODE_DRIVER_SETTINGS);
	if (handle != NULL) {
		status = B_NAME_NOT_FOUND;

		const char* value = get_driver_parameter(handle, parameter, NULL, NULL);
		if (value != NULL) {
			*_bufferSize = strlcpy(buffer, value, *_bufferSize);
			status = B_OK;
		}

		unload_driver_settings(handle);
	}

	if (status != B_OK) {
		// Try kernel settings file as a fall back
		handle = load_driver_settings("kernel");
		if (handle != NULL) {
			const char* value = get_driver_parameter(handle, parameter, NULL,
				NULL);
			if (value != NULL) {
				*_bufferSize = strlcpy(buffer, value, *_bufferSize);
				status = B_OK;
			}

			unload_driver_settings(handle);
		}
	}

	return status;
}


extern "C" bool
get_safemode_boolean(const char* parameter, bool defaultValue)
{
	char value[16];
	size_t length = sizeof(value);

	if (get_safemode_option(parameter, value, &length) != B_OK)
		return defaultValue;

	return !strcasecmp(value, "on") || !strcasecmp(value, "true")
		|| !strcmp(value, "1") || !strcasecmp(value, "yes")
		|| !strcasecmp(value, "enabled");
}


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
