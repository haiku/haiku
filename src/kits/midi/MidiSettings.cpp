/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <MidiSettings.h>

#include <File.h>
#include <FindDirectory.h>
#include <Path.h>

#include <driver_settings.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SETTINGS_FILE "Media/midi_settings"

namespace BPrivate {

status_t
read_midi_settings(struct midi_settings* settings)
{
	if (settings == NULL)
		return B_ERROR;

	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK)
		return status;

	path.Append(SETTINGS_FILE);
	void* handle = load_driver_settings(path.Path());
	if (handle == NULL)
		return B_ERROR;

	const char* soundfont = get_driver_parameter(handle, "soundfont", NULL,
		NULL);
	if (soundfont == NULL) {
		unload_driver_settings(handle);
		return B_ERROR;
	}
	strlcpy(settings->soundfont_file, soundfont,
		sizeof(settings->soundfont_file));

	unload_driver_settings(handle);
	return B_OK;
}


status_t
write_midi_settings(struct midi_settings settings)
{
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK)
		return status;

	path.Append(SETTINGS_FILE);

	BFile file;
	if (file.SetTo(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE)
		!= B_OK)
		return B_ERROR;

	char buffer[B_FILE_NAME_LENGTH + 128];
	snprintf(buffer, sizeof(buffer), "# Midi\n\tsoundfont \"%s\"\n",
			settings.soundfont_file);

	size_t bufferSize = strlen(buffer);
	if (file.InitCheck() != B_OK
		|| file.Write(buffer, bufferSize) != (ssize_t)bufferSize)
		return B_ERROR;

	return B_OK;
}

}
