/* Launches an application/document from the shell
**
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

#include <InterfaceDefs.h>
#include <Roster.h>
#include <Entry.h>
#include <String.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>


int
main(int argc, char **argv)
{
	char *progName = argv[0];
	char *openWith = "application/x-vnd.Be-TRAK";
	if (strrchr(progName, '/'))
		progName = strrchr(progName, '/') + 1;

	if (argc < 2)
		fprintf(stderr,"usage: %s <file or url or application signature> ...\n", progName);

	BRoster roster;

	while (*++argv) {
		status_t rc = B_OK;
		argc--;

		BEntry entry(*argv);
		if ((rc = entry.InitCheck()) == B_OK && entry.Exists()) {
			entry_ref ref;
			entry.GetRef(&ref);

			BMessenger target(openWith);
			if (target.IsValid()) {
				BMessage message(B_REFS_RECEIVED);
				message.AddRef("refs", &ref);
				/* tell the app to open the file */
				rc = target.SendMessage(&message);
			} else
				rc = roster.Launch(&ref);
		} else if (!strncasecmp("application/", *argv, 12)) {
			// maybe it's an application-mimetype?
			
			// subsequent files are open with that app
			openWith = *argv;
			
			// in the case the app is already started, 
			// don't start it twice if we have other args
			BList teams;
			if (argc > 1) {
				roster.GetAppList(*argv, &teams);
			}
			if (teams.IsEmpty())
				rc = roster.Launch(*argv);
		} else if (strstr(*argv, ":")) {
			BString mimetype = "application/x-vnd.Be.URL.";
			BString arg(*argv);
			if (strstr(*argv, "://"))
				mimetype.Append(arg, arg.FindFirst("://"));
			else
				mimetype.Append(arg, arg.FindFirst(":"));
			char *args[2] = { *argv, NULL };
			rc = roster.Launch(mimetype.String(), 1, args);
			if (rc == B_ALREADY_RUNNING)
				rc = B_OK;
		} else if (rc == B_OK)
			rc = B_ENTRY_NOT_FOUND;

		if (rc != B_OK)
			fprintf(stderr, "%s: \"%s\": %s\n", progName, *argv, strerror(rc));
	}

	return 0;
}
