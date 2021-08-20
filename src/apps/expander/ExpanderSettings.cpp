/*****************************************************************************/
// Expander
// Written by Jérôme Duval
//
// ExpanderSettings.cpp
//
// Code from Diskprobe by Axel Dörfler
//
// Copyright (c) 2004 Haiku Project
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
#include <Directory.h>
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


template<typename T> bool
read_data(BFile& file, T& value)
{
	return file.Read(&value, sizeof(T)) == (ssize_t)sizeof(T);
}


//	#pragma mark - ExpanderSettings


ExpanderSettings::ExpanderSettings()
	:
	fMessage(kMsgExpanderSettings),
	fUpdated(false)
{
	fMessage.AddBool("automatically_expand_files", false);
	fMessage.AddBool("close_when_done", true);
	fMessage.AddInt8("destination_folder", 0x63);
	entry_ref ref;
	fMessage.AddRef("destination_folder_use", &ref);
	fMessage.AddBool("open_destination_folder", true);
	fMessage.AddBool("show_contents_listing", false);
	fMessage.AddPoint("window_position", BPoint(50, 50));

	BFile file;
	if (Open(&file, B_READ_ONLY) != B_OK)
		return;

	// TODO: load/save settings as flattened BMessage - but not yet,
	//       since that will break compatibility with R5's Expander

	bool unknown;
	bool automaticallyExpandFiles;
	bool closeWhenDone;
	int8 destinationFolder;
	bool openDestinationFolder;
	bool showContentsListing;
	BPoint position;
	char name[B_FILE_NAME_LENGTH] = {'\0'};
	int32 nameSize;
	if (read_data(file, unknown)
		&& read_data(file, automaticallyExpandFiles)
		&& read_data(file, closeWhenDone)
		&& read_data(file, destinationFolder)
		&& read_data(file, unknown)
		&& read_data(file, ref.device)
		&& read_data(file, ref.directory)
		&& read_data(file, nameSize)
		&& (nameSize <= 0 || file.Read(name, nameSize) == nameSize)
		&& read_data(file, openDestinationFolder)
		&& read_data(file, showContentsListing)
		&& read_data(file, position)) {	
		if (nameSize > 0 && nameSize < B_FILE_NAME_LENGTH) {
			name[nameSize] = '\0';
			ref.set_name(name);
		}

		// check if the window position is on screen at all
		BScreen screen;
		if (screen.Frame().Contains(position))
			fMessage.ReplacePoint("window_position", position);

		fMessage.ReplaceBool("automatically_expand_files",
			automaticallyExpandFiles);
		fMessage.ReplaceBool("close_when_done", closeWhenDone);
		if (destinationFolder == 0x66
			|| destinationFolder == 0x63
			|| destinationFolder == 0x65) {
			fMessage.ReplaceInt8("destination_folder", destinationFolder);
		}

		BEntry entry(&ref);
		if (entry.Exists())
			fMessage.ReplaceRef("destination_folder_use", &ref);

		fMessage.ReplaceBool("open_destination_folder", openDestinationFolder);
		fMessage.ReplaceBool("show_contents_listing", showContentsListing);
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

	bool automaticallyExpandFiles;
	bool closeWhenDone;
	int8 destinationFolder;
	entry_ref ref;
	bool openDestinationFolder;
	bool showContentsListing;
	BPoint position;
	bool unknown = 1;

	if (fMessage.FindPoint("window_position", &position) == B_OK
		&& fMessage.FindBool("automatically_expand_files",
			&automaticallyExpandFiles) == B_OK
		&& fMessage.FindBool("close_when_done", &closeWhenDone) == B_OK
		&& fMessage.FindInt8("destination_folder", &destinationFolder) == B_OK
		&& fMessage.FindRef("destination_folder_use", &ref) == B_OK
		&& fMessage.FindBool("open_destination_folder",
			&openDestinationFolder) == B_OK
		&& fMessage.FindBool("show_contents_listing",
			&showContentsListing) == B_OK) {
		file.Write(&unknown, sizeof(unknown));
		file.Write(&automaticallyExpandFiles, sizeof(automaticallyExpandFiles));
		file.Write(&closeWhenDone, sizeof(closeWhenDone));
		file.Write(&destinationFolder, sizeof(destinationFolder));
		unknown = 0;
		file.Write(&unknown, sizeof(unknown));
		file.Write(&ref.device, sizeof(ref.device));
		file.Write(&ref.directory, sizeof(ref.directory));
		int32 nameSize = 0;
		if (ref.name)
			nameSize = strlen(ref.name);

		file.Write(&nameSize, sizeof(nameSize));
		file.Write(ref.name, nameSize);
		file.Write(&openDestinationFolder, sizeof(openDestinationFolder));
		file.Write(&showContentsListing, sizeof(showContentsListing));
		file.Write(&position, sizeof(position));
	}
}


/*static*/ status_t
ExpanderSettings::GetSettingsDirectoryPath(BPath& _path)
{
	status_t error = find_directory(B_USER_SETTINGS_DIRECTORY, &_path);
	return error == B_OK ? _path.Append("expander") : error;
}


status_t
ExpanderSettings::Open(BFile* file, int32 mode)
{
	BPath path;
	status_t error = GetSettingsDirectoryPath(path);
	if (error != B_OK)
		return error;

	// create the directory, if file creation is requested
	if ((mode & B_CREATE_FILE) != 0) {
		error = create_directory(path.Path(), 0755);
		if (error != B_OK)
			return error;
	}

	error = path.Append("settings");
	if (error != B_OK)
		return error;

	return file->SetTo(path.Path(), mode);
}


void
ExpanderSettings::UpdateFrom(BMessage* message)
{
	bool automaticallyExpandFiles;
	bool closeWhenDone;
	int8 destinationFolder;
	entry_ref ref;
	bool openDestinationFolder;
	bool showContentsListing;
	BPoint position;

	if (message->FindPoint("window_position", &position) == B_OK)
		fMessage.ReplacePoint("window_position", position);

	if (message->FindBool("automatically_expand_files",
			&automaticallyExpandFiles) == B_OK) {
		fMessage.ReplaceBool("automatically_expand_files",
			automaticallyExpandFiles);
	}

	if (message->FindBool("close_when_done", &closeWhenDone) == B_OK)
		fMessage.ReplaceBool("close_when_done", closeWhenDone);

	if (message->FindInt8("destination_folder", &destinationFolder) == B_OK)
		fMessage.ReplaceInt8("destination_folder", destinationFolder);

	if (message->FindRef("destination_folder_use", &ref) == B_OK)
		fMessage.ReplaceRef("destination_folder_use", &ref);

	if (message->FindBool("open_destination_folder",
			&openDestinationFolder) == B_OK)
		fMessage.ReplaceBool("open_destination_folder", openDestinationFolder);

	if (message->FindBool("show_contents_listing",
			&showContentsListing) == B_OK) {
		fMessage.ReplaceBool("show_contents_listing", showContentsListing);
	}

	fUpdated = true;
}
