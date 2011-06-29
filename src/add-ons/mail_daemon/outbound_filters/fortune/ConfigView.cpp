/* ConfigView - the configuration view for the Fortune filter
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include "ConfigView.h"

#include <Catalog.h>
#include <TextControl.h>
#include <String.h>
#include <Message.h>

#include <FileConfigView.h>


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "ConfigView"


ConfigView::ConfigView()
	:
	BView(BRect(0, 0, 20, 20), "fortune_filter", B_FOLLOW_LEFT | B_FOLLOW_TOP,
		0)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	// determine font height
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	float itemHeight = (int32)(fontHeight.ascent + fontHeight.descent
		+ fontHeight.leading) + 13;

	BRect rect(5,4,250,25);
	rect.bottom = rect.top - 2 + itemHeight;
	BMailFileConfigView *fview = new BMailFileConfigView(
		B_TRANSLATE("Fortune file:"), "fortune_file", false, "", B_FILE_NODE);
	AddChild(fview);

	rect.top = rect.bottom + 8;
	rect.bottom = rect.top - 2 + itemHeight;
	BTextControl * control = new BTextControl(rect, "tag_line",
		B_TRANSLATE("Tag line:"), NULL, NULL);
	control->SetDivider(control->StringWidth(control->Label()) + 6);
	AddChild(control);

	ResizeToPreferred();
}


void ConfigView::SetTo(const BMessage *archive)
{
	BMailFileConfigView* control = (BMailFileConfigView*)FindView(
		"fortune_file");
	if (control != NULL)
		control->SetTo(archive, NULL);

	BString path = archive->FindString("tag_line");
	if (!archive->HasString("tag_line"))
		path = B_TRANSLATE("Fortune cookie says:\n\n");

	path.Truncate(path.Length() - 2);
	if (BTextControl *control = (BTextControl *)FindView("tag_line"))
		control->SetText(path.String());
}


status_t ConfigView::Archive(BMessage *into,bool) const
{
	if (BMailFileConfigView *control = (BMailFileConfigView
		*)FindView("fortune_file"))
	{
		control->Archive(into);
	}

	if (BTextControl *control = (BTextControl *)FindView("tag_line"))
	{
		BString line = control->Text();
		if (line != B_EMPTY_STRING)
			line << "\n\n";
		if (into->ReplaceString("tag_line", line.String()) != B_OK)
			into->AddString("tag_line", line.String());
	}
	return B_OK;
}


void ConfigView::GetPreferredSize(float *width, float *height)
{
	*width = 258;
	*height = (ChildAt(0)->Bounds().Height() + 8) * CountChildren();
}

