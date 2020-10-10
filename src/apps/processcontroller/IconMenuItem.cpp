/*
 * Copyright 2000, Georges-Edouard Berenger. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "IconMenuItem.h"
#include <Application.h>
#include <NodeInfo.h>
#include <Bitmap.h>
#include <Roster.h>


IconMenuItem::IconMenuItem(BBitmap* icon, const char* title,
	BMessage* msg, bool drawText, bool purge)
	: BMenuItem(title, msg),
	fIcon(icon),
	fDrawText(drawText),
	fPurge(purge)
{
	if (!fIcon)
		DefaultIcon(NULL);
}


IconMenuItem::IconMenuItem(BBitmap* icon, BMenu* menu, bool drawText, bool purge)
	: BMenuItem(menu),
	fIcon(icon),
	fDrawText(drawText),
	fPurge(purge)

{
	if (!fIcon)
		DefaultIcon(NULL);
}


IconMenuItem::IconMenuItem(const char* mime, const char* title, BMessage* msg, bool drawText)
	: BMenuItem(title, msg),
	fIcon(NULL),
	fDrawText(drawText)
{
	DefaultIcon(mime);
}


IconMenuItem::~IconMenuItem()
{
	if (fPurge && fIcon)
		delete fIcon;
}


void IconMenuItem::DrawContent()
{
	BPoint	loc;

	DrawIcon();
	if (fDrawText) {
		loc = ContentLocation();
		loc.x += 20;
		Menu()->MovePenTo(loc);
		BMenuItem::DrawContent();
	}
}


void
IconMenuItem::Highlight(bool hilited)
{
	BMenuItem::Highlight(hilited);
	DrawIcon();
}


void
IconMenuItem::DrawIcon()
{
	// TODO: exact code duplication with TeamBarMenuItem::DrawIcon()
	if (!fIcon)
		return;

	BPoint loc = ContentLocation();
	BRect frame = Frame();

	loc.y = frame.top + (frame.bottom - frame.top - 15) / 2;

	BMenu* menu = Menu();

	if (fIcon->ColorSpace() == B_RGBA32) {
		menu->SetDrawingMode(B_OP_ALPHA);
		menu->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	} else
		menu->SetDrawingMode(B_OP_OVER);

	menu->DrawBitmap(fIcon, loc);

	menu->SetDrawingMode(B_OP_COPY);
}


void
IconMenuItem::GetContentSize(float* width, float* height)
{
	BMenuItem::GetContentSize(width, height);
	int	limit = IconMenuItem::MinHeight();
	if (*height < limit)
		*height = limit;
	if (fDrawText)
		*width += 20;
	else
		*width = 16;
}


void
IconMenuItem::DefaultIcon(const char* mime)
{
	BRect rect(0, 0, 15, 15);
	fIcon = new BBitmap(rect, B_COLOR_8_BIT);
	if (mime) {
		BMimeType mimeType(mime);
		if (mimeType.GetIcon(fIcon, B_MINI_ICON) != B_OK)
			fDrawText = true;
	} else {
		app_info info;
		be_app->GetAppInfo(&info);
		if (BNodeInfo::GetTrackerIcon(&info.ref, fIcon, B_MINI_ICON) != B_OK)
			fDrawText = true;
	}
	fPurge = true;
}


int IconMenuItem::MinHeight()
{
	static int	minheight = -1;
	if (minheight < 0)
		minheight = 17;
	return minheight;
}
