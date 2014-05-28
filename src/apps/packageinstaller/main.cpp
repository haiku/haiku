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
#include <Path.h>
#include <TextView.h>

#include <stdio.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Packageinstaller main"


bool gVerbose = false;


class PackageInstaller : public BApplication {
public:
								PackageInstaller();
	virtual						~PackageInstaller();

	virtual void				RefsReceived(BMessage* message);
	virtual void				ArgvReceived(int32 argc, char** argv);
	virtual void				ReadyToRun();

	virtual void				MessageReceived(BMessage* message);

private:
			void				_NewWindow(const entry_ref* ref);

private:
			BFilePanel*			fOpenPanel;
			uint32				fWindowCount;
};


PackageInstaller::PackageInstaller()
	:
	BApplication("application/x-vnd.Haiku-PackageInstaller"),
	fOpenPanel(new BFilePanel(B_OPEN_PANEL)),
	fWindowCount(0)
{
}


PackageInstaller::~PackageInstaller()
{
}


void
PackageInstaller::ReadyToRun()
{
	// We're ready to run - if no windows are yet visible, this means that
	// we should show a open panel
	if (fWindowCount == 0)
		fOpenPanel->Show();
}


void
PackageInstaller::RefsReceived(BMessage* message)
{
	entry_ref ref;
	for (int32 i = 0; message->FindRef("refs", i, &ref) == B_OK; i++)
		_NewWindow(&ref);
}


void
PackageInstaller::ArgvReceived(int32 argc, char** argv)
{
	for (int i = 1; i < argc; i++) {
		if (strcmp("--verbose", argv[i]) == 0 || strcmp("-v", argv[i]) == 0) {
			gVerbose = true;
			continue;
		}
		
		BPath path;
		if (path.SetTo(argv[i]) != B_OK) {
			fprintf(stderr, B_TRANSLATE("Error! \"%s\" is not a valid path.\n"),
				argv[i]);
			continue;
		}

		entry_ref ref;
		status_t ret = get_ref_for_path(path.Path(), &ref);
		if (ret != B_OK) {
			fprintf(stderr, B_TRANSLATE("Error (%s)! Could not open \"%s\".\n"),
				strerror(ret), argv[i]);
			continue;
		}

		_NewWindow(&ref);
	}
}


void
PackageInstaller::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case P_WINDOW_QUIT:
			fWindowCount--;
			// fall through
		case B_CANCEL:
			if (fWindowCount == 0)
				PostMessage(B_QUIT_REQUESTED);
			break;

		default:
			BApplication::MessageReceived(message);
	}
}


void
PackageInstaller::_NewWindow(const entry_ref* ref)
{
	PackageWindow* window = new PackageWindow(ref);
	window->Show();

	fWindowCount++;
}


int
main(void)
{
	PackageInstaller app;
	app.Run();

	return 0;
}

