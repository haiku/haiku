/*
	ProcessController © 2000, Georges-Edouard Berenger, All Rights Reserved.
	Copyright (C) 2004 beunited.org

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
	fPhysicalMemory = float(int(systemInfo.max_pages * B_PAGE_SIZE / 1024));
	fCommittedMemory = float(int(systemInfo.used_pages * B_PAGE_SIZE / 1024));
	fCachedMemory = float(int(systemInfo.cached_pages * B_PAGE_SIZE / 1024));
}


void
KernelMemoryBarMenuItem::DrawContent()
{
	DrawBar(true);
	Menu()->MovePenTo(ContentLocation());
	BMenuItem::DrawContent();
}


void
KernelMemoryBarMenuItem::UpdateSituation(float committedMemory,
	float cachedMemory)
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

	// draw the bar itself
	BRect cadre (frame.right - kMargin - kBarWidth, frame.top + 5,
		frame.right - kMargin, frame.top + 13);

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

	float grenze1 = cadre.left + (cadre.right - cadre.left)
						* fCachedMemory / fPhysicalMemory;
	float grenze2 = cadre.left + (cadre.right - cadre.left)
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
//		menu->SetHighColor(gKernelColor);
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
//		menu->SetHighColor(gUserColor);
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
	menu->SetHighColor(kBlack);
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
		menu->SetHighColor(kBlack);

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

