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
		SetHighColor(235,235,235);
		FillRect(Bounds());

		SetHighColor(0,0,0);
		BFont font(be_bold_font);
		font.SetSize(12.0);
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
	SetViewColor(255,255,255);
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
