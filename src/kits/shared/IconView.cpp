/*
 * Copyright 2004-2020, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 *		Michael Wilber
 */


#include "IconView.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Entry.h>
#include <IconUtils.h>
#include <Node.h>
#include <NodeInfo.h>


using std::nothrow;


IconView::IconView(icon_size iconSize)
	:
	BView("IconView", B_WILL_DRAW),
	fIconSize(iconSize),
	fIconBitmap(new BBitmap(BRect(iconSize), B_RGBA32)),
	fDrawIcon(false)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
}


IconView::~IconView()
{
	delete fIconBitmap;
	fIconBitmap = NULL;
}


status_t
IconView::SetIcon(const BPath& path, icon_size iconSize)
{
	fDrawIcon = false;
	
	if (iconSize != fIconSize) {
		BBitmap* bitmap = new BBitmap(BRect(iconSize), B_RGBA32);
		if (bitmap == NULL)
			return B_NO_MEMORY;

		delete fIconBitmap;
		fIconBitmap = bitmap;
		fIconSize = iconSize;
	}

	status_t status = fIconBitmap->InitCheck();
	if (status != B_OK)
		return status;

	BEntry entry(path.Path());
	BNode node(&entry);
	BNodeInfo info(&node);

	status = info.GetTrackerIcon(fIconBitmap, fIconSize);
	if (status != B_OK)
		return status;

	if (!fIconBitmap->IsValid())
		return fIconBitmap->InitCheck();

	_SetSize();

	fDrawIcon = true;
	Invalidate();
	return B_OK;
}


status_t
IconView::SetIcon(const uint8_t* data, size_t size, icon_size iconSize)
{
	fDrawIcon = false;
	
	if (iconSize != fIconSize) {
		BBitmap* bitmap = new BBitmap(BRect(iconSize), B_RGBA32);
		if (bitmap == NULL)
			return B_NO_MEMORY;

		delete fIconBitmap;
		fIconBitmap = bitmap;
		fIconSize = iconSize;
	}

	status_t status = fIconBitmap->InitCheck();
	if (status != B_OK)
		return status;

	status = BIconUtils::GetVectorIcon(data, size, fIconBitmap);
	if (status != B_OK)
		return status;

	if (!fIconBitmap->IsValid())
		return fIconBitmap->InitCheck();

	_SetSize();

	fDrawIcon = true;
	Invalidate();
	return B_OK;
}


void
IconView::DrawIcon(bool draw)
{
	if (draw == fDrawIcon)
		return;

	fDrawIcon = draw;
	Invalidate();
}


void
IconView::Draw(BRect area)
{
	if (fDrawIcon && fIconBitmap != NULL) {
		SetDrawingMode(B_OP_ALPHA);
		SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		DrawBitmap(fIconBitmap);
		SetDrawingMode(B_OP_COPY);
	} else
		BView::Draw(area);
}


status_t
IconView::InitCheck() const
{
	if (fIconBitmap == NULL)
		return B_NO_MEMORY;

	return fIconBitmap->InitCheck();
}


void
IconView::_SetSize()
{
	SetExplicitMinSize(BSize(fIconSize, fIconSize));
	SetExplicitMaxSize(BSize(fIconSize, fIconSize));
	SetExplicitPreferredSize(BSize(fIconSize, fIconSize));
}
