/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2025, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef FILE_WATCHER_H
#define FILE_WATCHER_H


#include <Handler.h>
#include <ObjectList.h>


class FileListener {
public:
	virtual						~FileListener();

	virtual	void				FileCreated(const char* path) = 0;
};


class FileWatcher : public BHandler {
public:
								FileWatcher();
	virtual						~FileWatcher();

			void				AddListener(FileListener* listener);
			void				RemoveListener(FileListener* listener);
			int32				CountListeners() const;

	virtual	void				MessageReceived(BMessage* message);

	static	status_t			Register(FileListener* listener, BPath& path);
	static	void				Unregister(FileListener* listener, BPath& path);

protected:
			BObjectList<FileListener>
								fListeners;
};


#endif // FILE_WATCHER_H
