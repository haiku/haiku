/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <drivers/safemode_settings.h>
#include <string.h>
#include <syscalls.h>


extern "C" bool
__get_safemode_boolean(const char* parameter, bool defaultValue)
{
	char buffer[32];
	size_t size = sizeof(buffer);
	if (_kern_get_safemode_option(parameter, buffer, &size) != B_OK)
		return defaultValue;

	if (strcasecmp(buffer, "enabled") == 0
		|| strcasecmp(buffer, "on") == 0
		|| strcasecmp(buffer, "true") == 0
		|| strcasecmp(buffer, "yes") == 0
		|| strcasecmp(buffer, "enable") == 0
		|| strcmp(buffer, "1") == 0) {
		return true;
	}

	return defaultValue;
}
