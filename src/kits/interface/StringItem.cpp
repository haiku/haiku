/*
 * Copyright 2001-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ulrich Wimboeck
 *		Marc Flerackers (mflerackers@androme.be)
 *		Rene Gollent
 */


#include <StringItem.h>

#include <stdlib.h>
#include <string.h>

#include <ControlLook.h>
#include <Message.h>
#include <View.h>


BStringItem::BStringItem(const char* text, uint32 level, bool expanded)
	:
	BListItem(level, expanded),
	fText(NULL),
	fBaselineOffset(0)
{
	SetText(text);
}


BStringItem::BStringItem(BMessage* archive)
	:
	BListItem(archive),
	fText(NULL),
	fBaselineOffset(0)
{
	const char* string;
	if (archive->FindString("_label", &string) == B_OK)
		SetText(string);
}


BStringItem::~BStringItem()
{
	free(fText);
}


BArchivable*
BStringItem::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "BStringItem"))
		return new BStringItem(archive);

	return NULL;
}


status_t
BStringItem::Archive(BMessage* archive, bool deep) const
{
	status_t status = BListItem::Archive(archive);

	if (status == B_OK && fText != NULL)
		status = archive->AddString("_label", fText);

	return status;
}


void
BStringItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	if (fText == NULL)
		return;

	rgb_color lowColor = owner->LowColor();

	if (IsSelected() || complete) {
		rgb_color color;
		if (IsSelected())
			color = ui_color(B_LIST_SELECTED_BACKGROUND_COLOR);
		else
			color = owner->ViewColor();

		owner->SetLowColor(color);
		owner->FillRect(frame, B_SOLID_LOW);
	} else
		owner->SetLowColor(owner->ViewColor());

	owner->MovePenTo(frame.left + be_control_look->DefaultLabelSpacing(),
		frame.top + fBaselineOffset);

	owner->DrawString(fText);

	owner->SetLowColor(lowColor);
}


void
BStringItem::SetText(const char* text)
{
	free(fText);
	fText = NULL;

	if (text)
		fText = strdup(text);
}


const char*
BStringItem::Text() const
{
	return fText;
}


void
BStringItem::Update(BView* owner, const BFont* font)
{
	if (fText != NULL) {
		SetWidth(font->StringWidth(fText)
			+ be_control_look->DefaultLabelSpacing());
	}

	font_height fheight;
	font->GetHeight(&fheight);

	fBaselineOffset = 2 + ceilf(fheight.ascent + fheight.leading / 2);

	SetHeight(ceilf(fheight.ascent) + ceilf(fheight.descent)
		+ ceilf(fheight.leading) + 4);
}


status_t
BStringItem::Perform(perform_code d, void* arg)
{
	return BListItem::Perform(d, arg);
}


float
BStringItem::BaselineOffset() const
{
	return fBaselineOffset;
}


void BStringItem::_ReservedStringItem1() {}
void BStringItem::_ReservedStringItem2() {}


BStringItem::BStringItem(const BStringItem &)
{
}


BStringItem	&
BStringItem::operator=(const BStringItem &)
{
	return *this;
}
