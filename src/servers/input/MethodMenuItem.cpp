// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//      Copyright (c) 2004, Haiku
//
//  This software is part of the Haiku distribution and is covered
//  by the Haiku license.
//
//
//  File:                       MethodMenuItem.cpp
//  Authors:            Jérôme Duval,
//
//  Description:        Input Server
//  Created:            October 19, 2004
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <string.h>
#include "MethodMenuItem.h"

MethodMenuItem::MethodMenuItem(void* cookie, const char* name, const uchar* icon, BMenu* subMenu, BMessenger& messenger)
	: BMenuItem(subMenu),
	fIcon(BRect(0, 0, MENUITEM_ICON_SIZE - 1, MENUITEM_ICON_SIZE - 1), B_CMAP8),
	fCookie(cookie)
{
	SetLabel(name);
	fIcon.SetBits(icon, MENUITEM_ICON_SIZE * MENUITEM_ICON_SIZE, 0, B_CMAP8);
	fMessenger = messenger;
}


MethodMenuItem::MethodMenuItem(void* cookie, const char* name, const uchar* icon)
	: BMenuItem(name, NULL),
	fIcon(BRect(0, 0, MENUITEM_ICON_SIZE - 1, MENUITEM_ICON_SIZE - 1), B_CMAP8),
	fCookie(cookie)
{
	fIcon.SetBits(icon, MENUITEM_ICON_SIZE * MENUITEM_ICON_SIZE, 0, B_CMAP8);
}


MethodMenuItem::~MethodMenuItem()
{
}


void
MethodMenuItem::SetName(const char *name)
{
	SetLabel(name);
}

void
MethodMenuItem::SetIcon(const uchar *icon)
{
	fIcon.SetBits(icon, MENUITEM_ICON_SIZE * MENUITEM_ICON_SIZE, 0, B_CMAP8);
}


void
MethodMenuItem::GetContentSize(float *width, float *height)
{
	*width = be_plain_font->StringWidth(Label()) + MENUITEM_ICON_SIZE + 3;

	font_height fheight;
	be_plain_font->GetHeight(&fheight);

	*height = fheight.ascent + fheight.descent + fheight.leading - 2;
	if (*height < MENUITEM_ICON_SIZE)
		*height = MENUITEM_ICON_SIZE;
}


void
MethodMenuItem::DrawContent()
{
	BMenu *menu = Menu();
	BPoint contLoc = ContentLocation();

	menu->SetDrawingMode(B_OP_OVER);
	menu->MovePenTo(contLoc);
	menu->DrawBitmapAsync(&fIcon);
	menu->SetDrawingMode(B_OP_COPY);
	menu->MovePenBy(MENUITEM_ICON_SIZE + 3, 2);
	BMenuItem::DrawContent();
}

