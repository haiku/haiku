/*
 * Copyright 2006-2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>

#include <Screen.h>
#include <GLRenderer.h>


extern "C" _EXPORT BGLRenderer*
instanciate_gl_renderer(BGLView* view, ulong options, BGLDispatcher* dispatcher)
{
	if (!view) {
		printf("view = NULL!\n");
		return NULL;
	}

	BWindow* window = view->Window();
	if (!window) {
		printf("view's window = NULL!\n");
		return NULL;
	}

	BScreen screen(window);
	if (!screen.IsValid()) {
		printf("Failed to get window's screen!\n");
		return NULL;
	}

	accelerant_device_info adi;
	if (screen.GetDeviceInfo(&adi) != B_OK) {
		printf("screen.GetDeviceInfo() failed!\n");
		return NULL;
	}

	printf("Accelerant device info:\n"
		"  version: %ud\n"
		"  name:    %s\n"
		"  chipset: %s\n"
		"  serial#: %s\n",
		(unsigned int) adi.version, adi.name, adi.chipset, adi.serial_no);

	// Check the view is attached to a screen driven by a Radeon chip:
	if (strcasecmp(adi.chipset, "radeon") == 0 ||
		strcasecmp(adi.chipset, "radeon") == 0) {
		// return new RadeonHardwareRenderer(view, options, dispatcher);
		return NULL;
	}

	// We can't be a renderer for this view, sorry.
	return NULL;
}
