#include "AddOnMonitorHandler.h"
#include <Directory.h>

#define ADD_ON_STABLE_SECONDS 15

/*
 * public functions
 */

AddOnMonitorHandler::AddOnMonitorHandler(const char * name)
	: NodeMonitorHandler(name)
{
}


AddOnMonitorHandler::~AddOnMonitorHandler()
{
}


/* virtual */ void
AddOnMonitorHandler::MessageReceived(BMessage * msg)
{
	if (msg->what == B_PULSE) {
		HandlePulse();
	}
	inherited::MessageReceived(msg);
}


/* virtual */ status_t
AddOnMonitorHandler::AddDirectory(const node_ref * nref)
{
	BDirectory directory(nref);
	status_t status = directory.InitCheck();
	if (status != B_OK) {
		return status;
	}

	add_on_directory_info dir_info;
	dir_info.nref = *nref;
	directories.push_back(dir_info);

	status = watch_node(nref, B_WATCH_DIRECTORY, this);
	if (status != B_OK) {
		directories.pop_back();
		return status;
	}

	BEntry entry;
	while (directory.GetNextEntry(&entry, true) == B_OK) {
		add_on_entry_info entry_info;
		if (entry.GetName(entry_info.name) != B_OK) {
			continue; // discard and proceed
		}
		if (entry.GetNodeRef(&entry_info.nref) != B_OK) {
			continue; // discard and proceed
		}
		entry_info.dir_nref = *nref;
		pending_entries.push_back(entry_info);
	}

	return B_OK;
}


/*
 * AddOnMonitorHandler hooks
 */

/* virtual */ void
AddOnMonitorHandler::AddOnCreated(const add_on_entry_info * entry_info)
{

}


/* virtual */ void
AddOnMonitorHandler::AddOnEnabled(const add_on_entry_info * entry_info)
{

}


/* virtual */ void
AddOnMonitorHandler::AddOnDisabled(const add_on_entry_info * entry_info)
{

}


/* virtual */ void
AddOnMonitorHandler::AddOnRemoved(const add_on_entry_info * entry_info)
{

}


/*
 * NodeMonitorHandler hooks
 */


/* virtual */ void
AddOnMonitorHandler::EntryCreated(const char *name, ino_t directory,
                           dev_t device, ino_t node)
{
	add_on_entry_info entry_info;
	strncpy(entry_info.name, name, sizeof(entry_info.name));
	make_node_ref(device, node, &entry_info.nref);
	make_node_ref(device, directory, &entry_info.dir_nref);
	pending_entries.push_back(entry_info);
}


/* virtual */ void
AddOnMonitorHandler::EntryRemoved(ino_t directory, dev_t device, ino_t node)
{
	node_ref entry_nref;
	make_node_ref(device, node, &entry_nref);

	// Search pending entries first, which can simply be discarded
	// We might have this entry in the pending list multiple times,
	// so we search entire list through, even after finding one.
	std::list<add_on_entry_info>::iterator eiter = pending_entries.begin();
	while (eiter != pending_entries.end()) {
		if (eiter->nref == entry_nref) {
			eiter = pending_entries.erase(eiter);
		} else {
			eiter++;
		}
	}

	add_on_entry_info info;
	node_ref dir_nref;
	make_node_ref(device, directory, &dir_nref);

	// find the entry's info, and the entry's directory info
	std::list<add_on_directory_info>::iterator diter = directories.begin();
	for( ; diter != directories.end() ; diter++) {
		if (diter->nref == dir_nref) {
			std::list<add_on_entry_info>::iterator eiter = diter->entries.begin();
			for( ; eiter != diter->entries.end() ; eiter++) {
				info = *eiter;
				if (eiter->nref == entry_nref) {
					info = *eiter;
					diter->entries.erase(eiter);
					break;
				}
			}
			break;
		}
	}

	// if it was not found, we're done
	if (diter == directories.end()) {
		return;
	}

	// Start at the top again, and search until the directory we found
	// the old add_on in.  If we find a add_on with the same name then
	// the old add_on was not enabled.  So we deallocate the old
	// add_on and return.
	std::list<add_on_directory_info>::iterator diter2 = directories.begin();
	for( ; diter2 != diter ; diter2++) {
		std::list<add_on_entry_info>::iterator eiter = diter2->entries.begin();
		for( ; eiter != diter2->entries.end() ; eiter++) {
			if (strcmp(eiter->name, info.name) == 0) {
				AddOnRemoved(&info);
				return;
			}
		}
	}

	// The active plugin was removed.  We need to disable and 
	// then subsequently deallocate it.
	AddOnDisabled(&info);
	AddOnRemoved(&info);

	// Continue searching for a add_on below us.  If we find a add_on
	// with the same name, we must enable it.
	for (diter++ ; diter != directories.end() ; diter++) {
		std::list<add_on_entry_info>::iterator eiter = diter->entries.begin();
		for( ; eiter != diter->entries.end() ; eiter++) {
			if (strcmp(eiter->name, info.name) == 0) {
				AddOnEnabled(&*eiter);
				return;
			}
		}
	}
}


/* virtual */ void
AddOnMonitorHandler::EntryMoved(const char *name, ino_t from_directory,
                         ino_t to_directory, dev_t device, ino_t node)
{
	node_ref from_nref;
	make_node_ref(device, from_directory, &from_nref);
	std::list<add_on_directory_info>::iterator from_iter = directories.begin();
	for ( ; from_iter != directories.end() ; from_iter++) {
		if (from_iter->nref == from_nref) {
			break;
		}
	}

	node_ref to_nref;
	make_node_ref(device, to_directory, &to_nref);
	std::list<add_on_directory_info>::iterator to_iter = directories.begin();
	for ( ; to_iter != directories.end() ; to_iter++) {
		if (to_iter->nref == to_nref) {
			break;
		}
	}

	if ((from_iter == directories.end()) && (to_iter == directories.end())) {
		// huh? whatever...
		return;
	}

	add_on_entry_info info;
	node_ref entry_nref;
	make_node_ref(device, node, &entry_nref);

	if (to_iter == directories.end()) {
		// moved out of our view
		std::list<add_on_entry_info>::iterator eiter = from_iter->entries.begin();
		for( ; eiter != from_iter->entries.end() ; eiter++) {
			if (entry_nref == eiter->nref) {
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
		// add_on into former_entries for later use
		if (strcmp(info.name, name) == 0) {
			former_entries.push_back(info);
		}

		// Start at the top again, and search until the from directory.
		// If we find a add_on with the same name then the moved add_on
		// was not enabled.  So we are done.
		std::list<add_on_directory_info>::iterator diter = directories.begin();
		for( ; diter != from_iter ; diter++) {
			std::list<add_on_entry_info>::iterator eiter2 = diter->entries.begin();
			for( ; eiter2 != diter->entries.end() ; eiter2++) {
				if (strcmp(eiter2->name, info.name) == 0) {
					return;
				}
			}
		}

		// finally disable the add_on
		AddOnDisabled(&info);

		// Continue searching for a add_on below us.  If we find a add_on
		// with the same name, we must enable it.
		for (from_iter++ ; from_iter != directories.end() ; from_iter++) {
			std::list<add_on_entry_info>::iterator eiter2 = from_iter->entries.begin();
			for( ; eiter2 != from_iter->entries.end() ; eiter2++) {
				if (strcmp(eiter2->name, info.name) == 0) {
					AddOnEnabled(&*eiter2);
					return;
				}
			}
		}

		// finally, if the new name is different, destroy the addon
		if (strcmp(info.name, name) != 0) {
			AddOnRemoved(&info);
		}

		// done
		return;
	}

	if (from_iter == directories.end()) {
		// moved into our view
		std::list<add_on_entry_info>::iterator eiter = former_entries.begin();
		for( ; eiter != former_entries.end() ; eiter++) {
			if (entry_nref == eiter->nref) {
				// save the info and remove the entry
				info = *eiter;
				former_entries.erase(eiter);
				break;
			}
		}

		if (eiter != former_entries.end()) {
			if (strcmp(info.name, name) != 0) {
				// name changed on the way in, remove the old one
				AddOnRemoved(&info);
			}
		}

		// update the info
		strncpy(info.name, name, sizeof(info.name));
		info.nref = entry_nref;
		info.dir_nref = to_nref;

		if (eiter == former_entries.end()) {
			// this add_on was not seen before
			AddOnCreated(&info);
		}

		// Start at the top again, and search until the to directory.
		// If we find a add_on with the same name then the moved add_on
		// is not to be enabled.  So we are done.
		std::list<add_on_directory_info>::iterator diter = directories.begin();
		for( ; diter != to_iter ; diter++) {
			std::list<add_on_entry_info>::iterator eiter2 = diter->entries.begin();
			for( ; eiter2 != diter->entries.end() ; eiter2++) {
				if (strcmp(eiter2->name, info.name) == 0) {
					return;
				}
			}
		}

		// The new add_on should be enabled, but first we check to see 
		// if there is an add_on below us.  If we find one, we disable it.
		bool shadowing = false;
		for (diter++ ; diter != directories.end() ; diter++) {
			std::list<add_on_entry_info>::iterator eiter2 = diter->entries.begin();
			for( ; eiter2 != diter->entries.end() ; eiter2++) {
				if (strcmp(eiter2->name, info.name) == 0) {
					AddOnDisabled(&*eiter2);
					shadowing = true;
					break;
				}
			}
			if (shadowing) {
				break;
			}
		}
		
		// enable the new add_on
		AddOnEnabled(&info);
		
		// put the new entry into the target directory
		to_iter->entries.push_back(info);

		// done
		return;
	}

	std::list<add_on_entry_info>::iterator eiter = from_iter->entries.begin();
	for( ; eiter != from_iter->entries.end() ; eiter++) {
		if (entry_nref == eiter->nref) {
			// save the old info and remove the entry
			info = *eiter;
			from_iter->entries.erase(eiter);
			break;
		}
	}

	if (strcmp(info.name, name) == 0) {
		// same name moved in heirarchy
		debugger("add_on moved inside the heirarchy");
	} else {
		// check to see if it was formerly enabled
		bool was_enabled = true;
		std::list<add_on_directory_info>::iterator old_iter = directories.begin();
		for( ; old_iter != from_iter ; old_iter++) {
			std::list<add_on_entry_info>::iterator eiter2 = old_iter->entries.begin();
			for( ; eiter2 != old_iter->entries.end() ; eiter2++) {
				if (strcmp(eiter2->name, info.name) == 0) {
					was_enabled = false;
					break;
				}
			}
			if (!was_enabled) {
				break;
			}
		}
		
		// if it was enabled, disable it and enable the one under us, if it exists
		if (was_enabled) {
			AddOnDisabled(&info);
			bool done = false;
			for( ; old_iter != directories.end() ; old_iter++) {
				std::list<add_on_entry_info>::iterator eiter2 = old_iter->entries.begin();
				for( ; eiter2 != old_iter->entries.end() ; eiter2++) {
					if (strcmp(eiter2->name, info.name) == 0) {
						AddOnEnabled(&*eiter2);
						done = true;
						break;
					}
				}
				if (done) {
					break;
				}
			}
		}

		// kaboom!
		AddOnRemoved(&info);

		// set up new addon info
		strncpy(info.name, name, sizeof(info.name));
		info.dir_nref = to_nref;

		// presto!
		AddOnCreated(&info);
	
		// check to see if we are newly enabled
		bool is_enabled = true;
		std::list<add_on_directory_info>::iterator new_iter = directories.begin();
		for( ; new_iter != to_iter ; new_iter++) {
			std::list<add_on_entry_info>::iterator eiter2 = new_iter->entries.begin();
			for( ; eiter2 != new_iter->entries.end() ; eiter2++) {
				if (strcmp(eiter2->name, info.name) == 0) {
					is_enabled = false;
					break;
				}
			}
			if (!is_enabled) {
				break;
			}
		}

		// if it is newly enabled, check under us for an enabled one, and disable that first
		if (is_enabled) {
			bool done = false;
			for( ; new_iter != directories.end() ; new_iter++) {
				std::list<add_on_entry_info>::iterator eiter2 = new_iter->entries.begin();
				for( ; eiter2 != new_iter->entries.end() ; eiter2++) {
					if (strcmp(eiter2->name, info.name) == 0) {
						AddOnDisabled(&*eiter2);
						done = true;
						break;
					}
				}
				if (done) {
					break;
				}
			}
			AddOnEnabled(&info);
		}
	}

	// put the new entry into the target directory
	to_iter->entries.push_back(info);
}


/*
 * process pending entries
 */

void
AddOnMonitorHandler::HandlePulse()
{
	BDirectory directory;
	std::list<add_on_entry_info>::iterator iter = pending_entries.begin();
	while (iter != pending_entries.end()) {
		add_on_entry_info info = *iter;
		
		node_ref dir_nref;
		if ((directory.GetNodeRef(&dir_nref) != B_OK) ||
			(dir_nref != info.dir_nref)) {
			if (directory.SetTo(&info.dir_nref) != B_OK) {
				// invalid directory, discard this pending entry
				iter = pending_entries.erase(iter);
				continue;
			}
			dir_nref = info.dir_nref;
		}

		struct stat st;
		if (directory.GetStatFor(info.name, &st) != B_OK) {
			// invalid file name, discard this pending entry
			iter = pending_entries.erase(iter);
			continue;
		}

		// stat units are seconds, real_time_clock units are seconds
		if (real_time_clock() - st.st_mtime < ADD_ON_STABLE_SECONDS) {
			// entry not stable, skip the entry for this pulse
			iter++;
			continue;
		}

		// we are going to deal with the stable entry, so remove it
		iter = pending_entries.erase(iter);

		// put the new entry into the directory info
		std::list<add_on_directory_info>::iterator diter = directories.begin();
		for( ; diter != directories.end() ; diter++) {
			if (diter->nref == dir_nref) {
				diter->entries.push_back(info);
				break;
			}
		}

		// report it
		AddOnCreated(&info);

		// Start at the top again, and search until the directory we put
		// the new add_on in.  If we find a add_on with the same name then
		// the new add_on should not be enabled.
		bool enabled = true;
		std::list<add_on_directory_info>::iterator diter2 = directories.begin();
		for( ; diter2 != diter ; diter2++) {
			std::list<add_on_entry_info>::iterator eiter = diter2->entries.begin();
			for( ; eiter != diter2->entries.end() ; eiter++) {
				if (strcmp(eiter->name, info.name) == 0) {
					enabled = false;
					break;
				}
			}
			if (!enabled) {
				break;
			}
		}
		if (!enabled) {
			// if we are not enabled, go on to the next pending entry
			continue;
		}

		// The new add_on should be enabled, but first we check to see 
		// if there is an add_on below us.  If we find one, we disable it.
		bool shadowing = false;
		for (diter++ ; diter != directories.end() ; diter++) {
			std::list<add_on_entry_info>::iterator eiter = diter->entries.begin();
			for( ; eiter != diter->entries.end() ; eiter++) {
				if (strcmp(eiter->name, info.name) == 0) {
					AddOnDisabled(&*eiter);
					shadowing = true;
					break;
				}
			}
			if (shadowing) {
				break;
			}
		}

		// finally, enable the new entry
		AddOnEnabled(&info);
	}
}
