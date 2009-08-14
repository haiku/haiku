/*
 * Copyright 2007 - 2009 Chris Roberts. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Chris Roberts, cpr420@gmail.com
 */

#include <StorageKit.h>
#include <SupportKit.h>
#include <AppKit.h>

#include <vector>

#include <add-ons/tracker/TrackerAddOn.h>

const char* kTerminalSignature = "application/x-vnd.Haiku-Terminal";
const directory_which kAppsDirectory = B_SYSTEM_APPS_DIRECTORY;

void
launch_terminal(BEntry* targetEntry, BPath* terminalPath) {

	BPath targetPath;

	if (targetEntry->GetPath(&targetPath) != B_OK)
		return;

	// Escape paths which contain an apostraphe
	BString target(targetPath.Path());
	target.ReplaceAll("'", "'\\''");

	BString terminal(terminalPath->Path());
	terminal.ReplaceAll("'", "'\\''");

	// Build the command to "cd '/my/target/folder'; '/path/to/Terminal' &"
	BString command("cd '");
	command << target << "'; '" << terminal << "' &";

	// Launch the Terminal.
	system(command.String());
}


void
process_refs(entry_ref base_ref, BMessage* message, void* reserved) 
{
	BPath terminalPath;

	bool terminalFound = false;

	// Search for Terminal path by asking BRoster.
	entry_ref terminalRef;
	if (be_roster->FindApp(kTerminalSignature, &terminalRef) == B_OK) {
		if (terminalPath.SetTo(&terminalRef) == B_OK)
			terminalFound = true;
	}

	// Fall back to manually creating a path if BRoster didn't find it.
	if (!terminalFound) {
		if (find_directory(kAppsDirectory, &terminalPath) != B_OK)
			return;

		if (terminalPath.Append("Terminal") != B_OK)
			return;
	}

	BEntry entry;
	int32 i;
	entry_ref tracker_ref;
	std::vector<BEntry> entries;

	// Iterate through the refs that Tracker has sent us.
	for (i = 0; message->FindRef("refs", i, &tracker_ref) == B_OK; i++) {

		// Pass 'true' as the traverse argument below, so that if the ref
		// is a symbolic link we get the target of the link to ensure that
		// it actually exists.
		if (entry.SetTo(&tracker_ref, true) != B_OK)
			continue;

		// If the entry is a file then look for the parent directory.
		if (!entry.IsDirectory()) {
			if (entry.GetParent(&entry) != B_OK)
				continue;
		}

		bool duplicate = false;

		// Check for duplicates.
		for (uint x = 0; x < entries.size(); x++) {
			if (entries[x] == entry) {
				duplicate = true;
				break;
			}
		}

		// This is a duplicate.  Continue to next ref.
		if (duplicate)
			continue;

		// Push entry onto the vector so we can check for duplicates later.
		entries.push_back(BEntry(entry));

		launch_terminal(&entry, &terminalPath);

	}

	// If nothing was selected we'll use the base folder.
	if (i == 0) {
		if (entry.SetTo(&base_ref) == B_OK)
			launch_terminal(&entry, &terminalPath);
	}
}
