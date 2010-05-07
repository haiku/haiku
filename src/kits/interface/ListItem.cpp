/*
 * Copyright 2001-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ulrich Wimboeck
 *		Marc Flerackers (mflerackers@androme.be)
 *		Rene Gollent
 */


#include <ListItem.h>

#include <Message.h>
#include <View.h>


BListItem::BListItem(uint32 level, bool expanded)
	:
	fTop(0.0),
	fTemporaryList(0),
	fWidth(0),
	fHeight(0),
	fLevel(level),
	fSelected(false),
	fEnabled(true),
	fExpanded(expanded),
	fHasSubitems(false),
	fVisible(true)
{
}


BListItem::BListItem(BMessage* data)
	: BArchivable(data),
	fTop(0.0),
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
BListItem::Archive(BMessage* archive, bool deep) const
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
BListItem::Update(BView* owner, const BFont* font)
{
	font_height fh;
	font->GetHeight(&fh);

	SetWidth(owner->Bounds().Width());
	SetHeight(ceilf(fh.ascent + fh.descent + fh.leading));
}


status_t
BListItem::Perform(perform_code d, void* arg)
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


void
BListItem::SetOutlineLevel(uint32 level)
{
	fLevel = level;
}


bool
BListItem::HasSubitems() const
{
	return fHasSubitems;
}


void BListItem::_ReservedListItem1() {}
void BListItem::_ReservedListItem2() {}


bool
BListItem::IsItemVisible() const
{
	return fVisible;
}


void
BListItem::SetTop(float top)
{
	fTop = top;
}


void
BListItem::SetItemVisible(bool visible)
{
	fVisible = visible;
}

