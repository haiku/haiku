//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		ListItem.cpp
//	Author:			Ulrich Wimboeck
//					Marc Flerackers (mflerackers@androme.be)
//	Description:	BListItem is the base class for BListView's items,
//					BStringItem is a subclass of BListItem which draws a string.
//------------------------------------------------------------------------------
#include <stdlib.h>
#include <string.h>

#include <ListItem.h>
#include <Message.h>
#include <View.h>


BListItem::BListItem(uint32 level, bool expanded)
	:	fWidth(0),
		fHeight(0),
		fLevel(level),
		fSelected(false),
		fEnabled(true),
		fExpanded(expanded),
		fHasSubitems(false),
		fVisible(true)
{
}


BListItem::BListItem(BMessage *data)
	:	BArchivable(data),
		fWidth(0),
		fHeight(0),
		fLevel(0),
		fSelected(false),
		fEnabled(true),
		fExpanded(false),
		fHasSubitems(false),
		fVisible(true)
{
	data->FindBool("_sel", &fSelected);
	
	if (data->FindBool("_disable", &fEnabled) != B_OK)
		fEnabled = true;
	else
		fEnabled = false;

	data->FindBool("_li_expanded", &fExpanded);
	data->FindInt32("_li_outline_level", (int32*)&fLevel);
}


BListItem::~BListItem()
{
}


status_t
BListItem::Archive(BMessage *archive, bool deep) const
{
	status_t status = BArchivable::Archive(archive, deep);

	if (status == B_OK && fSelected)
		status = archive->AddBool("_sel", true);
	
	if (status == B_OK && !fEnabled)
		status = archive->AddBool("_disable", true);

	if (status == B_OK && fExpanded)
		status = archive->AddBool("_li_expanded", true);

	if (status == B_OK && fLevel != 0)
		status = archive->AddInt32("_li_outline_level", fLevel);

	return status;
}


float
BListItem::Height() const
{
	return fHeight;
}


float
BListItem::Width() const
{
	return fWidth;
}


bool
BListItem::IsSelected() const
{
	return fSelected;
}


void
BListItem::Select()
{
	fSelected = true;
}


void
BListItem::Deselect()
{
	fSelected = false;
}


void
BListItem::SetEnabled(bool on)
{
	fEnabled = on;
}


bool
BListItem::IsEnabled() const
{
	return fEnabled;
}


void
BListItem::SetHeight(float height)
{
	fHeight = height;
}


void
BListItem::SetWidth(float width)
{
	fWidth = width;
}


void
BListItem::Update(BView *owner, const BFont *font)
{
	font_height fh;
	font->GetHeight(&fh);

	SetWidth(owner->Bounds().Width());
	SetHeight(fh.ascent + fh.descent + fh.leading);
}


status_t
BListItem::Perform(perform_code d, void *arg)
{
	return BArchivable::Perform(d, arg);
}


void
BListItem::SetExpanded(bool expanded)
{
	fExpanded = expanded;
}


bool
BListItem::IsExpanded() const
{
	return fExpanded;
}


uint32
BListItem::OutlineLevel() const
{
	return fLevel;
}


bool
BListItem::HasSubitems() const
{
	return fHasSubitems;
}


void BListItem::_ReservedListItem1() {}
void BListItem::_ReservedListItem2() {}


BListItem::BListItem(const BListItem &item)
{
}


BListItem &
BListItem::operator=(const BListItem &)
{
	return *this;
}


bool
BListItem::IsItemVisible() const
{
	return fVisible;
}


void
BListItem::SetItemVisible(bool visible)
{
	fVisible = visible;
}


// BStringItem
BStringItem::BStringItem(const char *text, uint32 level, bool expanded)
	:	BListItem(level, expanded),
		fText(NULL),
		fBaselineOffset(0)
{	
	SetText(text);
}


BStringItem::BStringItem(BMessage *archive)
	:	BListItem(archive),
		fText(NULL),
		fBaselineOffset(0)
{
	const char *string;

	if (archive->FindString("_label", &string) == B_OK)
		SetText(string);
}


BStringItem::~BStringItem()
{
	free(fText);
}


BArchivable	*
BStringItem::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BStringItem"))
		return new BStringItem(archive);
	else
		return NULL;
}


status_t
BStringItem::Archive(BMessage *archive, bool deep) const
{
	status_t status = BListItem::Archive(archive);

	if (status == B_OK && fText != NULL)
		status = archive->AddString("_label", fText);

	return status;
}


void
BStringItem::DrawItem(BView *owner, BRect frame, bool complete)
{
	if (fText == NULL)
		return;

	rgb_color highColor = owner->HighColor();
	rgb_color lowColor = owner->LowColor();

	if (IsSelected() || complete) {
		if (IsSelected())
			owner->SetHighColor(tint_color(lowColor, B_DARKEN_2_TINT));
		else
			owner->SetHighColor(lowColor);

		owner->FillRect(frame);
	}

	owner->MovePenTo(frame.left, frame.top + fBaselineOffset);

	rgb_color black = {0, 0, 0, 255};

	if (!IsEnabled())
		owner->SetHighColor(tint_color(black, B_LIGHTEN_2_TINT));
	else
		owner->SetHighColor(black);

	owner->DrawString(fText);

	owner->SetHighColor(highColor);
	owner->SetLowColor(lowColor);
}


void
BStringItem::SetText(const char *text)
{
	free(fText);
	fText = NULL;
	
	if (text)
		fText = strdup(text);
}


const char *
BStringItem::Text() const
{
	return fText;
}


void
BStringItem::Update(BView *owner, const BFont *font)
{
	if (fText)
		SetWidth(owner->StringWidth(fText));

	font_height fheight;
	font->GetHeight(&fheight);
	
	fBaselineOffset = fheight.ascent + fheight.leading;
	SetHeight((float)ceil(fheight.ascent + fheight.descent +
		fheight.leading) + 4);
}


status_t
BStringItem::Perform(perform_code d, void *arg)
{
	return BListItem::Perform(d, arg);
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
