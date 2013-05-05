/*
 * Copyright (c) 2007-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */


#include "PackageWindow.h"

#include <Alert.h>
#include <Application.h>
#include <Autolock.h>
#include <Catalog.h>
#include <Entry.h>
#include <FilePanel.h>
#include <List.h>
#include <Locale.h>
#include <TextView.h>

#include <stdio.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Packageinstaller main"

class PackageInstaller : public BApplication {
	public:
		PackageInstaller();
		~PackageInstaller();

		void RefsReceived(BMessage *msg);
		void ArgvReceived(int32 argc, char **argv);
		void ReadyToRun();

		void MessageReceived(BMessage *msg);

	private:
		BFilePanel	*fOpen;
		uint32		fWindowCount;
};


PackageInstaller::PackageInstaller()
	:	BApplication("application/x-vnd.Haiku-PackageInstaller"),
	fOpen(NULL),
	fWindowCount(0)
{
	fOpen = new BFilePanel(B_OPEN_PANEL);
}


PackageInstaller::~PackageInstaller()
{
}


void
PackageInstaller::ReadyToRun()
{
	// We're ready to run - if no windows are yet visible, this means that
	// we should show a open panel
	if (fWindowCount == 0) {
		fOpen->Show();
	}
}


void
PackageInstaller::RefsReceived(BMessage *msg)
{
	uint32 type;
	int32 i, count;
	status_t ret = msg->GetInfo("refs", &type, &count);
	if (ret != B_OK || type != B_REF_TYPE)
		return;

	entry_ref ref;
	PackageWindow *iter;
	for (i = 0; i < count; i++) {
		if (msg->FindRef("refs", i, &ref) == B_OK) {
			iter = new PackageWindow(&ref);
			fWindowCount++;
			iter->Show();
		}
	}
}


void
PackageInstaller::ArgvReceived(int32 argc, char **argv)
{
	int i;
	BPath path;
	entry_ref ref;
	status_t ret = B_OK;
	PackageWindow *iter = 0;

	for (i = 1; i < argc; i++) {
		if (path.SetTo(argv[i]) != B_OK) {
			fprintf(stderr,
					B_TRANSLATE("Error! \"%s\" is not a valid path.\n"),
					argv[i]);
			continue;
		}

		ret = get_ref_for_path(path.Path(), &ref);
		if (ret != B_OK) {
			fprintf(stderr,
					B_TRANSLATE("Error (%s)! Could not open \"%s\".\n"),
					strerror(ret), argv[i]);
			continue;
		}

		iter = new PackageWindow(&ref);
		fWindowCount++;
		iter->Show();
	}
}


void
PackageInstaller::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case P_WINDOW_QUIT:
			fWindowCount--;
		case B_CANCEL:
			if (fWindowCount == 0) {
				BAutolock lock(this);
				if (lock.IsLocked())
					Quit();
			}
			break;
		default:
			BApplication::MessageReceived(msg);
	}
}


int
main(void)
{
	PackageInstaller app;
	app.Run();

	return 0;
}

