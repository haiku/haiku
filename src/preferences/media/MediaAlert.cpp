// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        MediaAlert.cpp
//  Author:      Jérôme Duval
//  Description: Media Preferences
//  Created :    June 25, 2003
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~


// Standard Includes -----------------------------------------------------------
#include <string.h>

// System Includes -------------------------------------------------------------
#include "MediaAlert.h"
#include <Screen.h>
#include <View.h>

#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Resources.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

const int kWindowIconOffset			=  27;
const int kIconStripeWidth			=  30;
const int kTextIconOffset			=  kWindowIconOffset + kIconStripeWidth - 2;
const int kTextTopOffset			=   6;

//------------------------------------------------------------------------------
class TAlertView : public BView
{
	public:
		TAlertView(BRect frame);
		TAlertView(BMessage* archive);
		~TAlertView();

		virtual void	Draw(BRect updateRect);

		void			SetBitmap(BBitmap* Icon)	{ fIconBitmap = Icon; }
		BBitmap*		Bitmap()					{ return fIconBitmap; }

	private:
		BBitmap*	fIconBitmap;
};

//------------------------------------------------------------------------------
MediaAlert::MediaAlert(BRect rect, const char* title, const char* text)
	:	BWindow(rect, title, B_MODAL_WINDOW,
				B_NOT_CLOSABLE | B_NOT_RESIZABLE)
{
	fTextView = NULL;
	
	// Set up the "_master_" view
	TAlertView* MasterView = new TAlertView(Bounds());
	MasterView->SetBitmap(InitIcon());
	AddChild(MasterView);
	
	// Set up the text view
	BRect TextViewRect(kTextIconOffset, kTextTopOffset, 
		Bounds().right, Bounds().bottom);
	BRect rect = TextViewRect;
	rect.OffsetTo(B_ORIGIN);
	rect.InsetBy(4,2);
	fTextView = new BTextView(TextViewRect, "_tv_",
							  rect,
							  B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	fTextView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fTextView->SetText(text, strlen(text));
	fTextView->MakeEditable(false);
	fTextView->MakeSelectable(false);
	fTextView->SetWordWrap(true);

	fTextView->SetFontAndColor(be_bold_font);

	MasterView->AddChild(fTextView);
	
	BRect	screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());
	BPoint 	pt;
	pt.x = screenFrame.Width()/2 - Bounds().Width()/2;
	pt.y = screenFrame.Height()/2 - Bounds().Height()/2;

	if (screenFrame.Contains(pt))
		MoveTo(pt);
}

//------------------------------------------------------------------------------
MediaAlert::~MediaAlert()
{
	
}

//------------------------------------------------------------------------------
BTextView* MediaAlert::TextView() const
{
	return fTextView;
}

//------------------------------------------------------------------------------
BBitmap* MediaAlert::InitIcon()
{
	// After a bit of a search, I found the icons in app_server. =P
	BBitmap* Icon = NULL;
	BPath Path;
	if (find_directory(B_BEOS_SERVERS_DIRECTORY, &Path) == B_OK)
	{
		Path.Append("app_server");
		BFile File;
		if (File.SetTo(Path.Path(), B_READ_ONLY) == B_OK)
		{
			BResources Resources;
			if (Resources.SetTo(&File) == B_OK)
			{
				// Which icon are we trying to load?
				const char* iconName = "warn";
				
				// Load the raw icon data
				size_t size;
				const void* rawIcon =
					Resources.LoadResource('ICON', iconName, &size);

				if (rawIcon)
				{
					// Now build the bitmap
					Icon = new BBitmap(BRect(0, 0, 31, 31), B_CMAP8);
					Icon->SetBits(rawIcon, size, 0, B_CMAP8);
				}
			}
		}
	}

	return Icon;
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//	#pragma mark -
//	#pragma mark TAlertView
//	#pragma mark -
//------------------------------------------------------------------------------
TAlertView::TAlertView(BRect frame)
	:	BView(frame, "TAlertView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
		fIconBitmap(NULL)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}
//------------------------------------------------------------------------------
TAlertView::~TAlertView()
{
	if (fIconBitmap)
	{
		delete fIconBitmap;
	}
}
//------------------------------------------------------------------------------
void TAlertView::Draw(BRect updateRect)
{
	// Here's the fun stuff
	if (fIconBitmap)
	{
		BRect StripeRect = Bounds();
		StripeRect.right = kIconStripeWidth;
		SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
		FillRect(StripeRect);

		SetDrawingMode(B_OP_OVER);
		DrawBitmapAsync(fIconBitmap, BPoint(18, 6));
		SetDrawingMode(B_OP_COPY);
	}
}
//------------------------------------------------------------------------------



