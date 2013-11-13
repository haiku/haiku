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
	:
	BHandler ("power_button_monitor")
{
	fPowerButtonFD = open("/dev/power/button/power", O_RDONLY);
}


PowerButtonMonitor::~PowerButtonMonitor()
{
	if (fPowerButtonFD > 0)
		close(fPowerButtonFD);
}


void
PowerButtonMonitor::MessageReceived(BMessage *msg)
{
	if (msg->what != POLL_POWER_BUTTON_STATUS)
		return;

	if (fPowerButtonFD <= 0)
		return;

	uint8 button_pressed;
	read(fPowerButtonFD, &button_pressed, 1);

	if (button_pressed) {
		BRoster roster;
		BRoster::Private rosterPrivate(roster);

		rosterPrivate.ShutDown(false, false, false);
	}
}
