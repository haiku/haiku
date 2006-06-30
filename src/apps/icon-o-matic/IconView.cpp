/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "IconView.h"

#include <Bitmap.h>

#include "ui_defines.h"

#include "IconRenderer.h"

// constructor
IconView::IconView(BRect frame, const char* name)
	: BView(frame, name, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW),
	  fBitmap(new BBitmap(frame.OffsetToCopy(B_ORIGIN), 0, B_RGB32)),
	  fIcon(NULL),
	  fRenderer(new IconRenderer(fBitmap)),
	  fDirtyIconArea(fBitmap->Bounds()),

	  fScale((frame.Width() + 1.0) / 64.0)
{
	fRenderer->SetScale(fScale);
}

// destructor
IconView::~IconView()
{
	SetIcon(NULL);
	delete fRenderer;
	delete fBitmap;
}

// #pragma mark -

// AttachedToWindow
void
IconView::AttachedToWindow()
{
	SetViewColor(B_TRANSPARENT_COLOR);
}

// Draw
void
IconView::Draw(BRect updateRect)
{
	if (fDirtyIconArea.IsValid()) {
		fRenderer->Render(fDirtyIconArea);
		fDirtyIconArea.Set(LONG_MAX, LONG_MAX, LONG_MIN, LONG_MIN);
	}

	// icon
	DrawBitmap(fBitmap, B_ORIGIN);
}

// #pragma mark -

// AreaInvalidated
void
IconView::AreaInvalidated(const BRect& area)
{
	BRect scaledArea(area);
	scaledArea.left *= fScale;
	scaledArea.top *= fScale;
	scaledArea.right *= fScale;
	scaledArea.bottom *= fScale;

	if (fDirtyIconArea.Contains(scaledArea))
		return;

	fDirtyIconArea = fDirtyIconArea | scaledArea;

	Invalidate(scaledArea);
}


// #pragma mark -

// SetIcon
void
IconView::SetIcon(Icon* icon)
{
	if (fIcon == icon)
		return;

	if (fIcon)
		fIcon->RemoveListener(this);

	fIcon = icon;
	fRenderer->SetIcon(icon);

	if (fIcon)
		fIcon->AddListener(this);
}

