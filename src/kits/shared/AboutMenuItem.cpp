/*
 * Copyright 2007-2011 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ryan Leavengood, leavengood@gmail.com
 *		Jonas Sundström, jonas@kirilla.se
 */


#include <AboutMenuItem.h>
#include <Catalog.h>
#include <LocaleBackend.h>
#include <String.h>


using BPrivate::gLocaleBackend;
using BPrivate::LocaleBackend;


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "AboutMenuItem"


BAboutMenuItem::BAboutMenuItem(const char* appName)
	:
	BMenuItem("", new BMessage(B_ABOUT_REQUESTED))
{
	// we need to translate some strings, and in order to do so, we need
	// to use the LocaleBackend to reach liblocale.so
	if (gLocaleBackend == NULL)
		LocaleBackend::LoadBackend();

	const char* string = B_TRANSLATE_MARK("About %app%" B_UTF8_ELLIPSIS);
	if (gLocaleBackend) {
		string = gLocaleBackend->GetString(string, "AboutMenuItem");
	}

	BString label = string;
	label.ReplaceFirst("%app%", appName);
	SetLabel(label.String());
}
