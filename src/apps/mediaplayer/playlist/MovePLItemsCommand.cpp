/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "MovePLItemsCommand.h"

#include <new>
#include <stdio.h>

#include <Autolock.h>

#include "Playlist.h"


using std::nothrow;


MovePLItemsCommand::MovePLItemsCommand(Playlist* playlist,
		 const int32* indices, int32 count, int32 toIndex)
	: Command()
	, fPlaylist(playlist)
	, fRefs(count > 0 ? new (nothrow) entry_ref[count] : NULL)
	, fIndices(count > 0 ? new (nothrow) int32[count] : NULL)
	, fToIndex(toIndex)
	, fCount(count)
{
	if (!indices || !fPlaylist || !fRefs || !fIndices) {
		// indicate a bad object state
		delete[] fRefs;
		fRefs = NULL;
		return;
	}

	memcpy(fIndices, indices, fCount * sizeof(int32));

	// init original entry indices and
	// adjust toIndex compensating for items that
	// are removed before that index
	int32 itemsBeforeIndex = 0;
	for (int32 i = 0; i < fCount; i++) {
		if (fPlaylist->GetRefAt(fIndices[i], &fRefs[i]) < B_OK) {
			// indicate a bad object state
			delete[] fRefs;
			fRefs = NULL;
			return;
		}
		if (fIndices[i] < fToIndex)
			itemsBeforeIndex++;
	}
	fToIndex -= itemsBeforeIndex;
}


MovePLItemsCommand::~MovePLItemsCommand()
{
	delete[] fRefs;
	delete[] fIndices;
}


status_t
MovePLItemsCommand::InitCheck()
{
	if (!fRefs)
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

	// remember currently playling ref in case we move it
	entry_ref currentRef;
	bool adjustCurrentRef = fPlaylist->GetRefAt(fPlaylist->CurrentRefIndex(),
		&currentRef) == B_OK;

	// remove refs from playlist
	for (int32 i = 0; i < fCount; i++) {
		// "- i" to account for the items already removed
		fPlaylist->RemoveRef(fIndices[i] - i, false);
	}

	// add refs to playlist at the insertion index
	int32 index = fToIndex;
	for (int32 i = 0; i < fCount; i++) {
		if (!fPlaylist->AddRef(fRefs[i], index++)) {
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


status_t
MovePLItemsCommand::Undo()
{
	BAutolock _(fPlaylist);

	status_t ret = B_OK;

	// remember currently playling ref in case we move it
	entry_ref currentRef;
	bool adjustCurrentRef = fPlaylist->GetRefAt(fPlaylist->CurrentRefIndex(),
		&currentRef) == B_OK;

	// remove refs from playlist
	int32 index = fToIndex;
	for (int32 i = 0; i < fCount; i++) {
		fPlaylist->RemoveRef(index++, false);
	}

	// add ref to playlist at remembered indices
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
MovePLItemsCommand::GetName(BString& name)
{
	if (fCount > 1)
		name << "Move Entries";
	else
		name << "Move Entry";
}
