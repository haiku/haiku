//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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
//	Description:	BListView represents a one-dimensional list view.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdlib.h>
#include <string.h>

// System Includes -------------------------------------------------------------
#include <ListItem.h>
#include <View.h>
#include <Message.h>
#include <Errors.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
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
//------------------------------------------------------------------------------
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
//------------------------------------------------------------------------------
BListItem::~BListItem()
{

}
//------------------------------------------------------------------------------
status_t BListItem::Archive(BMessage *archive, bool deep) const
{
	BArchivable::Archive(archive, deep);

	if (fSelected)
		archive->AddBool("_sel", true);
	
	if (!fEnabled)
		archive->AddBool("_disable", true);

	if (fExpanded)
		archive->AddBool("_li_expanded", true);

	if (fLevel != 0)
		archive->AddInt32("_li_outline_level", fLevel);

	return B_OK;
}
//------------------------------------------------------------------------------
float BListItem::Height() const
{
	return fHeight;
}
//------------------------------------------------------------------------------
float BListItem::Width() const
{
	return fWidth;
}
//------------------------------------------------------------------------------
bool BListItem::IsSelected() const
{
	return fSelected;
}
//------------------------------------------------------------------------------
void BListItem::Select()
{
	fSelected = true;
}
//------------------------------------------------------------------------------
void BListItem::Deselect()
{
	fSelected = false;
}
//------------------------------------------------------------------------------
void BListItem::SetEnabled(bool on)
{
	fEnabled = on;
}
//------------------------------------------------------------------------------
bool BListItem::IsEnabled() const
{
	return fEnabled;
}
//------------------------------------------------------------------------------
void BListItem::SetHeight(float height)
{
	fHeight = height;
}
//------------------------------------------------------------------------------
void BListItem::SetWidth(float width)
{
	fWidth = width;
}
//------------------------------------------------------------------------------
void BListItem::Update(BView *owner, const BFont *font)
{
	font_height fh;
	font->GetHeight(&fh);

	SetWidth(owner->Bounds().Width());
	SetHeight(fh.ascent + fh.descent + fh.leading);
}
//------------------------------------------------------------------------------
status_t BListItem::Perform(perform_code d, void *arg)
{
	return BArchivable::Perform(d, arg);
}
//------------------------------------------------------------------------------
void BListItem::SetExpanded(bool expanded)
{
	fExpanded = expanded;
}
//------------------------------------------------------------------------------
bool BListItem::IsExpanded() const
{
	return fExpanded;
}
//------------------------------------------------------------------------------
uint32 BListItem::OutlineLevel() const
{
	return fLevel;
}
//------------------------------------------------------------------------------
bool BListItem::HasSubitems() const
{
	return fHasSubitems;
}
//------------------------------------------------------------------------------
void BListItem::_ReservedListItem1() {}
void BListItem::_ReservedListItem2() {}
//------------------------------------------------------------------------------
BListItem::BListItem(const BListItem &item)
{
}
//------------------------------------------------------------------------------
BListItem &BListItem::operator=(const BListItem &)
{
	return *this;
}
//------------------------------------------------------------------------------
bool BListItem::IsItemVisible() const
{
	return fVisible;
}
//------------------------------------------------------------------------------
void BListItem::SetItemVisible(bool visible)
{
	fVisible = visible;
}
//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
BStringItem::BStringItem(const char *text, uint32 level, bool expanded)
	:	BListItem(level, expanded),
		fText(NULL),
		fBaselineOffset(0)
{	
	SetText(text);
}
//------------------------------------------------------------------------------
BStringItem::BStringItem(BMessage *archive)
	:	BListItem(archive),
		fText(NULL),
		fBaselineOffset(0)
{
	const char *string;

	if (archive->FindString("_label", &string) == B_OK)
		SetText(string);
}
//------------------------------------------------------------------------------
BStringItem::~BStringItem()
{
	if (fText)
		free(fText);
}
//------------------------------------------------------------------------------
BArchivable	*BStringItem::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BStringItem"))
		return new BStringItem(archive);
	else
		return NULL;
}
//------------------------------------------------------------------------------
status_t BStringItem::Archive(BMessage *archive, bool deep) const
{
	BListItem::Archive(archive);

	if (fText)
		archive->AddString("_label", fText);

	return B_OK;
}
//------------------------------------------------------------------------------
void BStringItem::DrawItem(BView *owner, BRect frame, bool complete)
{
	if (fText == NULL)
		return;

	rgb_color highColor = owner->HighColor();
	rgb_color lowColor = owner->LowColor();

	if (IsSelected() || complete)
	{
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
//------------------------------------------------------------------------------
void BStringItem::SetText(const char *text)
{
	if (fText)
	{
		free(fText);
		fText = NULL;
	}

	if (text)
		fText = strdup(text);
}
//------------------------------------------------------------------------------
const char *BStringItem::Text() const
{
	return fText;
}
//------------------------------------------------------------------------------
void BStringItem::Update(BView *owner, const BFont *font)
{
	if (fText)
		SetWidth(owner->StringWidth(fText));

	font_height fheight;

	font->GetHeight(&fheight);
	
	fBaselineOffset = fheight.ascent + fheight.leading;
	SetHeight((float)ceil(fheight.ascent + fheight.descent +
		fheight.leading) + 4);
}
//------------------------------------------------------------------------------
status_t BStringItem::Perform(perform_code d, void *arg)
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
void BStringItem::_ReservedStringItem1() {}
void BStringItem::_ReservedStringItem2() {}
//------------------------------------------------------------------------------
BStringItem::BStringItem(const BStringItem &)
{
}
//------------------------------------------------------------------------------
BStringItem	&BStringItem::operator=(const BStringItem &)
{
	return *this;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */
