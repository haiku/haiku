/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <SpaceLayoutItem.h>


// constructor
BSpaceLayoutItem::BSpaceLayoutItem(BSize minSize, BSize maxSize,
	BSize preferredSize, BAlignment alignment)
	: fFrame(),
	  fMinSize(minSize),
	  fMaxSize(maxSize),
	  fPreferredSize(preferredSize),
	  fAlignment(alignment),
	  fVisible(true)
{
}

// destructor
BSpaceLayoutItem::~BSpaceLayoutItem()
{
}

// CreateGlue
BSpaceLayoutItem*
BSpaceLayoutItem::CreateGlue() {
	return new BSpaceLayoutItem(
		BSize(-1, -1),
		BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED),
		BSize(-1, -1),
		BAlignment(B_ALIGN_HORIZONTAL_CENTER, B_ALIGN_VERTICAL_CENTER));
}

// CreateHorizontalStrut
BSpaceLayoutItem*
BSpaceLayoutItem::CreateHorizontalStrut(float width) {
	return new BSpaceLayoutItem(
		BSize(width, -1),
		BSize(width, B_SIZE_UNLIMITED),
		BSize(width, -1),
		BAlignment(B_ALIGN_HORIZONTAL_CENTER, B_ALIGN_VERTICAL_CENTER));
}

// CreateVerticalStrut
BSpaceLayoutItem*
BSpaceLayoutItem::CreateVerticalStrut(float height) {
	return new BSpaceLayoutItem(
		BSize(-1, height),
		BSize(B_SIZE_UNLIMITED, height),
		BSize(-1, height),
		BAlignment(B_ALIGN_HORIZONTAL_CENTER, B_ALIGN_VERTICAL_CENTER));
}

// MinSize
BSize
BSpaceLayoutItem::MinSize()
{
	return fMinSize;
}

// MaxSize
BSize
BSpaceLayoutItem::MaxSize()
{
	return fMaxSize;
}

// PreferredSize
BSize
BSpaceLayoutItem::PreferredSize()
{
	return fPreferredSize;
}

// Alignment
BAlignment
BSpaceLayoutItem::Alignment()
{
	return fAlignment;
}

// SetExplicitMinSize
void
BSpaceLayoutItem::SetExplicitMinSize(BSize size)
{
	if (size.IsWidthSet())
		fMinSize.width = size.width;
	if (size.IsHeightSet())
		fMinSize.height = size.height;

	InvalidateLayout();
}

// SetExplicitMaxSize
void
BSpaceLayoutItem::SetExplicitMaxSize(BSize size)
{
	if (size.IsWidthSet())
		fMaxSize.width = size.width;
	if (size.IsHeightSet())
		fMaxSize.height = size.height;

	InvalidateLayout();
}

// SetExplicitPreferredSize
void
BSpaceLayoutItem::SetExplicitPreferredSize(BSize size)
{
	if (size.IsWidthSet())
		fPreferredSize.width = size.width;
	if (size.IsHeightSet())
		fPreferredSize.height = size.height;

	InvalidateLayout();
}

// SetExplicitAlignment
void
BSpaceLayoutItem::SetExplicitAlignment(BAlignment alignment)
{
	if (alignment.IsHorizontalSet())
		fAlignment.horizontal = alignment.horizontal;
	if (alignment.IsVerticalSet())
		fAlignment.vertical = alignment.vertical;

	InvalidateLayout();
}

// IsVisible
bool
BSpaceLayoutItem::IsVisible()
{
	return fVisible;
}

// SetVisible
void
BSpaceLayoutItem::SetVisible(bool visible)
{
	fVisible = visible;
}

// Frame
BRect
BSpaceLayoutItem::Frame()
{
	return fFrame;
}

// SetFrame
void
BSpaceLayoutItem::SetFrame(BRect frame)
{
	fFrame = frame;
}
