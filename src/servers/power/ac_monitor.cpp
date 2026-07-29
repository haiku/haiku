/*
 * Copyright 2013, 2026, Jérôme Duval, jerome.duval@gmail.com.
 * Copyright 2005, Nathan Whitehorn.
 *
 * Distributed under the terms of the MIT License.
 */


#include "ac_monitor.h"

#include <Messenger.h>
#include <Roster.h>

#include <stdio.h>


ACMonitor::ACMonitor()
{
	int fd = open("/dev/power/acpi_ac/0", O_RDONLY);
	if (fd > 0)
		fFDs.insert(fd);
}


ACMonitor::~ACMonitor()
{
	for (std::set<int>::iterator it = fFDs.begin(); it != fFDs.end(); ++it)
		close(*it);
}


void
ACMonitor::HandleEvent(int fd)
{
	uint8 status;
	if (read(fd, &status, 1) != 1)
		return;

	if (status == 1)
		printf("ac status 1\n");
}
