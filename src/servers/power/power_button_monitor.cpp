/*
 * Copyright 2005-2013, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Nathan Whitehorn
 */


#include "power_button_monitor.h"

#include <Messenger.h>
#include <Roster.h>

#include <RosterPrivate.h>


PowerButtonMonitor::PowerButtonMonitor()
{
	fFD = open("/dev/power/button/power", O_RDONLY);
}


PowerButtonMonitor::~PowerButtonMonitor()
{
	if (fFD > 0)
		close(fFD);
}


void
PowerButtonMonitor::HandleEvent()
{
	if (fFD <= 0)
		return;

	uint8 button_pressed;
	read(fFD, &button_pressed, 1);

	if (button_pressed) {
		BRoster roster;
		BRoster::Private rosterPrivate(roster);

		rosterPrivate.ShutDown(false, false, false);
	}
}
