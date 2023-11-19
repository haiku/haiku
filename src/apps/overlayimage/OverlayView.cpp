/*
 * Copyright 1999-2010, Be Incorporated. All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 *
 * OverlayImage is based on the code presented in this article:
 * http://www.haiku-os.org/documents/dev/replishow_a_replicable_image_viewer
 *
 * Authors:
 *			Seth Flexman
 *			Hartmuth Reh
 *			Humdinger		<humdingerb@gmail.com>
 */

#include "OverlayView.h"

#include <AboutWindow.h>
#include <Catalog.h>
#include <InterfaceDefs.h>
#include <Locale.h>
#include <String.h>
#include <TextView.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Main view"

const float kDraggerSize = 7;


OverlayView::OverlayView(BRect frame)
	:
	BView(frame, "OverlayImage", B_FOLLOW_NONE, B_WILL_DRAW)
{
	fBitmap = NULL;
	fReplicated = false;

	frame.left = frame.right - kDraggerSize;
	frame.top = frame.bottom - kDraggerSize;
	BDragger* dragger = new BDragger(frame, this, B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	AddChild(dragger);

	SetViewColor(B_TRANSPARENT_COLOR);

	fText = new BTextView(Bounds(), "bgView", Bounds(), B_FOLLOW_ALL, B_WILL_DRAW);
	fText->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	rgb_color color = ui_color(B_PANEL_TEXT_COLOR);
	fText->SetFontAndColor(be_plain_font, B_FONT_ALL, &color);
	AddChild(fText);
	BString text;
	text << B_TRANSLATE(
		"Enable \"Show replicants\" in Deskbar.\n"
		"Drag & drop an image.\n"
		"Drag the replicant to the Desktop.");
	fText->SetText(text);
	fText->SetAlignment(B_ALIGN_CENTER);
	fText->MakeEditable(false);
	fText->MakeSelectable(false);
	fText->MoveBy(0, (Bounds().bottom - fText->TextRect().bottom) / 2);
}


OverlayView::OverlayView(BMessage* archive)
	:
	BView(archive)
{
	fReplicated = true;
	fBitmap = new BBitmap(archive);
}


OverlayView::~OverlayView()
{
	delete fBitmap;
}


void
OverlayView::Draw(BRect)
{
	SetDrawingMode(B_OP_ALPHA);
	SetViewColor(B_TRANSPARENT_COLOR);

	if (fBitmap)
		DrawBitmap(fBitmap, B_ORIGIN);
}


void
OverlayView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case B_SIMPLE_DATA:
		{
			if (fReplicated)
				break;

			entry_ref ref;
			msg->FindRef("refs", &ref);
			BEntry entry(&ref);
			BPath path(&entry);

			delete fBitmap;
			fBitmap = BTranslationUtils::GetBitmap(path.Path());

			if (fBitmap != NULL) {
				if (fText != NULL) {
					RemoveChild(fText);
					fText = NULL;
				}

				BRect rect = fBitmap->Bounds();
				if (!fReplicated) {
					Window()->ResizeTo(rect.right, rect.bottom);
					Window()->Activate(true);
				}
				ResizeTo(rect.right, rect.bottom);
				Invalidate();
			}
			break;
		}
		case B_ABOUT_REQUESTED:
		{
			OverlayAboutRequested();
			break;
		}
		case B_COLORS_UPDATED:
		{
			rgb_color color;
			if (msg->FindColor(ui_color_name(B_PANEL_TEXT_COLOR), &color) == B_OK)
				fText->SetFontAndColor(be_plain_font, B_FONT_ALL, &color);
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}


BArchivable*
OverlayView::Instantiate(BMessage* data)
{
	return new OverlayView(data);
}


status_t
OverlayView::Archive(BMessage* archive, bool deep) const
{
	BView::Archive(archive, deep);

	archive->AddString("add_on", kAppSignature);
	archive->AddString("class", "OverlayImage");

	if (fBitmap) {
		fBitmap->Lock();
		fBitmap->Archive(archive);
		fBitmap->Unlock();
	}

	return B_OK;
}


void
OverlayView::OverlayAboutRequested()
{
	BAboutWindow* aboutwindow
		= new BAboutWindow(B_TRANSLATE_SYSTEM_NAME("OverlayImage"), kAppSignature);

	const char* authors[] = {
		"Seth Flaxman",
		"Hartmuth Reh",
		"Humdinger",
		NULL
	};

	aboutwindow->AddCopyright(1999, "Seth Flaxman");
	aboutwindow->AddCopyright(2010, "Haiku, Inc.");
	aboutwindow->AddAuthors(authors);
	aboutwindow->Show();
}
