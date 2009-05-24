/*
 * Copyright © 2007-2009 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "RemovePLItemsCommand.h"

#include <new>
#include <stdio.h>

#include <Alert.h>
#include <Autolock.h>

#include "Playlist.h"


using std::nothrow;


RemovePLItemsCommand::RemovePLItemsCommand(Playlist* playlist,
		 const int32* indices, int32 count, bool moveFilesToTrash)
	:
	PLItemsCommand(),
	fPlaylist(playlist),
	fItems(count > 0 ? new (nothrow) PlaylistItem*[count] : NULL),
	fIndices(count > 0 ? new (nothrow) int32[count] : NULL),
	fCount(count),
	fMoveFilesToTrash(moveFilesToTrash),
	fMoveErrorShown(false),
	fItemsRemoved(false)
{
	if (!indices || !fPlaylist || !fItems || !fIndices) {
		// indicate a bad object state
		delete[] fItems;
		fItems = NULL;
		return;
	}

	memcpy(fIndices, indices, fCount * sizeof(int32));
	memset(fItems, 0, fCount * sizeof(PlaylistItem*));

	// init original entry indices
	for (int32 i = 0; i < fCount; i++) {
		fItems[i] = fPlaylist->ItemAt(fIndices[i]);
		if (fItems[i] == NULL) {
			delete[] fItems;
			fItems = NULL;
			return;
		}
	}
}


RemovePLItemsCommand::~RemovePLItemsCommand()
{
	_CleanUp(fItems, fCount, fItemsRemoved);
	delete[] fIndices;
}


status_t
RemovePLItemsCommand::InitCheck()
{
	if (!fPlaylist || !fItems || !fIndices)
		return B_NO_INIT;
	return B_OK;
}


status_t
RemovePLItemsCommand::Perform()
{
	BAutolock _(fPlaylist);

	fItemsRemoved = true;

	int32 lastRemovedIndex = -1;

	// remove refs from playlist
	for (int32 i = 0; i < fCount; i++) {
		// "- i" to account for the items already removed
		lastRemovedIndex = fIndices[i] - i;
		fPlaylist->RemoveItem(lastRemovedIndex);
	}

	// in case we removed the currently playing file
	if (fPlaylist->CurrentItemIndex() == -1)
		fPlaylist->SetCurrentItemIndex(lastRemovedIndex);

	if (fMoveFilesToTrash) {
		BString errorFiles;
		status_t moveError = B_OK;
		bool errorOnAllFiles = true;
		for (int32 i = 0; i < fCount; i++) {
			status_t err = fItems[i]->MoveIntoTrash();
			if (err != B_OK) {
				moveError = err;
				if (errorFiles.Length() > 0)
					errorFiles << ' ';
				errorFiles << fItems[i]->Name();
			} else
				errorOnAllFiles = false;
		}
		// Show an error alert if necessary
		if (!fMoveErrorShown && moveError != B_OK) {
			fMoveErrorShown = true;
			BString message;
			if (errorOnAllFiles)
				message << "All ";
			else
				message << "Some ";
			message << "files could not be moved into the Trash.\n\n";
			message << "Error: " << strerror(moveError);
			(new BAlert("Move Into Trash Error", message.String(),
				"Ok", NULL, NULL, B_WIDTH_AS_USUAL,
				B_WARNING_ALERT))->Go(NULL);
		}
	}

	return B_OK;
}


status_t
RemovePLItemsCommand::Undo()
{
	BAutolock _(fPlaylist);

	fItemsRemoved = false;

	if (fMoveFilesToTrash) {
		for (int32 i = 0; i < fCount; i++) {
			fItems[i]->RestoreFromTrash();
		}
	}

	// remember currently playling item in case we move it
	PlaylistItem* current = fPlaylist->ItemAt(fPlaylist->CurrentItemIndex());

	// add items to playlist at remembered indices
	status_t ret = B_OK;
	for (int32 i = 0; i < fCount; i++) {
		if (!fPlaylist->AddItem(fItems[i], fIndices[i])) {
			ret = B_NO_MEMORY;
			break;
		}
	}
	if (ret < B_OK)
		return ret;

	// take care about currently played ref
	if (current != NULL)
		fPlaylist->SetCurrentItemIndex(fPlaylist->IndexOf(current));

	return B_OK;
}


void
RemovePLItemsCommand::GetName(BString& name)
{
	if (fCount > 1)
		name << "Remove Entries";
	else
		name << "Remove Entry";

	if (fMoveFilesToTrash)
		name << " into Trash";
}
