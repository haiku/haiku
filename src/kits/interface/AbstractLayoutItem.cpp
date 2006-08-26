/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <AbstractLayoutItem.h>

#include <LayoutUtils.h>


// constructor
BAbstractLayoutItem::BAbstractLayoutItem()
	: fMinSize(),
	  fMaxSize(),
	  fPreferredSize(),
	  fAlignment()
{
}

// destructor
BAbstractLayoutItem::~BAbstractLayoutItem()
{
}

// MinSize
BSize
BAbstractLayoutItem::MinSize()
{
	return BLayoutUtils::ComposeSize(fMinSize, BaseMinSize());
}

// MaxSize
BSize
BAbstractLayoutItem::MaxSize()
{
	return BLayoutUtils::ComposeSize(fMaxSize, BaseMaxSize());
}

// PreferredSize
BSize
BAbstractLayoutItem::PreferredSize()
{
	return BLayoutUtils::ComposeSize(fMaxSize, BaseMaxSize());
}

// Alignment
BAlignment
BAbstractLayoutItem::Alignment()
{
	return BLayoutUtils::ComposeAlignment(fAlignment, BaseAlignment());
}

// SetExplicitMinSize
void
BAbstractLayoutItem::SetExplicitMinSize(BSize size)
{
	fMinSize = size;
}

// SetExplicitMaxSize
void
BAbstractLayoutItem::SetExplicitMaxSize(BSize size)
{
	fMaxSize = size;
}

// SetExplicitPreferredSize
void
BAbstractLayoutItem::SetExplicitPreferredSize(BSize size)
{
	fPreferredSize = size;
}

// SetExplicitAlignment
void
BAbstractLayoutItem::SetExplicitAlignment(BAlignment alignment)
{
	fAlignment = alignment;
}

// BaseMinSize
BSize
BAbstractLayoutItem::BaseMinSize()
{
	return BSize(0, 0);
}

// BaseMaxSize
BSize
BAbstractLayoutItem::BaseMaxSize()
{
	return BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
}

// BasePreferredSize
BSize
BAbstractLayoutItem::BasePreferredSize()
{
	return BSize(0, 0);
}

// BaseAlignment
BAlignment
BAbstractLayoutItem::BaseAlignment()
{
	return BAlignment(B_ALIGN_HORIZONTAL_CENTER, B_ALIGN_VERTICAL_CENTER);
}
