/*
 * Copyright 2004-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Bachmann
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "AddOnMonitorHandler.h"

#include <string.h>

#include <Autolock.h>
#include <Directory.h>


#ifndef ADD_ON_STABLE_SECONDS
#	define ADD_ON_STABLE_SECONDS 1
#endif


AddOnMonitorHandler::AddOnMonitorHandler(const char* name)
	:
	NodeMonitorHandler(name != NULL ? name : "AddOnMonitorHandler")
{
}


AddOnMonitorHandler::~AddOnMonitorHandler()
{
	// TODO: Actually calling watch_node() here should be too late, since we
	// are likely not attached to a looper anymore, and thus consitute no valid
	// BMessenger at this time.
	DirectoryList::iterator it = fDirectories.begin();
	for (; it != fDirectories.end(); it++) {
		EntryList::iterator eiter = it->entries.begin();
		for (; eiter != it->entries.end(); eiter++)
			watch_node(&eiter->addon_nref, B_STOP_WATCHING, this);
		watch_node(&it->nref, B_STOP_WATCHING, this);
	}
}


void
AddOnMonitorHandler::MessageReceived(BMessage* msg)
{
	if (msg->what == B_PULSE)
		_HandlePendingEntries();

	inherited::MessageReceived(msg);
}


status_t
AddOnMonitorHandler::AddDirectory(const node_ref* nref, bool sync)
{
	// Keep the looper thread locked, since this method is likely to be called
	// in a thread other than the looper thread. Otherwise we may access the
	// lists concurrently with the looper thread, when node monitor
	// notifications arrive while we are still adding initial entries from the
	// directory, or (much more likely) if the looper thread handles a pulse
	// message and wants to process pending entries while we are still adding
	// them.
	BAutolock _(Looper());

	// ignore directories added twice
	DirectoryList::iterator it = fDirectories.begin();
	for (; it != fDirectories.end(); it++) {
		if (it->nref == *nref)
			return B_OK;
	}

	BDirectory directory(nref);
	status_t status = directory.InitCheck();
	if (status != B_OK)
		return status;

	status = watch_node(nref, B_WATCH_DIRECTORY, this);
	if (status != B_OK)
		return status;

	add_on_directory_info dirInfo;
	dirInfo.nref = *nref;
	fDirectories.push_back(dirInfo);

	add_on_entry_info entryInfo;
	entryInfo.dir_nref = *nref;

	BEntry entry;
	while (directory.GetNextEntry(&entry) == B_OK) {
		if (entry.GetName(entryInfo.name) != B_OK
			|| entry.GetNodeRef(&entryInfo.nref) != B_OK) {
			continue;
		}

		fPendingEntries.push_back(entryInfo);
	}

	if (sync)
		_HandlePendingEntries();

	return B_OK;
}


//	#pragma mark - AddOnMonitorHandler hooks


void
AddOnMonitorHandler::AddOnCreated(const add_on_entry_info* entryInfo)
{

}


void
AddOnMonitorHandler::AddOnEnabled(const add_on_entry_info* entryInfo)
{
}


void
AddOnMonitorHandler::AddOnDisabled(const add_on_entry_info* entryInfo)
{
}


void
AddOnMonitorHandler::AddOnRemoved(const add_on_entry_info* entryInfo)
{
}


//	#pragma mark - NodeMonitorHandler hooks


void
AddOnMonitorHandler::EntryCreated(const char* name, ino_t directory,
	dev_t device, ino_t node)
{
	add_on_entry_info entryInfo;
	strlcpy(entryInfo.name, name, sizeof(entryInfo.name));
	make_node_ref(device, node, &entryInfo.nref);
	make_node_ref(device, directory, &entryInfo.dir_nref);
	fPendingEntries.push_back(entryInfo);
}


void
AddOnMonitorHandler::EntryRemoved(const char* name, ino_t directory,
	dev_t device, ino_t node)
{
	node_ref entryNodeRef;
	make_node_ref(device, node, &entryNodeRef);

	// Search pending entries first, which can simply be discarded
	// We might have this entry in the pending list multiple times,
	// so we search entire list through, even after finding one.
	EntryList::iterator eiter = fPendingEntries.begin();
	while (eiter != fPendingEntries.end()) {
		if (eiter->nref == entryNodeRef)
			eiter = fPendingEntries.erase(eiter);
		else
			eiter++;
	}

	// Find the directory of the entry.
	DirectoryList::iterator diter = fDirectories.begin();
	if (!_FindDirectory(directory, device, diter)) {
		// If it was not found, we're done
		return;
	}

	eiter = diter->entries.begin();
	if (!_FindEntry(entryNodeRef, diter->entries, eiter)) {
		// This must be the directory, but we didn't find the entry.
		return;
	}

	add_on_entry_info info = *eiter;
	watch_node(&entryNodeRef, B_STOP_WATCHING, this);
	diter->entries.erase(eiter);

	// Start at the top again, and search until the directory we found
	// the old add-on in. If we find an add-on with the same name then
	// the old add-on was not enabled. So we deallocate the old add-on and
	// return.
	DirectoryList::iterator diter2 = fDirectories.begin();
	for (; diter2 != diter; diter2++) {
		if (_HasEntry(info.name, diter2->entries)) {
			AddOnRemoved(&info);
			return;
		}
	}

	// An active add-on was removed. We need to disable and then subsequently
	// deallocate it.
	AddOnDisabled(&info);
	AddOnRemoved(&info);

	// Continue searching for an add-on below us. If we find an add-on
	// with the same name, we must enable it.
	for (diter++; diter != fDirectories.end(); diter++) {
		eiter = diter->entries.begin();
		if (_FindEntry(info.name, diter->entries, eiter)) {
			AddOnEnabled(&*eiter);
			break;
		}
	}
}


void
AddOnMonitorHandler::EntryMoved(const char* name, const char* fromName,
	ino_t fromDirectory, ino_t toDirectory, dev_t device, ino_t node,
	dev_t nodeDevice)
{
	node_ref toNodeRef;
	make_node_ref(device, toDirectory, &toNodeRef);

	// Search the "from" and "to" directory in the known directories
	DirectoryList::iterator fromIter = fDirectories.begin();
	bool watchingFromDirectory = _FindDirectory(fromDirectory, device,
		fromIter);

	DirectoryList::iterator toIter = fDirectories.begin();
	bool watchingToDirectory = _FindDirectory(toNodeRef, toIter);

	if (!watchingFromDirectory && !watchingToDirectory) {
		// It seems the notification was for a directory we are not
		// actually watching (at least not by intention).
		return;
	}

	add_on_entry_info info;

	node_ref entryNodeRef;
	make_node_ref(device, node, &entryNodeRef);

	if (!watchingToDirectory) {
		// moved out of our view
		EntryList::iterator eiter = fromIter->entries.begin();
		if (!_FindEntry(entryNodeRef, fromIter->entries, eiter)) {
			// we don't know anything about this entry yet.. ignore it
			return;
		}

		// save the info and remove the entry
		info = *eiter;
		watch_node(&entryNodeRef, B_STOP_WATCHING, this);
		fromIter->entries.erase(eiter);

		// Start at the top again, and search until the from directory.
		// If we find a add-on with the same name then the moved add-on
		// was not enabled.  So we are done.
		DirectoryList::iterator diter = fDirectories.begin();
		for (; diter != fromIter; diter++) {
			eiter = diter->entries.begin();
			if (_FindEntry(info.name, diter->entries, eiter))
				return;
		}

		// finally disable the add-on
		AddOnDisabled(&info);

		// Continue searching for a add-on below us.  If we find a add-on
		// with the same name, we must enable it.
		for (fromIter++; fromIter != fDirectories.end(); fromIter++) {
			eiter = fromIter->entries.begin();
			if (_FindEntry(info.name, fromIter->entries, eiter)) {
				AddOnEnabled(&*eiter);
				return;
			}
		}

		// finally destroy the addon
		AddOnRemoved(&info);

		// done
		return;
	}

	if (!watchingFromDirectory) {
		// moved into our view

		// update the info
		strlcpy(info.name, name, sizeof(info.name));
		info.nref = entryNodeRef;
		info.dir_nref = toNodeRef;

		AddOnCreated(&info);

		// Start at the top again, and search until the to directory.
		// If we find an add-on with the same name then the moved add-on
		// is not to be enabled. So we are done.
		DirectoryList::iterator diter = fDirectories.begin();
		for (; diter != toIter; diter++) {
			if (_HasEntry(info.name, diter->entries)) {
				// The new add-on is being shadowed.
				return;
			}
		}

		// The new add-on should be enabled, but first we check to see
		// if there is an add-on below us. If we find one, we disable it.
		for (diter++ ; diter != fDirectories.end(); diter++) {
			EntryList::iterator eiter = diter->entries.begin();
			if (_FindEntry(info.name, diter->entries, eiter)) {
				AddOnDisabled(&*eiter);
				break;
			}
		}

		// enable the new add-on
		AddOnEnabled(&info);

		// put the new entry into the target directory
		_AddNewEntry(toIter->entries, info);

		// done
		return;
	}

	// The add-on was renamed, or moved within our hierarchy.

	EntryList::iterator eiter = fromIter->entries.begin();
	if (_FindEntry(entryNodeRef, fromIter->entries, eiter)) {
		// save the old info and remove the entry
		info = *eiter;
	} else {
		// If an entry moved from one watched directory into another watched
		// directory, there will be two notifications, and this may be the
		// second. We have handled everything in the first. In that case the
		// entry was already removed from the fromDirectory and added in the
		// toDirectory list.
		return;
	}

	if (strcmp(info.name, name) == 0) {
		// It should be impossible for the name to stay the same, unless the
		// node moved in the watched hierarchy. Handle this case by removing
		// the entry and readding it. TODO: This can temporarily enable add-ons
		// which should in fact stay hidden (moving add-on from home to common
		// folder or vice versa, the system add-on should remain hidden).
		EntryRemoved(name, fromDirectory, device, node);
		info.dir_nref = toNodeRef;
		_EntryCreated(info);
	} else {
		// Erase the entry
		fromIter->entries.erase(eiter);

		// check to see if it was formerly enabled
		bool wasEnabled = true;
		DirectoryList::iterator oldIter = fDirectories.begin();
		for (; oldIter != fromIter; oldIter++) {
			if (_HasEntry(info.name, oldIter->entries)) {
				wasEnabled = false;
				break;
			}
		}

		// If it was enabled, disable it and enable the one under us, if it
		// exists.
		if (wasEnabled) {
			AddOnDisabled(&info);
			for (; oldIter != fDirectories.end(); oldIter++) {
				eiter = oldIter->entries.begin();
				if (_FindEntry(info.name, oldIter->entries, eiter)) {
					AddOnEnabled(&*eiter);
					break;
				}
			}
		}

		// kaboom!
		AddOnRemoved(&info);

		// set up new addon info
		strlcpy(info.name, name, sizeof(info.name));
		info.dir_nref = toNodeRef;

		// presto!
		AddOnCreated(&info);

		// check to see if we are newly enabled
		bool isEnabled = true;
		DirectoryList::iterator newIter = fDirectories.begin();
		for (; newIter != toIter; newIter++) {
			if (_HasEntry(info.name, newIter->entries)) {
				isEnabled = false;
				break;
			}
		}

		// if it is newly enabled, check under us for an enabled one, and
		// disable that first
		if (isEnabled) {
			for (; newIter != fDirectories.end(); newIter++) {
				eiter = newIter->entries.begin();
				if (_FindEntry(info.name, newIter->entries, eiter)) {
					AddOnDisabled(&*eiter);
					break;
				}
			}
			AddOnEnabled(&info);
		}
		// put the new entry into the target directory
		toIter->entries.push_back(info);
	}
}


void
AddOnMonitorHandler::StatChanged(ino_t node, dev_t device, int32 statFields)
{
	// This notification is received for the add-ons themselves.

	// TODO: Add the entry to the pending list, disable/enable it
	// when the modification time remains stable.

	node_ref entryNodeRef;
	make_node_ref(device, node, &entryNodeRef);

	DirectoryList::iterator diter = fDirectories.begin();
	for (; diter != fDirectories.end(); diter++) {
		EntryList::iterator eiter = diter->entries.begin();
		for (; eiter != diter->entries.end(); eiter++) {
			if (eiter->addon_nref == entryNodeRef) {
				// Trigger reloading of the add-on
				const add_on_entry_info* info = &*eiter;
				AddOnDisabled(info);
				AddOnRemoved(info);
				AddOnCreated(info);
				AddOnEnabled(info);
				return;
			}
		}
	}
}


// #pragma mark - private


//!	Process pending entries.
void
AddOnMonitorHandler::_HandlePendingEntries()
{
	BDirectory directory;
	EntryList::iterator iter = fPendingEntries.begin();
	while (iter != fPendingEntries.end()) {
		add_on_entry_info info = *iter;

		// Initialize directory, or re-use from previous iteration, if
		// directory node_ref remained the same from the last pending entry.
		node_ref dirNodeRef;
		if (directory.GetNodeRef(&dirNodeRef) != B_OK
			|| dirNodeRef != info.dir_nref) {
			if (directory.SetTo(&info.dir_nref) != B_OK) {
				// invalid directory, discard this pending entry
				iter = fPendingEntries.erase(iter);
				continue;
			}
			dirNodeRef = info.dir_nref;
		}

		struct stat st;
		if (directory.GetStatFor(info.name, &st) != B_OK) {
			// invalid file name, discard this pending entry
			iter = fPendingEntries.erase(iter);
			continue;
		}

		// stat units are seconds, real_time_clock units are seconds
		if (real_time_clock() - st.st_mtime < ADD_ON_STABLE_SECONDS) {
			// entry not stable, skip the entry for this pulse
			iter++;
			continue;
		}

		// we are going to deal with the stable entry, so remove it
		iter = fPendingEntries.erase(iter);

		_EntryCreated(info);
	}
}


void
AddOnMonitorHandler::_EntryCreated(add_on_entry_info& info)
{
	// put the new entry into the directory info
	DirectoryList::iterator diter = fDirectories.begin();
	for (; diter != fDirectories.end(); diter++) {
		if (diter->nref == info.dir_nref) {
			_AddNewEntry(diter->entries, info);
			break;
		}
	}

	// report it
	AddOnCreated(&info);

	// Start at the top again, and search until the directory we put
	// the new add-on in.  If we find an add-on with the same name then
	// the new add-on should not be enabled.
	bool enabled = true;
	DirectoryList::iterator diter2 = fDirectories.begin();
	for (; diter2 != diter; diter2++) {
		if (_HasEntry(info.name, diter2->entries)) {
			enabled = false;
			break;
		}
	}
	if (!enabled)
		return;

	// The new add-on should be enabled, but first we check to see
	// if there is an add-on shadowed by the new one.  If we find one,
	// we disable it.
	for (diter++ ; diter != fDirectories.end(); diter++) {
		EntryList::iterator eiter = diter->entries.begin();
		if (_FindEntry(info.name, diter->entries, eiter)) {
			AddOnDisabled(&*eiter);
			break;
		}
	}

	// enable the new entry
	AddOnEnabled(&info);
}


bool
AddOnMonitorHandler::_FindEntry(const node_ref& entry, const EntryList& list,
	EntryList::iterator& it) const
{
	for (; EntryList::const_iterator(it) != list.end(); it++) {
		if (it->nref == entry)
			return true;
	}
	return false;
}


bool
AddOnMonitorHandler::_FindEntry(const char* name, const EntryList& list,
	EntryList::iterator& it) const
{
	for (; EntryList::const_iterator(it) != list.end(); it++) {
		if (strcmp(it->name, name) == 0)
			return true;
	}
	return false;
}


bool
AddOnMonitorHandler::_HasEntry(const node_ref& entry, EntryList& list) const
{
	EntryList::iterator it = list.begin();
	return _FindEntry(entry, list, it);
}


bool
AddOnMonitorHandler::_HasEntry(const char* name, EntryList& list) const
{
	EntryList::iterator it = list.begin();
	return _FindEntry(name, list, it);
}


bool
AddOnMonitorHandler::_FindDirectory(ino_t directory, dev_t device,
	DirectoryList::iterator& it) const
{
	node_ref nodeRef;
	make_node_ref(device, directory, &nodeRef);
	return _FindDirectory(nodeRef, it, fDirectories.end());
}


bool
AddOnMonitorHandler::_FindDirectory(const node_ref& directoryNodeRef,
	DirectoryList::iterator& it) const
{
	return _FindDirectory(directoryNodeRef, it, fDirectories.end());
}


bool
AddOnMonitorHandler::_FindDirectory(ino_t directory, dev_t device,
	DirectoryList::iterator& it,
	const DirectoryList::const_iterator& end) const
{
	node_ref nodeRef;
	make_node_ref(device, directory, &nodeRef);
	return _FindDirectory(nodeRef, it, end);
}


bool
AddOnMonitorHandler::_FindDirectory(const node_ref& directoryNodeRef,
	DirectoryList::iterator& it,
	const DirectoryList::const_iterator& end) const
{
	for (; DirectoryList::const_iterator(it) != end; it++) {
		if (it->nref == directoryNodeRef)
			return true;
	}
	return false;
}


void
AddOnMonitorHandler::_AddNewEntry(EntryList& list, add_on_entry_info& info)
{
	BDirectory directory(&info.dir_nref);
	BEntry entry(&directory, info.name, true);

	node_ref addOnRef;	
	if (entry.GetNodeRef(&addOnRef) == B_OK) {
		watch_node(&addOnRef, B_WATCH_STAT, this);
		info.addon_nref = addOnRef;
	} else
		info.addon_nref = info.nref;

	list.push_back(info);
}
