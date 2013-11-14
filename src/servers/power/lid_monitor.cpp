/*
 * Copyright 2013, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2005, Nathan Whitehorn.
 *
 * Distributed under the terms of the MIT License.
 */


#include "lid_monitor.h"

#include <Messenger.h>
#include <Roster.h>

#include <stdio.h>

#include <RosterPrivate.h>


LidMonitor::LidMonitor()
{
	fFD = open("/dev/power/acpi_lid/0", O_RDONLY);
}


LidMonitor::~LidMonitor()
{
	if (fFD > 0)
		close(fFD);
}


void
LidMonitor::HandleEvent()
{
	if (fFD <= 0)
		return;

	uint8 status;
	read(fFD, &status, 1);

	if (status == 1) {
		printf("lid status 1\n");
	}
}
