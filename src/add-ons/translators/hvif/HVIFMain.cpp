/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 */

#include <Application.h>
#include "HVIFTranslator.h"
#include "TranslatorWindow.h"

int
main(int argc, char *argv[])
{
	BApplication application("application/x-vnd.haiku.hvif-translator");
	if (LaunchTranslatorWindow(new HVIFTranslator, "HVIF Settings",
			BRect(0, 0, 250, 150)) != B_OK)
		return 1;

	application.Run();
	return 0;
}
