/*
 * Copyright 2000, Georges-Edouard Berenger. All rights reserved.
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "IconMenuItem.h"

#include <ControlLook.h>
#include <Bitmap.h>


IconMenuItem::IconMenuItem(BBitmap* icon, const char* title,
	BMessage* msg, bool drawText, bool purge)
	:
	BMenuItem(title, msg),
	fIcon(icon),
	fDrawText(drawText),
	fPurge(purge)
{
}


IconMenuItem::IconMenuItem(BBitmap* icon, BMenu* menu, bool drawText, bool purge)
	:
	BMenuItem(menu),
	fIcon(icon),
	fDrawText(drawText),
	fPurge(purge)
{
}


IconMenuItem::~IconMenuItem()
{
	if (fPurge)
		delete fIcon;
}


void
IconMenuItem::Reset(BBitmap* icon, bool purge)
{
	if (fPurge)
		delete fIcon;

	fPurge = purge;
	fIcon = icon;
}


void
IconMenuItem::DrawContent()
{
	DrawIcon();

	if (fDrawText) {
		BPoint loc = ContentLocation();
		loc.x += ceilf(be_control_look->DefaultLabelSpacing() * 3.3f);
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
	if (fIcon == NULL)
		return;

	BPoint loc = ContentLocation();
	BRect frame = Frame();

	loc.y = frame.top + (frame.bottom - frame.top - fIcon->Bounds().Height()) / 2;

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
	if (fIcon == NULL)
		return;

	const float limit = ceilf(fIcon->Bounds().Height() +
		(be_control_look->DefaultLabelSpacing() / 3.0f));
	if (*height < limit)
		*height = limit;
	if (fDrawText)
		*width += fIcon->Bounds().Width() + be_control_look->DefaultLabelSpacing();
	else
		*width = fIcon->Bounds().Width() + 1;
}
