/*
 * Copyright 2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Adrien Destugues <pulkomandy@pulkomandy.ath.cx>
 */


#include "ApplicationWindow.h"

#include <Application.h>
#include <Catalog.h>
#include <Entry.h>
#include <TextView.h>
#include <Locale.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "main"


class PackageManager : public BApplication {
	public:
		PackageManager();
		~PackageManager();

		void RefsReceived(BMessage *msg);
		void ArgvReceived(int32 argc, char **argv);
		void ReadyToRun();

		void MessageReceived(BMessage *msg);

	private:
		ApplicationWindow* fMainWindow;
};


PackageManager::PackageManager()
	:	BApplication("application/x-vnd.Haiku-PackageManager")
{
}


PackageManager::~PackageManager()
{
}


void
PackageManager::ReadyToRun()
{
	// Open the main window
	BRect frame(20, 40, 600, 400);
	fMainWindow = new ApplicationWindow(frame, true);

	// Add some test content to the window
	fMainWindow->AddCategory("Test category", "development",
		"Thisis a short description of the category");

	BMessage entryInfo;
	// This message is a convenient way of storing various infos about an app.
	// What's inside :
	// icon as an archived BBitmap
	entryInfo.AddString("appname", "Test Application");
	entryInfo.AddString("appdesc", "Some text telling what it does");
	entryInfo.AddFloat("appver", 1.302);
		// as a float so it can be compared to decide if there is an update
	entryInfo.AddInt32("appsize", 123456); // this is in bytes

	fMainWindow->AddApplication(&entryInfo);

	fMainWindow->Show();
}


void
PackageManager::RefsReceived(BMessage *msg)
{
	uint32 type;
	int32 i, count;
	status_t ret = msg->GetInfo("refs", &type, &count);
	if (ret != B_OK || type != B_REF_TYPE)
		return;

	entry_ref ref;
	for (i = 0; i < count; i++) {
		if (msg->FindRef("refs", i, &ref) == B_OK) {
			// TODO: handle bundle-files for installation
		}
	}
}


void
PackageManager::ArgvReceived(int32 argc, char **argv)
{
	// TODO: handle command-line driven actions
}


void
PackageManager::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		default:
			BApplication::MessageReceived(msg);
	}
}


int
main(void)
{
	PackageManager app;
	app.Run();

	return 0;
}

