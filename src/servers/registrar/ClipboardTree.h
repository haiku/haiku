// ClipboardTree.h

#ifndef CLIPBOARD_TREE_H
#define CLIPBOARD_TREE_H

#include <String.h>
#include <Message.h>
#include <Messenger.h>
#include "WatchingService.h"

class ClipboardTree {
public:
	ClipboardTree();
	~ClipboardTree();
        void AddNode(BString name);
        ClipboardTree* GetNode(BString name);
	uint32 GetCount();
	uint32 IncrementCount();
	BMessage *GetData();
	void SetData(BMessage *data);
	BMessenger *GetDataSource();
	void SetDataSource(BMessenger *dataSource);
	bool AddWatcher(BMessenger *watcher);
	bool RemoveWatcher(BMessenger *watcher);
	void NotifyWatchers();
private:
	BString fName;
	BMessage fData;
	BMessenger fDataSource;
	ClipboardTree *fLeftChild;
	ClipboardTree *fRightChild;
	uint32 fCount;
	WatchingService fWatchingService;
};

#endif	// CLIPBOARD_TREE_H

