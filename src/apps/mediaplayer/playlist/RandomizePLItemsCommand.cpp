/*
 * Copyright © 2008 Stephan Aßmus. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "RandomizePLItemsCommand.h"

#include <new>
#include <stdio.h>
#include <stdlib.h>

#include <Autolock.h>

#include "Playlist.h"


using std::nothrow;


RandomizePLItemsCommand::RandomizePLItemsCommand(Playlist* playlist,
		 const int32* indices, int32 count)
	: Command()
	, fPlaylist(playlist)
	, fRefs(count > 0 ? new (nothrow) entry_ref[count] : NULL)
	, fListIndices(count > 0 ? new (nothrow) int32[count] : NULL)
	, fRandomInternalIndices(count > 0 ? new (nothrow) int32[count] : NULL)
	, fCount(count)
{
	if (!indices || !fPlaylist || !fRefs || !fListIndices
			|| !fRandomInternalIndices) {
		// indicate a bad object state
		delete[] fRefs;
		fRefs = NULL;
		return;
	}

	memcpy(fListIndices, indices, fCount * sizeof(int32));

	// put the available indices into a "set"
	BList indexSet;
	for (int32 i = 0; i < fCount; i++) {
		if (fPlaylist->GetRefAt(fListIndices[i], &fRefs[i]) < B_OK
			|| !indexSet.AddItem((void*)i)) {
			// indicate a bad object state
			delete[] fRefs;
			fRefs = NULL;
			return;
		}
	}

	// remove the indices from the set in random order
	for (int32 i = 0; i < fCount; i++) {
		int32 randomSetIndex = rand() % indexSet.CountItems();
		fRandomInternalIndices[i] = (int32)indexSet.RemoveItem(randomSetIndex);
	}
}


RandomizePLItemsCommand::~RandomizePLItemsCommand()
{
	delete[] fRefs;
	delete[] fListIndices;
	delete[] fRandomInternalIndices;
}


status_t
RandomizePLItemsCommand::InitCheck()
{
	if (!fRefs)
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
	name << "Randomize Entries";
}


status_t
RandomizePLItemsCommand::_Sort(bool random)
{
	BAutolock _(fPlaylist);

	// remember currently playling ref in case we move it
	entry_ref currentRef;
	bool adjustCurrentRef = fPlaylist->GetRefAt(fPlaylist->CurrentRefIndex(),
		&currentRef) == B_OK;

	// remove refs from playlist
	for (int32 i = 0; i < fCount; i++) {
		// "- i" to account for the items already removed
		fPlaylist->RemoveRef(fListIndices[i] - i, false);
	}

	// add refs to playlist at the randomized indices
	if (random) {
		for (int32 i = 0; i < fCount; i++) {
			if (!fPlaylist->AddRef(fRefs[fRandomInternalIndices[i]],
					fListIndices[i])) {
				return B_NO_MEMORY;
			}
		}
	} else {
		for (int32 i = 0; i < fCount; i++) {
			if (!fPlaylist->AddRef(fRefs[i], fListIndices[i])) {
				return B_NO_MEMORY;
			}
		}
	}

	// take care about currently played ref
	if (adjustCurrentRef)
		fPlaylist->SetCurrentRefIndex(fPlaylist->IndexOf(currentRef));

	return B_OK;
}
