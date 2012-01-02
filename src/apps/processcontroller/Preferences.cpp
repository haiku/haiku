/*
	ProcessController © 2000, Georges-Edouard Berenger, All Rights Reserved.
	Copyright (C) 2004 beunited.org 

	This library is free software; you can redistribute it and/or 
	modify it under the terms of the GNU Lesser General Public 
	License as published by the Free Software Foundation; either 
	version 2.1 of the License, or (at your option) any later version. 

	This library is distributed in the hope that it will be useful, 
	but WITHOUT ANY WARRANTY; without even the implied warranty of 
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
	Lesser General Public License for more details. 

	You should have received a copy of the GNU Lesser General Public 
	License along with this library; if not, write to the Free Software 
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA	
*/


#include "Preferences.h"
#include "Utilities.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Alert.h>
#include <Catalog.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Locker.h>
#include <Mime.h>
#include <Path.h>

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "ProcessController"

Preferences::Preferences(const char* name, const char* signature, bool doSave)
	: BMessage('Pref'), BLocker("Preferences", true),
	fSavePreferences(doSave)
{
	fNewPreferences = false;
	fSettingsFile = 0;
	BPath prefpath;
	fName = strdup(name);
	if (signature)
		fSignature = strdup(signature);
	else
		fSignature = NULL;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &prefpath) == B_OK) {
		BDirectory prefdir(prefpath.Path());
		BEntry entry;
		prefdir.FindEntry(fName, &entry);

		BFile file(&entry, B_READ_ONLY);
		if (file.InitCheck() == B_OK)
			Unflatten(&file);
		else
			fNewPreferences = true;
	}
}


Preferences::Preferences(const entry_ref &ref, const char* signature, bool doSave)
	: BMessage('Pref'), BLocker("Preferences", true),
	fSavePreferences(doSave)
{
	fSettingsFile = new entry_ref(ref);
	fNewPreferences = false;
	BPath prefpath;
	fName = NULL;
	if (signature)
		fSignature = strdup(signature);
	else
		fSignature = NULL;

	BFile file(fSettingsFile, B_READ_ONLY);
	if (file.InitCheck() == B_OK)
		Unflatten(&file);
	else
		fNewPreferences = true;
}


Preferences::~Preferences()
{
	if (fSavePreferences) {
		BFile file;
		status_t set = B_ERROR;
		if (fSettingsFile)
			file.SetTo(fSettingsFile, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		else {
			BPath prefpath;
			if (find_directory(B_USER_SETTINGS_DIRECTORY, &prefpath, true) == B_OK) {
				BDirectory prefdir(prefpath.Path());
				set = prefdir.CreateFile(fName, &file, false);
			}
		}

		if (file.InitCheck () == B_OK) {
			Flatten(&file);
			if (fSignature) {
				file.WriteAttr("BEOS:TYPE", B_MIME_STRING_TYPE, 0, fSignature,
					strlen(fSignature) + 1);
			}
		} else {
			// implement saving somewhere else!
			BString error;
			snprintf(error.LockBuffer(256), 256,
				B_TRANSLATE("Your setting file could not be saved!\n(%s)"),
				strerror(file.InitCheck()));
			error.UnlockBuffer();
			BAlert *alert = new BAlert(B_TRANSLATE("Error saving file"),
				error.String(), B_TRANSLATE("Damned!"), NULL, NULL,
				B_WIDTH_AS_USUAL, B_STOP_ALERT);
			alert->SetShortcut(0, B_ESCAPE);

			alert->Go();
		}
	}
	delete fSettingsFile;
	free(fName);
	free(fSignature);
}


status_t
Preferences::MakeEmpty()
{
	Lock();
	status_t status = BMessage::MakeEmpty();
	Unlock();
	return status;
}


void
Preferences::SaveWindowPosition(BWindow* window, const char* name)
{
	Lock();

	BRect rect = window->Frame();
	if (HasPoint(name))
		ReplacePoint(name, rect.LeftTop());
	else
		AddPoint(name, rect.LeftTop());

	Unlock();
}


void
Preferences::LoadWindowPosition(BWindow* window, const char* name)
{
	Lock();

	BPoint p;
	if (FindPoint(name, &p) == B_OK) {
		window->MoveTo(p);
		make_window_visible(window);
	}

	Unlock();
}


void
Preferences::SaveWindowFrame(BWindow* window, const char* name)
{
	Lock();

	BRect rect = window->Frame();
	if (HasRect(name))
		ReplaceRect(name, rect);
	else
		AddRect(name, rect);

	Unlock();
}


void
Preferences::LoadWindowFrame(BWindow* window, const char* name)
{
	Lock();

	BRect frame;
	if (FindRect(name, &frame) == B_OK) {
		window->MoveTo(frame.LeftTop());
		window->ResizeTo(frame.Width(), frame.Height());
		make_window_visible(window);
	}

	Unlock();
}


void
Preferences::SaveInt32(int32 value, const char* name)
{
	Lock();

	if (HasInt32(name))
		ReplaceInt32(name, value);
	else
		AddInt32(name, value);

	Unlock();
}


bool
Preferences::ReadInt32(int32 &val, const char* name)
{
	Lock();
	int32 readVal;
	bool found = FindInt32(name, &readVal) == B_OK;
	if (found)
		val = readVal;
	Unlock();
	return found;
}


void
Preferences::SaveFloat(float val, const char* name)
{
	Lock();
	if (HasFloat(name))
		ReplaceFloat(name, val);
	else
		AddFloat(name, val);
	Unlock();
}


bool
Preferences::ReadFloat(float &val, const char* name)
{
	Lock();
	float readVal;
	bool found = FindFloat(name, &readVal) == B_OK;
	if (found)
		val = readVal;
	Unlock();
	return found;
}


void
Preferences::SaveRect(BRect& rect, const char* name)
{
	Lock();
	if (HasRect(name))
		ReplaceRect(name, rect);
	else
		AddRect(name, rect);
	Unlock();
}


BRect &
Preferences::ReadRect(BRect& rect, const char* name)
{
	Lock();
	BRect loaded;
	if (FindRect(name, &loaded) == B_OK)
		rect = loaded;
	Unlock();
	return rect;
}


void
Preferences::SaveString(BString &string, const char* name)
{
	Lock();
	if (HasString(name))
		ReplaceString(name, string);
	else
		AddString(name, string);
	Unlock();
}


void
Preferences::SaveString(const char* string, const char* name)
{
	Lock();
	if (HasString(name))
		ReplaceString(name, string);
	else
		AddString(name, string);
	Unlock();
}


bool
Preferences::ReadString(BString &string, const char* name)
{
	Lock();
	bool loaded = FindString(name, &string) == B_OK;
	Unlock();
	return loaded;
}
