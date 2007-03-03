/*****************************************************************************/
// Expander
// Written by Jérôme Duval
//
// ExpanderSettings.cpp
//
// Code from Diskprobe by Axel Dörfler
//
// Copyright (c) 2004 OpenBeOS Project
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
#include "ExpanderSettings.h"

#include <ByteOrder.h>
#include <Screen.h>
#include <FindDirectory.h>
#include <Entry.h>
#include <stdlib.h>
#include <string.h>
#include <Path.h>

//	Format of Expander_Settings
//	1st byte :	unknown (0x1)
//	2nd byte :	Automatically expand files (default : 0x0)
//	3rd byte :	Close when done (default : 0x0)
//	4th byte :	0x66
//				0x63 (default)
//				0x65
//	5th byte : 	unknown (0x0)
//	4 bytes : 	dev_t	(default : 0xffff)
//	8 bytes : 	ino_t	(default : 0xfffffffff)
//	4 bytes : 	name length (big endian) (default : 0x0)
//	n bytes : 	name	(default : "")
//	1 byte : 	Open destination folder (default : 0x1)
//	1 byte : 	Show content listing (default : 0x0)
//	4 bytes : 	window position topleft x (default : 0x4842)
//	4 bytes : 	window position topleft y (default : 0x4842)

ExpanderSettings::ExpanderSettings()
	:
	fMessage(kMsgExpanderSettings),
	fUpdated(false)
{
	fMessage.AddBool("automatically_expand_files", false);
	fMessage.AddBool("close_when_done", false);
	fMessage.AddInt8("destination_folder", 0x63);
	entry_ref ref;
	fMessage.AddRef("destination_folder_use", &ref);
	fMessage.AddBool("open_destination_folder", true);
	fMessage.AddBool("show_contents_listing", false);
	fMessage.AddPoint("window_position", BPoint(50, 50));

	BFile file;
	if (Open(&file, B_READ_ONLY) != B_OK)
		return;

	// ToDo: load/save settings as flattened BMessage - but not yet,
	//		since that will break compatibility with R5's Expander

	bool unknown;
	bool automatically_expand_files;
	bool close_when_done;
	int8 destination_folder;
	bool open_destination_folder;
	bool show_contents_listing;
	BPoint position;
	int32 name_size;
	if (
		(file.Read(&unknown, sizeof(unknown)) == sizeof(unknown))
		&& (file.Read(&automatically_expand_files, sizeof(automatically_expand_files)) == sizeof(automatically_expand_files))
		&& (file.Read(&close_when_done, sizeof(close_when_done)) == sizeof(close_when_done))
		&& (file.Read(&destination_folder, sizeof(destination_folder)) == sizeof(destination_folder))
		&& (file.Read(&unknown, sizeof(unknown)) == sizeof(unknown))
		&& (file.Read(&ref.device, sizeof(ref.device)) == sizeof(ref.device))
		&& (file.Read(&ref.directory, sizeof(ref.directory)) == sizeof(ref.directory))
		&& (file.Read(&name_size, sizeof(name_size)) == sizeof(name_size))
		&& ((name_size <= 0) || (ref.name = (char*)malloc(name_size + 1)))
		&& (file.Read(ref.name, name_size) == name_size)
		&& (file.Read(&open_destination_folder, sizeof(open_destination_folder)) == sizeof(open_destination_folder))
		&& (file.Read(&show_contents_listing, sizeof(show_contents_listing)) == sizeof(show_contents_listing))
		&& (file.Read(&position, sizeof(position)) == sizeof(position))
	   ) {

		// check if the window position is on screen at all
		BScreen screen;
		if (screen.Frame().Contains(position))
			fMessage.ReplacePoint("window_position", position);

		fMessage.ReplaceBool("automatically_expand_files", automatically_expand_files);
		fMessage.ReplaceBool("close_when_done", close_when_done);
		if (destination_folder == 0x66 || destination_folder == 0x63 || destination_folder == 0x65)
			fMessage.ReplaceInt8("destination_folder", destination_folder);
		if (name_size > 0)
			ref.name[name_size] = '\0';
		BEntry entry(&ref);
		if (entry.Exists())
			fMessage.ReplaceRef("destination_folder_use", &ref);
		fMessage.ReplaceBool("open_destination_folder", open_destination_folder);
		fMessage.ReplaceBool("show_contents_listing", show_contents_listing);
	}
}


ExpanderSettings::~ExpanderSettings()
{
	// only save the settings if something has changed
	if (!fUpdated)
		return;

	BFile file;
	if (Open(&file, B_CREATE_FILE | B_WRITE_ONLY) != B_OK)
		return;

	bool automatically_expand_files;
	bool close_when_done;
	int8 destination_folder;
	entry_ref ref;
	bool open_destination_folder;
	bool show_contents_listing;
	BPoint position;
	bool unknown = 1;

	if ((fMessage.FindPoint("window_position", &position) == B_OK)
		&& (fMessage.FindBool("automatically_expand_files", &automatically_expand_files) == B_OK)
		&& (fMessage.FindBool("close_when_done", &close_when_done) == B_OK)
		&& (fMessage.FindInt8("destination_folder", &destination_folder) == B_OK)
		&& (fMessage.FindRef("destination_folder_use", &ref) == B_OK)
		&& (fMessage.FindBool("open_destination_folder", &open_destination_folder) == B_OK)
		&& (fMessage.FindBool("show_contents_listing", &show_contents_listing) == B_OK)) {

		file.Write(&unknown, sizeof(unknown));
		file.Write(&automatically_expand_files, sizeof(automatically_expand_files));
		file.Write(&close_when_done, sizeof(close_when_done));
		file.Write(&destination_folder, sizeof(destination_folder));
		unknown = 0;
		file.Write(&unknown, sizeof(unknown));
		file.Write(&ref.device, sizeof(ref.device));
		file.Write(&ref.directory, sizeof(ref.directory));
		int32 name_size = 0;
		if (ref.name)
			name_size = strlen(ref.name);
		file.Write(&name_size, sizeof(name_size));
		file.Write(ref.name, name_size);
		file.Write(&open_destination_folder, sizeof(open_destination_folder));
		file.Write(&show_contents_listing, sizeof(show_contents_listing));
		file.Write(&position, sizeof(position));
	}

}


status_t
ExpanderSettings::Open(BFile *file, int32 mode)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return B_ERROR;

	path.Append("Expander_Settings");

	return file->SetTo(path.Path(), mode);
}


void
ExpanderSettings::UpdateFrom(BMessage *message)
{
	bool automatically_expand_files;
	bool close_when_done;
	int8 destination_folder;
	entry_ref ref;
	bool open_destination_folder;
	bool show_contents_listing;
	BPoint position;

	if (message->FindPoint("window_position", &position) == B_OK)
		fMessage.ReplacePoint("window_position", position);

	if (message->FindBool("automatically_expand_files", &automatically_expand_files) == B_OK)
		fMessage.ReplaceBool("automatically_expand_files", automatically_expand_files);

	if (message->FindBool("close_when_done", &close_when_done) == B_OK)
		fMessage.ReplaceBool("close_when_done", close_when_done);

	if (message->FindInt8("destination_folder", &destination_folder) == B_OK)
		fMessage.ReplaceInt8("destination_folder", destination_folder);

	if (message->FindRef("destination_folder_use", &ref) == B_OK)
		fMessage.ReplaceRef("destination_folder_use", &ref);

	if (message->FindBool("open_destination_folder", &open_destination_folder) == B_OK)
		fMessage.ReplaceBool("open_destination_folder", open_destination_folder);

	if (message->FindBool("show_contents_listing", &show_contents_listing) == B_OK)
		fMessage.ReplaceBool("show_contents_listing", show_contents_listing);

	fUpdated = true;
}
