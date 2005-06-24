/*****************************************************************************/
// IconView
// IconView.cpp
// Author: Michael Wilber
//
//
// This BView based object displays an icon
//
//
// Copyright (C) Haiku, uses the MIT license
/*****************************************************************************/

#include <stdio.h>
#include <string.h>
#include "IconView.h"

IconView::IconView(const BRect &frame, const char *name,
	uint32 resize, uint32 flags)
	:	BView(frame, name, resize, flags)
{
	fDrawIcon = false;
	
	SetDrawingMode(B_OP_OVER);
		// to preserve transparent areas of the icon
		
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fIconBitmap = new BBitmap(BRect(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON - 1), B_CMAP8);
}

IconView::~IconView()
{
	delete fIconBitmap;
	fIconBitmap = NULL;
}

bool
IconView::SetIcon(const BPath &path)
{
	fDrawIcon = false;
	
	BEntry entry(path.Path());
	BNode node(&entry);
	BNodeInfo info(&node);
	
	if (info.GetTrackerIcon(fIconBitmap) != B_OK)
		return false;
	
	fDrawIcon = true;
	Invalidate();
	return true;
}

bool
IconView::DrawIcon(bool draw)
{
	bool prev = fDrawIcon;
	fDrawIcon = draw;
	if (prev != fDrawIcon)
		Invalidate();

	return prev;
}

void
IconView::Draw(BRect area)
{
	if (fDrawIcon)
		DrawBitmap(fIconBitmap);
}

