/*
 * Copyright 2003-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/* Launches an application/document from the shell */


#include <Roster.h>
#include <Entry.h>
#include <String.h>
#include <List.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>


const char *kTrackerSignature = "application/x-vnd.Be-TRAK";


int
main(int argc, char **argv)
{
	const char *openWith = kTrackerSignature;

	char *progName = argv[0];
	if (strrchr(progName, '/'))
		progName = strrchr(progName, '/') + 1;

	if (argc < 2)
		fprintf(stderr,"usage: %s <file or url or application signature> ...\n", progName);

	BRoster roster;

	while (*++argv) {
		status_t status = B_OK;
		argc--;

		BEntry entry(*argv);
		if ((status = entry.InitCheck()) == B_OK && entry.Exists()) {
			entry_ref ref;
			entry.GetRef(&ref);

			BMessenger target(openWith);
			if (target.IsValid()) {
				BMessage message(B_REFS_RECEIVED);
				message.AddRef("refs", &ref);
				// tell the app to open the file
				status = target.SendMessage(&message);
			} else
				status = roster.Launch(&ref);
		} else if (!strncasecmp("application/", *argv, 12)) {
			// maybe it's an application-mimetype?
			
			// subsequent files are open with that app
			openWith = *argv;

			// in the case the app is already started, 
			// don't start it twice if we have other args
			BList teams;
			if (argc > 1)
				roster.GetAppList(*argv, &teams);

			if (teams.IsEmpty())
				status = roster.Launch(*argv);
		} else if (strchr(*argv, ':')) {
			// try to open it as an URI
			BString mimeType = "application/x-vnd.Be.URL.";
			BString arg(*argv);
			if (arg.FindFirst("://") >= 0)
				mimeType.Append(arg, arg.FindFirst("://"));
			else
				mimeType.Append(arg, arg.FindFirst(":"));

			char *args[2] = { *argv, NULL };
			status = roster.Launch(mimeType.String(), 1, args);
		} else
			status = B_ENTRY_NOT_FOUND;

		if (status != B_OK && status != B_ALREADY_RUNNING)
			fprintf(stderr, "%s: \"%s\": %s\n", progName, *argv, strerror(status));
	}

	return 0;
}
