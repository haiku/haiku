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


#include "FolderWatcher.h"

#include <stdio.h>

// BeOS
#include <kernel/fs_attr.h>
#include <Node.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>


// Implementation of FolderWatcher

FolderWatcher::FolderWatcher(BLooper* looper, const BDirectory& folder, bool watchAttrChanges) 
	: fFolder(folder)
	, fListener(NULL)
	, fWatchAttrChanges(watchAttrChanges)
{
		// add this handler to the application for node monitoring
	if (looper->Lock()) {
		looper->AddHandler(this);
		looper->Unlock();
	}

		// start attribute change watching for existing files
	if (watchAttrChanges) {
		BEntry entry;
		node_ref node;
		while (fFolder.GetNextEntry(&entry) == B_OK && entry.GetNodeRef(&node) == B_OK) {
			StartAttrWatching(&node);
		}
	}

		// start watching the spooler directory
	node_ref ref;
	fFolder.GetNodeRef(&ref);
	watch_node(&ref, B_WATCH_DIRECTORY, this);
}

FolderWatcher::~FolderWatcher() {
		// stop node watching for spooler directory
	node_ref ref;
	fFolder.GetNodeRef(&ref);
	watch_node(&ref, B_STOP_WATCHING, this);
		// stop sending notifications to this handler
	stop_watching(this);
	
	if (LockLooper()) {
		BLooper* looper = Looper();
			// and remove it
		looper->RemoveHandler(this);
		looper->Unlock();
	}
}

void FolderWatcher::SetListener(FolderListener* listener) {
	fListener = listener;
}

bool FolderWatcher::BuildEntryRef(BMessage* msg, const char* dirName, entry_ref* entry) {
	const char* name;
	if (msg->FindInt32("device", &entry->device) == B_OK &&
		msg->FindInt64(dirName, &entry->directory) == B_OK &&
		msg->FindString("name", &name) == B_OK) {
		entry->set_name(name);
		return true;
	}
	return false;
}

bool FolderWatcher::BuildNodeRef(BMessage* msg, node_ref* node) {
	return (msg->FindInt32("device", &node->device) == B_OK &&
		msg->FindInt64("node", &node->node) == B_OK);
}

void FolderWatcher::HandleCreatedEntry(BMessage* msg, const char* dirName) {
	node_ref node;
	entry_ref entry;
	if (BuildEntryRef(msg, dirName, &entry) &&
		BuildNodeRef(msg, &node)) {
		if (fWatchAttrChanges) StartAttrWatching(&node);
		fListener->EntryCreated(&node, &entry);
	}
}

void FolderWatcher::HandleRemovedEntry(BMessage* msg) {
	node_ref node;
	if (BuildNodeRef(msg, &node)) {
		if (fWatchAttrChanges) StopAttrWatching(&node);
		fListener->EntryRemoved(&node);
	}
}

void FolderWatcher::HandleChangedAttr(BMessage* msg) {
	node_ref node;
	if (BuildNodeRef(msg, &node)) {
		fListener->AttributeChanged(&node);
	}
}

void FolderWatcher::MessageReceived(BMessage* msg) {
	int32 opcode;
	node_ref folder;
	ino_t dir;
	
	if (msg->what == B_NODE_MONITOR) {
		if (fListener == NULL || msg->FindInt32("opcode", &opcode) != B_OK) return;
		
		switch (opcode) {
			case B_ENTRY_CREATED: 
				HandleCreatedEntry(msg, "directory");
				break;
			case B_ENTRY_REMOVED:
				HandleRemovedEntry(msg);
				break;
			case B_ENTRY_MOVED:
				fFolder.GetNodeRef(&folder);
				if (msg->FindInt64("to directory", &dir) == B_OK && folder.node == dir) {
					// entry moved into this folder
					HandleCreatedEntry(msg, "to directory");
				} else if (msg->FindInt64("from directory", &dir) == B_OK && folder.node == dir) {
					// entry removed from this folder
					HandleRemovedEntry(msg);
				}
				break;
			case B_ATTR_CHANGED: 
				HandleChangedAttr(msg);
				break;
			default: // nothing to do
				break;
		}
	} else {
		inherited::MessageReceived(msg);
	}
}

status_t FolderWatcher::StartAttrWatching(node_ref* node) {
	return watch_node(node, B_WATCH_ATTR, this);
}

status_t FolderWatcher::StopAttrWatching(node_ref* node) {
	return watch_node(node, B_STOP_WATCHING, this);
}
