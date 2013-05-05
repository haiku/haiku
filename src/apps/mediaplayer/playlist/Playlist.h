/*
 * Playlist.h - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen 	<marcus@overhagen.de>
 * Copyright (C) 2007-2009 Stephan Aßmus <superstippi@gmx.de> (MIT ok)
 * Copyright (C) 2008-2009 Fredrik Modéen <[FirstName]@[LastName].se> (MIT ok)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#ifndef __PLAYLIST_H
#define __PLAYLIST_H

#include <List.h>
#include <Locker.h>

#include "FilePlaylistItem.h"
#include "PlaylistItem.h"

class BDataIO;
class BMessage;
class BString;
struct entry_ref;


// special append index values
#define APPEND_INDEX_REPLACE_PLAYLIST	-1
#define APPEND_INDEX_APPEND_LAST		-2

extern const uint32 kPlaylistMagicBytes;
extern const char* kTextPlaylistMimeString;
extern const char* kBinaryPlaylistMimeString;


class Playlist : public BLocker {
public:
	class Listener {
	public:
								Listener();
		virtual					~Listener();

		virtual	void			ItemAdded(PlaylistItem* item, int32 index);
		virtual	void			ItemRemoved(int32 index);

		virtual	void			ItemsSorted();

		virtual	void			CurrentItemChanged(int32 newIndex, bool play);

		virtual	void			ImportFailed();
	};

public:
								Playlist();
								~Playlist();
			// archiving
			status_t			Unarchive(const BMessage* archive);
			status_t			Archive(BMessage* into) const;

			status_t			Unflatten(BDataIO* stream);
			status_t			Flatten(BDataIO* stream) const;


			// list functionality
			void				MakeEmpty(bool deleteItems = true);
			int32				CountItems() const;
			bool				IsEmpty() const;

			void				Sort();

			bool				AddItem(PlaylistItem* item);
			bool				AddItem(PlaylistItem* item, int32 index);
			PlaylistItem*		RemoveItem(int32 index,
									bool careAboutCurrentIndex = true);

			bool				AdoptPlaylist(Playlist& other);
			bool				AdoptPlaylist(Playlist& other, int32 index);

			int32				IndexOf(PlaylistItem* item) const;
			PlaylistItem*		ItemAt(int32 index) const;
			PlaylistItem*		ItemAtFast(int32 index) const;

			// navigating current ref
			bool				SetCurrentItemIndex(int32 index,
									bool notify = true);
			int32				CurrentItemIndex() const;

			void				GetSkipInfo(bool* canSkipPrevious,
									bool* canSkipNext) const;

			// listener support
			bool				AddListener(Listener* listener);
			void				RemoveListener(Listener* listener);

			// support functions
			void				AppendRefs(const BMessage* refsReceivedMessage,
									int32 appendIndex
										= APPEND_INDEX_REPLACE_PLAYLIST);
	static	void				AppendToPlaylistRecursive(const entry_ref& ref,
									Playlist* playlist);
	static	void				AppendPlaylistToPlaylist(const entry_ref& ref,
									Playlist* playlist);
	static	void				AppendQueryToPlaylist(const entry_ref& ref,
									Playlist* playlist);

			void				NotifyImportFailed();

	static	bool				ExtraMediaExists(Playlist* playlist, const entry_ref& ref);

private:
								Playlist(const Playlist& other);
			Playlist&			operator=(const Playlist& other);
									// unimplemented

	static	bool 				_IsImageFile(const BString& mimeString);
	static	bool 				_IsMediaFile(const BString& mimeString);
	static	bool				_IsTextPlaylist(const BString& mimeString);
	static	bool				_IsBinaryPlaylist(const BString& mimeString);
	static	bool				_IsPlaylist(const BString& mimeString);
	static	bool				_IsQuery(const BString& mimeString);
	static	BString				_MIMEString(const entry_ref* ref);
	static	void				_BindExtraMedia(PlaylistItem* item);
	static	void				_BindExtraMedia(FilePlaylistItem* fileItem, const BEntry& entry);
	static	BString				_GetExceptExtension(const BString& path);

			void				_NotifyItemAdded(PlaylistItem*,
									int32 index) const;
			void				_NotifyItemRemoved(int32 index) const;
			void				_NotifyItemsSorted() const;
			void				_NotifyCurrentItemChanged(int32 newIndex,
									bool play) const;
			void				_NotifyImportFailed() const;

private:
			BList				fItems;
			BList				fListeners;

			int32				fCurrentIndex;
};

#endif
