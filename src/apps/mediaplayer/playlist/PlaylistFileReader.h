/*
 * PlaylistFileReader.h - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen 	<marcus@overhagen.de>
 * Copyright (C) 2007-2009 Stephan Aßmus <superstippi@gmx.de> (MIT ok)
 * Copyright (C) 2008-2009 Fredrik Modéen <[FirstName]@[LastName].se> (MIT ok)
 *
 * Released under the terms of the MIT license.
 */
#ifndef __PLAYLIST_FILE_READER_H
#define __PLAYLIST_FILE_READER_H


#include <SupportDefs.h>

class BString;
struct entry_ref;
class Playlist;

enum PlaylistFileType {m3u, pls, unknown};


class PlaylistFileReader {
public:
	virtual						~PlaylistFileReader() {}
		// Defined to enable deletion of PlaylistFileReader* target objects.

	virtual	void				AppendToPlaylist(const entry_ref& ref,
									Playlist* playlist) = 0;
	static	PlaylistFileReader*	GenerateReader(const entry_ref& ref);
		// Returns a pointer to an object of the appropriate derived class,
		// or returns NULL if the argument is not a valid playlist file.

protected:
			int32				_AppendItemToPlaylist(const BString& entry, Playlist* playlist);
		// Returns the track's playlist index if it was successfully added, else returns -1.
		// BString& entry is a (absolute or relative) file path or URL

private:
	static	PlaylistFileType	_IdentifyType(const entry_ref& ref);
};


class M3uReader : public PlaylistFileReader {
public:
	virtual	void				AppendToPlaylist(const entry_ref& ref, Playlist* playlist);
};


class PlsReader : public PlaylistFileReader {
public:
	virtual void				AppendToPlaylist(const entry_ref& ref,
									Playlist* playlist);
private:
			status_t			_ParseTitleLine(const BString& title, Playlist* playlist,
									const int32 lastAssignedIndex);
			status_t			_ParseLengthLine(const BString& length, Playlist* playlist,
									const int32 lastAssignedIndex);
};


#endif
