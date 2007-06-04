/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "CopyPLItemsCommand.h"

#include <new>
#include <stdio.h>

#include <Autolock.h>

#include "Playlist.h"


using std::nothrow;


CopyPLItemsCommand::CopyPLItemsCommand(Playlist* playlist,
		 const int32* indices, int32 count, int32 toIndex)
	: Command()
	, fPlaylist(playlist)
	, fRefs(count > 0 ? new (nothrow) entry_ref[count] : NULL)
	, fToIndex(toIndex)
	, fCount(count)
{
	if (!indices || !fPlaylist || !fRefs) {
		// indicate a bad object state
		delete[] fRefs;
		fRefs = NULL;
		return;
	}

	// init original entries and
	for (int32 i = 0; i < fCount; i++) {
		if (fPlaylist->GetRefAt(indices[i], &fRefs[i]) < B_OK) {
			delete[] fRefs;
			fRefs = NULL;
			return;
		}
	}
}


CopyPLItemsCommand::~CopyPLItemsCommand()
{
	delete[] fRefs;
}


status_t
CopyPLItemsCommand::InitCheck()
{
	if (!fPlaylist || !fRefs)
		return B_NO_INIT;
	return B_OK;
}


status_t
CopyPLItemsCommand::Perform()
{
	BAutolock _(fPlaylist);

	status_t ret = B_OK;

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

	return B_OK;
}


status_t
CopyPLItemsCommand::Undo()
{
	BAutolock _(fPlaylist);

	// remember currently playling ref in case we copy items over it
	entry_ref currentRef;
	bool adjustCurrentRef = fPlaylist->GetRefAt(fPlaylist->CurrentRefIndex(),
		&currentRef) == B_OK;

	// remove refs from playlist
	int32 index = fToIndex;
	for (int32 i = 0; i < fCount; i++) {
		fPlaylist->RemoveRef(index++, false);
	}

	// take care about currently played ref
	if (adjustCurrentRef)
		fPlaylist->SetCurrentRefIndex(fPlaylist->IndexOf(currentRef));

	return B_OK;
}


void
CopyPLItemsCommand::GetName(BString& name)
{
	if (fCount > 1)
		name << "Copy Entries";
	else
		name << "Copy Entry";
}
