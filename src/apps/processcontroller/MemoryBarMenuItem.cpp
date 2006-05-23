/*

	MemoryBarMenuItem.cpp

	ProcessController
	© 2000, Georges-Edouard Berenger, All Rights Reserved.
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

#include "PCView.h"
#include "MemoryBarMenuItem.h"
#include "MemoryBarMenu.h"
#include "Colors.h"
#include <Bitmap.h>
#include <stdio.h>

// --------------------------------------------------------------
MemoryBarMenuItem::MemoryBarMenuItem (const char *label, team_id team, BBitmap* icon, bool DeleteIcon, BMessage* message)
			:BMenuItem (label, message), fTeamID (team), fIcon (icon), fDeleteIcon (DeleteIcon)
{
	Init ();
}

// --------------------------------------------------------------
void MemoryBarMenuItem::Init ()
{
	fWriteMemory = -1;
	fAllMemory = -1;
	fGrenze1 = -1;
	fGrenze2 = -1;
	fLastCommited = -1;
	fLastWrite = -1;
	fLastAll = -1;
}

// --------------------------------------------------------------
MemoryBarMenuItem::~MemoryBarMenuItem ()
{
	if (fDeleteIcon)
		delete fIcon;
}

// --------------------------------------------------------------
void MemoryBarMenuItem::DrawContent ()
{
	BPoint	loc;

	DrawIcon ();
	if (fWriteMemory < 0)
		BarUpdate ();
	else
		DrawBar (true);
	loc = ContentLocation ();
	loc.x += 20;
	Menu ()->MovePenTo (loc);
	BMenuItem::DrawContent ();
}

// --------------------------------------------------------------
void MemoryBarMenuItem::DrawIcon ()
{
	BPoint	loc;

	loc = ContentLocation ();
	BRect	frame = Frame();
	loc.y = frame.top + (frame.bottom - frame.top - 15) / 2;
	Menu ()->SetDrawingMode (B_OP_OVER);
	if (fIcon)
		Menu ()->DrawBitmap (fIcon, loc);
}

// --------------------------------------------------------------
void MemoryBarMenuItem::DrawBar (bool force)
{
	bool	selected = IsSelected ();
	BRect	frame = Frame ();
	BMenu*	menu = Menu ();
	// draw the bar itself
	BRect	cadre (frame.right - kMargin - kBarWidth, frame.top + 5, frame.right - kMargin, frame.top + 13);
	if (fWriteMemory < 0)
		return;
	if (fGrenze1 < 0)
		force = true;
	if (force) {
		if (selected)
			menu->SetHighColor (gFrameColorSelected);
		else
			menu->SetHighColor (gFrameColor);
		menu->StrokeRect (cadre);
	}
	cadre.InsetBy (1, 1);
	BRect	r = cadre;
	float	grenze1 = cadre.left + (cadre.right - cadre.left) * float (fWriteMemory) / fCommitedMemory;
	float	grenze2 = cadre.left + (cadre.right - cadre.left) * float (fAllMemory) / fCommitedMemory;
	if (grenze1 > cadre.right)
		grenze1 = cadre.right;
	if (grenze2 > cadre.right)
		grenze2 = cadre.right;
	r.right = grenze1;
	if (!force)
		r.left = fGrenze1;
	if (r.left < r.right) {
		if (selected)
			menu->SetHighColor (gKernelColorSelected);
		else
			menu->SetHighColor (gKernelColor);
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
			menu->SetHighColor (gUserColorSelected);
		else
			menu->SetHighColor (gUserColor);
		menu->FillRect (r);
	}
	r.left = grenze2;
	r.right = cadre.right;
	if (!force)
		r.right = fGrenze2;
	if (r.left < r.right) {
		if (selected)
			menu->SetHighColor (gWhiteSelected);
		else
			menu->SetHighColor (kWhite);
		menu->FillRect (r);
	}
	menu->SetHighColor (kBlack);
	fGrenze1 = grenze1;
	fGrenze2 = grenze2;

	// draw the value
	if (force || fCommitedMemory != fLastCommited || fWriteMemory != fLastWrite || fAllMemory != fLastAll)
	{
		if (selected)
			menu->SetLowColor (gMenuBackColorSelected);
		else
			menu->SetLowColor (gMenuBackColor);
		if (force || fWriteMemory != fLastWrite || fAllMemory != fLastAll)
		{
			menu->SetHighColor (menu->LowColor ());
			BRect trect (cadre.left - kMargin - kTextWidth, frame.top, cadre.left - kMargin, frame.bottom);
			menu->FillRect (trect);
			fLastWrite = fWriteMemory;
			fLastAll = fAllMemory;
		}
		fLastCommited = fCommitedMemory;
		menu->SetHighColor (kBlack);
		char	infos[128];
//		if (fWriteMemory >= 1024)
//			sprintf (infos, "%.1f MB", float (fWriteMemory) / 1024.f);
//		else
			sprintf (infos, "%d KB", fWriteMemory);
		BPoint	loc (cadre.left - kMargin - kTextWidth / 2 - menu->StringWidth (infos), cadre.bottom + 1);
		menu->DrawString (infos, loc);
//		if (fAllMemory >= 1024)
//			sprintf (infos, "%.1f MB", float (fAllMemory) / 1024.f);
//		else
			sprintf (infos, "%d KB", fAllMemory);
		loc.x = cadre.left - kMargin - menu->StringWidth (infos);
		menu->DrawString (infos, loc);
	}
}

// --------------------------------------------------------------
void MemoryBarMenuItem::GetContentSize (float* width, float* height)
{
	BMenuItem::GetContentSize (width, height);
	if (*height < 16)
		*height = 16;
	*width += 30 + kBarWidth + kMargin + kTextWidth;
}

// --------------------------------------------------------------
int MemoryBarMenuItem::UpdateSituation (int commitedMemory)
{
	fCommitedMemory = commitedMemory;
	BarUpdate ();
	return fWriteMemory;
}

// --------------------------------------------------------------
void MemoryBarMenuItem::BarUpdate ()
{
	area_info	ainfo;
	int32		cookie = 0;
	size_t		lram_size = 0;
	size_t		lwram_size = 0;
	bool		exists = false;
	while (get_next_area_info (fTeamID, &cookie, &ainfo) == B_OK)
	{
		exists = true;
		lram_size += ainfo.ram_size;
		int zone = (int (ainfo.address) & 0xf0000000) >> 24;
		if ((ainfo.protection & B_WRITE_AREA)
			&& (zone & 0xf0) != 0xA0			// Exclude media buffers
			&& (fTeamID != gAppServerTeamID || zone != 0x90))	// Exclude app_server side of bitmaps
			lwram_size += ainfo.ram_size;
	}
	if (!exists) {
		team_info	infos;
		exists = get_team_info(fTeamID, &infos) == B_OK;
	}
	if (exists) {
		fWriteMemory = lwram_size / 1024;
		fAllMemory = lram_size / 1024;
		DrawBar (false);
	} else
		fWriteMemory = -1;
}

// --------------------------------------------------------------
void MemoryBarMenuItem::Reset (char* name, team_id team, BBitmap* icon, bool DeleteIcon)
{
	SetLabel (name);
	fTeamID = team;
	if (fDeleteIcon)
		delete fIcon;
	fDeleteIcon = DeleteIcon;
	fIcon = icon;
	Init ();
}
