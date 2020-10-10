/*
 * Copyright 2000, Georges-Edouard Berenger. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "KernelMemoryBarMenuItem.h"

#include "Colors.h"
#include "MemoryBarMenu.h"
#include "ProcessController.h"

#include <stdio.h>

#include <Catalog.h>
#include <StringForSize.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProcessController"


KernelMemoryBarMenuItem::KernelMemoryBarMenuItem(system_info& systemInfo)
	: BMenuItem(B_TRANSLATE("System resources & caches" B_UTF8_ELLIPSIS), NULL)
{
	fLastSum = -1;
	fGrenze1 = -1;
	fGrenze2 = -1;
	fPhysicalMemory = (int64)systemInfo.max_pages * B_PAGE_SIZE / 1024LL;
	fCommittedMemory = (int64)systemInfo.used_pages * B_PAGE_SIZE / 1024LL;
	fCachedMemory = (int64)systemInfo.cached_pages * B_PAGE_SIZE / 1024LL;
}


void
KernelMemoryBarMenuItem::DrawContent()
{
	DrawBar(true);
	Menu()->MovePenTo(ContentLocation());
	BMenuItem::DrawContent();
}


void
KernelMemoryBarMenuItem::UpdateSituation(int64 committedMemory,
	int64 cachedMemory)
{
	fCommittedMemory = committedMemory;
	fCachedMemory = cachedMemory;
	DrawBar(false);
}


void
KernelMemoryBarMenuItem::DrawBar(bool force)
{
	bool selected = IsSelected();
	BRect frame = Frame();
	BMenu* menu = Menu();
	rgb_color highColor = menu->HighColor();

	BFont font;
	menu->GetFont(&font);
	BRect cadre = bar_rect(frame, &font);

	// draw the bar itself
	if (fLastSum < 0)
		force = true;
	if (force) {
		if (selected)
			menu->SetHighColor(gFrameColorSelected);
		else
			menu->SetHighColor(gFrameColor);
		menu->StrokeRect (cadre);
	}
	cadre.InsetBy(1, 1);
	BRect r = cadre;

	double grenze1 = cadre.left + (cadre.right - cadre.left)
						* fCachedMemory / fPhysicalMemory;
	double grenze2 = cadre.left + (cadre.right - cadre.left)
						* fCommittedMemory / fPhysicalMemory;
	if (grenze1 > cadre.right)
		grenze1 = cadre.right;
	if (grenze2 > cadre.right)
		grenze2 = cadre.right;
	r.right = grenze1;
	if (!force)
		r.left = fGrenze1;
	if (r.left < r.right) {
		if (selected)
			menu->SetHighColor(gKernelColorSelected);
		else
			menu->SetHighColor(gKernelColor);
		menu->FillRect (r);
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
		if (selected)
			menu->SetHighColor(tint_color (kLavender, B_HIGHLIGHT_BACKGROUND_TINT));
		else
			menu->SetHighColor(kLavender);
		menu->FillRect (r);
	}
	r.left = grenze2;
	r.right = cadre.right;
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

	// draw the value
	double sum = fCachedMemory * FLT_MAX + fCommittedMemory;
	if (force || sum != fLastSum) {
		if (selected) {
			menu->SetLowColor(gMenuBackColorSelected);
			menu->SetHighColor(gMenuBackColorSelected);
		} else {
			menu->SetLowColor(gMenuBackColor);
			menu->SetHighColor(gMenuBackColor);
		}
		BRect trect(cadre.left - kMargin - gMemoryTextWidth, frame.top,
			cadre.left - kMargin, frame.bottom);
		menu->FillRect(trect);
		menu->SetHighColor(highColor);

		char infos[128];
		string_for_size(fCachedMemory * 1024.0, infos, sizeof(infos));
		BPoint loc(cadre.left, cadre.bottom + 1);
		loc.x -= kMargin + gMemoryTextWidth / 2 + menu->StringWidth(infos);
		menu->DrawString(infos, loc);
		string_for_size(fCommittedMemory * 1024.0, infos, sizeof(infos));
		loc.x = cadre.left - kMargin - menu->StringWidth(infos);
		menu->DrawString(infos, loc);
		fLastSum = sum;
	}
}


void
KernelMemoryBarMenuItem::GetContentSize(float* _width, float* _height)
{
	BMenuItem::GetContentSize(_width, _height);
	if (*_height < 16)
		*_height = 16;

	*_width += 20 + kBarWidth + kMargin + gMemoryTextWidth;
}

