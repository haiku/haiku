/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "PowerStatus.h"
#include "PowerStatusWindow.h"

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <Deskbar.h>
#include <Entry.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PowerStatus"


class PowerStatus : public BApplication {
	public:
		PowerStatus();
		virtual	~PowerStatus();

		virtual	void	ReadyToRun();
		virtual void	AboutRequested();
};


const char* kSignature = "application/x-vnd.Haiku-PowerStatus";
const char* kDeskbarSignature = "application/x-vnd.Be-TSKB";
const char* kDeskbarItemName = "PowerStatus";


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


PowerStatus::PowerStatus()
	: BApplication(kSignature)
{
}


PowerStatus::~PowerStatus()
{
}


void
PowerStatus::ReadyToRun()
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
		BAlert* alert = new BAlert("", 
			B_TRANSLATE("You can run PowerStatus in a window "
			"or install it in the Deskbar."), B_TRANSLATE("Run in window"),
			B_TRANSLATE("Install in Deskbar"), NULL, B_WIDTH_AS_USUAL,
			B_WARNING_ALERT);

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

	BWindow* window = new PowerStatusWindow();
	window->Show();
}


void
PowerStatus::AboutRequested()
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
	PowerStatus app;
	app.Run();

	return 0;
}

