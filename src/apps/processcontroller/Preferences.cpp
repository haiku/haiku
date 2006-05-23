/*
	
	Preferences.cpp
	
	ProcessController
	(c) 2000, Georges-Edouard Berenger, All Rights Reserved.
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
#include "Locker.h"
#include "WindowsTools.h"

#include <Path.h>
#include <string.h>
#include <stdlib.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <File.h>
#include <Mime.h>
#include <Alert.h>
#include <stdio.h>

const char* kDirectory = "Geb";

GebsPreferences::GebsPreferences(const char* thename, const char* thesignature, bool doSave) : BMessage('Pref'), BLocker("Preferences", true), fSavePreferences(doSave)
{
	fNewPreferences = false;
	fSettingsFile = 0;
	BPath	prefpath;
	fName=strdup(thename);
	if (thesignature)
		fSignature=strdup(thesignature);
	else
		fSignature=NULL;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &prefpath)==B_OK) {
		BDirectory	prefdir(prefpath.Path());
		BEntry		entry;
		prefdir.FindEntry(kDirectory, &entry, true);
		prefdir.SetTo(&entry);
		prefdir.FindEntry(fName, &entry);
		BFile		file(&entry, B_READ_ONLY);
		if (file.InitCheck()==B_OK)
			Unflatten(&file);
		else
			fNewPreferences=true;
	}
}

GebsPreferences::GebsPreferences(const entry_ref &ref, const char* thesignature, bool doSave)
	: BMessage('Pref'), BLocker("Preferences", true), fSavePreferences (doSave)
{
	fSettingsFile = new entry_ref (ref);
	fNewPreferences = false;
	BPath	prefpath;
	fName = 0;
	if (thesignature)
		fSignature = strdup(thesignature);
	else
		fSignature = NULL;
	BFile	file (fSettingsFile, B_READ_ONLY);
	if (file.InitCheck() == B_OK)
		Unflatten(&file);
	else
		fNewPreferences = true;
}

GebsPreferences::~GebsPreferences()
{
	if (fSavePreferences) {
		BFile		file;
		status_t	set = B_ERROR;
		if (fSettingsFile)
			file.SetTo (fSettingsFile, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		else
		{
			BPath	prefpath;
			if (find_directory(B_USER_SETTINGS_DIRECTORY, &prefpath, true)==B_OK) {
				BDirectory	prefdir(prefpath.Path());
				BDirectory	gebprefdir;
				BEntry		entry;
				if (prefdir.FindEntry(kDirectory, &entry, true)==B_OK)
					gebprefdir.SetTo(&entry);
				else
					prefdir.CreateDirectory (kDirectory, &gebprefdir);
				if (gebprefdir.InitCheck () == B_OK)
					set = gebprefdir.CreateFile (fName, &file, false);
			}
		}
		if (file.InitCheck () == B_OK)
		{
			Flatten (&file);
			if (fSignature)
				file.WriteAttr ("BEOS:TYPE", B_MIME_STRING_TYPE, 0, fSignature, strlen(fSignature)+1);
		}
		else
		{
			// implement saving somewhere else!
			char	error[1024];
			sprintf (error, "Your setting file could not be saved!\n(%s)", strerror (file.InitCheck ()));
			BAlert *alert = new BAlert("Error saving file", error,
								"Damned!", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
			alert->Go();
		}
	}
	free (fName);
	if (fSignature)
		free (fSignature);
}

status_t GebsPreferences::MakeEmpty()
{
	status_t	st;
	Lock();
	st=BMessage::MakeEmpty();
	Unlock();
	return st;
}

void GebsPreferences::SaveWindowPosition(BWindow* window, const char* name)
{
	BRect	rect=window->Frame();
	Lock();
	if (HasPoint(name))
		ReplacePoint(name, rect.LeftTop());
	else
		AddPoint(name, rect.LeftTop());
	Unlock();
}

void GebsPreferences::LoadWindowPosition(BWindow* window, const char* name)
{
	BPoint	p;
	Lock();
	if (FindPoint(name, &p)==B_OK) {
		window->MoveTo(p);
		visible_window(window);
	}
	Unlock();
}

void GebsPreferences::SaveWindowFrame(BWindow* window, const char* name)
{
	BRect	rect=window->Frame();
	Lock();
	if (HasRect(name))
		ReplaceRect(name, rect);
	else
		AddRect(name, rect);
	Unlock();
}

void GebsPreferences::LoadWindowFrame(BWindow* window, const char* name)
{
	BRect	f;
	Lock();
	if (FindRect(name, &f)==B_OK) {
		window->MoveTo(f.LeftTop());
		window->ResizeTo(f.Width(), f.Height());
		visible_window(window);
	}
	Unlock();
}

void GebsPreferences::SaveInt32(int32 val, const char* name)
{
	Lock();
	if (HasInt32(name))
		ReplaceInt32(name, val);
	else
		AddInt32(name, val);
	Unlock();
}

bool GebsPreferences::ReadInt32(int32 &val, const char* name)
{
	Lock();
	int32	readVal;
	bool found=FindInt32(name, &readVal)==B_OK;
	if (found)
		val=readVal;
	Unlock();
	return found;
}

void GebsPreferences::SaveFloat(float val, const char* name)
{
	Lock();
	if (HasFloat(name))
		ReplaceFloat(name, val);
	else
		AddFloat(name, val);
	Unlock();
}

bool GebsPreferences::ReadFloat(float &val, const char* name)
{
	Lock();
	float	readVal;
	bool found=FindFloat(name, &readVal)==B_OK;
	if (found)
		val=readVal;
	Unlock();
	return found;
}

void GebsPreferences::SaveRect(BRect &rect, const char* name)
{
	Lock();
	if (HasRect(name))
		ReplaceRect(name, rect);
	else
		AddRect(name, rect);
	Unlock();
}

BRect & GebsPreferences::ReadRect(BRect &rect, const char* name)
{
	Lock();
	BRect	loaded;
	if (FindRect(name, &loaded)==B_OK)
		rect=loaded;
	Unlock();
	return rect;
}

void GebsPreferences::SaveString (BString &string, const char* name)
{
	Lock ();
	if (HasString (name))
		ReplaceString (name, string);
	else
		AddString (name, string);
	Unlock ();
}

void GebsPreferences::SaveString (const char* string, const char* name)
{
	Lock ();
	if (HasString (name))
		ReplaceString (name, string);
	else
		AddString (name, string);
	Unlock ();
}

bool GebsPreferences::ReadString (BString &string, const char* name)
{
	Lock();
	bool loaded = FindString (name, &string) == B_OK;
	Unlock();
	return loaded;
}
