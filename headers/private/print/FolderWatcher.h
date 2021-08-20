/*****************************************************************************/
// FolderWatcher
//
// Author
//   Michael Pfeiffer
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2002 Haiku Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#ifndef _FOLDER_WATCHER_H
#define _FOLDER_WATCHER_H

#include <Directory.h>
#include <Entry.h>
#include <Handler.h>
#include <Looper.h>
#include <String.h>
#include <StorageDefs.h>


class FolderListener {
public:
	virtual ~FolderListener() {};
		// entry created or moved into folder
	virtual void EntryCreated(node_ref* node, entry_ref* entry) {};
		// entry removed from folder (or moved to another folder) 
	virtual void EntryRemoved(node_ref* node) {};
		// attribute of an entry has changed
	virtual void AttributeChanged(node_ref* node) {};
};


// The class FolderWatcher watches creation, deletion of files in a directory
// and optionally watches attribute changes of files in the directory.
class FolderWatcher : public BHandler {
	typedef BHandler inherited;

private:
	BDirectory fFolder;
	FolderListener* fListener;
	bool fWatchAttrChanges;

	bool BuildEntryRef(BMessage* msg, const char* dirName, entry_ref* entry);
	bool BuildNodeRef(BMessage* msg, node_ref* node);

	void HandleCreatedEntry(BMessage* msg, const char* dirName);
	void HandleRemovedEntry(BMessage* msg);
	void HandleChangedAttr(BMessage* msg);	

public:
		// start node watching (optionally watch attribute changes)
	FolderWatcher(BLooper* looper, const BDirectory& folder, bool watchAttrChanges = false);
		// stop node watching
	virtual ~FolderWatcher();

	void MessageReceived(BMessage* msg);

		// the directory
	BDirectory* Folder() { return &fFolder; }

		// set listener that is notified of changes in the directory
	void SetListener(FolderListener* listener);

		// start/stop watching of attribute changes 
	status_t StartAttrWatching(node_ref* node);
	status_t StopAttrWatching(node_ref* node);
};

#endif
