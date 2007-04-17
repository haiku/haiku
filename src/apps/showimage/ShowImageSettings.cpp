/*****************************************************************************/
// ShowImageSettings
// Written by Michael Pfeiffer
//
// ShowImageSettings.cpp
//
//
// Copyright (c) 2003 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#include <File.h>
#include <FindDirectory.h>
#include <Path.h>

#include "ShowImageSettings.h"

ShowImageSettings::ShowImageSettings()
{
	Load();
}

ShowImageSettings::~ShowImageSettings()
{
	if (Lock()) {
		Save();
		Unlock();
	}
}

bool
ShowImageSettings::Lock()
{
	return fLock.Lock();
}

void
ShowImageSettings::Unlock()
{
	fLock.Unlock();
}

bool
ShowImageSettings::GetBool(const char* name, bool defaultValue)
{
	bool value;
	if (fSettings.FindBool(name, &value) == B_OK) {
		return value;
	} else {
		return defaultValue;
	}
}

int32 
ShowImageSettings::GetInt32(const char* name, int32 defaultValue)
{
	int32 value;
	if (fSettings.FindInt32(name, &value) == B_OK) {
		return value;
	} else {
		return defaultValue;
	}
}

float 
ShowImageSettings::GetFloat(const char* name, float defaultValue)
{
	float value;
	if (fSettings.FindFloat(name, &value) == B_OK) {
		return value;
	} else {
		return defaultValue;
	}
}

BRect
ShowImageSettings::GetRect(const char* name, BRect defaultValue)
{
	BRect value;
	if (fSettings.FindRect(name, &value) == B_OK) {
		return value;
	} else {
		return defaultValue;
	}
}

const char*
ShowImageSettings::GetString(const char* name, const char* defaultValue)
{
	const char* value;
	if (fSettings.FindString(name, &value) == B_OK) {
		return value;
	} else {
		return defaultValue;
	}
}

void
ShowImageSettings::SetBool(const char* name, bool value)
{
	if (fSettings.HasBool(name)) {
		fSettings.ReplaceBool(name, value);
	} else {
		fSettings.AddBool(name, value);
	}
}

void
ShowImageSettings::SetInt32(const char* name, int32 value)
{
	if (fSettings.HasInt32(name)) {
		fSettings.ReplaceInt32(name, value);
	} else {
		fSettings.AddInt32(name, value);
	}
}

void
ShowImageSettings::SetFloat(const char* name, float value)
{
	if (fSettings.HasFloat(name)) {
		fSettings.ReplaceFloat(name, value);
	} else {
		fSettings.AddFloat(name, value);
	}
}

void
ShowImageSettings::SetRect(const char* name, BRect value)
{
	if (fSettings.HasRect(name)) {
		fSettings.ReplaceRect(name, value);
	} else {
		fSettings.AddRect(name, value);
	}
}

void
ShowImageSettings::SetString(const char* name, const char* value)
{
	if (fSettings.HasString(name)) {
		fSettings.ReplaceString(name, value);
	} else {
		fSettings.AddString(name, value);
	}
}

bool
ShowImageSettings::OpenSettingsFile(BFile* file, bool forReading)
{
	status_t st;
	BPath path;
	uint32 openMode;
	
	st = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (st != B_OK) return false;
	
	path.Append("ShowImage_settings");
	if (forReading) {
		openMode = B_READ_ONLY;
	} else {
		openMode = B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE;
	}
	st = file->SetTo(path.Path(), openMode);
	return st == B_OK;
}

void
ShowImageSettings::Load()
{
	BFile file;
	if (OpenSettingsFile(&file, true)) {
		fSettings.Unflatten(&file);
	}
}

void
ShowImageSettings::Save()
{
	BFile file;
	if (OpenSettingsFile(&file, false)) {
		fSettings.Flatten(&file);
	}
}
