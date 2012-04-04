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
#include <Roster.h>
#include <String.h>
#include <SystemCatalog.h>

using BPrivate::gSystemCatalog;


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

	const char* string = B_TRANSLATE_MARK("About %app%");
	string = gSystemCatalog->GetString(string, "AboutMenuItem");

	BString label = string;
	if (name != NULL)
		label.ReplaceFirst("%app%", name);
	else
		label.ReplaceFirst("%app%", "(NULL)");
	SetLabel(label.String());
}
