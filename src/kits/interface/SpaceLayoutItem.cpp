/*
 * Copyright 2010, Haiku, Inc.
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <SpaceLayoutItem.h>

#include <new>

#include <Message.h>


namespace {
	const char* const kSizesField = "BSpaceLayoutItem:sizes";
		// kSizesField = {min, max, preferred}
	const char* const kAlignmentField = "BSpaceLayoutItem:alignment";
	const char* const kFrameField = "BSpaceLayoutItem:frame";
	const char* const kVisibleField = "BSpaceLayoutItem:visible";
}


BSpaceLayoutItem::BSpaceLayoutItem(BSize minSize, BSize maxSize,
	BSize preferredSize, BAlignment alignment)
	:
	fFrame(),
	fMinSize(minSize),
	fMaxSize(maxSize),
	fPreferredSize(preferredSize),
	fAlignment(alignment),
	fVisible(true)
{
}


BSpaceLayoutItem::BSpaceLayoutItem(BMessage* archive)
	:
	BLayoutItem(archive)
{
	archive->FindSize(kSizesField, 0, &fMinSize);
	archive->FindSize(kSizesField, 1, &fMaxSize);
	archive->FindSize(kSizesField, 2, &fPreferredSize);

	archive->FindAlignment(kAlignmentField, &fAlignment);

	archive->FindRect(kFrameField, &fFrame);
	archive->FindBool(kVisibleField, &fVisible);
}


BSpaceLayoutItem::~BSpaceLayoutItem()
{
}


BSpaceLayoutItem*
BSpaceLayoutItem::CreateGlue()
{
	return new BSpaceLayoutItem(
		BSize(-1, -1),
		BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED),
		BSize(-1, -1),
		BAlignment(B_ALIGN_HORIZONTAL_CENTER, B_ALIGN_VERTICAL_CENTER));
}


BSpaceLayoutItem*
BSpaceLayoutItem::CreateHorizontalStrut(float width)
{
	return new BSpaceLayoutItem(
		BSize(width, -1),
		BSize(width, B_SIZE_UNLIMITED),
		BSize(width, -1),
		BAlignment(B_ALIGN_HORIZONTAL_CENTER, B_ALIGN_VERTICAL_CENTER));
}


BSpaceLayoutItem*
BSpaceLayoutItem::CreateVerticalStrut(float height)
{
	return new BSpaceLayoutItem(
		BSize(-1, height),
		BSize(B_SIZE_UNLIMITED, height),
		BSize(-1, height),
		BAlignment(B_ALIGN_HORIZONTAL_CENTER, B_ALIGN_VERTICAL_CENTER));
}


BSize
BSpaceLayoutItem::MinSize()
{
	return fMinSize;
}


BSize
BSpaceLayoutItem::MaxSize()
{
	return fMaxSize;
}


BSize
BSpaceLayoutItem::PreferredSize()
{
	return fPreferredSize;
}


BAlignment
BSpaceLayoutItem::Alignment()
{
	return fAlignment;
}


void
BSpaceLayoutItem::SetExplicitMinSize(BSize size)
{
	if (size.IsWidthSet())
		fMinSize.width = size.width;
	if (size.IsHeightSet())
		fMinSize.height = size.height;

	InvalidateLayout();
}


void
BSpaceLayoutItem::SetExplicitMaxSize(BSize size)
{
	if (size.IsWidthSet())
		fMaxSize.width = size.width;
	if (size.IsHeightSet())
		fMaxSize.height = size.height;

	InvalidateLayout();
}


void
BSpaceLayoutItem::SetExplicitPreferredSize(BSize size)
{
	if (size.IsWidthSet())
		fPreferredSize.width = size.width;
	if (size.IsHeightSet())
		fPreferredSize.height = size.height;

	InvalidateLayout();
}


void
BSpaceLayoutItem::SetExplicitAlignment(BAlignment alignment)
{
	if (alignment.IsHorizontalSet())
		fAlignment.horizontal = alignment.horizontal;
	if (alignment.IsVerticalSet())
		fAlignment.vertical = alignment.vertical;

	InvalidateLayout();
}


bool
BSpaceLayoutItem::IsVisible()
{
	return fVisible;
}


void
BSpaceLayoutItem::SetVisible(bool visible)
{
	fVisible = visible;
}


BRect
BSpaceLayoutItem::Frame()
{
	return fFrame;
}


void
BSpaceLayoutItem::SetFrame(BRect frame)
{
	fFrame = frame;
}


status_t
BSpaceLayoutItem::Archive(BMessage* into, bool deep) const
{
	status_t err = BLayoutItem::Archive(into, deep);

	if (err == B_OK)
		err = into->AddRect(kFrameField, fFrame);

	if (err == B_OK)
		err = into->AddSize(kSizesField, fMinSize);

	if (err == B_OK)
		err = into->AddSize(kSizesField, fMaxSize);

	if (err == B_OK)
		err = into->AddSize(kSizesField, fPreferredSize);

	if (err == B_OK)
		err = into->AddAlignment(kAlignmentField, fAlignment);

	if (err == B_OK)
		err = into->AddBool(kVisibleField, fVisible);

	return err;
}


BArchivable*
BSpaceLayoutItem::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "BSpaceLayoutItem"))
		return new(std::nothrow) BSpaceLayoutItem(from);
	return NULL;
}


void BSpaceLayoutItem::_ReservedSpaceLayoutItem1() {}
void BSpaceLayoutItem::_ReservedSpaceLayoutItem2() {}
void BSpaceLayoutItem::_ReservedSpaceLayoutItem3() {}
void BSpaceLayoutItem::_ReservedSpaceLayoutItem4() {}
void BSpaceLayoutItem::_ReservedSpaceLayoutItem5() {}
void BSpaceLayoutItem::_ReservedSpaceLayoutItem6() {}
void BSpaceLayoutItem::_ReservedSpaceLayoutItem7() {}
void BSpaceLayoutItem::_ReservedSpaceLayoutItem8() {}
void BSpaceLayoutItem::_ReservedSpaceLayoutItem9() {}
void BSpaceLayoutItem::_ReservedSpaceLayoutItem10() {}

