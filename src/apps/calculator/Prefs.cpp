/*
 * Copyright (c) 2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Peter Wagner <pwagner@stanfordalumni.org>
 */
#include <StorageKit.h>
#include <string.h>
#include <iostream.h>
#include "Prefs.h"

bool ReadPrefs(const char *prefsname, void *prefs, size_t sz)
{
	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);

	BDirectory dir;
	dir.SetTo(path.Path());

	BEntry entry;
	if (dir.FindEntry(prefsname, &entry) == B_OK) {
		BFile file(&entry, O_RDONLY);
		if (file.InitCheck() == B_OK) {
			if ((size_t)file.Read(prefs, sz) == sz)
				return true;
		}
	}
	
	memset(prefs, 0, sz);
	return false;
}


bool WritePrefs(const char *prefsname, void *prefs, size_t sz)
{
	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);

	BDirectory dir;
	dir.SetTo(path.Path());

	BFile file;
	if (dir.CreateFile(prefsname, &file, false) == B_OK) {
		if ((size_t)file.Write(prefs, sz) == sz)
			return true;
	}

	return false;
}
