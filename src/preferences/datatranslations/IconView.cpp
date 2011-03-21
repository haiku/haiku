// Author: Michael Wilber
// Copyright (C) Haiku, uses the MIT license


#include "IconView.h"

#include <stdio.h>
#include <string.h>

#include <Entry.h>
#include <Node.h>
#include <NodeInfo.h>


IconView::IconView(const BRect& frame, const char* name, uint32 resize,
	uint32 flags)
	:
	BView(frame, name, resize, flags),
	fIconBitmap(new BBitmap(BRect(B_LARGE_ICON), B_RGBA32)),
	fDrawIcon(false)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


IconView::~IconView()
{
	delete fIconBitmap;
	fIconBitmap = NULL;
}


bool
IconView::SetIcon(const BPath& path)
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
	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

	if (fDrawIcon)
		DrawBitmap(fIconBitmap);

	SetDrawingMode(B_OP_COPY);
}

