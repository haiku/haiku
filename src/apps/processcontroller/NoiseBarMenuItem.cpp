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


#include "NoiseBarMenuItem.h"
#include "Catalog.h"
#include "Colors.h"
#include "ProcessController.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProcessController"

NoiseBarMenuItem::NoiseBarMenuItem()
	: BMenuItem(B_TRANSLATE("Gone teams"B_UTF8_ELLIPSIS), NULL)
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
	frame.right -= 24;
	frame.left = frame.right - kBarWidth;
	frame.top += 5;
	frame.bottom = frame.top + 8;
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
		menu->SetHighColor(kBlack);
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
	menu->SetHighColor(kBlack);
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
