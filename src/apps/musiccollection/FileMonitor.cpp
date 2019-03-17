/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */

#include "FileMonitor.h"

#include <Looper.h>

#include <Messenger.h>
#include <NodeMonitor.h>


FileMonitor::FileMonitor(EntryViewInterface* listener)
	:
	fListener(listener),
	fCurrentReadList(NULL),
	fCurrentReadIndex(0)
{

}


FileMonitor::~FileMonitor()
{
	Reset();
}


void
FileMonitor::SetReadThread(ReadThread* readThread)
{
	fReadThread = readThread;
}


void
FileMonitor::Reset()
{
	fWatchedFileList.clear();
	stop_watching(this);

	BMessenger messenger(this);
	messenger.SendMessage(kMsgCleared);

	if (fCurrentReadList != NULL)
		fCurrentReadIndex = fCurrentReadList->size();
}


void
FileMonitor::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kMsgAddRefs:
		{
			if (fCurrentReadList == NULL)
				fCurrentReadList = fReadThread->ReadRefList();
			uint32 terminate =  fCurrentReadIndex + 50;
			for (; fCurrentReadIndex < terminate; fCurrentReadIndex++) {
				if (fCurrentReadIndex >= fCurrentReadList->size()) {
					fCurrentReadList = NULL;
					fCurrentReadIndex = 0;
					fReadThread->ReadDone();
					break;
				}

				entry_ref& entry = (*fCurrentReadList)[fCurrentReadIndex];
				node_ref nodeRef;
				BNode node(&entry);
				if (node.GetNodeRef(&nodeRef) != B_OK)
					continue;

				EntryCreated(entry.name, entry.directory, entry.device,
					nodeRef.node);
			}
			if (fCurrentReadList)
				Looper()->PostMessage(kMsgAddRefs, this);

			break;
		}

		case kMsgCleared:
			fListener->EntriesCleared();
			break;

		default:
			NodeMonitorHandler::MessageReceived(msg);
			break;
	}
}


void
FileMonitor::EntryCreated(const char *name, ino_t directory, dev_t device,
	ino_t node)
{
	WatchedFile file;
	NodeMonitorHandler::make_node_ref(device, node, &file.node);
	if (fWatchedFileList.find(file.node) != fWatchedFileList.end())
		return;

	NodeMonitorHandler::make_entry_ref(device, directory, name, &file.entry);
	fWatchedFileList[file.node] = file;

	watch_node(&file.node, B_WATCH_NAME | B_WATCH_STAT | B_WATCH_ATTR, this);
	fListener->EntryCreated(&fWatchedFileList[file.node]);
}


void
FileMonitor::EntryRemoved(const char *name, ino_t directory, dev_t device,
	ino_t node)
{
	WatchedFile* file = _FindFile(device, node);
	if (file == NULL)
		return;

	fListener->EntryRemoved(file);
	fWatchedFileList.erase(file->node);
}


void
FileMonitor::EntryMoved(const char *name, const char *fromName,
	ino_t fromDirectory, ino_t toDirectory, dev_t device, ino_t node,
	dev_t nodeDevice)
{
	WatchedFile* file = _FindFile(device, node);
	if (file == NULL)
		return;
	NodeMonitorHandler::make_entry_ref(device, toDirectory, name, &file->entry);
	NodeMonitorHandler::make_node_ref(device, node, &file->node);
	fListener->EntryMoved(file);
}


void
FileMonitor::StatChanged(ino_t node, dev_t device, int32 statFields)
{
	WatchedFile* file = _FindFile(device, node);
	if (file == NULL)
		return;
	fListener->StatChanged(file);
}


void
FileMonitor::AttrChanged(ino_t node, dev_t device)
{
	WatchedFile* file = _FindFile(device, node);
	if (file == NULL)
		return;
	fListener->AttrChanged(file);
}


WatchedFile*
FileMonitor::_FindFile(dev_t device, ino_t node)
{
	node_ref nodeRef;
	NodeMonitorHandler::make_node_ref(device, node, &nodeRef);

	WatchedFileList::iterator it = fWatchedFileList.find(nodeRef);
	if (it == fWatchedFileList.end())
		return NULL;

	return &it->second;
}


int32
ReadThreadFunction(void *data)
{
	ReadThread* that = (ReadThread*)data;
	return that->Process();
}


ReadThread::ReadThread(FileMonitor* target)
	:
	fTarget(target),
	fReading(false),
	fStopped(false),
	fThreadId(-1),
	fNReaded(0),
	fRunning(false)
{
	fWriteRefList = &fRefList1;
	fReadRefList = &fRefList2;
}


status_t
ReadThread::Run()
{
	if (fThreadId >= 0)
		return B_ERROR;

	fStopped = false;
	fThreadId = spawn_thread(ReadThreadFunction, "file reader", B_LOW_PRIORITY,
		this);
	fRunning = true;
	status_t status = resume_thread(fThreadId);
	if (status != B_OK)
		fRunning = false;
	return status;
}


bool
ReadThread::Running()
{
	return fRunning;
}


status_t
ReadThread::Wait()
{
	status_t exitValue;
	return wait_for_thread(fThreadId, &exitValue);
}


void
ReadThread::Stop()
{
	fStopped = true;
}


bool
ReadThread::Stopped()
{
	return fStopped;
}


RefList*
ReadThread::ReadRefList()
{
	return fReadRefList;
}


void
ReadThread::ReadDone()
{
	fReadRefList->clear();
	// and release the list
	fReading = false;

	if (!fRunning && fWriteRefList->size() != 0) {
		BMessenger messenger(fTarget);
		_PublishEntrys(messenger);
	}
}


int32
ReadThread::Process()
{
	BMessenger messenger(fTarget);

	entry_ref entry;
	while (ReadNextEntry(entry)) {
		if (Stopped()) {
			fWriteRefList->clear();
			break;
		}

		fWriteRefList->push_back(entry);

		fNReaded++;
		if (fNReaded >= 50)
			_PublishEntrys(messenger);
	}

	fRunning = false;

	_PublishEntrys(messenger);

	fThreadId = -1;
	return B_OK;
}


void
ReadThread::_SwapLists()
{
	RefList* lastReadList = fReadRefList;
	fReadRefList = fWriteRefList;
	fWriteRefList = lastReadList;
}


void
ReadThread::_PublishEntrys(BMessenger& messenger)
{
	if (fReading || Stopped())
		return;
	_SwapLists();
	fReading = true;
	fNReaded = 0;
	messenger.SendMessage(kMsgAddRefs);
}
