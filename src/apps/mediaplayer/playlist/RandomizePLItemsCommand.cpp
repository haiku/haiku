/*
 * Copyright © 2008-2009 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "RandomizePLItemsCommand.h"

#include <new>
#include <stdio.h>
#include <stdlib.h>

#include <Autolock.h>
#include <Catalog.h>
#include <Locale.h>

#include "Playlist.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MediaPlayer-RandomizePLItemsCmd"


using std::nothrow;


RandomizePLItemsCommand::RandomizePLItemsCommand(Playlist* playlist,
		 BList indices)
	:
	PLItemsCommand(),
	fPlaylist(playlist),
	fCount(indices.CountItems()),
	fItems(fCount > 0 ? new (nothrow) PlaylistItem*[fCount] : NULL),
	fListIndices(fCount > 0 ? new (nothrow) int32[fCount] : NULL),
	fRandomInternalIndices(fCount > 0 ? new (nothrow) int32[fCount] : NULL)
{
	if (indices.IsEmpty() || !fPlaylist || !fItems || !fListIndices
			|| !fRandomInternalIndices) {
		// indicate a bad object state
		delete[] fItems;
		fItems = NULL;
		return;
	}

	memset(fItems, 0, fCount * sizeof(PlaylistItem*));

	// put the available indices into a "set"
	BList indexSet;
	for (int32 i = 0; i < fCount; i++) {
		fListIndices[i] = (int32)(addr_t)indices.ItemAt(i);
		fItems[i] = fPlaylist->ItemAt(fListIndices[i]);
		if (fItems[i] == NULL || !indexSet.AddItem((void*)(addr_t)i)) {
			// indicate a bad object state
			delete[] fItems;
			fItems = NULL;
			return;
		}
	}

	// remove the indices from the set in random order
	for (int32 i = 0; i < fCount; i++) {
		int32 randomSetIndex = rand() % indexSet.CountItems();
		fRandomInternalIndices[i] = (int32)(addr_t)indexSet.RemoveItem(randomSetIndex);
	}
}


RandomizePLItemsCommand::~RandomizePLItemsCommand()
{
	delete[] fItems;
	delete[] fListIndices;
	delete[] fRandomInternalIndices;
}


status_t
RandomizePLItemsCommand::InitCheck()
{
	if (!fItems)
		return B_NO_INIT;

	return B_OK;
}


status_t
RandomizePLItemsCommand::Perform()
{
	return _Sort(true);
}


status_t
RandomizePLItemsCommand::Undo()
{
	return _Sort(false);
}


void
RandomizePLItemsCommand::GetName(BString& name)
{
	name << B_TRANSLATE("Randomize Entries");
}


status_t
RandomizePLItemsCommand::_Sort(bool random)
{
	BAutolock _(fPlaylist);

	// remember currently playling item in case we move it
	PlaylistItem* current = fPlaylist->ItemAt(fPlaylist->CurrentItemIndex());

	// remove refs from playlist
	for (int32 i = 0; i < fCount; i++) {
		// "- i" to account for the items already removed
		fPlaylist->RemoveItem(fListIndices[i] - i, false);
	}

	// add refs to playlist at the randomized indices
	if (random) {
		for (int32 i = 0; i < fCount; i++) {
			if (!fPlaylist->AddItem(fItems[fRandomInternalIndices[i]],
					fListIndices[i])) {
				return B_NO_MEMORY;
			}
		}
	} else {
		for (int32 i = 0; i < fCount; i++) {
			if (!fPlaylist->AddItem(fItems[i], fListIndices[i])) {
				return B_NO_MEMORY;
			}
		}
	}

	// take care about currently played item
	if (current != NULL)
		fPlaylist->SetCurrentItemIndex(fPlaylist->IndexOf(current), false);

	return B_OK;
}
