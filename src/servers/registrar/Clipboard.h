// Clipboard.h

#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include <String.h>
#include <Message.h>
#include <Messenger.h>
#include "WatchingService.h"

class Clipboard {
public:
	Clipboard(const char *name);
	~Clipboard();

	void SetData(const BMessage *data, BMessenger dataSource);

	const BMessage *Data() const;
	BMessenger DataSource() const;
	int32 Count() const;

	bool AddWatcher(BMessenger watcher);
	bool RemoveWatcher(BMessenger watcher);
	void NotifyWatchers();

private:
	BString			fName;
	BMessage		fData;
	BMessenger		fDataSource;
	int32			fCount;
	WatchingService	fWatchingService;
};

#endif	// CLIPBOARD_H
