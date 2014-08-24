/*
 * Copyright 2005-2014, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Rene Gollent
 *		Nathan Whitehorn
 */


#include "power_button_monitor.h"

#include <string.h>

#include <Directory.h>
#include <Messenger.h>
#include <Roster.h>
#include <String.h>

#include <RosterPrivate.h>


static const char* kBasePath = "/dev/power/button";


PowerButtonMonitor::PowerButtonMonitor()
	:
	fFDs()
{
	BDirectory dir;
	if (dir.SetTo(kBasePath) != B_OK)
		return;

	entry_ref ref;
	while (dir.GetNextRef(&ref) == B_OK) {
		if (strncmp(ref.name, "power", 5) == 0) {
			BString path;
			path.SetToFormat("%s/%s", kBasePath, ref.name);
			int fd = open(path.String(), O_RDONLY);
			if (fd > 0)
				fFDs.insert(fd);
		}
	}
}


PowerButtonMonitor::~PowerButtonMonitor()
{
	for (std::set<int>::iterator it = fFDs.begin(); it != fFDs.end(); ++it)
		close(*it);
}


void
PowerButtonMonitor::HandleEvent(int fd)
{
	uint8 button_pressed;
	if (read(fd, &button_pressed, 1) != 1)
		return;

	if (button_pressed) {
		BRoster roster;
		BRoster::Private rosterPrivate(roster);

		rosterPrivate.ShutDown(false, false, false);
	}
}
