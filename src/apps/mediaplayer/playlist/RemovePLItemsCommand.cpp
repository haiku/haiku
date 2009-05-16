/*
 * Copyright 2007-2009, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "RemovePLItemsCommand.h"

#include <new>
#include <stdio.h>

#include <Alert.h>
#include <Autolock.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Path.h>

#include "Playlist.h"


using std::nothrow;


RemovePLItemsCommand::RemovePLItemsCommand(Playlist* playlist,
		 const int32* indices, int32 count, bool moveFilesToTrash)
	:
	Command(),
	fPlaylist(playlist),
	fRefs(count > 0 ? new (nothrow) entry_ref[count] : NULL),
	fNamesInTrash(NULL),
	fIndices(count > 0 ? new (nothrow) int32[count] : NULL),
	fCount(count),
	fMoveFilesToTrash(moveFilesToTrash),
	fMoveErrorShown(false)
{
	if (!indices || !fPlaylist || !fRefs || !fIndices) {
		// indicate a bad object state
		delete[] fRefs;
		fRefs = NULL;
		return;
	}

	memcpy(fIndices, indices, fCount * sizeof(int32));

	if (fMoveFilesToTrash) {
		fNamesInTrash = new (nothrow) BString[count];
		if (fNamesInTrash == NULL)
			return;
	}

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
	delete[] fNamesInTrash;
}


status_t
RemovePLItemsCommand::InitCheck()
{
	if (!fPlaylist || !fRefs || !fIndices
		|| (fMoveFilesToTrash && !fNamesInTrash)) {
		return B_NO_INIT;
	}
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

	if (fMoveFilesToTrash) {
		BString errorFiles;
		status_t moveError = B_OK;
		bool errorOnAllFiles = true;
		char trashPath[B_PATH_NAME_LENGTH];
		for (int32 i = 0; i < fCount; i++) {
			status_t err = find_directory(B_TRASH_DIRECTORY, fRefs[i].device,
				true /*create it*/, trashPath, B_PATH_NAME_LENGTH);
			if (err != B_OK) {
				fprintf(stderr, "failed to find Trash: %s\n", strerror(err));
				continue;
			}

			BEntry entry(&fRefs[i]);
			err = entry.InitCheck();
			if (err != B_OK) {
				fprintf(stderr, "failed to init BEntry for %s: %s\n",
					fRefs[i].name, strerror(err));
				continue;
			}
			BDirectory trashDir(trashPath);
			if (err != B_OK) {
				fprintf(stderr, "failed to init BDirectory for %s: %s\n",
					trashPath, strerror(err));
				continue;
			}

			// Find a unique name for the entry in the trash
			fNamesInTrash[i] = fRefs[i].name;
			int32 uniqueNameIndex = 1;
			while (true) {
				BEntry test(&trashDir, fNamesInTrash[i].String());
				if (!test.Exists())
					break;
				fNamesInTrash[i] = fRefs[i].name;
				fNamesInTrash[i] << ' ' << uniqueNameIndex;
				uniqueNameIndex++;
			}

			// Finally, move the entry into the trash
			err = entry.MoveTo(&trashDir, fNamesInTrash[i].String());
			if (err != B_OK) {
				moveError = err;
				if (errorFiles.Length() > 0)
					errorFiles << ' ';
				errorFiles << fRefs[i].name;
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

	status_t ret = B_OK;

	if (fMoveFilesToTrash) {
		char trashPath[B_PATH_NAME_LENGTH];
		for (int32 i = 0; i < fCount; i++) {
			status_t err = find_directory(B_TRASH_DIRECTORY, fRefs[i].device,
				false /*create it*/, trashPath, B_PATH_NAME_LENGTH);
			if (err != B_OK) {
				fprintf(stderr, "failed to find Trash: %s\n", strerror(err));
				continue;
			}
			// construct the entry to the file in the trash
// TODO: BEntry(const BDirectory* directory, const char* path) is broken!
//			BEntry entry(trashPath, fNamesInTrash[i].String());
BPath path(trashPath, fNamesInTrash[i].String());
BEntry entry(path.Path());
			err = entry.InitCheck();
			if (err != B_OK) {
				fprintf(stderr, "failed to init BEntry for %s: %s\n",
					fNamesInTrash[i].String(), strerror(err));
				continue;
			}
//entry.GetPath(&path);
//printf("moving '%s'\n", path.Path());

			// construct the folder of the original entry_ref
			node_ref nodeRef;
			nodeRef.device = fRefs[i].device;
			nodeRef.node = fRefs[i].directory;
			BDirectory originalDir(&nodeRef);

			if (err != B_OK) {
				fprintf(stderr, "failed to init original BDirectory for "
					"%s: %s\n", fRefs[i].name, strerror(err));
				continue;
			}

//path.SetTo(&originalDir, fRefs[i].name);
//printf("as '%s'\n", path.Path());

			// Finally, move the entry back into the original folder
			err = entry.MoveTo(&originalDir, fRefs[i].name);
			if (err != B_OK)
				ret = err;
		}
	}

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

	if (fMoveFilesToTrash)
		name << " into Trash";
}
