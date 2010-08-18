/*
 * Copyright 2004-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Bachmann
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
}


void
AddOnMonitorHandler::MessageReceived(BMessage* msg)
{
	if (msg->what == B_PULSE)
		_HandlePulse();

	inherited::MessageReceived(msg);
}


status_t
AddOnMonitorHandler::AddDirectory(const node_ref* nref)
{
	// Keep the looper thread locked, since this method is likely to be called
	// in a thread other than the looper thread. Otherwise we may access the
	// lists concurrently with the looper thread, when node monitor notifications
	// arrive while we are still adding initial entries from the directory, or
	// (much more likely) if the looper thread handles a pulse message and wants
	// to process pending entries while we are still adding them.
	BAutolock _(Looper());

	// ignore directories added twice
	std::list<add_on_directory_info>::iterator iterator = fDirectories.begin();
	for (; iterator != fDirectories.end() ; iterator++) {
		if (iterator->nref == *nref)
			return B_OK;
	}

	BDirectory directory(nref);
	status_t status = directory.InitCheck();
	if (status != B_OK)
		return status;

	add_on_directory_info dirInfo;
	dirInfo.nref = *nref;
	fDirectories.push_back(dirInfo);

	status = watch_node(nref, B_WATCH_DIRECTORY, this);
	if (status != B_OK) {
		fDirectories.pop_back();
		return status;
	}

	BEntry entry;
	while (directory.GetNextEntry(&entry, true) == B_OK) {
		add_on_entry_info entryInfo;
		if (entry.GetName(entryInfo.name) != B_OK
			|| entry.GetNodeRef(&entryInfo.nref) != B_OK) {
			continue;
		}

		entryInfo.dir_nref = *nref;
		fPendingEntries.push_back(entryInfo);
	}

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
	strncpy(entryInfo.name, name, sizeof(entryInfo.name));
	make_node_ref(device, node, &entryInfo.nref);
	make_node_ref(device, directory, &entryInfo.dir_nref);
	fPendingEntries.push_back(entryInfo);
}


void
AddOnMonitorHandler::EntryRemoved(ino_t directory, dev_t device, ino_t node)
{
	node_ref entryNodeRef;
	make_node_ref(device, node, &entryNodeRef);

	// Search pending entries first, which can simply be discarded
	// We might have this entry in the pending list multiple times,
	// so we search entire list through, even after finding one.
	std::list<add_on_entry_info>::iterator eiter = fPendingEntries.begin();
	while (eiter != fPendingEntries.end()) {
		if (eiter->nref == entryNodeRef) {
			eiter = fPendingEntries.erase(eiter);
		} else {
			eiter++;
		}
	}

	add_on_entry_info info;
	node_ref dirNodeRef;
	make_node_ref(device, directory, &dirNodeRef);

	// find the entry's info, and the entry's directory info
	std::list<add_on_directory_info>::iterator diter = fDirectories.begin();
	for (; diter != fDirectories.end() ; diter++) {
		if (diter->nref == dirNodeRef) {
			std::list<add_on_entry_info>::iterator eiter
				= diter->entries.begin();
			for (; eiter != diter->entries.end() ; eiter++) {
				info = *eiter;
				if (eiter->nref == entryNodeRef) {
					info = *eiter;
					diter->entries.erase(eiter);
					break;
				}
			}
			break;
		}
	}

	// if it was not found, we're done
	if (diter == fDirectories.end())
		return;

	// Start at the top again, and search until the directory we found
	// the old add-on in. If we find an add-on with the same name then
	// the old add-on was not enabled. So we deallocate the old
	// add-on and return.
	std::list<add_on_directory_info>::iterator diter2 = fDirectories.begin();
	for (; diter2 != diter ; diter2++) {
		std::list<add_on_entry_info>::iterator eiter = diter2->entries.begin();
		for (; eiter != diter2->entries.end() ; eiter++) {
			if (strcmp(eiter->name, info.name) == 0) {
				AddOnRemoved(&info);
				return;
			}
		}
	}

	// The active add-on was removed. We need to disable and
	// then subsequently deallocate it.
	AddOnDisabled(&info);
	AddOnRemoved(&info);

	// Continue searching for an add-on below us. If we find an add-on
	// with the same name, we must enable it.
	for (diter++ ; diter != fDirectories.end() ; diter++) {
		std::list<add_on_entry_info>::iterator eiter = diter->entries.begin();
		for (; eiter != diter->entries.end() ; eiter++) {
			if (strcmp(eiter->name, info.name) == 0) {
				AddOnEnabled(&*eiter);
				return;
			}
		}
	}
}


void
AddOnMonitorHandler::EntryMoved(const char* name, ino_t fromDirectory,
	ino_t toDirectory, dev_t device, ino_t node)
{
	// Search the "from" directory in the known entries
	node_ref fromNodeRef;
	make_node_ref(device, fromDirectory, &fromNodeRef);
	std::list<add_on_directory_info>::iterator from_iter = fDirectories.begin();
	for (; from_iter != fDirectories.end(); from_iter++) {
		if (from_iter->nref == fromNodeRef)
			break;
	}

	// Search the "to" directory in the known entries
	node_ref toNodeRef;
	make_node_ref(device, toDirectory, &toNodeRef);
	std::list<add_on_directory_info>::iterator to_iter = fDirectories.begin();
	for ( ; to_iter != fDirectories.end() ; to_iter++) {
		if (to_iter->nref == toNodeRef)
			break;
	}

	if (from_iter == fDirectories.end() && to_iter == fDirectories.end()) {
		// It seems the notification was for a directory we are not
		// actually watching by intention.
		return;
	}

	add_on_entry_info info;
	node_ref entryNodeRef;
	make_node_ref(device, node, &entryNodeRef);

	if (to_iter == fDirectories.end()) {
		// moved out of our view
		std::list<add_on_entry_info>::iterator eiter
			= from_iter->entries.begin();
		for (; eiter != from_iter->entries.end() ; eiter++) {
			if (entryNodeRef == eiter->nref) {
				// save the info and remove the entry
				info = *eiter;
				from_iter->entries.erase(eiter);
				break;
			}
		}

		if (eiter == from_iter->entries.end()) {
			// we don't know anything about this entry yet.. ignore it
			return;
		}

		// if the name is the same, save the information about this
		// add-on into fFormerEntries for later use
		if (strcmp(info.name, name) == 0)
			fFormerEntries.push_back(info);

		// Start at the top again, and search until the from directory.
		// If we find a add-on with the same name then the moved add-on
		// was not enabled.  So we are done.
		std::list<add_on_directory_info>::iterator diter = fDirectories.begin();
		for (; diter != from_iter ; diter++) {
			std::list<add_on_entry_info>::iterator eiter2
				= diter->entries.begin();
			for (; eiter2 != diter->entries.end() ; eiter2++) {
				if (strcmp(eiter2->name, info.name) == 0) {
					return;
				}
			}
		}

		// finally disable the add-on
		AddOnDisabled(&info);

		// Continue searching for a add-on below us.  If we find a add-on
		// with the same name, we must enable it.
		for (from_iter++ ; from_iter != fDirectories.end() ; from_iter++) {
			std::list<add_on_entry_info>::iterator eiter2
				= from_iter->entries.begin();
			for (; eiter2 != from_iter->entries.end() ; eiter2++) {
				if (strcmp(eiter2->name, info.name) == 0) {
					AddOnEnabled(&*eiter2);
					return;
				}
			}
		}

		// finally, if the new name is different, destroy the addon
		if (strcmp(info.name, name) != 0)
			AddOnRemoved(&info);

		// done
		return;
	}

	if (from_iter == fDirectories.end()) {
		// moved into our view
		std::list<add_on_entry_info>::iterator eiter = fFormerEntries.begin();
		for (; eiter != fFormerEntries.end() ; eiter++) {
			if (entryNodeRef == eiter->nref) {
				// save the info and remove the entry
				info = *eiter;
				fFormerEntries.erase(eiter);
				break;
			}
		}

		if (eiter != fFormerEntries.end()) {
			if (strcmp(info.name, name) != 0) {
				// name changed on the way in, remove the old one
				AddOnRemoved(&info);
			}
		}

		// update the info
		strncpy(info.name, name, sizeof(info.name));
		info.nref = entryNodeRef;
		info.dir_nref = toNodeRef;

		if (eiter == fFormerEntries.end()) {
			// this add-on was not seen before
			AddOnCreated(&info);
		}

		// Start at the top again, and search until the to directory.
		// If we find an add-on with the same name then the moved add-on
		// is not to be enabled. So we are done.
		std::list<add_on_directory_info>::iterator diter = fDirectories.begin();
		for (; diter != to_iter ; diter++) {
			std::list<add_on_entry_info>::iterator eiter2 = diter->entries.begin();
			for (; eiter2 != diter->entries.end() ; eiter2++) {
				if (strcmp(eiter2->name, info.name) == 0)
					return;
			}
		}

		// The new add-on should be enabled, but first we check to see
		// if there is an add-on below us. If we find one, we disable it.
		bool shadowing = false;
		for (diter++ ; diter != fDirectories.end() ; diter++) {
			std::list<add_on_entry_info>::iterator eiter2 = diter->entries.begin();
			for (; eiter2 != diter->entries.end() ; eiter2++) {
				if (strcmp(eiter2->name, info.name) == 0) {
					AddOnDisabled(&*eiter2);
					shadowing = true;
					break;
				}
			}
			if (shadowing)
				break;
		}

		// enable the new add-on
		AddOnEnabled(&info);

		// put the new entry into the target directory
		to_iter->entries.push_back(info);

		// done
		return;
	}

	std::list<add_on_entry_info>::iterator eiter = from_iter->entries.begin();
	for (; eiter != from_iter->entries.end() ; eiter++) {
		if (entryNodeRef == eiter->nref) {
			// save the old info and remove the entry
			info = *eiter;
			from_iter->entries.erase(eiter);
			break;
		}
	}

	if (strcmp(info.name, name) == 0) {
		// same name moved in heirarchy
		debugger("add-on moved inside the heirarchy");
	} else {
		// check to see if it was formerly enabled
		bool was_enabled = true;
		std::list<add_on_directory_info>::iterator old_iter
			= fDirectories.begin();
		for (; old_iter != from_iter ; old_iter++) {
			std::list<add_on_entry_info>::iterator eiter2
				= old_iter->entries.begin();
			for (; eiter2 != old_iter->entries.end() ; eiter2++) {
				if (strcmp(eiter2->name, info.name) == 0) {
					was_enabled = false;
					break;
				}
			}
			if (!was_enabled) {
				break;
			}
		}

		// if it was enabled, disable it and enable the one under us, if it
		// exists
		if (was_enabled) {
			AddOnDisabled(&info);
			bool done = false;
			for (; old_iter != fDirectories.end() ; old_iter++) {
				std::list<add_on_entry_info>::iterator eiter2
					= old_iter->entries.begin();
				for (; eiter2 != old_iter->entries.end() ; eiter2++) {
					if (strcmp(eiter2->name, info.name) == 0) {
						AddOnEnabled(&*eiter2);
						done = true;
						break;
					}
				}
				if (done)
					break;
			}
		}

		// kaboom!
		AddOnRemoved(&info);

		// set up new addon info
		strncpy(info.name, name, sizeof(info.name));
		info.dir_nref = toNodeRef;

		// presto!
		AddOnCreated(&info);

		// check to see if we are newly enabled
		bool isEnabled = true;
		std::list<add_on_directory_info>::iterator new_iter
			= fDirectories.begin();
		for (; new_iter != to_iter ; new_iter++) {
			std::list<add_on_entry_info>::iterator eiter2
				= new_iter->entries.begin();
			for (; eiter2 != new_iter->entries.end() ; eiter2++) {
				if (strcmp(eiter2->name, info.name) == 0) {
					isEnabled = false;
					break;
				}
			}
			if (!isEnabled)
				break;
		}

		// if it is newly enabled, check under us for an enabled one, and
		// disable that first
		if (isEnabled) {
			bool done = false;
			for (; new_iter != fDirectories.end() ; new_iter++) {
				std::list<add_on_entry_info>::iterator eiter2
					= new_iter->entries.begin();
				for (; eiter2 != new_iter->entries.end() ; eiter2++) {
					if (strcmp(eiter2->name, info.name) == 0) {
						AddOnDisabled(&*eiter2);
						done = true;
						break;
					}
				}
				if (done)
					break;
			}
			AddOnEnabled(&info);
		}
	}

	// put the new entry into the target directory
	to_iter->entries.push_back(info);
}


//!	Process pending entries.
void
AddOnMonitorHandler::_HandlePulse()
{
	BDirectory directory;
	std::list<add_on_entry_info>::iterator iter = fPendingEntries.begin();
	while (iter != fPendingEntries.end()) {
		add_on_entry_info info = *iter;

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

		// put the new entry into the directory info
		std::list<add_on_directory_info>::iterator diter = fDirectories.begin();
		for (; diter != fDirectories.end() ; diter++) {
			if (diter->nref == dirNodeRef) {
				diter->entries.push_back(info);
				break;
			}
		}

		// report it
		AddOnCreated(&info);

		// Start at the top again, and search until the directory we put
		// the new add-on in.  If we find an add-on with the same name then
		// the new add-on should not be enabled.
		bool enabled = true;
		std::list<add_on_directory_info>::iterator diter2
			= fDirectories.begin();
		for (; diter2 != diter ; diter2++) {
			std::list<add_on_entry_info>::iterator eiter
				= diter2->entries.begin();
			for (; eiter != diter2->entries.end() ; eiter++) {
				if (strcmp(eiter->name, info.name) == 0) {
					enabled = false;
					break;
				}
			}
			if (!enabled)
				break;
		}
		if (!enabled) {
			// if we are not enabled, go on to the next pending entry
			continue;
		}

		// The new add-on should be enabled, but first we check to see
		// if there is an add-on below us.  If we find one, we disable it.
		bool shadowing = false;
		for (diter++ ; diter != fDirectories.end() ; diter++) {
			std::list<add_on_entry_info>::iterator eiter = diter->entries.begin();
			for (; eiter != diter->entries.end() ; eiter++) {
				if (strcmp(eiter->name, info.name) == 0) {
					AddOnDisabled(&*eiter);
					shadowing = true;
					break;
				}
			}
			if (shadowing)
				break;
		}

		// finally, enable the new entry
		AddOnEnabled(&info);
	}
}
