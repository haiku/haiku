/*
 * Copyright 2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */


#include "VirtualDirectoryPoseView.h"

#include <new>

#include <AutoLocker.h>
#include <NotOwningEntryRef.h>
#include <PathMonitor.h>
#include <storage_support.h>

#include "Commands.h"
#include "Tracker.h"
#include "VirtualDirectoryEntryList.h"
#include "VirtualDirectoryManager.h"


namespace BPrivate {

//	#pragma mark - VirtualDirectoryPoseView


VirtualDirectoryPoseView::VirtualDirectoryPoseView(Model* model)
	:
	BPoseView(model, kListMode),
	fDirectoryPaths(),
	fRootDefinitionFileRef(-1, -1),
	fFileChangeTime(-1),
	fIsRoot(false)
{
	VirtualDirectoryManager* manager = VirtualDirectoryManager::Instance();
	if (manager == NULL)
		return;

	AutoLocker<VirtualDirectoryManager> managerLocker(manager);
	if (_UpdateDirectoryPaths() != B_OK)
		return;

	manager->GetRootDefinitionFile(*model->NodeRef(), fRootDefinitionFileRef);
	fIsRoot = fRootDefinitionFileRef == *model->NodeRef();
}


VirtualDirectoryPoseView::~VirtualDirectoryPoseView()
{
}


void
VirtualDirectoryPoseView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		// ignore all edit operations
		case B_CUT:
		case kCutMoreSelectionToClipboard:
		case kDuplicateSelection:
		case kDelete:
		case kMoveToTrash:
		case kNewEntryFromTemplate:
		case kNewFolder:
		case kEditItem:
			break;

		default:
			_inherited::MessageReceived(message);
			break;
	}
}


void
VirtualDirectoryPoseView::AttachedToWindow()
{
	_inherited::AttachedToWindow();
	SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR, B_DARKEN_1_TINT);
	SetLowUIColor(B_DOCUMENT_BACKGROUND_COLOR, B_DARKEN_1_TINT);
}


void
VirtualDirectoryPoseView::RestoreState(AttributeStreamNode* node)
{
	_inherited::RestoreState(node);
	fViewState->SetViewMode(kListMode);
}


void
VirtualDirectoryPoseView::RestoreState(const BMessage& message)
{
	_inherited::RestoreState(message);
	fViewState->SetViewMode(kListMode);
}


void
VirtualDirectoryPoseView::SavePoseLocations(BRect* frameIfDesktop)
{
}


void
VirtualDirectoryPoseView::SetViewMode(uint32 newMode)
{
}


EntryListBase*
VirtualDirectoryPoseView::InitDirentIterator(const entry_ref* ref)
{
	if (fRootDefinitionFileRef.node < 0 || *ref != *TargetModel()->EntryRef())
		return NULL;

	Model sourceModel(ref, false, true);
	if (sourceModel.InitCheck() != B_OK)
		return NULL;

	VirtualDirectoryEntryList* entryList
		= new(std::nothrow) VirtualDirectoryEntryList(
			*TargetModel()->NodeRef(), fDirectoryPaths);
	if (entryList == NULL || entryList->InitCheck() != B_OK) {
		delete entryList;
		return NULL;
	}

	return entryList;
}


void
VirtualDirectoryPoseView::StartWatching()
{
	// watch the directories
	int32 count = fDirectoryPaths.CountStrings();
	for (int32 i = 0; i < count; i++) {
		BString path = fDirectoryPaths.StringAt(i);
		BPathMonitor::StartWatching(path, B_WATCH_DIRECTORY, this);
	}

	// watch the definition file
	TTracker::WatchNode(TargetModel()->NodeRef(),
		B_WATCH_NAME | B_WATCH_STAT | B_WATCH_ATTR, this);

	// also watch the root definition file
	if (!fIsRoot)
		TTracker::WatchNode(&fRootDefinitionFileRef, B_WATCH_STAT, this);
}


void
VirtualDirectoryPoseView::StopWatching()
{
	BPathMonitor::StopWatching(this);
	stop_watching(this);
}


bool
VirtualDirectoryPoseView::FSNotification(const BMessage* message)
{
	switch (message->GetInt32("opcode", 0)) {
		case B_ENTRY_CREATED:
			return _EntryCreated(message);

		case B_ENTRY_REMOVED:
			return _EntryRemoved(message);

		case B_ENTRY_MOVED:
			return _EntryMoved(message);

		case B_STAT_CHANGED:
			return _NodeStatChanged(message);

		default:
			return _inherited::FSNotification(message);
	}
}


bool
VirtualDirectoryPoseView::_EntryCreated(const BMessage* message)
{
	NotOwningEntryRef entryRef;
	node_ref nodeRef;

	if (message->FindInt32("device", &nodeRef.device) != B_OK
		|| message->FindInt64("node", &nodeRef.node) != B_OK
		|| message->FindInt64("directory", &entryRef.directory) != B_OK
		|| message->FindString("name", (const char**)&entryRef.name) != B_OK) {
		return true;
	}
	entryRef.device = nodeRef.device;

	// It might be one of our directories.
	BString path;
	if (message->FindString("path", &path) == B_OK
		&& fDirectoryPaths.HasString(path)) {
		// Iterate through the directory and generate an entry-created message
		// for each entry.
		BDirectory directory;
		if (directory.SetTo(&nodeRef) != B_OK)
			return true;

		BPrivate::Storage::LongDirEntry entry;
		while (directory.GetNextDirents(&entry, sizeof(entry), 1) == 1) {
			if (strcmp(entry.d_name, ".") != 0
				&& strcmp(entry.d_name, "..") != 0) {
				_DispatchEntryCreatedOrRemovedMessage(B_ENTRY_CREATED,
					node_ref(entry.d_dev, entry.d_ino),
					NotOwningEntryRef(entry.d_pdev, entry.d_pino,
						entry.d_name),
					NULL, false);
			}
		}
		return true;
	}

	// See, if this entry actually becomes visible. If not, we can simply ignore
	// it.
	struct stat st;
	entry_ref visibleEntryRef;
	if (!_GetEntry(entryRef.name, visibleEntryRef, &st)
		|| visibleEntryRef != entryRef) {
		return true;
	}

	// If it is a directory, translate it.
	VirtualDirectoryManager* manager = VirtualDirectoryManager::Instance();
	AutoLocker<VirtualDirectoryManager> managerLocker(manager);

	bool entryTranslated = S_ISDIR(st.st_mode);
	if (entryTranslated) {
		if (manager == NULL)
			return true;

		if (manager->TranslateDirectoryEntry(*TargetModel()->NodeRef(),
				entryRef, nodeRef) != B_OK) {
			return true;
		}
	}

	// The entry might replace another entry. If it does, we'll fake a removed
	// message for the old one first.
	BPose* pose = fPoseList->FindPoseByFileName(entryRef.name);
	if (pose != NULL) {
		if (nodeRef == *pose->TargetModel()->NodeRef()) {
			// apparently not really a new entry -- can happen for
			// subdirectories
			return true;
		}

		// It may be a directory, so tell the manager.
		if (manager != NULL)
			manager->DirectoryRemoved(*pose->TargetModel()->NodeRef());

		managerLocker.Unlock();

		BMessage removedMessage(B_NODE_MONITOR);
		_DispatchEntryCreatedOrRemovedMessage(B_ENTRY_REMOVED,
			*pose->TargetModel()->NodeRef(), *pose->TargetModel()->EntryRef());
	} else
		managerLocker.Unlock();

	return entryTranslated
		? (_DispatchEntryCreatedOrRemovedMessage(B_ENTRY_CREATED, nodeRef,
			entryRef), true)
		: _inherited::FSNotification(message);
}


bool
VirtualDirectoryPoseView::_EntryRemoved(const BMessage* message)
{
	NotOwningEntryRef entryRef;
	node_ref nodeRef;

	if (message->FindInt32("device", &nodeRef.device) != B_OK
		|| message->FindInt64("node", &nodeRef.node) != B_OK
		|| message->FindInt64("directory", &entryRef.directory)
			!= B_OK
		|| message->FindString("name", (const char**)&entryRef.name) != B_OK) {
		return true;
	}
	entryRef.device = nodeRef.device;

	// It might be our definition file.
	if (nodeRef == *TargetModel()->NodeRef())
		return _inherited::FSNotification(message);

	// It might be one of our directories.
	BString path;
	if (message->FindString("path", &path) == B_OK
		&& fDirectoryPaths.HasString(path)) {
		// Find all poses that stem from that directory and generate an
		// entry-removed message for each.
		PoseList poses;
		for (int32 i = 0; BPose* pose = fPoseList->ItemAt(i); i++) {
			NotOwningEntryRef poseEntryRef = *pose->TargetModel()->EntryRef();
			if (poseEntryRef.DirectoryNodeRef() == nodeRef)
				poses.AddItem(pose);
		}

		for (int32 i = 0; BPose* pose = poses.ItemAt(i); i++) {
			_DispatchEntryCreatedOrRemovedMessage(B_ENTRY_REMOVED,
				*pose->TargetModel()->NodeRef(),
				*pose->TargetModel()->EntryRef(), NULL, false);
		}

		return true;
	}

	// If it is a directory, translate it.
	entry_ref* actualEntryRef = &entryRef;
	node_ref* actualNodeRef = &nodeRef;
	entry_ref definitionEntryRef;
	node_ref definitionNodeRef;

	VirtualDirectoryManager* manager = VirtualDirectoryManager::Instance();
	AutoLocker<VirtualDirectoryManager> managerLocker(manager);

	if (manager != NULL
		&& manager->GetSubDirectoryDefinitionFile(*TargetModel()->NodeRef(),
			entryRef.name, definitionEntryRef, definitionNodeRef)) {
		actualEntryRef = &definitionEntryRef;
		actualNodeRef = &definitionNodeRef;
	}

	// Check the pose. It might have been an entry that wasn't visible anyway.
	// In that case we can just ignore the notification.
	BPose* pose = fPoseList->FindPoseByFileName(actualEntryRef->name);
	if (pose == NULL || *actualNodeRef != *pose->TargetModel()->NodeRef())
		return true;

	// See, if another entry becomes visible, now.
	struct stat st;
	entry_ref visibleEntryRef;
	node_ref visibleNodeRef;
	if (_GetEntry(actualEntryRef->name, visibleEntryRef, &st)) {
		// If the new entry is a directory, translate it.
		visibleNodeRef = node_ref(st.st_dev, st.st_ino);
		if (S_ISDIR(st.st_mode)) {
			if (manager == NULL || manager->TranslateDirectoryEntry(
					*TargetModel()->NodeRef(), visibleEntryRef, visibleNodeRef)
					!= B_OK) {
				return true;
			}

			// Effectively nothing changes, when the removed entry was a
			// directory as well.
			if (visibleNodeRef == *actualNodeRef)
				return true;
		}
	}

	if (actualEntryRef == &entryRef) {
		managerLocker.Unlock();
		if (_inherited::FSNotification(message))
			pendingNodeMonitorCache.Add(message);
	} else {
		// tell the manager that the directory has been removed
		manager->DirectoryRemoved(*actualNodeRef);
		managerLocker.Unlock();

		_DispatchEntryCreatedOrRemovedMessage(B_ENTRY_REMOVED, *actualNodeRef,
			*actualEntryRef);
	}

	_DispatchEntryCreatedOrRemovedMessage(B_ENTRY_CREATED, visibleNodeRef,
		visibleEntryRef);

	return true;
}


bool
VirtualDirectoryPoseView::_EntryMoved(const BMessage* message)
{
	NotOwningEntryRef fromEntryRef;
	NotOwningEntryRef toEntryRef;
	node_ref nodeRef;

	if (message->FindInt32("node device", &nodeRef.device) != B_OK
		|| message->FindInt64("node", &nodeRef.node) != B_OK
		|| message->FindInt32("device", &fromEntryRef.device) != B_OK
		|| message->FindInt64("from directory", &fromEntryRef.directory) != B_OK
		|| message->FindInt64("to directory", &toEntryRef.directory) != B_OK
		|| message->FindString("from name", (const char**)&fromEntryRef.name)
			!= B_OK
		|| message->FindString("name", (const char**)&toEntryRef.name)
			!= B_OK) {
		return true;
	}
	toEntryRef.device = fromEntryRef.device;

	// TODO: That's the lazy approach. Ideally we'd analyze the situation and
	// forward a B_ENTRY_MOVED, if possible. There are quite a few cases to
	// consider, though.
	_DispatchEntryCreatedOrRemovedMessage(B_ENTRY_REMOVED, nodeRef,
		fromEntryRef, message->GetString("from path", NULL), false);
	_DispatchEntryCreatedOrRemovedMessage(B_ENTRY_CREATED, nodeRef,
		toEntryRef, message->GetString("path", NULL), false);

	return true;
}


bool
VirtualDirectoryPoseView::_NodeStatChanged(const BMessage* message)
{
	node_ref nodeRef;
	if (message->FindInt32("device", &nodeRef.device) != B_OK
		|| message->FindInt64("node", &nodeRef.node) != B_OK) {
		return true;
	}

	if (nodeRef == fRootDefinitionFileRef) {
		if ((message->GetInt32("fields", 0) & B_STAT_MODIFICATION_TIME) != 0) {
			VirtualDirectoryManager* manager
				= VirtualDirectoryManager::Instance();
			if (manager != NULL) {
				AutoLocker<VirtualDirectoryManager> managerLocker(manager);
				if (!manager->DefinitionFileChanged(
						*TargetModel()->NodeRef())) {
					// The definition file no longer exists. Ignore the message
					// -- we'll get a remove notification soon.
					return true;
				}

				bigtime_t fileChangeTime;
				manager->GetDefinitionFileChangeTime(*TargetModel()->NodeRef(),
					fileChangeTime);
				if (fileChangeTime != fFileChangeTime) {
					_UpdateDirectoryPaths();
					managerLocker.Unlock();
					Refresh();
						// TODO: Refresh() is rather radical. Or rather its
						// implementation is. Ideally it would just compare the
						// currently added poses with what a new dir iterator
						// returns and remove/add poses as needed.
				}
			}
		}

		if (!fIsRoot)
			return true;
	}

	return _inherited::FSNotification(message);
}


void
VirtualDirectoryPoseView::_DispatchEntryCreatedOrRemovedMessage(int32 opcode,
	const node_ref& nodeRef, const entry_ref& entryRef, const char* path,
	bool dispatchToSuperClass)
{
	BMessage message(B_NODE_MONITOR);
	message.AddInt32("opcode", opcode);
	message.AddInt32("device", nodeRef.device);
	message.AddInt64("node", nodeRef.node);
	message.AddInt64("directory", entryRef.directory);
	message.AddString("name", entryRef.name);
	if (path != NULL && path[0] != '\0')
		message.AddString("path", path);
	bool result = dispatchToSuperClass
		? _inherited::FSNotification(&message)
		: FSNotification(&message);
	if (!result)
		pendingNodeMonitorCache.Add(&message);
}


bool
VirtualDirectoryPoseView::_GetEntry(const char* name, entry_ref& _ref,
	struct stat* _st)
{
	return VirtualDirectoryManager::GetEntry(fDirectoryPaths, name, &_ref, _st);
}


status_t
VirtualDirectoryPoseView::_UpdateDirectoryPaths()
{
	VirtualDirectoryManager* manager = VirtualDirectoryManager::Instance();
	Model* model = TargetModel();
	status_t error = manager->ResolveDirectoryPaths(*model->NodeRef(),
		*model->EntryRef(), fDirectoryPaths);
	if (error != B_OK)
		return error;

	manager->GetDefinitionFileChangeTime(*model->NodeRef(), fFileChangeTime);
	return B_OK;
}

} // namespace BPrivate
