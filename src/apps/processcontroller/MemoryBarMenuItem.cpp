/*
 * Copyright 2000, Georges-Edouard Berenger. All rights reserved.
 * Distributed under the terms of the MIT License.
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
	fLastCommitted = -1;
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
		&& fCommittedMemory == fLastCommitted)
		return;

	bool selected = IsSelected();
	BRect frame = Frame();
	BMenu* menu = Menu();
	rgb_color highColor = menu->HighColor();

	BFont font;
	menu->GetFont(&font);
	BRect rect = bar_rect(frame, &font);

	// draw the bar itself
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
	double grenze1 = rect.left + (rect.right - rect.left) * float(fWriteMemory)
		/ fCommittedMemory;
	double grenze2 = rect.left + (rect.right - rect.left) * float(fAllMemory)
		/ fCommittedMemory;
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

	menu->SetHighColor(highColor);
	fGrenze1 = grenze1;
	fGrenze2 = grenze2;

	fLastCommitted = fCommittedMemory;

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

	if (selected)
		menu->SetHighColor(ui_color(B_MENU_SELECTED_ITEM_TEXT_COLOR));
	else
		menu->SetHighColor(ui_color(B_MENU_ITEM_TEXT_COLOR));

	char infos[128];
	string_for_size(fWriteMemory * 1024.0, infos, sizeof(infos));

	BPoint loc(rect.left - kMargin - gMemoryTextWidth / 2 - menu->StringWidth(infos),
		rect.bottom + 1);
	menu->DrawString(infos, loc);

	string_for_size(fAllMemory * 1024.0, infos, sizeof(infos));
	loc.x = rect.left - kMargin - menu->StringWidth(infos);
	menu->DrawString(infos, loc);
	menu->SetHighColor(highColor);
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
MemoryBarMenuItem::UpdateSituation(int64 committedMemory)
{
	fCommittedMemory = committedMemory;
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
