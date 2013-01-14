/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef SETTINGS_FILE_H
#define SETTINGS_FILE_H


#include <File.h>
#include <FindDirectory.h>
#include <Path.h>


class SettingsFile
{
public:
	int32		UndoLimit;
	// TODO: Add more settings here (1/3).

				SettingsFile(const char* name);
				~SettingsFile();

	void 		Defaults();
	void 		Load();
	void 		Save();

private:
	BPath 		fPath;
	BFile* 		fFile;
};


#endif
