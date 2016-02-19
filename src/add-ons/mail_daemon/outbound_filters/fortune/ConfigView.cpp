/*
 * Copyright 2004-2012, Haiku, Inc. All rights reserved.
 * Copyright 2001, Dr. Zoidberg Enterprises. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include "ConfigView.h"

#include <Catalog.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <String.h>
#include <TextControl.h>


using namespace BPrivate;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ConfigView"


ConfigView::ConfigView()
	:
	BMailSettingsView("fortune_filter")
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	fFileView = new MailFileConfigView(B_TRANSLATE("Fortune file:"),
		"fortune_file", false, "", B_FILE_NODE);
	fTagControl = new BTextControl("tag_line", B_TRANSLATE("Tag line:"),
		NULL, NULL);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(fFileView)
		.Add(fTagControl);
}


void
ConfigView::SetTo(const BMailAddOnSettings& settings)
{
	fFileView->SetTo(&settings, NULL);

	BString path = settings.GetString("tag_line",
		B_TRANSLATE("Fortune cookie says:\n\n"));

	path.Truncate(path.Length() - 2);
	fTagControl->SetText(path.String());
}


status_t
ConfigView::SaveInto(BMailAddOnSettings& settings) const
{
	fFileView->SaveInto(settings);

	BString line = fTagControl->Text();
	if (line != B_EMPTY_STRING)
		line << "\n\n";
	return settings.SetString("tag_line", line.String());
}
