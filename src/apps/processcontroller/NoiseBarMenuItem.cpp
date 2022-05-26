/*
 * Copyright 2000, Georges-Edouard Berenger. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "NoiseBarMenuItem.h"
#include "Catalog.h"
#include "Colors.h"
#include "ProcessController.h"
#include "Utilities.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProcessController"

NoiseBarMenuItem::NoiseBarMenuItem()
	: BMenuItem(B_TRANSLATE("Gone teams" B_UTF8_ELLIPSIS), NULL)
{
	fBusyWaiting = -1;
	fLost = -1;
	fGrenze1 = -1;
	fGrenze2 = -1;
}


void
NoiseBarMenuItem::DrawContent()
{
	DrawBar(true);
	Menu()->MovePenTo(ContentLocation());
	BMenuItem::DrawContent();
}


void
NoiseBarMenuItem::DrawBar(bool force)
{
	bool selected = IsSelected();
	BRect frame = Frame();
	BMenu* menu = Menu();
	rgb_color highColor = menu->HighColor();

	BFont font;
	menu->GetFont(&font);
	frame = bar_rect(frame, &font);

	if (fBusyWaiting < 0)
		return;

	if (fGrenze1 < 0)
		force = true;

	if (force) {
		if (selected)
			menu->SetHighColor(gFrameColorSelected);
		else
			menu->SetHighColor(gFrameColor);
		menu->StrokeRect(frame);
	}

	frame.InsetBy(1, 1);
	BRect r = frame;
	float grenze1 = frame.left+(frame.right-frame.left)*fBusyWaiting;
	float grenze2 = frame.left+(frame.right-frame.left)*(fBusyWaiting+fLost);
	if (grenze1 > frame.right)
		grenze1 = frame.right;
	if (grenze2 > frame.right)
		grenze2 = frame.right;
	r.right = grenze1;
	if (!force)
		r.left = fGrenze1;
	if (r.left < r.right) {
		if (selected)
			menu->SetHighColor(tint_color (kGreen, B_HIGHLIGHT_BACKGROUND_TINT));
		else
			menu->SetHighColor(kGreen);
//		menu->SetHighColor(gKernelColor);
		menu->FillRect(r);
	}
	r.left = grenze1;
	r.right = grenze2;
	if (!force) {
		if (fGrenze2 > r.left && r.left >= fGrenze1)
			r.left = fGrenze2;
		if (fGrenze1 < r.right && r.right <= fGrenze2)
			r.right = fGrenze1;
	}
	if (r.left < r.right) {
		menu->SetHighColor(highColor);
//		menu->SetHighColor(gUserColor);
		menu->FillRect(r);
	}
	r.left = grenze2;
	r.right = frame.right;
	if (!force)
		r.right = fGrenze2;
	if (r.left < r.right) {
		if (selected)
			menu->SetHighColor(gWhiteSelected);
		else
			menu->SetHighColor(kWhite);
		menu->FillRect(r);
	}
	menu->SetHighColor(highColor);
	fGrenze1 = grenze1;
	fGrenze2 = grenze2;
}


void
NoiseBarMenuItem::GetContentSize(float* width, float* height)
{
	BMenuItem::GetContentSize(width, height);
	if (*height < 16)
		*height = 16;
	*width += 20 + kBarWidth;
}
