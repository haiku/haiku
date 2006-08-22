/*
 * Copyright 2003-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jonas Sundström, jonas.sundstrom@kirilla.com
 */
#ifndef ZIPOMATIC_SETTINGS_H
#define ZIPOMATIC_SETTINGS_H


#include <FindDirectory.h>
#include <Message.h>
#include <String.h>
#include <Volume.h>


class ZippoSettings : public BMessage {
	public:
					ZippoSettings();
					ZippoSettings(BMessage& message);
					~ZippoSettings();

		status_t	SetTo(const char* filename, BVolume* volume = NULL,
						directory_which baseDir = B_USER_SETTINGS_DIRECTORY,
						const char* relativePath = NULL);

		status_t	InitCheck();

		status_t	ReadSettings();
		status_t	WriteSettings();

	private:
		status_t	_GetSettingsFile(BFile* file, uint32 openMode);

		BVolume		fVolume;
		directory_which	fBaseDir;
		BString		fRelativePath;
		BString		fFilename;
};

#endif	// ZIPOMATIC_SETTINGS_H
