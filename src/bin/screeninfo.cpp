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


int
main()
{
	// BScreen usage requires BApplication for AppServerLink
	BApplication app("application/x-vnd.Haiku-screen_info");

	BScreen screen(B_MAIN_SCREEN_ID);

	do {
		screen_id screenIndex = screen.ID();
		accelerant_device_info info;

		// At the moment, screen.ID() is always 0;
		printf("Screen %" B_PRId32 ":", screen.ID().id);
		if (screen.GetDeviceInfo(&info) != B_OK) {
			printf(" unavailable\n");
		} else {
			printf(" attached\n");
			printf("  version: %" B_PRId32 "\n", info.version);
			printf("  name:    %s\n", info.name);
			printf("  chipset: %s\n", info.chipset);
			printf("  serial:  %s\n", info.serial_no);
		}
	} while (screen.SetToNext() == B_OK);

	return 0;
}
