/*
 * Copyright 2006-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "NetworkStatus.h"
#include "NetworkStatusWindow.h"

#include <Alert.h>
#include <Application.h>
#include <Deskbar.h>
#include <Entry.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


class NetworkStatus : public BApplication {
	public:
		NetworkStatus();
		virtual	~NetworkStatus();

		virtual	void	ReadyToRun();
		virtual void	AboutRequested();
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
	: BApplication(kSignature)
{
}


NetworkStatus::~NetworkStatus()
{
}


void
NetworkStatus::ReadyToRun()
{
	bool isInstalled = false;
	bool isDeskbarRunning = true;

	{
		// if the Deskbar is not alive at this point, it might be after having
		// acknowledged the requester below
		BDeskbar deskbar;
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
		isDeskbarRunning = deskbar.IsRunning();
#endif
		isInstalled = deskbar.HasItem(kDeskbarItemName);
	}

	if (isDeskbarRunning && !isInstalled) {
		BAlert* alert = new BAlert("", "Do you want NetworkStatus to live in the Deskbar?",
			"Don't", "Install", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->SetShortcut(0, B_ESCAPE);

		if (alert->Go()) {
			image_info info;
			entry_ref ref;

			if (our_image(info) == B_OK
				&& get_ref_for_path(info.name, &ref) == B_OK) {
				BDeskbar deskbar;
				deskbar.AddItem(&ref);
			}

			Quit();
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


//	#pragma mark -


int
main(int, char**)
{
	NetworkStatus app;
	app.Run();

	return 0;
}

