/*
 * Copyright 2003-2008 Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 */

#include "MediaAlert.h"

#include <string.h>

#include <File.h>
#include <FindDirectory.h>
#include <IconUtils.h>
#include <Path.h>
#include <Resources.h>
#include <Screen.h>
#include <View.h>


const int kWindowIconOffset = 27;
const int kIconStripeWidth = 30;
const int kTextIconOffset = kWindowIconOffset + kIconStripeWidth - 2;
const int kTextTopOffset = 6;

class TAlertView : public BView {
public:
							TAlertView(BRect frame);
							TAlertView(BMessage* archive);
							~TAlertView();

	virtual void			Draw(BRect updateRect);

			void			SetBitmap(BBitmap* Icon)	{ fIconBitmap = Icon; }
			BBitmap*		Bitmap()					{ return fIconBitmap; }

private:
		BBitmap*			fIconBitmap;
};


MediaAlert::MediaAlert(BRect _rect, const char* title, const char* text)
	:
	BWindow(_rect, title, B_MODAL_WINDOW, B_NOT_CLOSABLE | B_NOT_RESIZABLE),
	fTextView(NULL)
{
	// Set up the "_master_" view
	TAlertView* masterView = new TAlertView(Bounds());
	masterView->SetBitmap(InitIcon());
	AddChild(masterView);

	// Set up the text view
	BRect textViewRect(kTextIconOffset, kTextTopOffset,
		Bounds().right, Bounds().bottom);
	BRect rect = textViewRect;
	rect.OffsetTo(B_ORIGIN);
	rect.InsetBy(4, 2);

	fTextView = new BTextView(textViewRect, "_tv_", rect,
		B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	fTextView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fTextView->SetText(text, strlen(text));
	fTextView->MakeEditable(false);
	fTextView->MakeSelectable(false);
	fTextView->SetWordWrap(true);
	fTextView->SetFontAndColor(be_bold_font);

	masterView->AddChild(fTextView);

	BRect screenFrame = BScreen(B_MAIN_SCREEN_ID).Frame();
	BPoint pt;
	pt.x = screenFrame.Width() / 2 - Bounds().Width() / 2;
	pt.y = screenFrame.Height() / 2 - Bounds().Height() / 2;

	if (screenFrame.Contains(pt))
		MoveTo(pt);
}


MediaAlert::~MediaAlert()
{
}


BTextView*
MediaAlert::TextView() const
{
	return fTextView;
}


BBitmap*
MediaAlert::InitIcon()
{
	// The alert icons are in the app_server resources
	BBitmap* icon = NULL;
	BPath path;
	if (find_directory(B_BEOS_SERVERS_DIRECTORY, &path) == B_OK) {
		path.Append("app_server");
		BFile file;
		if (file.SetTo(path.Path(), B_READ_ONLY) == B_OK) {
			BResources resources;
			if (resources.SetTo(&file) == B_OK) {
				// Which icon are we trying to load?
				const char* iconName = "warn";

				// Load the raw icon data
				size_t size;
				const void* rawIcon =
					resources.LoadResource(B_VECTOR_ICON_TYPE, iconName, &size);

				if (rawIcon != NULL) {
					// Now build the bitmap
					icon = new BBitmap(BRect(0, 0, 31, 31), B_RGBA32);
					if (BIconUtils::GetVectorIcon((const uint8*)rawIcon, size,
							icon) != B_OK) {
						delete icon;
						return NULL;
					}
				}
			}
		}
	}

	return icon;
}


//	#pragma mark - TAlertView


TAlertView::TAlertView(BRect frame)
	:
	BView(frame, "TAlertView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
	fIconBitmap(NULL)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


TAlertView::~TAlertView()
{
	delete fIconBitmap;
}


void
TAlertView::Draw(BRect updateRect)
{
	// Here's the fun stuff
	if (fIconBitmap) {
		BRect stripeRect = Bounds();
		stripeRect.right = kIconStripeWidth;
		SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
		FillRect(stripeRect);

		SetDrawingMode(B_OP_ALPHA);
		DrawBitmapAsync(fIconBitmap, BPoint(18, 6));
		SetDrawingMode(B_OP_COPY);
	}
}
