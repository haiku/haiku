/*
 * Copyright © 2007-2009 Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "CopyPLItemsCommand.h"

#include <new>
#include <stdio.h>

#include <Autolock.h>
#include <Catalog.h>
#include <Locale.h>

#include "Playlist.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "MediaPlayer-CopyPLItemsCmd"


using std::nothrow;


CopyPLItemsCommand::CopyPLItemsCommand(Playlist* playlist,
		 const int32* indices, int32 count, int32 toIndex)
	:
	PLItemsCommand(),
	fPlaylist(playlist),
	fItems(count > 0 ? new (nothrow) PlaylistItem*[count] : NULL),
	fToIndex(toIndex),
	fCount(count),
	fItemsCopied(false)
{
	if (!indices || !fPlaylist || !fItems) {
		// indicate a bad object state
		delete[] fItems;
		fItems = NULL;
		return;
	}

	memset(fItems, 0, sizeof(PlaylistItem*) * fCount);

	// init original entries and
	for (int32 i = 0; i < fCount; i++) {
		PlaylistItem* item = fPlaylist->ItemAt(indices[i]);
		if (item != NULL)
			fItems[i] = item->Clone();
		if (fItems[i] == NULL) {
			// indicate a bad object state
			_CleanUp(fItems, fCount, true);
			return;
		}
	}
}


CopyPLItemsCommand::~CopyPLItemsCommand()
{
	_CleanUp(fItems, fCount, !fItemsCopied);
}


status_t
CopyPLItemsCommand::InitCheck()
{
	if (!fPlaylist || !fItems)
		return B_NO_INIT;
	return B_OK;
}


status_t
CopyPLItemsCommand::Perform()
{
	BAutolock _(fPlaylist);

	status_t ret = B_OK;

	fItemsCopied = true;

	// add refs to playlist at the insertion index
	int32 index = fToIndex;
	for (int32 i = 0; i < fCount; i++) {
		if (!fPlaylist->AddItem(fItems[i], index++)) {
			ret = B_NO_MEMORY;
			break;
		}
	}
	return ret;
}


status_t
CopyPLItemsCommand::Undo()
{
	BAutolock _(fPlaylist);

	fItemsCopied = false;
	// remember currently playling item in case we copy items over it
	PlaylistItem* current = fPlaylist->ItemAt(fPlaylist->CurrentItemIndex());

	// remove refs from playlist
	int32 index = fToIndex;
	for (int32 i = 0; i < fCount; i++) {
		fPlaylist->RemoveItem(index++, false);
	}

	// take care about currently played item
	if (current != NULL)
		fPlaylist->SetCurrentItemIndex(fPlaylist->IndexOf(current), false);

	return B_OK;
}


void
CopyPLItemsCommand::GetName(BString& name)
{
	if (fCount > 1)
		name << B_TRANSLATE("Copy Entries");
	else
		name << B_TRANSLATE("Copy Entry");
}
