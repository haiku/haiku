/*
 * Copyright 2000, Georges-Edouard Berenger. All rights reserved.
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "MemoryBarMenuItem.h"

#include "Colors.h"
#include "MemoryBarMenu.h"
#include "ProcessController.h"

#include <Bitmap.h>
#include <ControlLook.h>
#include <KernelExport.h>
#include <StringForSize.h>

#include <stdio.h>


MemoryBarMenuItem::MemoryBarMenuItem(const char *label, team_id team,
		BBitmap* icon, bool deleteIcon, BMessage* message)
	:
	IconMenuItem(icon, label, message, true, deleteIcon),
	fTeamID(team)
{
	Init();
}


MemoryBarMenuItem::~MemoryBarMenuItem()
{
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
	loc.x += ceilf(be_control_look->DefaultLabelSpacing() * 3.3f);
	Menu()->MovePenTo(loc);
	BMenuItem::DrawContent();
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
	IconMenuItem::GetContentSize(_width, _height);
	*_width += ceilf(be_control_look->DefaultLabelSpacing() * 2.0f)
		+ kBarWidth + kMargin + gMemoryTextWidth;
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

	const bool isAppServer = (strcmp(Label(), "app_server") == 0);

	while (get_next_area_info(fTeamID, &cookie, &areaInfo) == B_OK) {
		exists = true;
		lram_size += areaInfo.ram_size;

		if (isAppServer && strncmp(areaInfo.name, "a:", 2) == 0) {
			// app_server side of client memory (e.g. bitmaps), ignore.
			continue;
		}
		if (fTeamID == B_SYSTEM_TEAM) {
			if ((areaInfo.protection & B_KERNEL_WRITE_AREA) != 0)
				areaInfo.protection |= B_WRITE_AREA;
		}
		// TODO: Exclude media buffers

		if ((areaInfo.protection & B_WRITE_AREA) != 0)
			lwram_size += areaInfo.ram_size;
	}
	if (fTeamID == B_SYSTEM_TEAM) {
		system_info info;
		status_t status = get_system_info(&info);
		if (status == B_OK) {
			// block_cache memory will be in writable areas
			lwram_size -= info.block_cache_pages * B_PAGE_SIZE;
		}
	} else if (!exists) {
		team_info info;
		exists = get_team_info(fTeamID, &info) == B_OK;
		if (!exists) {
			fWriteMemory = -1;
			return;
		}
	}

	fWriteMemory = lwram_size / 1024;
	fAllMemory = lram_size / 1024;
	DrawBar(false);
}


void
MemoryBarMenuItem::Reset(char* name, team_id team, BBitmap* icon,
	bool deleteIcon)
{
	SetLabel(name);
	fTeamID = team;
	IconMenuItem::Reset(icon, deleteIcon);

	Init();
}
