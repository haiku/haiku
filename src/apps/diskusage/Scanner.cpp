/*
 * Copyright (c) 2008 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT/X11 license.
 *
 * Copyright (c) 1999 Mike Steed. You are free to use and distribute this software
 * as long as it is accompanied by it's documentation and this copyright notice.
 * The software comes with no warranty, etc.
 */
#include "Scanner.h"

#include <stdlib.h>
#include <string.h>

#include <Directory.h>

#include "Common.h"

using std::vector;

Scanner::Scanner(BVolume *v, BHandler *handler)
	: BLooper(),
	  fListener(handler),
	  fDoneMessage(kScanDone),
	  fProgressMessage(kScanProgress),

	  fVolume(v),
	  fSnapshot(NULL),
	  fDesiredPath(),
	  fTask(),
	  fBusy(false),
	  fQuitRequested(false)
{
	fDoneMessage.AddPointer(kNameVolPtr, fVolume);
	fProgressMessage.AddPointer(kNameVolPtr, fVolume);

	Run();
}


Scanner::~Scanner()
{
	delete fSnapshot;
}


void
Scanner::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kScanRefresh:
		{
			FileInfo* startInfo;
			if (message->FindPointer(kNameFilePtr, (void **)&startInfo) == B_OK)
				_RunScan(startInfo);
			break;
		}

		default:
			BLooper::MessageReceived(message);
			break;
	}
}


void
Scanner::Refresh(FileInfo* startInfo)
{
	if (fBusy)
		return;

	fBusy = true;

	// Remember the current directory, if any, so we can return to it after
	// the scanning is done.
	if (fSnapshot != NULL && fSnapshot->currentDir != NULL)
		fSnapshot->currentDir->GetPath(fDesiredPath);

	fTask.assign(kEmptyStr);

	BMessage msg(kScanRefresh);
	msg.AddPointer(kNameFilePtr, startInfo);
	PostMessage(&msg);
}


void
Scanner::SetDesiredPath(string &path)
{
	fDesiredPath = path;

	if (fSnapshot == NULL)
		Refresh();
	else
		_ChangeToDesired();
}


void
Scanner::RequestQuit()
{
	// If the looper thread is scanning, we won't succeed to grab the lock,
	// since the thread is scanning from within MessageReceived(). Then by
	// setting fQuitRequested, the thread will stop scanning and only after
	// that happened we succeed to grab the lock. So this line of action
	// should be safe.
	fQuitRequested = true;
	Lock();
	Quit();
}


// #pragma mark - private


void
Scanner::_RunScan(FileInfo* startInfo)
{
	if (startInfo == NULL || startInfo == fSnapshot->rootDir) {
		delete fSnapshot;
		fSnapshot = new VolumeSnapshot(fVolume);
		fTask = kStrScanningX + fSnapshot->name;
		fVolumeBytesInUse = fSnapshot->capacity - fSnapshot->freeBytes;
		fVolumeBytesScanned = 0;
		fProgress = 0.0;
		fLastReport = -100.0;

		BDirectory root;
		fVolume->GetRootDirectory(&root);
		fSnapshot->rootDir = _GetFileInfo(&root, NULL);

		FileInfo* freeSpace = new FileInfo;
		freeSpace->pseudo = true;
		string s = "Free on " + fSnapshot->name;
		freeSpace->ref.set_name(s.c_str());
		freeSpace->size = fSnapshot->freeBytes;
		fSnapshot->freeSpace = freeSpace;

		fSnapshot->currentDir = NULL;

	} else {
		fSnapshot->capacity = fVolume->Capacity();
		fSnapshot->freeBytes = fVolume->FreeBytes();
		fTask = kStrScanningX + string(startInfo->ref.name);
		fVolumeBytesInUse = fSnapshot->capacity - fSnapshot->freeBytes;
		fVolumeBytesScanned = fVolumeBytesInUse - startInfo->size; // best guess
		fProgress = 100.0 * fVolumeBytesScanned / fVolumeBytesInUse;
		fLastReport = -100.0;

		BDirectory startDir(&startInfo->ref);
		if (startDir.InitCheck() == B_OK) {
			FileInfo *parent = startInfo->parent;
			vector<FileInfo *>::iterator i = parent->children.begin();
			FileInfo* newInfo = _GetFileInfo(&startDir, parent);
			while (i != parent->children.end() && *i != startInfo)
				i++;

			int idx = i - parent->children.begin();
			parent->children[idx] = newInfo;

			// Fixup count and size fields in parent directory.
			parent->size += newInfo->size - startInfo->size;
			parent->count += newInfo->count - startInfo->count;

			delete startInfo;
		}
	}

	fBusy = false;
	_ChangeToDesired();
	fListener.SendMessage(&fDoneMessage);
}


FileInfo*
Scanner::_GetFileInfo(BDirectory* dir, FileInfo* parent)
{
	FileInfo* thisDir = new FileInfo;
	BEntry entry;
	dir->GetEntry(&entry);
	entry.GetRef(&thisDir->ref);
	thisDir->parent = parent;
	thisDir->count = 0;

	while (true) {
		if (fQuitRequested)
			break;

		if (dir->GetNextEntry(&entry) == B_ENTRY_NOT_FOUND)
			break;
		if (entry.IsSymLink())
			continue;

		if (entry.IsFile()) {
			FileInfo *child = new FileInfo;
			entry.GetRef(&child->ref);
			entry.GetSize(&child->size);
			child->parent = thisDir;
			thisDir->children.push_back(child);

			// Send a progress report periodically.
			fVolumeBytesScanned += child->size;
			fProgress = 100.0 * fVolumeBytesScanned / fVolumeBytesInUse;
			if (fProgress - fLastReport > kReportInterval) {
				fLastReport = fProgress;
				fListener.SendMessage(&fProgressMessage);
			}
		}
		else if (entry.IsDirectory()) {
			BDirectory childDir(&entry);
			thisDir->children.push_back(_GetFileInfo(&childDir, thisDir));
		}
		thisDir->count++;
	}

	vector<FileInfo *>::iterator i = thisDir->children.begin();
	while (i != thisDir->children.end()) {
		thisDir->size += (*i)->size;
		thisDir->count += (*i)->count;
		i++;
	}

	return thisDir;
}


void
Scanner::_ChangeToDesired()
{
	if (fBusy || fDesiredPath.size() <= 0)
		return;

	char* workPath = strdup(fDesiredPath.c_str());
	char* pathPtr = &workPath[1]; // skip leading '/'
	char* endOfPath = pathPtr + strlen(pathPtr);

	// Check the name of the root directory.  It is a special case because
	// it has no parent data structure.
	FileInfo* checkDir = fSnapshot->rootDir;
	FileInfo* prefDir = NULL;
	char* sep = strchr(pathPtr, '/');
	if (sep != NULL)
		*sep = '\0';

	if (strcmp(pathPtr, checkDir->ref.name) == 0) {
		// Root directory matches, so follow the remainder of the path as
		// far as possible.
		prefDir = checkDir;
		pathPtr += 1 + strlen(pathPtr);
		while (pathPtr < endOfPath) {
			sep = strchr(pathPtr, '/');
			if (sep != NULL)
				*sep = '\0';

			checkDir = prefDir->FindChild(pathPtr);
			if (checkDir == NULL || checkDir->children.size() == 0)
				break;

			pathPtr += 1 + strlen(pathPtr);
			prefDir = checkDir;
		}
	}

	// If the desired path is the volume's root directory, default to the
	// volume display.
	if (prefDir == fSnapshot->rootDir)
		prefDir = NULL;

	ChangeDir(prefDir);

	free(workPath);
	fDesiredPath.erase();
}


