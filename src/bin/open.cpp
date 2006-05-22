/*
 * Copyright 2003-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2005-2006, François Revol, revol@free.fr.
 * Distributed under the terms of the MIT License.
 */

/* Launches an application/document from the shell */


#include <Roster.h>
#include <Entry.h>
#include <String.h>
#include <List.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


const char *kTrackerSignature = "application/x-vnd.Be-TRAK";

const char *openWith = kTrackerSignature;


status_t OpenFile(BEntry &entry, int32 line=-1, int32 col=-1)
{
	status_t status;
	entry_ref ref;
	entry.GetRef(&ref);

	BMessenger target(openWith);
	if (target.IsValid()) {
		BMessage message(B_REFS_RECEIVED);
		message.AddRef("refs", &ref);
		if (line > -1)
			message.AddInt32("be:line", line);
		if (col > -1)
			message.AddInt32("be:column", col);
		// tell the app to open the file
		status = target.SendMessage(&message);
	} else
		status = be_roster->Launch(&ref);
	return status;
}

int
main(int argc, char **argv)
{
	int exitcode = EXIT_SUCCESS;

	char *progName = argv[0];
	if (strrchr(progName, '/'))
		progName = strrchr(progName, '/') + 1;

	if (argc < 2)
		fprintf(stderr,"usage: %s <file[:line[:column]] or url or application signature> ...\n", progName);

	while (*++argv) {
		status_t status = B_OK;
		argc--;

		BEntry entry(*argv);
		if ((status = entry.InitCheck()) == B_OK && entry.Exists()) {
			status = OpenFile(entry);
		} else if (!strncasecmp("application/", *argv, 12)) {
			// maybe it's an application-mimetype?
			
			// subsequent files are open with that app
			openWith = *argv;
			// clear possible ENOENT from above
			status = B_OK;

			// in the case the app is already started, 
			// don't start it twice if we have other args
			BList teams;
			if (argc > 1)
				be_roster->GetAppList(*argv, &teams);

			if (teams.IsEmpty())
				status = be_roster->Launch(*argv);
		} else if (strchr(*argv, ':')) {
			// try to open it as an URI
			BString mimeType = "application/x-vnd.Be.URL.";
			BString arg(*argv);
			if (arg.FindFirst("://") >= 0)
				mimeType.Append(arg, arg.FindFirst("://"));
			else
				mimeType.Append(arg, arg.FindFirst(":"));

			char *args[2] = { *argv, NULL };
			status = be_roster->Launch(mimeType.String(), 1, args);
			if (status == B_OK)
				continue;
			
			// maybe it's "file:line" or "file:line:col"
			int line = 0, col = 0, i;
			status = B_ENTRY_NOT_FOUND;
			// remove gcc error's last :
			if (arg[arg.Length()-1] == ':')
				arg.Truncate(arg.Length()-1);
			i = arg.FindLast(':');
			if (i > 0) {
				line = atoi(arg.String()+i+1);
				arg.Truncate(i);
				entry.SetTo(arg.String());
				if (entry.Exists())
					status = OpenFile(entry, line-1, col-1);
				if (status == B_OK)
					continue;
				// get the col
				col = line;
				i = arg.FindLast(':');
				line = atoi(arg.String()+i+1);
				arg.Truncate(i);
				entry.SetTo(arg.String());
				if (entry.Exists())
					status = OpenFile(entry, line-1, col-1);
			}
		} else {
			status = B_ENTRY_NOT_FOUND;
		}

		if (status != B_OK && status != B_ALREADY_RUNNING) {
			fprintf(stderr, "%s: \"%s\": %s\n", progName, *argv, strerror(status));
			// make sure the shell knows this
			exitcode = EXIT_FAILURE;
		}
	}

	return exitcode;
}
