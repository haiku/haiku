/*

	IconMenuItem.cpp

	ProcessController
	(c) 2000, Georges-Edouard Berenger, All Rights Reserved.
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

#include "IconMenuItem.h"
#include <Application.h>
#include <NodeInfo.h>
#include <Bitmap.h>
#include <Roster.h>
#include <parsedate.h>


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
		minheight = before_dano() ? 16 : 17;
	return minheight;
}


bool
before_dano()
{
	static int old_version = -1;
	if (old_version < 0) {
		system_info sys_info;
		get_system_info(&sys_info);
		time_t kernelTime = parsedate(sys_info.kernel_build_date, time(NULL));
		struct tm* date = gmtime(&kernelTime);
		old_version = (date->tm_year < 101 || (date->tm_year == 101 && date->tm_mon < 10));
	}

	return old_version;
}
