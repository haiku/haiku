/*
 * Copyright 2009 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef PLAYLIST_ITEM_H
#define PLAYLIST_ITEM_H

#include <List.h>
#include <String.h>

class BDataIO;
class BMediaFile;
class BMessage;

class PlaylistItem {
public:
	class Listener {
	public:
						Listener();
		virtual			~Listener();

		virtual	void	ItemChanged(const PlaylistItem* item);
	};

public:
								PlaylistItem();
	virtual						~PlaylistItem();

	// archiving
//	virtual	status_t			Unarchive(const BMessage* archive) = 0;
//	virtual	status_t			Archive(BMessage* into) const = 0;
//
//	virtual	status_t			Unflatten(BDataIO* stream) = 0;
//	virtual	status_t			Flatten(BDataIO* stream) const = 0;

	// properties
	virtual	status_t			SetName(const BString& name) = 0;
	virtual	status_t			GetName(BString& name) const = 0;

	virtual	status_t			SetTitle(const BString& title) = 0;
	virtual	status_t			GetTitle(BString& title) const = 0;

	virtual	status_t			SetAuthor(const BString& author) = 0;
	virtual	status_t			GetAuthor(BString& author) const = 0;

	virtual	status_t			SetAlbum(const BString& album) = 0;
	virtual	status_t			GetAlbum(BString& album) const = 0;

	virtual	status_t			SetTrackNumber(int32 trackNumber) = 0;
	virtual	status_t			GetTrackNumber(int32& trackNumber) const = 0;

	virtual	status_t			SetBitRate(int32 bitRate) = 0;
	virtual	status_t			GetBitRate(int32& bitRate) const = 0;

	virtual status_t			GetDuration(bigtime_t& duration) const = 0;

	// methods
	virtual	status_t			MoveIntoTrash() = 0;
	virtual	status_t			RestoreFromTrash() = 0;

	// playback
	virtual	BMediaFile*			CreateMediaFile() const = 0;

	// listener support
			bool				AddListener(Listener* listener);
			void				RemoveListener(Listener* listener);

protected:
			void				_NotifyListeners() const;

private:
			BList				fListeners;
};

#endif // PLAYLIST_ITEM_H
