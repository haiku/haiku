/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "SettingsFile.h"


SettingsFile::SettingsFile(const char* name)
{
	find_directory(B_USER_SETTINGS_DIRECTORY, &fPath);
	fPath.Append(name, true);
}


SettingsFile::~SettingsFile()
{

}


void
SettingsFile::Defaults()
{
	UndoLimit = 100;
}


void
SettingsFile::Load()
{
	fFile = new BFile(fPath.Path(), B_READ_ONLY);

	if (fFile->InitCheck() == B_OK) {
		fFile->Read(&UndoLimit, sizeof(UndoLimit));
		// TODO: Add more settings here (2/3).
	} else
		Defaults();

	delete fFile;
}


void
SettingsFile::Save()
{
	fFile = new BFile(fPath.Path(), B_WRITE_ONLY | B_CREATE_FILE);

	if (fFile->InitCheck() == B_OK) {
		fFile->Write(&UndoLimit, sizeof(UndoLimit));
		// TODO: Add more settings here (3/3).
	}

	delete fFile;
}
