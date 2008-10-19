/*
 * Copyright 2004-2008, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ADD_ON_MONITOR_HANDLER_H
#define _ADD_ON_MONITOR_HANDLER_H

#include <list>

#include "NodeMonitorHandler.h"


namespace BPrivate {
namespace Storage {

struct add_on_entry_info {
	char name[B_FILE_NAME_LENGTH];
	node_ref nref;
	node_ref dir_nref;
};

struct add_on_directory_info {
	node_ref nref;
	std::list<add_on_entry_info> entries;
};

class AddOnMonitorHandler : public NodeMonitorHandler {
public:
						AddOnMonitorHandler(const char* name = NULL);
	virtual				~AddOnMonitorHandler();

	virtual void		MessageReceived(BMessage* msg);

	// supply the add-on directories here, in the order you want them checked
	virtual status_t	AddDirectory(const node_ref* nref);

protected:
	// hooks for subclass
	virtual void		AddOnCreated(const add_on_entry_info* entryInfo);
	virtual void		AddOnEnabled(const add_on_entry_info* entryInfo);
	virtual void		AddOnDisabled(const add_on_entry_info* entryInfo);
	virtual void		AddOnRemoved(const add_on_entry_info* entryInfo);

protected:
	virtual void		EntryCreated(const char* name, ino_t directory,
							dev_t device, ino_t node);
	virtual void		EntryRemoved(ino_t directory, dev_t device, ino_t node);
	virtual void		EntryMoved(const char* name, ino_t fromDirectory,
							ino_t toDirectory, dev_t device, ino_t node);

private:
	typedef NodeMonitorHandler inherited;

			void		_HandlePulse();

	std::list<add_on_directory_info> fDirectories;
	std::list<add_on_entry_info> fPendingEntries;
	std::list<add_on_entry_info> fFormerEntries;
};


}; // namespace Storage
}; // namespace BPrivate

using namespace BPrivate::Storage;

#endif	// _ADD_ON_MONITOR_HANDLER_H
