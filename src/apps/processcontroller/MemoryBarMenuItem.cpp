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


#include "MemoryBarMenuItem.h"

#include "Colors.h"
#include "MemoryBarMenu.h"
#include "ProcessController.h"

#include <Bitmap.h>
#include <StringForSize.h>

#include <stdio.h>


MemoryBarMenuItem::MemoryBarMenuItem(const char *label, team_id team,
		BBitmap* icon, bool deleteIcon, BMessage* message)
	: BMenuItem(label, message),
	fTeamID(team),
	fIcon(icon),
	fDeleteIcon(deleteIcon)
{
	Init();
}


MemoryBarMenuItem::~MemoryBarMenuItem()
{
	if (fDeleteIcon)
		delete fIcon;
}


void
MemoryBarMenuItem::Init()
{
	fWriteMemory = -1;
	fAllMemory = -1;
	fGrenze1 = -1;
	fGrenze2 = -1;
	fLastCommited = -1;
	fLastWrite = -1;
	fLastAll = -1;
}


void
MemoryBarMenuItem::DrawContent()
{
	DrawIcon();
	if (fWriteMemory < 0)
		BarUpdate();
	else
		DrawBar(true);

	BPoint loc = ContentLocation();
	loc.x += 20;
	Menu()->MovePenTo(loc);
	BMenuItem::DrawContent();
}


void
MemoryBarMenuItem::DrawIcon()
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
MemoryBarMenuItem::DrawBar(bool force)
{
	// only draw anything if something has changed
	if (!force && fWriteMemory == fLastWrite && fAllMemory == fLastAll
		&& fCommitedMemory == fLastCommited)
		return;

	bool selected = IsSelected();
	BRect frame = Frame();
	BMenu* menu = Menu();

	// draw the bar itself

	BRect rect(frame.right - kMargin - kBarWidth, frame.top + 5,
		frame.right - kMargin, frame.top + 13);
	if (fWriteMemory < 0)
		return;

	if (fGrenze1 < 0)
		force = true;

	if (force) {
		if (selected)
			menu->SetHighColor(gFrameColorSelected);
		else
			menu->SetHighColor(gFrameColor);
		menu->StrokeRect(rect);
	}

	rect.InsetBy(1, 1);
	BRect r = rect;
	double grenze1 = rect.left + (rect.right - rect.left) * float(fWriteMemory) / fCommitedMemory;
	double grenze2 = rect.left + (rect.right - rect.left) * float(fAllMemory) / fCommitedMemory;
	if (grenze1 > rect.right)
		grenze1 = rect.right;
	if (grenze2 > rect.right)
		grenze2 = rect.right;
	r.right = grenze1;
	if (!force)
		r.left = fGrenze1;
	if (r.left < r.right) {
		if (selected)
			menu->SetHighColor(gKernelColorSelected);
		else
			menu->SetHighColor(gKernelColor);
		menu->FillRect(r);
	}

	r.left = grenze1;
	r.right = grenze2;

	if (!force) {
		if (fGrenze2 > r.left && r.left >= fGrenze1)
			r.left = fGrenze2;
		if (fGrenze1 < r.right && r.right  <= fGrenze2)
			r.right = fGrenze1;
	}

	if (r.left < r.right) {
		if (selected)
			menu->SetHighColor(gUserColorSelected);
		else
			menu->SetHighColor(gUserColor);
		menu->FillRect(r);
	}

	r.left = grenze2;
	r.right = rect.right;

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

	fLastCommited = fCommitedMemory;

	// Draw the values if necessary; if only fCommitedMemory changes, only
	// the bar might have to be updated

	if (!force && fWriteMemory == fLastWrite && fAllMemory == fLastAll)
		return;

	if (selected)
		menu->SetLowColor(gMenuBackColorSelected);
	else
		menu->SetLowColor(gMenuBackColor);

	BRect textRect(rect.left - kMargin - gMemoryTextWidth, frame.top,
		rect.left - kMargin, frame.bottom);
	menu->FillRect(textRect, B_SOLID_LOW);

	fLastWrite = fWriteMemory;
	fLastAll = fAllMemory;

	menu->SetHighColor(kBlack);

	char infos[128];
	string_for_size(fWriteMemory * 1024.0, infos, sizeof(infos));

	BPoint loc(rect.left - kMargin - gMemoryTextWidth / 2 - menu->StringWidth(infos),
		rect.bottom + 1);
	menu->DrawString(infos, loc);

	string_for_size(fAllMemory * 1024.0, infos, sizeof(infos));
	loc.x = rect.left - kMargin - menu->StringWidth(infos);
	menu->DrawString(infos, loc);
}


void
MemoryBarMenuItem::GetContentSize(float* _width, float* _height)
{
	BMenuItem::GetContentSize(_width, _height);
	if (*_height < 16)
		*_height = 16;
	*_width += 30 + kBarWidth + kMargin + gMemoryTextWidth;
}


int
MemoryBarMenuItem::UpdateSituation(int64 commitedMemory)
{
	fCommitedMemory = commitedMemory;
	BarUpdate();
	return fWriteMemory;
}


void
MemoryBarMenuItem::BarUpdate()
{
	area_info areaInfo;
	ssize_t cookie = 0;
	int64 lram_size = 0;
	int64 lwram_size = 0;
	bool exists = false;

	while (get_next_area_info(fTeamID, &cookie, &areaInfo) == B_OK) {
		exists = true;
		lram_size += areaInfo.ram_size;

		// TODO: this won't work this way anymore under Haiku!
//		int zone = (int (areaInfo.address) & 0xf0000000) >> 24;
		if ((areaInfo.protection & B_WRITE_AREA) != 0)
			lwram_size += areaInfo.ram_size;
//			&& (zone & 0xf0) != 0xA0			// Exclude media buffers
//			&& (fTeamID != gAppServerTeamID || zone != 0x90))	// Exclude app_server side of bitmaps
	}
	if (!exists) {
		team_info info;
		exists = get_team_info(fTeamID, &info) == B_OK;
	}
	if (exists) {
		fWriteMemory = lwram_size / 1024;
		fAllMemory = lram_size / 1024;
		DrawBar(false);
	} else
		fWriteMemory = -1;
}


void
MemoryBarMenuItem::Reset(char* name, team_id team, BBitmap* icon,
	bool deleteIcon)
{
	SetLabel(name);
	fTeamID = team;
	if (fDeleteIcon)
		delete fIcon;

	fDeleteIcon = deleteIcon;
	fIcon = icon;
	Init();
}
