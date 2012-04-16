/*
 * Copyright 2006-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "NetworkStatus.h"
#include "NetworkStatusWindow.h"

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <Deskbar.h>
#include <Entry.h>
#include <Locale.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "NetworkStatus"


class NetworkStatus : public BApplication {
	public:
		NetworkStatus();
		virtual	~NetworkStatus();

		virtual	void	ArgvReceived(int32 argc, char** argv);
		virtual	void	ReadyToRun();
		virtual void	AboutRequested();
	private:
				void	_InstallReplicantInDeskbar();

				bool	fAutoInstallInDeskbar;
				bool	fQuitImmediately;
};


const char* kSignature = "application/x-vnd.Haiku-NetworkStatus";
const char* kDeskbarSignature = "application/x-vnd.Be-TSKB";
const char* kDeskbarItemName = "NetworkStatus";


status_t
our_image(image_info& image)
{
	int32 cookie = 0;
	while (get_next_image_info(B_CURRENT_TEAM, &cookie, &image) == B_OK) {
		if ((char *)our_image >= (char *)image.text
			&& (char *)our_image <= (char *)image.text + image.text_size)
			return B_OK;
	}

	return B_ERROR;
}


//	#pragma mark -


NetworkStatus::NetworkStatus()
	:
	BApplication(kSignature),
	fAutoInstallInDeskbar(false),
	fQuitImmediately(false)
{
}


NetworkStatus::~NetworkStatus()
{
}


void
NetworkStatus::ArgvReceived(int32 argc, char** argv)
{
	if (argc <= 1)
		return;

	if (strcmp(argv[1], "--help") == 0
		|| strcmp(argv[1], "-h") == 0) {
		const char* str = B_TRANSLATE("NetworkStatus options:\n"
			"\t--deskbar\tautomatically add replicant to Deskbar\n"
			"\t--help\t\tprint this info and exit\n");
		printf(str);
		fQuitImmediately = true;
		return;
	}

	if (strcmp(argv[1], "--deskbar") == 0)
		fAutoInstallInDeskbar = true;
}


void
NetworkStatus::ReadyToRun()
{
	if (fQuitImmediately) {
		// we printed the help message into the Terminal and
		// should just quit
		Quit();
		return;
	}

	bool isDeskbarRunning = true;
	bool isInstalled = false;

	{
		// if the Deskbar is not alive at this point, it might be after having
		// acknowledged the requester below
		BDeskbar deskbar;
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
		isDeskbarRunning = deskbar.IsRunning();
#endif
		isInstalled = deskbar.HasItem(kDeskbarItemName);
	}

	if (fAutoInstallInDeskbar) {
		if (isInstalled) {
			Quit();
			return;
		}
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
		// wait up to 10 seconds for Deskbar to become available
		// in case it is not running (yet?)
		int32 tries = 10;
		while (!isDeskbarRunning && --tries) {
			BDeskbar deskbar;
			if (deskbar.IsRunning()) {
				isDeskbarRunning = true;
				break;
			}
			snooze(1000000);
		}
#endif
		if (!isDeskbarRunning) {
			printf("Deskbar is not running, giving up.\n");
			Quit();
			return;
		}

		_InstallReplicantInDeskbar();
		return;
	}

	if (isDeskbarRunning && !isInstalled) {
		BAlert* alert = new BAlert("", B_TRANSLATE("You can run NetworkStatus "
			"in a window or install it in the Deskbar."),
			B_TRANSLATE("Run in window"), B_TRANSLATE("Install in Deskbar"),
			NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->SetShortcut(0, B_ESCAPE);

		if (alert->Go() == 1) {
			_InstallReplicantInDeskbar();
			return;
		}
	}

	BWindow* window = new NetworkStatusWindow();
	window->Show();
}


void
NetworkStatus::AboutRequested()
{
	BWindow* window = WindowAt(0);
	if (window == NULL)
		return;

	BView* view = window->FindView(kDeskbarItemName);
	if (view == NULL)
		return;

	BMessenger target((BHandler*)view);
	BMessage about(B_ABOUT_REQUESTED);
	target.SendMessage(&about);
}


void
NetworkStatus::_InstallReplicantInDeskbar()
{
	image_info info;
	entry_ref ref;

	if (our_image(info) == B_OK
		&& get_ref_for_path(info.name, &ref) == B_OK) {
		BDeskbar deskbar;
		deskbar.AddItem(&ref);
	}

	Quit();
}


//	#pragma mark -


int
main(int, char**)
{
	NetworkStatus app;
	app.Run();

	return 0;
}

