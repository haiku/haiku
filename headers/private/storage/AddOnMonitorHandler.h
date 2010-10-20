/*
 * Copyright 2004-2010, Haiku, Inc. All rights reserved.
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
	node_ref addon_nref;
};


class AddOnMonitorHandler : public NodeMonitorHandler {
public:
								AddOnMonitorHandler(const char* name = NULL);
	virtual						~AddOnMonitorHandler();

	virtual	void				MessageReceived(BMessage* message);

	// Supply the add-on directories here, in the order you want them checked.
	// Add-ons in directories added earlier will shadow add-ons in directories
	// added later, if they share the same file name. If an add-on is removed
	// from or renamed in a directory and it has previously shadowed another
	// add-on, the previously shadowed add-on shall become enabled
	// (AddOnEnabled()). If an add-on appears in a directory, or is renamed,
	// it can cause another add-on to become disabled, if it has the same name.
	// Note that directories are not watched recursively, and all entries
	// are reported as add-ons regardless of their node type (files,
	// directories, symlinks).
	// If sync is true all pending add-on entries are handled immediately.
	// Including entries from other directories.
	virtual	status_t			AddDirectory(const node_ref* nref,
									bool sync = false);

protected:
	// hooks for sub-class
	virtual	void				AddOnCreated(
									const add_on_entry_info* entryInfo);
	virtual	void				AddOnEnabled(
									const add_on_entry_info* entryInfo);
	virtual	void				AddOnDisabled(
									const add_on_entry_info* entryInfo);
									// name field will be invalid!
	virtual	void				AddOnRemoved(
									const add_on_entry_info* entryInfo);
									// name field will be invalid!

protected:
	virtual	void				EntryCreated(const char* name, ino_t directory,
									dev_t device, ino_t node);
	virtual	void				EntryRemoved(const char *name, ino_t directory,
									dev_t device, ino_t node);
	virtual	void				EntryMoved(const char *name,
									const char *fromName, ino_t fromDirectory,
									ino_t toDirectory, dev_t device,
									ino_t node, dev_t nodeDevice);
	virtual	void				StatChanged(ino_t node, dev_t device,
									int32 statFields);

private:
			void				_HandlePendingEntries();
			void				_EntryCreated(add_on_entry_info& info);

			typedef NodeMonitorHandler inherited;
			typedef std::list<add_on_entry_info> EntryList;

			struct add_on_directory_info {
				node_ref		nref;
				EntryList		entries;
			};

			typedef std::list<add_on_directory_info> DirectoryList;

			bool				_FindEntry(const node_ref& entry,
									const EntryList& list,
									EntryList::iterator& it) const;
			bool				_FindEntry(const char* name,
									const EntryList& list,
									EntryList::iterator& it) const;

			bool				_HasEntry(const node_ref& entry,
									EntryList& list) const;
			bool				_HasEntry(const char* name,
									EntryList& list) const;

			bool				_FindDirectory(ino_t directory, dev_t device,
									DirectoryList::iterator& it) const;
			bool				_FindDirectory(
									const node_ref& directoryNodeRef,
									DirectoryList::iterator& it) const;
			bool				_FindDirectory(ino_t directory, dev_t device,
									DirectoryList::iterator& it,
									const DirectoryList::const_iterator& end)
										const;
			bool				_FindDirectory(
									const node_ref& directoryNodeRef,
									DirectoryList::iterator& it,
									const DirectoryList::const_iterator& end)
										const;

			void				_AddNewEntry(EntryList& list,
									add_on_entry_info& info);

private:
			DirectoryList		fDirectories;
			EntryList			fPendingEntries;
};


}; // namespace Storage
}; // namespace BPrivate


using namespace BPrivate::Storage;


#endif	// _ADD_ON_MONITOR_HANDLER_H
