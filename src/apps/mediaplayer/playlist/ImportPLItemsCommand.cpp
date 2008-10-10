/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "ImportPLItemsCommand.h"

#include <new>
#include <stdio.h>

#include <Alert.h>
#include <Autolock.h>

#include "Playlist.h"


using std::nothrow;


ImportPLItemsCommand::ImportPLItemsCommand(Playlist* playlist,
		 const BMessage* refsMessage, int32 toIndex)
	: Command()
	, fPlaylist(playlist)

	, fOldRefs(NULL)
	, fOldCount(0)

	, fNewRefs(NULL)
	, fNewCount(0)

	, fToIndex(toIndex)
{
	if (!fPlaylist)
		return;

	Playlist temp;
	temp.AppendRefs(refsMessage);

	fNewCount = temp.CountItems();
	if (fNewCount <= 0) {
		BAlert* alert = new BAlert("Nothing to Play", "None of the files "
			"you wanted to play appear to be media files.", "Ok");
		alert->Go(NULL);
		return;
	}

	fNewRefs = new (nothrow) entry_ref[fNewCount];
	if (!fNewRefs)
		return;

	// init new entries
	for (int32 i = 0; i < fNewCount; i++) {
		if (temp.GetRefAt(i, &fNewRefs[i]) < B_OK) {
			delete[] fNewRefs;
			fNewRefs = NULL;
			return;
		}
	}

	if (fToIndex < 0) {
		fOldCount = fPlaylist->CountItems();
		if (fOldCount > 0) {
			fOldRefs = new (nothrow) entry_ref[fOldCount];
			if (!fOldRefs) {
				// indicate bad object init
				delete[] fNewRefs;
				fNewRefs = NULL;
				return;
			}
		}
	}

	for (int32 i = 0; i < fOldCount; i++) {
		if (fPlaylist->GetRefAt(i, &fOldRefs[i]) < B_OK) {
			// indicate bad object init
			delete[] fNewRefs;
			fNewRefs = NULL;
			return;
		}
	}
}


ImportPLItemsCommand::~ImportPLItemsCommand()
{
	delete[] fOldRefs;
	delete[] fNewRefs;
}


status_t
ImportPLItemsCommand::InitCheck()
{
	if (!fPlaylist || !fNewRefs)
		return B_NO_INIT;
	return B_OK;
}


status_t
ImportPLItemsCommand::Perform()
{
	BAutolock _(fPlaylist);

	int32 index = fToIndex;
	if (fToIndex < 0) {
		fPlaylist->MakeEmpty();
		index = 0;
	}

	bool startPlaying = fPlaylist->CountItems() == 0;

	// add refs to playlist at the insertion index
	for (int32 i = 0; i < fNewCount; i++) {
		if (!fPlaylist->AddRef(fNewRefs[i], index++))
			return B_NO_MEMORY;
	}

	if (startPlaying) {
		// open first file
		fPlaylist->SetCurrentRefIndex(0);
	}

	return B_OK;
}


status_t
ImportPLItemsCommand::Undo()
{
	BAutolock _(fPlaylist);

	if (fToIndex < 0) {
		// remove new refs from playlist and restore old refs
		fPlaylist->MakeEmpty();
		for (int32 i = 0; i < fOldCount; i++) {
			if (!fPlaylist->AddRef(fOldRefs[i], i))
				return B_NO_MEMORY;
		}
	} else {
		// remove refs from playlist
		for (int32 i = 0; i < fNewCount; i++) {
			fPlaylist->RemoveRef(fToIndex);
		}
	}

	return B_OK;
}


void
ImportPLItemsCommand::GetName(BString& name)
{
	if (fNewCount > 1)
		name << "Import Entries";
	else
		name << "Import Entry";
}
