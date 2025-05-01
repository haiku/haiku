/*
 * PlaylistFileReader.cpp - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 * Copyright (C) 2007-2009 Stephan Aßmus <superstippi@gmx.de> (MIT ok)
 * Copyright (C) 2008-2009 Fredrik Modéen <[FirstName]@[LastName].se> (MIT ok)
 *
 * Released under the terms of the MIT license.
 */

#include "PlaylistFileReader.h"

#include <cctype>
#include <new>
#include <stdio.h>
#include <strings.h>

#include <Path.h>
#include <Url.h>

#include "FileReadWrite.h"
#include "Playlist.h"

using std::isdigit;
using std::nothrow;


/*static*/ PlaylistFileReader*
PlaylistFileReader::GenerateReader(const entry_ref& ref)
{
	PlaylistFileType type = _IdentifyType(ref);
	PlaylistFileReader* reader = NULL;
	if (type == m3u)
		reader = new (nothrow) M3uReader;
	else if (type == pls)
		reader = new (nothrow) PlsReader;
	return reader;
}


int32
PlaylistFileReader::_AppendItemToPlaylist(const BString& entry, Playlist* playlist)
{
	bool validItem = true;
	int32 assignedIndex = -1;
	BPath path(entry.String());
	entry_ref refPath;
	status_t err = get_ref_for_path(path.Path(), &refPath);
	PlaylistItem* item = NULL;

	// Create a PlaylistItem if entry is a valid file path or URL
	if (err == B_OK)
		item = new (nothrow) FilePlaylistItem(refPath);
	else {
		BUrl url(entry, true);
		if (url.IsValid())
			item = new (nothrow) UrlPlaylistItem(url);
		else {
			printf("Error - %s: [%" B_PRIx32 "]\n", strerror(err), err);
			validItem = false;
		}
	}

	// If creation of a PlaylistItem was attempted, try to add it to the Playlist
	if (validItem) {
		if (item == NULL)
			delete item;
		else {
			bool itemAdded = playlist->AddItem(item);
			if (!itemAdded)
				delete item;
			else
				assignedIndex = playlist->IndexOf(item);
		}
	}

	return assignedIndex;
}


/*static*/ PlaylistFileType
PlaylistFileReader::_IdentifyType(const entry_ref& ref)
{
	BString path(BPath(&ref).Path());
	PlaylistFileType type = unknown;
	if (path.FindLast(".m3u") == path.CountChars() - 4
		|| path.FindLast(".m3u8") == path.CountChars() - 5)
		type = m3u;
	else if (path.FindLast(".pls") == path.CountChars() - 4)
		type = pls;
	return type;
}


/*virtual*/ void
M3uReader::AppendToPlaylist(const entry_ref& ref, Playlist* playlist)
{
	BFile file(&ref, B_READ_ONLY);
	FileReadWrite lineReader(&file);

	BString line;
	while (lineReader.Next(line)) {
		if (line.FindFirst("#") != 0)
			// Current version of this function ignores all comment lines
			_AppendItemToPlaylist(line, playlist);
		line.Truncate(0);
	}
}


/*virtual*/ void
PlsReader::AppendToPlaylist(const entry_ref& ref, Playlist* playlist)
{
	BString plsEntries;
		// The total number of tracks in the PLS file, taken from the NumberOfEntries line.
		// This variable is not used for anything at this time.
	BString plsVersion;
		// The version of the PLS standard used in this playlist file, taken from the Version line.
		// This variable is not used for anything at this time.
	int32 lastAssignedIndex = -1;
		// The index that MediaPlayer assigned to the most recently added PlaylistItem.
		// If an attempted assignment fails, this will be set to -1 again.

	BFile file(&ref, B_READ_ONLY);
	FileReadWrite lineReader(&file);
	BString line;

	// Check for the "[playlist]" header on the first line
	lineReader.Next(line);
	if (line != "[playlist]") {
		printf("Error - Invalid .pls file\n");
		return;
	}
	line.Truncate(0);

	while (true) {
		bool lineRead = lineReader.Next(line);
		if (lineRead == false)
			break;

		// All valid PLS lines after the header contain an equal sign
		int32 equalIndex = line.FindFirst("=");
		if (equalIndex == B_ERROR) {
			line.Truncate(0);
			continue;
		}

		BString lineType;
			// Will be set for each line to one of: File, Title, Length, NumberOfEntries, Version
		BString trackNumber;
			// Number of the track being processed, using the (one-based) explicit numbering
			// from the .pls file
		if (isdigit(line[equalIndex - 1])) {
			// Distinguish between lines that specify a track number, and those that don't
			int32 trackIndex = equalIndex - 1;
				// The string index where the track number begins
			while (isdigit(line[trackIndex - 1]))
				trackIndex--;
			line.CopyInto(lineType, 0, trackIndex);
			line.CopyInto(trackNumber, trackIndex, equalIndex - trackIndex);
		} else {
			line.CopyInto(lineType, 0, equalIndex);
		}

		BString lineContent;
			// Everything after the equal sign
		line.CopyInto(lineContent, equalIndex + 1, line.Length() - (equalIndex + 1));

		if (lineType == "File") {
			lastAssignedIndex = _AppendItemToPlaylist(lineContent, playlist);
		// A File line may be followed by optional Title and/or Length lines.
		} else if (lineType == "Title") {
			_ParseTitleLine(lineContent, playlist, lastAssignedIndex);
		} else if (lineType == "Length") {
			_ParseLengthLine(lineContent, playlist, lastAssignedIndex);
		// The file should include one NumberOfEntries line and one Version line.
		} else if (lineType == "NumberOfEntries") {
			plsEntries = lineContent;
		} else if (lineType == "Version") {
			plsVersion = lineContent;
		} else {
			// Ignore the line
		}

	line.Truncate(0);
	}
}


status_t
PlsReader::_ParseTitleLine(const BString& title, Playlist* playlist,
	const int32 lastAssignedIndex)
{
	status_t err;
	if (lastAssignedIndex != -1) {
		// Only use this Title if most recent File was successfully added to the playlist.
		err = playlist->ItemAt(lastAssignedIndex)->SetAttribute(
			PlaylistItem::ATTR_STRING_TITLE, title);
	} else
		err = B_CANCELED;

	if (err != B_OK)
		printf("Error - %s: [%" B_PRIx32 "]\n", strerror(err), err);

	return err;
}


status_t
PlsReader::_ParseLengthLine(const BString& length, Playlist* playlist,
	const int32 lastAssignedIndex)
{
	status_t err;
	if (lastAssignedIndex != -1)
	{
		int64 lengthInt = strtoll(length.String(), NULL, 10);
			// Track length in seconds, or -1 for an infinite streaming track.

		err = playlist->ItemAt(lastAssignedIndex)->SetAttribute(
			PlaylistItem::ATTR_INT64_DURATION, lengthInt);
			// This does nothing if the track in question is streaming, because
			// UrlPlaylistItem::SetAttribute(const Attribute&, const int32&) is not implemented.
	} else
		err = B_CANCELED;

	if (err != B_OK)
		printf("Error - %s: [%" B_PRIx32 "]\n", strerror(err), err);

	return err;
}
