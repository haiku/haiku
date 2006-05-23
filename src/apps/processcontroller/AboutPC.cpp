/*
	AboutPC.cpp
	
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

#include "PCWorld.h"
#include "AboutPC.h"
#include "WindowsTools.h"
#include <Button.h>
#include <Bitmap.h>
#include <Mime.h>
#include <stdio.h>
#include "URLView.h"

const char*	kProgramName = "ProcessController 3.1.1";
const char*	SendTexte = "Send your bug reports & comments to:";

const char* kAboutProgramName = "ProcessController";
const char* kAboutEmail = "contact@beuntied.org";
//const char* kAboutEmail = "berenger@francenet.fr";
//const char* kAboutWeb = "http://home.foni.net/~berenger/";
//const char* kAboutWeb = "http://stv.steinberg.de/~georges/besoft/";
const char* kAboutWeb = "http://www.beunited.org/index.php?page=developer";

AboutPC::AboutPC(BRect r):BWindow(CenterRect(r, 530, 200), B_EMPTY_STRING,
					B_MODAL_WINDOW, B_NOT_RESIZABLE+B_NOT_ZOOMABLE+B_NOT_MINIMIZABLE+B_NOT_CLOSABLE)
{
	BRect		rect;
	AboutView	*topview;

	// View de fond
	rect = Bounds();
	AddChild(topview = new AboutView(rect));
	BButton	*ok;
	topview->AddChild(ok = new BButton(BRect(rect.right-125, rect.bottom-40, rect.right-25, rect.bottom-16), NULL, "Thanks!", new BMessage(B_QUIT_REQUESTED), B_FOLLOW_LEFT));
	ok->MakeDefault(true);
	visible_window(this);
	Show();
}

#define kTop 163
#define kStep 12
#define kLeft 70

AboutView::AboutView(BRect frame)
	: BView(frame, NULL, B_FOLLOW_NONE, B_WILL_DRAW)
{
	fIcon = new BBitmap(BRect(0, 0, 31, 31), B_COLOR_8_BIT);
	BMimeType(kSignature).GetIcon(fIcon, B_LARGE_ICON);
	float middle = Frame ().Width () / 2;
	BRect rect (middle, 112, 0, 0);
	char subject[256];
	sprintf (subject, "About %s (%s, %s)", kProgramName, __DATE__, __TIME__);
	mailtoView* mailto = new mailtoView (rect, kAboutEmail, subject);
	AddChild (mailto);
	rect.Set (kLeft - 2, kTop + kStep + 2, kLeft - 2, 0);
	URLView* urlview = new URLView (rect, kAboutWeb);
	AddChild (urlview);
}

AboutView::~AboutView()
{
	if (fIcon)
		delete fIcon;
}

void AboutView::AttachedToWindow()
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

void AboutView::Draw(BRect /*updateRect*/)
{
	static rgb_color	panel_color = ui_color(B_PANEL_BACKGROUND_COLOR);
	static rgb_color	dark_panel_color = tint_color(panel_color, B_DARKEN_1_TINT);
	BRect	r = Bounds();
	SetDrawingMode(B_OP_OVER);
	DrawBitmap(fIcon, BPoint(20, 156));
	SetDrawingMode(B_OP_COPY);
	r.bottom = 80;
	SetHighColor(dark_panel_color);
	FillRect(r);
	SetHighColor(kBordeaux);
	SetLowColor(dark_panel_color);
	if (fFont.SetFamilyAndFace ("Baskerville", B_BOLD_FACE | B_ITALIC_FACE) != B_OK)
		fFont.SetFamilyAndFace ("Baskerville BT", B_BOLD_FACE | B_ITALIC_FACE);
	fFont.SetSize(45);
	SetFont(&fFont, B_FONT_FAMILY_AND_STYLE | B_FONT_SIZE);
	DrawString(kProgramName, BPoint((r.Width()-StringWidth(kProgramName))/2, 50));
	fFont.SetFamilyAndFace ("Swis721 BT", B_REGULAR_FACE);
	fFont.SetSize(10);
	SetFont(&fFont, B_FONT_FAMILY_AND_STYLE | B_FONT_SIZE);
	SetHighColor(kBlack);
	SetLowColor(panel_color);
	DrawString(SendTexte, BPoint((r.Width()-StringWidth(SendTexte))/2, 106));
	MovePenTo(kLeft, kTop);
	DrawString("Halifax, ");
	DrawString(__DATE__);
	DrawString(", ");
	DrawString(__TIME__);
	DrawString(".");
	DrawString("Â© www.beuntied.org, 2004. Original by Georges-Edouard Berenger.", BPoint(kLeft, kTop + kStep));
}
