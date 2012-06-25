/*
 * Copyright 2005, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Inspired by SoundCapture from Be newsletter (Media Kit Basics: Consumers 
 * and Producers)
 */
#include <Catalog.h>
#include <Entry.h>
#include <Locale.h>

#include "SoundListView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SoundListView"


SoundListView::SoundListView(
	const BRect & area,
	const char * name,
	uint32 resize) :
	BListView(area, name, B_SINGLE_SELECTION_LIST, resize)
{
}


SoundListView::~SoundListView()
{
}


void
SoundListView::Draw(BRect updateRect)
{
	if (IsEmpty()) {
		SetLowColor(ViewColor());
		FillRect(Bounds(), B_SOLID_LOW);

		SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
		BFont font(be_bold_font);
		SetFont(&font);
		font_height height;
		font.GetHeight(&height);
		float width = font.StringWidth(B_TRANSLATE("Drop files here"));

		BPoint pt;
		pt.x = (Bounds().Width() - width) / 2;
		pt.y = (Bounds().Height() + height.ascent + height.descent)/ 2;
		DrawString(B_TRANSLATE("Drop files here"), pt);
	}
	BListView::Draw(updateRect);
}


void
SoundListView::AttachedToWindow()
{
	BListView::AttachedToWindow();
	SetViewColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
		B_LIGHTEN_1_TINT));
}


SoundListItem::SoundListItem(
	const BEntry & entry,
	bool isTemp)
	: BStringItem(""),
		fEntry(entry),
		fIsTemp(isTemp)
{
	char name[256];
	fEntry.GetName(name);
	SetText(name);
}


SoundListItem::~SoundListItem()
{
}
