/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "ViewLayoutItem.h"

#include <View.h>


// constructor
BViewLayoutItem::BViewLayoutItem(BView* view)
	: fView(view)
{
}

// destructor
BViewLayoutItem::~BViewLayoutItem()
{
}

// MinSize
BSize
BViewLayoutItem::MinSize()
{
	return fView->MinSize();
}

// MaxSize
BSize
BViewLayoutItem::MaxSize()
{
	return fView->MaxSize();
}

// PreferredSize
BSize
BViewLayoutItem::PreferredSize()
{
	return fView->PreferredSize();
}

// Alignment
BAlignment
BViewLayoutItem::Alignment()
{
	return fView->Alignment();
}

// SetExplicitMinSize
void
BViewLayoutItem::SetExplicitMinSize(BSize size)
{
	fView->SetExplicitMinSize(size);
}

// SetExplicitMaxSize
void
BViewLayoutItem::SetExplicitMaxSize(BSize size)
{
	fView->SetExplicitMaxSize(size);
}

// SetExplicitPreferredSize
void
BViewLayoutItem::SetExplicitPreferredSize(BSize size)
{
	fView->SetExplicitPreferredSize(size);
}

// SetExplicitAlignment
void
BViewLayoutItem::SetExplicitAlignment(BAlignment alignment)
{
	fView->SetExplicitAlignment(alignment);
}

// IsVisible
bool
BViewLayoutItem::IsVisible()
{
	return !fView->IsHidden(fView);
}

// SetVisible
void
BViewLayoutItem::SetVisible(bool visible)
{
	if (visible != IsVisible()) {
		if (visible)
			fView->Show();
		else
			fView->Hide();
	}
}

// Frame
BRect
BViewLayoutItem::Frame()
{
	return fView->Frame();
}

// SetFrame
void
BViewLayoutItem::SetFrame(BRect frame)
{
	fView->MoveTo(frame.LeftTop());
	fView->ResizeTo(frame.Width(), frame.Height());
}

// HasHeightForWidth
bool
BViewLayoutItem::HasHeightForWidth()
{
	return fView->HasHeightForWidth();
}

// GetHeightForWidth
void
BViewLayoutItem::GetHeightForWidth(float width, float* min, float* max,
	float* preferred)
{
	fView->GetHeightForWidth(width, min, max, preferred);
}

// View
BView*
BViewLayoutItem::View()
{
	return fView;
}

// InvalidateLayout
void
BViewLayoutItem::InvalidateLayout()
{
	fView->InvalidateLayout();
}
