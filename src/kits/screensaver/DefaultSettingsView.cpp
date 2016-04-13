/*
 * Copyright 2009-2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ryan Leavengood, leavengood@gmail.com
 *		John Scipione, jscipione@gmail.com
 */


#include <DefaultSettingsView.h>

#include <LayoutBuilder.h>
#include <StringView.h>


namespace BPrivate {

// Provides a consistent look for the settings view for screen savers
// that don't provide any configuration settings.
void
BuildDefaultSettingsView(BView* view, const char* moduleName, const char* info)
{
	view->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	BStringView* nameStringView = new BStringView("module", moduleName);
	nameStringView->SetFont(be_bold_font);

	BStringView* infoStringView = new BStringView("info", info);

	BLayoutBuilder::Group<>(view, B_VERTICAL, B_USE_SMALL_SPACING)
		.Add(nameStringView)
		.Add(infoStringView)
		.AddGlue()
		.SetInsets(B_USE_DEFAULT_SPACING)
		.End();
}

}	// namespace BPrivate
