/*
 * Copyright 2003-2010 Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 */


#include "PasswordAlert.h"

#include <string.h>

#include <File.h>
#include <FindDirectory.h>
#include <IconUtils.h>
#include <Path.h>
#include <Resources.h>
#include <Screen.h>
#include <View.h>


static const int kWindowIconOffset = 27;
static const int kIconStripeWidth = 30;
static const int kTextIconOffset = kWindowIconOffset + kIconStripeWidth - 2;
static const int kTextTopOffset = 6;
static const int kSemTimeOut = 50000;


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


PasswordAlert::PasswordAlert(const char* title, const char* text)
	:
	BWindow(BRect(0, 0, 450, 45), title, B_MODAL_WINDOW, B_NOT_CLOSABLE | B_NOT_RESIZABLE),
	fTextControl(NULL),
	fAlertSem(-1)
{
	// Set up the "_master_" view
	TAlertView* masterView = new TAlertView(Bounds());
	masterView->SetBitmap(InitIcon());
	AddChild(masterView);

	// Set up the text view
	BRect textControlRect(kTextIconOffset, kTextTopOffset,
		Bounds().right - 5, Bounds().bottom);
	
	fTextControl = new BTextControl(textControlRect, "_password_", text, NULL, new BMessage('pass'), 
		B_FOLLOW_ALL);
	fTextControl->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fTextControl->TextView()->HideTyping(true);
	fTextControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	fTextControl->SetDivider(10 + fTextControl->StringWidth(text));

	masterView->AddChild(fTextControl);
	
	BRect screenFrame = BScreen(B_MAIN_SCREEN_ID).Frame();
	BPoint pt;
	pt.x = screenFrame.Width() / 2 - Bounds().Width() / 2;
	pt.y = screenFrame.Height() / 2 - Bounds().Height() / 2;

	if (screenFrame.Contains(pt))
		MoveTo(pt);

	fTextControl->MakeFocus();
}


PasswordAlert::~PasswordAlert()
{
}


BBitmap*
PasswordAlert::InitIcon()
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


void
PasswordAlert::Go(BString &password)
{
	fAlertSem = create_sem(0, "AlertSem");
	if (fAlertSem < B_OK) {
		Quit();
		return;
	}

	// Get the originating window, if it exists
	BWindow* window =
		dynamic_cast<BWindow*>(BLooper::LooperForThread(find_thread(NULL)));

	Show();

	// Heavily modified from TextEntryAlert code; the original didn't let the
	// blocked window ever draw.
	if (window) {
		status_t err;
		for (;;) {
			do {
				err = acquire_sem_etc(fAlertSem, 1, B_RELATIVE_TIMEOUT,
									  kSemTimeOut);
				// We've (probably) had our time slice taken away from us
			} while (err == B_INTERRUPTED);

			if (err == B_BAD_SEM_ID) {
				// Semaphore was finally nuked in MessageReceived
				break;
			}
			window->UpdateIfNeeded();
		}
	} else {
		// No window to update, so just hang out until we're done.
		while (acquire_sem(fAlertSem) == B_INTERRUPTED) {
		}
	}

	// Have to cache the value since we delete on Quit()
	password = fTextControl->Text();
	if (Lock())
		Quit();
}


void
PasswordAlert::MessageReceived(BMessage* msg)
{
	if (msg->what != 'pass')
		return BWindow::MessageReceived(msg);

	delete_sem(fAlertSem);
	fAlertSem = -1;
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
