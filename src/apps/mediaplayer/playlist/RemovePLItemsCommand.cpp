/*
 * Copyright © 2007-2009 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "RemovePLItemsCommand.h"

#include <new>
#include <stdio.h>

#include <Alert.h>
#include <Autolock.h>
#include <Catalog.h>
#include <Locale.h>

#include "Playlist.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MediaPlayer-RemovePLItemsCmd"


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
				message << 
					B_TRANSLATE("All files could not be moved into Trash.");
			else
				message << 
					B_TRANSLATE("Some files could not be moved into Trash.");
			message << "\n\n" << B_TRANSLATE("Error: ") << strerror(moveError);
			BAlert* alert = new BAlert(B_TRANSLATE("Move into trash error"),
				message.String(), B_TRANSLATE("OK"), NULL, NULL,
				B_WIDTH_AS_USUAL, B_WARNING_ALERT);
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			alert->Go(NULL);
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
		fPlaylist->SetCurrentItemIndex(fPlaylist->IndexOf(current), false);

	return B_OK;
}


void
RemovePLItemsCommand::GetName(BString& name)
{
	if (fMoveFilesToTrash) {
		if (fCount > 1)
			name << B_TRANSLATE("Remove Entries into Trash");
		else
			name << B_TRANSLATE("Remove Entry into Trash");
	}
	else
	{	
		if (fCount > 1)
			name << B_TRANSLATE("Remove Entries");
		else
			name << B_TRANSLATE("Remove Entry");
	}
}
