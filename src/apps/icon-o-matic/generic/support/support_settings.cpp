/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "support_settings.h"

#include <stdio.h>

#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Path.h>

// load_settings
status_t
load_settings(BMessage* message, const char* fileName, const char* folder)
{
	status_t ret = B_BAD_VALUE;
	if (message) {
		BPath path;
		if ((ret = find_directory(B_USER_SETTINGS_DIRECTORY, &path)) == B_OK) {
			// passing folder is optional
			if (folder)
				ret = path.Append(folder);
			if (ret == B_OK && (ret = path.Append(fileName)) == B_OK) {
				BFile file(path.Path(), B_READ_ONLY);
				if ((ret = file.InitCheck()) == B_OK) {
					ret = message->Unflatten(&file);
					file.Unset();
				}
			}
		}
	}
	return ret;
}

// save_settings
status_t
save_settings(BMessage* message, const char* fileName, const char* folder)
{
	status_t ret = B_BAD_VALUE;
	if (message) {
		BPath path;
		if ((ret = find_directory(B_USER_SETTINGS_DIRECTORY, &path)) == B_OK) {
			// passing folder is optional
			if (folder && (ret = path.Append(folder)) == B_OK)
				ret = create_directory(path.Path(), 0777);
			if (ret == B_OK && (ret = path.Append(fileName)) == B_OK) {
				BFile file(path.Path(),
						   B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
				if ((ret = file.InitCheck()) == B_OK) {
					ret = message->Flatten(&file);
					file.Unset();
				}
			}
		}
	}
	return ret;
}

