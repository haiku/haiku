/*
 * Copyright 2013-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV <kallisti5@unixzen.com>
 */


#include <Application.h>
#include <Screen.h>
#include <stdio.h>


// screen_id currently locked to 1 screen
// TODO: This should likely be provided by our API
#define MAX_SCREENS 1


int
main()
{
	// BScreen usage requires BApplication for AppServerLink
	BApplication app("application/x-vnd.Haiku-screen_info");

	for (int id = 0; id < MAX_SCREENS; id++) {
		screen_id screenIndex = {id};
		BScreen screen(screenIndex);
		accelerant_device_info info;

		// At the moment, screen.ID() is always 0;
		printf("Screen %" B_PRId32 ":", screen.ID().id);
		if (screen.GetDeviceInfo(&info) != B_OK) {
			printf(" unavailable\n");
		} else {
			printf(" attached\n");
			printf("  version: %u\n", info.version);
			printf("  name:    %s\n", info.name);
			printf("  chipset: %s\n", info.chipset);
			printf("  serial:  %s\n", info.serial_no);
		}
	}

	return 0;
}
