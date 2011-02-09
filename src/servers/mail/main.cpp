/*
 * Copyright 2007-2011, Haiku, Inc. All rights reserved.
 * Copyright 2001-2002 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "MailDaemon.h"


int
main(int argc, const char** argv)
{
	bool remakeMIMETypes = false;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-E") == 0) {
			if (!BMailSettings().DaemonAutoStarts())
				return 0;
		}
		if (strcmp(argv[i], "-M") == 0) {
			remakeMIMETypes = true;
		}
	}

	MailDaemonApp app;
	if (remakeMIMETypes)
		app.MakeMimeTypes(true);
	app.Run();
	return 0;
}
