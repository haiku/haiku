/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2010, Adrien Destugues <pulkomandy@pulkomandy.ath.cx>. All rightts reserved.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <string.h>

#include <AboutWindow.h>
#include <Application.h>
#include <Alert.h>
#include <Catalog.h>
#include <TextView.h>
#include <Locale.h>

#include "LocalePreflet.h"
#include "LocaleWindow.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Locale Preflet"


const char* kSignature = "application/x-vnd.Haiku-Locale";


class LocalePreflet : public BApplication {
	public:
							LocalePreflet();
		virtual				~LocalePreflet();

		virtual	void		AboutRequested();

private:
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


//	#pragma mark -


int
main(int argc, char** argv)
{
	LocalePreflet app;
	app.Run();
	return 0;
}

