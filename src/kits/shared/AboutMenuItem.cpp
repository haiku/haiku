/*
 * Copyright 2007-2011 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ryan Leavengood, leavengood@gmail.com
 *		Jonas Sundström, jonas@kirilla.se
 */


#include <AboutMenuItem.h>
#include <Application.h>
#include <Catalog.h>
#include <LocaleBackend.h>
#include <Roster.h>
#include <String.h>


using BPrivate::gLocaleBackend;
using BPrivate::LocaleBackend;


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "AboutMenuItem"


BAboutMenuItem::BAboutMenuItem()
	:
	BMenuItem("", new BMessage(B_ABOUT_REQUESTED))
{
	app_info info;
	const char* name = NULL;
	if (be_app != NULL && be_app->GetAppInfo(&info) == B_OK)
		name = B_TRANSLATE_NOCOLLECT_SYSTEM_NAME(info.ref.name);

	// we need to translate some strings, and in order to do so, we need
	// to use the LocaleBackend to reach liblocale.so
	if (gLocaleBackend == NULL)
		LocaleBackend::LoadBackend();

	const char* string = B_TRANSLATE_MARK("About %app%");
	if (gLocaleBackend) {
		string = gLocaleBackend->GetString(string, "AboutMenuItem");
	}

	BString label = string;
	if (name != NULL)
		label.ReplaceFirst("%app%", name);
	else
		label.ReplaceFirst("%app%", "(NULL)");
	SetLabel(label.String());
}
