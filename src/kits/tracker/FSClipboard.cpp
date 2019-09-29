/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software..

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/


#include "FSClipboard.h"

#include <Clipboard.h>
#include <Alert.h>
#include <Catalog.h>
#include <Locale.h>
#include <NodeMonitor.h>

#include "Commands.h"
#include "FSUtils.h"
#include "Tracker.h"


// prototypes
static void MakeNodeFromName(node_ref* node, char* name);
static inline void MakeRefName(char* refName, const node_ref* node);
static inline void MakeModeName(char* modeName, const node_ref* node);
static inline void MakeModeNameFromRefName(char* modeName, char* refName);
static inline bool CompareModeAndRefName(const char* modeName,
	const char* refName);

#if 0
static bool
FSClipboardCheckIntegrity()
{
	return true;
}
#endif

static void
MakeNodeFromName(node_ref* node, char* name)
{
	char* nodeString = strchr(name, '_');
	if (nodeString != NULL) {
		node->node = strtoll(nodeString + 1, (char**)NULL, 10);
		node->device = atoi(name + 1);
	}
}


static inline void
MakeRefName(char* refName, const node_ref* node)
{
	sprintf(refName, "r%" B_PRIdDEV "_%" B_PRIdINO, node->device, node->node);
}


static inline void
MakeModeName(char* modeName, const node_ref* node)
{
	sprintf(modeName, "m%" B_PRIdDEV "_%" B_PRIdINO, node->device, node->node);
}


static inline void
MakeModeName(char* name)
{
	name[0] = 'm';
}


static inline void
MakeModeNameFromRefName(char* modeName, char* refName)
{
	strcpy(modeName, refName);
	modeName[0] = 'm';
}


static inline bool
CompareModeAndRefName(const char* modeName, const char* refName)
{
	return !strcmp(refName + 1, modeName + 1);
}


//	#pragma mark - FSClipBoard


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FSClipBoard"


bool
FSClipboardHasRefs()
{
	bool result = false;

	if (be_clipboard->Lock()) {
		BMessage* clip = be_clipboard->Data();
		if (clip != NULL) {
#ifdef B_BEOS_VERSION_DANO
			const
#endif
			char* refName;
#ifdef B_BEOS_VERSION_DANO
			const
#endif
			char* modeName;
			uint32 type;
			int32 count;
			if (clip->GetInfo(B_REF_TYPE, 0, &refName, &type, &count) == B_OK
				&& clip->GetInfo(B_INT32_TYPE, 0, &modeName, &type, &count)
					== B_OK) {
				result = CompareModeAndRefName(modeName, refName);
			}
		}
		be_clipboard->Unlock();
	}
	return result;
}


void
FSClipboardStartWatch(BMessenger target)
{
	TTracker* tracker = dynamic_cast<TTracker*>(be_app);
	if (tracker != NULL && tracker->ClipboardRefsWatcher() != NULL)
		tracker->ClipboardRefsWatcher()->AddToNotifyList(target);
	else {
		// this code is used by external apps using objects using FSClipboard
		// functions, i.e. applications using FilePanel
		BMessenger messenger(kTrackerSignature);
		if (messenger.IsValid()) {
			BMessage message(kStartWatchClipboardRefs);
			message.AddMessenger("target", target);
			messenger.SendMessage(&message);
		}
	}
}


void
FSClipboardStopWatch(BMessenger target)
{
	TTracker* tracker = dynamic_cast<TTracker*>(be_app);
	if (tracker != NULL && tracker->ClipboardRefsWatcher() != NULL)
		tracker->ClipboardRefsWatcher()->AddToNotifyList(target);
	else {
		// this code is used by external apps using objects using FSClipboard
		// functions, i.e. applications using FilePanel
		BMessenger messenger(kTrackerSignature);
		if (messenger.IsValid()) {
			BMessage message(kStopWatchClipboardRefs);
			message.AddMessenger("target", target);
			messenger.SendMessage(&message);
		}
	}
}


void
FSClipboardClear()
{
	if (!be_clipboard->Lock())
		return;

	be_clipboard->Clear();
	be_clipboard->Commit();
	be_clipboard->Unlock();
}


/** This function adds the given poses list to the clipboard, for both copy
 *	and cut. All poses in the list must have "directory" as parent.
 *	"moveMode" is either kMoveSelection or kCopySelection.
 *	It will check if the entries are already present, so that there can only
 *	be one reference to them in the clipboard.
 */
uint32
FSClipboardAddPoses(const node_ref* directory, PoseList* list,
	uint32 moveMode, bool clearClipboard)
{
	uint32 refsAdded = 0;
	int32 listCount = list->CountItems();

	if (listCount == 0 || !be_clipboard->Lock())
		return 0;

	// update message to be send to all listeners
	BMessage updateMessage(kFSClipboardChanges);
	updateMessage.AddInt32("device", directory->device);
	updateMessage.AddInt64("directory", directory->node);
	updateMessage.AddBool("clearClipboard", clearClipboard);

	TClipboardNodeRef clipNode;
	clipNode.moveMode = moveMode;

	if (clearClipboard)
		be_clipboard->Clear();

	BMessage* clip = be_clipboard->Data();
	if (clip != NULL) {
		for (int32 index = 0; index < listCount; index++) {
			char refName[64], modeName[64];
			BPose* pose = (BPose*)list->ItemAt(index);
			Model* model = pose->TargetModel();
			const node_ref* node = model->NodeRef();

			BEntry entry;
			model->GetEntry(&entry);
			if (model->IsVolume()
				|| model->IsRoot()
				|| model->IsTrash()
				|| model->IsDesktop())
				continue;

			MakeRefName(refName, node);
			MakeModeNameFromRefName(modeName, refName);

			if (clearClipboard) {
				if (clip->AddInt32(modeName, (int32)moveMode) == B_OK) {
					if (clip->AddRef(refName, model->EntryRef()) == B_OK) {
						pose->SetClipboardMode(moveMode);

						clipNode.node = *node;
						updateMessage.AddData("tcnode", T_CLIPBOARD_NODE,
							&clipNode, sizeof(TClipboardNodeRef), true,
							listCount);

						refsAdded++;
					} else
						clip->RemoveName(modeName);
				}
			} else {
				if (clip->ReplaceInt32(modeName, (int32)moveMode) == B_OK) {
					// replace old mode if entry already exists in clipboard
					if (clip->ReplaceRef(refName, model->EntryRef()) == B_OK) {
						pose->SetClipboardMode(moveMode);

						clipNode.node = *node;
						updateMessage.AddData("tcnode", T_CLIPBOARD_NODE,
							&clipNode, sizeof(TClipboardNodeRef), true,
							listCount);

						refsAdded++;
					} else {
						clip->RemoveName(modeName);

						clipNode.node = *node;
						clipNode.moveMode = kDelete;	// note removing node
						updateMessage.AddData("tcnode", T_CLIPBOARD_NODE,
							&clipNode, sizeof(TClipboardNodeRef), true,
							listCount);
						clipNode.moveMode = moveMode;
							// set it back to current value
					}
				} else {
					// add it if it doesn't exist
					if (clip->AddRef(refName, model->EntryRef()) == B_OK
						&& clip->AddInt32(modeName, (int32)moveMode) == B_OK) {
						pose->SetClipboardMode(moveMode);

						clipNode.node = *node;
						updateMessage.AddData("tcnode", T_CLIPBOARD_NODE,
							&clipNode, sizeof(TClipboardNodeRef), true,
							listCount);

						refsAdded++;
					} else {
						clip->RemoveName(modeName);
						clip->RemoveName(refName);
						// here notifying delete isn't needed as node didn't
						// exist in clipboard
					}
				}
			}
		}
		be_clipboard->Commit();
	}
	be_clipboard->Unlock();

	BMessenger(kTrackerSignature).SendMessage(&updateMessage);
		// Tracker will notify all listeners

	return refsAdded;
}


uint32
FSClipboardRemovePoses(const node_ref* directory, PoseList* list)
{
	if (!be_clipboard->Lock())
		return 0;

	// update message to be send to all listeners
	BMessage updateMessage(kFSClipboardChanges);
	updateMessage.AddInt32("device", directory->device);
	updateMessage.AddInt64("directory", directory->node);
	updateMessage.AddBool("clearClipboard", false);

	TClipboardNodeRef clipNode;
	clipNode.moveMode = kDelete;

	uint32 refsRemoved = 0;

	BMessage* clip = be_clipboard->Data();
	if (clip != NULL) {
		int32 listCount = list->CountItems();

		for (int32 index = 0; index < listCount; index++) {
			char refName[64], modeName[64];
			BPose* pose = (BPose*)list->ItemAt(index);

			clipNode.node = *pose->TargetModel()->NodeRef();
			MakeRefName(refName, &clipNode.node);
			MakeModeName(modeName);

			if (clip->RemoveName(refName) == B_OK
				&& clip->RemoveName(modeName)) {
				updateMessage.AddData("tcnode", T_CLIPBOARD_NODE, &clipNode,
					sizeof(TClipboardNodeRef), true, listCount);
				refsRemoved++;
			}
		}
		be_clipboard->Commit();
	}
	be_clipboard->Unlock();

	BMessenger(kTrackerSignature).SendMessage(&updateMessage);
		// Tracker will notify all listeners

	return refsRemoved;
}


/** Pastes entries from the clipboard to the target model's directory.
 *	Updates moveModes and notifies listeners if necessary.
 */
bool
FSClipboardPaste(Model* model, uint32 linksMode)
{
	if (!FSClipboardHasRefs())
		return false;

	BMessenger tracker(kTrackerSignature);

	node_ref* destNodeRef = (node_ref*)model->NodeRef();

	// these will be passed to the asynchronous copy/move process
	BObjectList<entry_ref>* moveList = new BObjectList<entry_ref>(0, true);
	BObjectList<entry_ref>* copyList = new BObjectList<entry_ref>(0, true);
	BObjectList<entry_ref>* duplicateList = new BObjectList<entry_ref>(0, true);

	if ((be_clipboard->Lock())) {
		BMessage* clip = be_clipboard->Data();
		if (clip != NULL) {
			char modeName[64];
			uint32 moveMode = 0;

			BMessage* updateMessage = NULL;
			node_ref updateNodeRef;
			updateNodeRef.device = -1;

			char* refName;
			type_code type;
			int32 count;
			for (int32 index = 0; clip->GetInfo(B_REF_TYPE, index,
#ifdef B_BEOS_VERSION_DANO
				(const char**)
#endif
				&refName, &type, &count) == B_OK; index++) {
				entry_ref ref;
				if (clip->FindRef(refName, &ref) != B_OK)
					continue;

				// If the entry_ref's directory has changed, send previous notification
				// (if any), and start new one for the new directory
				if (updateNodeRef.device != ref.device
					|| updateNodeRef.node != ref.directory) {
					if (updateMessage != NULL) {
						tracker.SendMessage(updateMessage);
						delete updateMessage;
					}

					updateNodeRef.device = ref.device;
					updateNodeRef.node = ref.directory;

					updateMessage = new BMessage(kFSClipboardChanges);
					updateMessage->AddInt32("device", updateNodeRef.device);
					updateMessage->AddInt64("directory", updateNodeRef.node);
				}

				// we need this data later on
				MakeModeNameFromRefName(modeName, refName);
				if (!linksMode && clip->FindInt32(modeName, (int32*)&moveMode)
					!= B_OK) {
					continue;
				}

				BEntry entry(&ref);

				uint32 newMoveMode = 0;
				bool sameDirectory = destNodeRef->device == ref.device
					&& destNodeRef->node == ref.directory;

				if (!entry.Exists()) {
					// The entry doesn't exist anymore, so we'll remove
					// that entry from the clipboard as well
					clip->RemoveName(refName);
					clip->RemoveName(modeName);

					newMoveMode = kDelete;
				} else {
					// the entry does exist, so lets see what we will
					// do with it
					if (!sameDirectory) {
						if (linksMode || moveMode == kMoveSelectionTo) {
							// the linksMode uses the moveList as well
							moveList->AddItem(new entry_ref(ref));
						} else if (moveMode == kCopySelectionTo)
							copyList->AddItem(new entry_ref(ref));
					} else {
						// if the entry should have been removed from its
						// directory, we want to copy that entry next time, no
						// matter if the items don't have to be moved at all
						// (source == target)
						if (moveMode == kMoveSelectionTo)
							newMoveMode = kCopySelectionTo;
						else {
							// we are copying a file into its same directory, do
							// a duplicate
							duplicateList->AddItem(new entry_ref(ref));
						}
					}
				}

				// add the change to the update message (if necessary)
				if (newMoveMode) {
					clip->ReplaceInt32(modeName, kCopySelectionTo);

					TClipboardNodeRef clipNode;
					MakeNodeFromName(&clipNode.node, modeName);
					clipNode.moveMode = kDelete;
					updateMessage->AddData("tcnode", T_CLIPBOARD_NODE,
						&clipNode, sizeof(TClipboardNodeRef), true);
				}
			}
			be_clipboard->Commit();

			// send notification for the last directory
			if (updateMessage != NULL) {
				tracker.SendMessage(updateMessage);
				delete updateMessage;
			}
		}
		be_clipboard->Unlock();
	}

	bool okToMove = true;

	// can't copy/paste to root('/') directory
	if (model->IsRoot()) {
		BAlert* alert = new BAlert("",
			B_TRANSLATE("You must drop items on one of the disk icons "
			"in the \"Disks\" window."), B_TRANSLATE("Cancel"), NULL, NULL,
			B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
		okToMove = false;
	}

	BEntry entry;
	model->GetEntry(&entry);

	// can't copy items into the trash
	if (copyList->CountItems() > 0 && model->IsTrash()) {
		BAlert* alert = new BAlert("",
			B_TRANSLATE("Sorry, you can't copy items to the Trash."),
			B_TRANSLATE("Cancel"), NULL, NULL, B_WIDTH_AS_USUAL,
			B_WARNING_ALERT);
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
		okToMove = false;
	}

	if (!okToMove) {
		// there was some problem with our target, so we bail out here
		delete moveList;
		delete copyList;
		delete duplicateList;
		return false;
	}

	// asynchronous calls take over ownership of the objects passed to it
	if (moveList->CountItems() > 0) {
		FSMoveToFolder(moveList, new BEntry(entry),
			linksMode ? linksMode : kMoveSelectionTo);
	} else
		delete moveList;

	if (copyList->CountItems() > 0)
		FSMoveToFolder(copyList, new BEntry(entry), kCopySelectionTo);
	else
		delete copyList;

	if (duplicateList->CountItems() > 0)
		FSMoveToFolder(duplicateList, new BEntry(entry), kDuplicateSelection);
	else
		delete duplicateList;

	return true;
}


// Seek node in clipboard, if found return it's moveMode
// else return 0
uint32
FSClipboardFindNodeMode(Model* model, bool autoLock, bool updateRefIfNeeded)
{
	int32 moveMode = 0;
	if (autoLock) {
		if (!be_clipboard->Lock())
			return 0;
	}
	bool remove = false;
	bool change = false;

	BMessage* clip = be_clipboard->Data();
	if (clip != NULL) {
		const node_ref* node = model->NodeRef();
		char modeName[64];
		MakeModeName(modeName, node);
		if ((clip->FindInt32(modeName, &moveMode) == B_OK)) {
			const entry_ref* ref = model->EntryRef();
			entry_ref clipref;
			char refName[64];
			MakeRefName(refName, node);
			if ((clip->FindRef(refName, &clipref) == B_OK)) {
				if (clipref != *ref) {
					if (updateRefIfNeeded) {
						clip->ReplaceRef(refName, ref);
						change = true;
					} else {
						clip->RemoveName(refName);
						clip->RemoveName(modeName);
						change = true;
						remove = true;
						moveMode = 0;
					}
				}
			} else {
				clip->RemoveName(modeName);
				change = true;
				remove = true;
				moveMode = 0;
			}
		}
	}
	if (change)
		be_clipboard->Commit();

	if (autoLock)
		be_clipboard->Unlock();

	if (remove)
		FSClipboardRemove(model);

	return (uint32)moveMode;
}


void
FSClipboardRemove(Model* model)
{
	BMessenger messenger(kTrackerSignature);
	if (messenger.IsValid()) {
		BMessage* report = new BMessage(kFSClipboardChanges);
		TClipboardNodeRef tcnode;
		tcnode.node = *model->NodeRef();
		tcnode.moveMode = kDelete;
		const entry_ref* ref = model->EntryRef();
		report->AddInt32("device", ref->device);
		report->AddInt64("directory", ref->directory);
		report->AddBool("clearClipboard", false);
		report->AddData("tcnode", T_CLIPBOARD_NODE, &tcnode, sizeof(tcnode),
			true);
		messenger.SendMessage(report);
		delete report;
	}
}


//	#pragma mark -


BClipboardRefsWatcher::BClipboardRefsWatcher()
	:	BLooper("ClipboardRefsWatcher", B_LOW_PRIORITY, 4096),
	fNotifyList(10, false)
{
	watch_node(NULL, B_WATCH_MOUNT, this);
	fRefsInClipboard = FSClipboardHasRefs();
	be_clipboard->StartWatching(this);
}


BClipboardRefsWatcher::~BClipboardRefsWatcher()
{
	stop_watching(this);
	be_clipboard->StopWatching(this);
}


void
BClipboardRefsWatcher::AddToNotifyList(BMessenger target)
{
	if (Lock()) {
		// add the messenger if it's not already in the list
		// ToDo: why do we have to care about that?
		BMessenger* messenger;
		bool found = false;

		for (int32 index = 0; (messenger = fNotifyList.ItemAt(index)) != NULL;
				index++) {
			if (*messenger == target) {
				found = true;
				break;
			}
		}
		if (!found)
			fNotifyList.AddItem(new BMessenger(target));

		Unlock();
	}
}


void
BClipboardRefsWatcher::RemoveFromNotifyList(BMessenger target)
{
	if (Lock()) {
		BMessenger* messenger;

		for (int32 index = 0; (messenger = fNotifyList.ItemAt(index)) != NULL;
				index++) {
			if (*messenger == target) {
				delete fNotifyList.RemoveItemAt(index);
				break;
			}
		}
		Unlock();
	}
}


void
BClipboardRefsWatcher::AddNode(const node_ref* node)
{
	TTracker::WatchNode(node, B_WATCH_NAME, this);
	fRefsInClipboard = true;
}


void
BClipboardRefsWatcher::RemoveNode(node_ref* node, bool removeFromClipboard)
{
	watch_node(node, B_STOP_WATCHING, this);

	if (!removeFromClipboard)
		return;

	if (be_clipboard->Lock()) {
		BMessage* clip = be_clipboard->Data();
		if (clip != NULL) {
			char name[64];
			MakeRefName(name, node);
			clip->RemoveName(name);
			MakeModeName(name);
			clip->RemoveName(name);

			be_clipboard->Commit();
		}
		be_clipboard->Unlock();
	}
}


void
BClipboardRefsWatcher::RemoveNodesByDevice(dev_t device)
{
	if (!be_clipboard->Lock())
		return;

	BMessage* clip = be_clipboard->Data();
	if (clip != NULL) {
		char deviceName[6];
		sprintf(deviceName, "r%" B_PRIdDEV "_", device);

		int32 index = 0;
		char* refName;
		type_code type;
		int32 count;
		while (clip->GetInfo(B_REF_TYPE, index,
#ifdef B_BEOS_VERSION_DANO
			(const char**)
#endif
			&refName, &type, &count) == B_OK) {
			if (!strncmp(deviceName, refName, strlen(deviceName))) {
				clip->RemoveName(refName);
				MakeModeName(refName);
				clip->RemoveName(refName);

				node_ref node;
				MakeNodeFromName(&node, refName);
				watch_node(&node, B_STOP_WATCHING, this);
			}
			index++;
		}
		be_clipboard->Commit();
	}
	be_clipboard->Unlock();
}


void
BClipboardRefsWatcher::UpdateNode(node_ref* node, entry_ref* ref)
{
	if (!be_clipboard->Lock())
		return;

	BMessage* clip = be_clipboard->Data();
	if (clip != NULL) {
		char name[64];
		MakeRefName(name, node);
		if ((clip->ReplaceRef(name, ref)) != B_OK) {
			clip->RemoveName(name);
			MakeModeName(name);
			clip->RemoveName(name);

			RemoveNode(node);
		}
		be_clipboard->Commit();
	}
	be_clipboard->Unlock();
}


void
BClipboardRefsWatcher::Clear()
{
	stop_watching(this);
	watch_node(NULL, B_WATCH_MOUNT, this);

	BMessage message(kFSClipboardChanges);
	message.AddBool("clearClipboard", true);
	if (Lock()) {
		int32 items = fNotifyList.CountItems();
		for (int32 i = 0;i < items;i++) {
			fNotifyList.ItemAt(i)->SendMessage(&message);
		}
		Unlock();
	}
}


//void
//BClipboardRefsWatcher::UpdatePoseViews(bool clearClipboard,
//	const node_ref* node)
//{
//	BMessage message(kFSClipboardChanges);
//	message.AddInt32("device", node->device);
//	message.AddInt64("directory", node->node);
//	message.AddBool("clearClipboard", clearClipboard);
//
//	if (Lock()) {
//		int32 items = fNotifyList.CountItems();
//		for (int32 i = 0;i < items;i++) {
//			fNotifyList.ItemAt(i)->SendMessage(&message);
//		}
//		Unlock();
//	}
//}


void
BClipboardRefsWatcher::UpdatePoseViews(BMessage* reportMessage)
{
	if (Lock()) {
		// check if it was cleared, if so clear watching
		bool clearClipboard = false;
		if (reportMessage->FindBool("clearClipboard", &clearClipboard) == B_OK
			&& clearClipboard) {
			stop_watching(this);
			watch_node(NULL, B_WATCH_MOUNT, this);
		}

		// loop through reported node_ref's movemodes:
		// move or copy: start watching node_ref
		// remove: stop watching node_ref
		int32 index = 0;
		TClipboardNodeRef* tcnode = NULL;
		ssize_t size;
		while (reportMessage->FindData("tcnode", T_CLIPBOARD_NODE, index,
				(const void**)&tcnode, &size) == B_OK) {
			if (tcnode->moveMode == kDelete) {
				watch_node(&tcnode->node, B_STOP_WATCHING, this);
			} else {
				watch_node(&tcnode->node, B_STOP_WATCHING, this);
				TTracker::WatchNode(&tcnode->node, B_WATCH_NAME, this);
				fRefsInClipboard = true;
			}
			index++;
		}

		// send report
		int32 items = fNotifyList.CountItems();
		for (int32 i = 0;i < items;i++) {
			fNotifyList.ItemAt(i)->SendMessage(reportMessage);
		}
		Unlock();
	}
}


void
BClipboardRefsWatcher::MessageReceived(BMessage* message)
{
	if (message->what == B_CLIPBOARD_CHANGED && fRefsInClipboard) {
		if (!(fRefsInClipboard = FSClipboardHasRefs()))
			Clear();
		return;
	} else if (message->what != B_NODE_MONITOR) {
		_inherited::MessageReceived(message);
		return;
	}

	switch (message->FindInt32("opcode")) {
		case B_ENTRY_MOVED:
		{
			ino_t toDir;
			ino_t fromDir;
			node_ref node;
			const char* name = NULL;
			message->FindInt64("from directory", &fromDir);
			message->FindInt64("to directory", &toDir);
			message->FindInt64("node", &node.node);
			message->FindInt32("device", &node.device);
			message->FindString("name", &name);
			entry_ref ref(node.device, toDir, name);
			UpdateNode(&node, &ref);
			break;
		}

		case B_DEVICE_UNMOUNTED:
		{
			dev_t device;
			message->FindInt32("device", &device);
			RemoveNodesByDevice(device);
			break;
		}

		case B_ENTRY_REMOVED:
		{
			node_ref node;
			message->FindInt64("node", &node.node);
			message->FindInt32("device", &node.device);
			RemoveNode(&node, true);
			break;
		}
	}
}
