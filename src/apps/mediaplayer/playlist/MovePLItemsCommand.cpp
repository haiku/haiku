/*
 * Copyright 2007-2009 Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "MovePLItemsCommand.h"

#include <new>
#include <stdio.h>

#include <Autolock.h>
#include <Catalog.h>
#include <Locale.h>

#include "Playlist.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "MediaPlayer-MovePLItemsCmd"


using std::nothrow;


MovePLItemsCommand::MovePLItemsCommand(Playlist* playlist,
		 const int32* indices, int32 count, int32 toIndex)
	:
	PLItemsCommand(),
	fPlaylist(playlist),
	fItems(count > 0 ? new (nothrow) PlaylistItem*[count] : NULL),
	fIndices(count > 0 ? new (nothrow) int32[count] : NULL),
	fToIndex(toIndex),
	fCount(count)
{
	if (!indices || !fPlaylist || !fItems || !fIndices) {
		// indicate a bad object state
		delete[] fItems;
		fItems = NULL;
		return;
	}

	memset(fItems, 0, sizeof(PlaylistItem*) * fCount);
	memcpy(fIndices, indices, fCount * sizeof(int32));

	// init original entry indices and
	// adjust toIndex compensating for items that
	// are removed before that index
	int32 itemsBeforeIndex = 0;
	for (int32 i = 0; i < fCount; i++) {
		fItems[i] = fPlaylist->ItemAt(fIndices[i]);
		if (fItems[i] == NULL) {
			// indicate a bad object state
			delete[] fItems;
			fItems = NULL;
			return;
		}
		if (fIndices[i] < fToIndex)
			itemsBeforeIndex++;
	}
	fToIndex -= itemsBeforeIndex;
}


MovePLItemsCommand::~MovePLItemsCommand()
{
	delete[] fItems;
	delete[] fIndices;
}


status_t
MovePLItemsCommand::InitCheck()
{
	if (!fItems)
		return B_NO_INIT;

	// analyse the move, don't return B_OK in case
	// the container state does not change...

	int32 index = fIndices[0];
		// NOTE: fIndices == NULL if fCount < 1

	if (index != fToIndex) {
		// a change is guaranteed
		return B_OK;
	}

	// the insertion index is the same as the index of the first
	// moved item, a change only occures if the indices of the
	// moved items is not contiguous
	bool isContiguous = true;
	for (int32 i = 1; i < fCount; i++) {
		if (fIndices[i] != index + 1) {
			isContiguous = false;
			break;
		}
		index = fIndices[i];
	}
	if (isContiguous) {
		// the container state will not change because of the move
		return B_ERROR;
	}

	return B_OK;
}


status_t
MovePLItemsCommand::Perform()
{
	BAutolock _(fPlaylist);

	status_t ret = B_OK;

	// remember currently playling item in case we move it
	PlaylistItem* current = fPlaylist->ItemAt(fPlaylist->CurrentItemIndex());

	// remove refs from playlist
	for (int32 i = 0; i < fCount; i++) {
		// "- i" to account for the items already removed
		fPlaylist->RemoveItem(fIndices[i] - i, false);
	}

	// add refs to playlist at the insertion index
	int32 index = fToIndex;
	for (int32 i = 0; i < fCount; i++) {
		if (!fPlaylist->AddItem(fItems[i], index++)) {
			ret = B_NO_MEMORY;
			break;
		}
	}
	if (ret < B_OK)
		return ret;

	// take care about currently played item
	if (current != NULL)
		fPlaylist->SetCurrentItemIndex(fPlaylist->IndexOf(current), false);

	return B_OK;
}


status_t
MovePLItemsCommand::Undo()
{
	BAutolock _(fPlaylist);

	status_t ret = B_OK;

	// remember currently playling item in case we move it
	PlaylistItem* current = fPlaylist->ItemAt(fPlaylist->CurrentItemIndex());

	// remove refs from playlist
	int32 index = fToIndex;
	for (int32 i = 0; i < fCount; i++) {
		fPlaylist->RemoveItem(index++, false);
	}

	// add ref to playlist at remembered indices
	for (int32 i = 0; i < fCount; i++) {
		if (!fPlaylist->AddItem(fItems[i], fIndices[i])) {
			ret = B_NO_MEMORY;
			break;
		}
	}
	if (ret < B_OK)
		return ret;

	// take care about currently played item
	if (current != NULL)
		fPlaylist->SetCurrentItemIndex(fPlaylist->IndexOf(current), false);

	return B_OK;
}


void
MovePLItemsCommand::GetName(BString& name)
{
	if (fCount > 1)
		name << B_TRANSLATE("Move Entries");
	else
		name << B_TRANSLATE("Move Entry");
}
