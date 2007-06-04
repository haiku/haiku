/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "RemovePLItemsCommand.h"

#include <new>
#include <stdio.h>

#include <Autolock.h>

#include "Playlist.h"


using std::nothrow;


RemovePLItemsCommand::RemovePLItemsCommand(Playlist* playlist,
		 const int32* indices, int32 count)
	: Command()
	, fPlaylist(playlist)
	, fRefs(count > 0 ? new (nothrow) entry_ref[count] : NULL)
	, fIndices(count > 0 ? new (nothrow) int32[count] : NULL)
	, fCount(count)
{
	if (!indices || !fPlaylist || !fRefs || !fIndices) {
		// indicate a bad object state
		delete[] fRefs;
		fRefs = NULL;
		return;
	}

	memcpy(fIndices, indices, fCount * sizeof(int32));

	// init original entry indices
	for (int32 i = 0; i < fCount; i++) {
		if (fPlaylist->GetRefAt(fIndices[i], &fRefs[i]) < B_OK) {
			delete[] fRefs;
			fRefs = NULL;
			return;
		}
	}
}


RemovePLItemsCommand::~RemovePLItemsCommand()
{
	delete[] fRefs;
	delete[] fIndices;
}


status_t
RemovePLItemsCommand::InitCheck()
{
	if (!fPlaylist || !fRefs || !fIndices)
		return B_NO_INIT;
	return B_OK;
}


status_t
RemovePLItemsCommand::Perform()
{
	BAutolock _(fPlaylist);

	int32 lastRemovedIndex = -1;

	// remove refs from playlist
	for (int32 i = 0; i < fCount; i++) {
		// "- i" to account for the items already removed
		lastRemovedIndex = fIndices[i] - i;
		fPlaylist->RemoveRef(lastRemovedIndex);
	}

	// in case we removed the currently playing file
	if (fPlaylist->CurrentRefIndex() == -1)
		fPlaylist->SetCurrentRefIndex(lastRemovedIndex);

	return B_OK;
}


status_t
RemovePLItemsCommand::Undo()
{
	BAutolock _(fPlaylist);

	status_t ret = B_OK;

	// remember currently playling ref in case we move it
	entry_ref currentRef;
	bool adjustCurrentRef = fPlaylist->GetRefAt(fPlaylist->CurrentRefIndex(),
		&currentRef) == B_OK;

	// add refs to playlist at remembered indices
	for (int32 i = 0; i < fCount; i++) {
		if (!fPlaylist->AddRef(fRefs[i], fIndices[i])) {
			ret = B_NO_MEMORY;
			break;
		}
	}
	if (ret < B_OK)
		return ret;

	// take care about currently played ref
	if (adjustCurrentRef)
		fPlaylist->SetCurrentRefIndex(fPlaylist->IndexOf(currentRef));

	return B_OK;
}


void
RemovePLItemsCommand::GetName(BString& name)
{
	if (fCount > 1)
		name << "Remove Entries";
	else
		name << "Remove Entry";
}
