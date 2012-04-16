/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2010, Adrien Destugues <pulkomandy@pulkomandy.ath.cx>. All rightts reserved.
 * Distributed under the terms of the MIT License.
 */


#include <AboutWindow.h>
#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <Locale.h>
#include <Roster.h>
#include <TextView.h>

#include "LocalePreflet.h"
#include "LocaleWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Locale Preflet"


const char* kSignature = "application/x-vnd.Haiku-Locale";


class LocalePreflet : public BApplication {
	public:
							LocalePreflet();
		virtual				~LocalePreflet();

		virtual	void		AboutRequested();
		virtual	void		MessageReceived(BMessage* message);

private:
		status_t			_RestartApp(const char* signature) const;
		
		LocaleWindow*		fLocaleWindow;
};


//	#pragma mark -


LocalePreflet::LocalePreflet()
	:
	BApplication(kSignature)
{
	fLocaleWindow = new LocaleWindow();

	fLocaleWindow->Show();
}


LocalePreflet::~LocalePreflet()
{
}


void
LocalePreflet::AboutRequested()
{
	const char* authors[] = {
		"Axel Dörfler",
		"Adrien Destugues",
		"Oliver Tappe",
		NULL
	};
	BAboutWindow about(B_TRANSLATE("Locale"), 2005, authors);
	about.Show();
}


void
LocalePreflet::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgRestartTrackerAndDeskbar:
			if (message->FindInt32("which") == 1) {
				_RestartApp("application/x-vnd.Be-TRAK");
				_RestartApp("application/x-vnd.Be-TSKB");
			}
			break;
		
		default:
			BApplication::MessageReceived(message);
			break;
	}
}


status_t
LocalePreflet::_RestartApp(const char* signature) const
{
	app_info info;
	status_t status = be_roster->GetAppInfo(signature, &info);
	if (status != B_OK)
		return status;

	BMessenger application(signature);
	status = application.SendMessage(B_QUIT_REQUESTED);
	if (status != B_OK)
		return status;

	status_t exit;
	wait_for_thread(info.thread, &exit);

	return be_roster->Launch(signature);
}


//	#pragma mark -


int
main(int argc, char** argv)
{
	LocalePreflet app;
	app.Run();
	return 0;
}

