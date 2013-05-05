/*
 * Copyright (c) 1998-2007 Matthijs Hollemans
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "App.h"

#include <TrackerAddOnAppLaunch.h>

#include <stdio.h>

#include <Catalog.h>

#include "DiskUsage.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DiskUsage"

entry_ref helpFileRef;
bool helpFileWasFound = false;

void
size_to_string(off_t byteCount, char* name, int maxLength)
{
	struct {
		off_t		limit;
		float		divisor;
		const char*	format;
	} scale[] = {
		{ 0x100000,				1024.0,
			B_TRANSLATE("%.2f KiB") },
		{ 0x40000000,			1048576.0,
			B_TRANSLATE("%.2f MiB") },
		{ 0x10000000000ull,		1073741824.0,
			B_TRANSLATE("%.2f GiB") },
		{ 0x4000000000000ull,	1.09951162778e+12,
			B_TRANSLATE("%.2f TiB") }
	};

	if (byteCount < 1024) {
		snprintf(name, maxLength, B_TRANSLATE("%lld bytes"),
			byteCount);
	} else {
		int i = 0;
		while (byteCount >= scale[i].limit)
			i++;

		snprintf(name, maxLength, scale[i].format,
			byteCount / scale[i].divisor);
	}
}

int
main() 
{
	App app;
	app.Run();
	return 0;
}

