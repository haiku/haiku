/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef FILE_MONITOR_H
#define FILE_MONITOR_H


#include <map>
#include <vector>

#include <Entry.h>
#include <Node.h>

#include "NodeMonitorHandler.h"


struct WatchedFile {
	entry_ref		entry;
	node_ref		node;
	/*! Don't use it as the primary cookie storage. To be set in EntryCreated
	in EntryViewInterface. */
	void*			cookie;
};


class NodeRefComp {
public:
	bool
	operator()(const node_ref& a, const node_ref& b)
	{
		return a.node < b.node;
	}
};


typedef std::map<node_ref, WatchedFile, NodeRefComp> WatchedFileList;


class EntryViewInterface {
public:
	virtual						~EntryViewInterface() {};

	virtual void				EntryCreated(WatchedFile* file) {};
	virtual void				EntryRemoved(WatchedFile* file) {};
	virtual void				EntryMoved(WatchedFile* file) {};
	virtual void				StatChanged(WatchedFile* file) {};
	virtual void				AttrChanged(WatchedFile* file) {};

	virtual void				EntriesCleared() {};
};


const uint32 kMsgAddRefs = '&adr';
const uint32 kMsgCleared = '&clr';


typedef std::vector<entry_ref> RefList;


class ReadThread;


class FileMonitor : public NodeMonitorHandler {
public:
								FileMonitor(EntryViewInterface* listener);
								~FileMonitor();

			void				SetReadThread(ReadThread* readThread);

			void				Reset();

	virtual	void				MessageReceived(BMessage* message);

	virtual void				EntryCreated(const char *name, ino_t directory,
									dev_t device, ino_t node);
	virtual void				EntryRemoved(const char *name, ino_t directory,
									dev_t device, ino_t node);
	virtual void				EntryMoved(const char *name,
									const char *fromName, ino_t fromDirectory,
									ino_t toDirectory, dev_t device,
									ino_t node, dev_t nodeDevice);
	virtual void				StatChanged(ino_t node, dev_t device,
									int32 statFields);
	virtual void				AttrChanged(ino_t node, dev_t device);

private:
			WatchedFile*		_FindFile(dev_t device, ino_t node);

			EntryViewInterface*	fListener;
			WatchedFileList		fWatchedFileList;

			ReadThread*			fReadThread;
			RefList*			fCurrentReadList;
			uint32				fCurrentReadIndex;
};


class ReadThread {
public:
								ReadThread(FileMonitor* target);
	virtual						~ReadThread() {}

			status_t			Run();
			bool				Running();
			status_t			Wait();

			void				Stop();
			bool				Stopped();

			RefList*			ReadRefList();
			void				ReadDone();

protected:
	virtual	bool				ReadNextEntry(entry_ref& entry) = 0;

			int32				Process();

	friend	int32 ReadThreadFunction(void *data);

			BHandler*			fTarget;

			RefList				fRefList1;
			RefList				fRefList2;
			RefList*			fWriteRefList;
			RefList*			fReadRefList;
			bool				fReading;

private:
			void				_SwapLists();
	inline	void				_PublishEntrys(BMessenger& messenger);

			bool				fStopped;
			thread_id			fThreadId;

			int16				fNReaded;
			bool				fRunning;
};


#endif // FILE_MONITOR_H
