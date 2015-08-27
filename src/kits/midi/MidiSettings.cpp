/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <MidiSettings.h>

#include <File.h>
#include <FindDirectory.h>
#include <Path.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SETTINGS_FILE "midi"

namespace BPrivate {

status_t
read_midi_settings(struct midi_settings* settings)
{
	if (settings == NULL)
		return B_ERROR;

	char buffer[B_FILE_NAME_LENGTH + 128];
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK)
		return status;

	path.Append("midi");
	BFile file(path.Path(), B_READ_ONLY);
	if (file.InitCheck() != B_OK
			|| file.Read(buffer, sizeof(buffer)) <= 0)
		return B_ERROR;

	sscanf(buffer, "# Midi Settings\n soundfont = %s\n",
		settings->soundfont_file);

	return B_OK;
}


status_t
write_midi_settings(struct midi_settings settings)
{
	char buffer[B_FILE_NAME_LENGTH + 128];
	snprintf(buffer, sizeof(buffer), "# Midi Settings\n soundfont = %s\n",
			settings.soundfont_file);

	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK)
		return status;

	path.Append(SETTINGS_FILE);
	BFile file(path.Path(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);

	size_t bufferSize = strlen(buffer);
	if (file.InitCheck() != B_OK
		|| file.Write(buffer, bufferSize) != (ssize_t)bufferSize)
		return B_ERROR;

	return B_OK;
}

}
