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
	BDragger *dragger = new BDragger(frame, this, B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	AddChild(dragger);
	
	SetViewColor(B_TRANSPARENT_COLOR);
	
	fText = new BTextView(Bounds(), "bgView", Bounds(), B_FOLLOW_ALL, B_WILL_DRAW);
	fText->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
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


OverlayView::OverlayView(BMessage *archive) 
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
OverlayView::MessageReceived(BMessage *msg)
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
	      	OverlayAboutRequested(); 
    		break; 
	
		default:
			BView::MessageReceived(msg);
			break;
	}
}


BArchivable *OverlayView::Instantiate(BMessage *data)
{
	return new OverlayView(data);
}


status_t
OverlayView::Archive(BMessage *archive, bool deep) const
{
	BView::Archive(archive, deep);

	archive->AddString("add_on", "application/x-vnd.Haiku-OverlayImage");
	archive->AddString("class", "OverlayImage");

	if (fBitmap) {
		fBitmap->Lock();
		fBitmap->Archive(archive);
		fBitmap->Unlock();
	}			
	//archive->PrintToStream();

	return B_OK;
}


void
OverlayView::OverlayAboutRequested()
{
	BAlert *alert = new BAlert("about",
		"OverlayImage\n"
		"Copyright 1999-2010\n\n\t"
		"originally by Seth Flaxman\n\t"
		"modified by Hartmuth Reh\n\t"
		"further modified by Humdinger\n",
		"OK");	
	BTextView *view = alert->TextView();
	BFont font;
	view->SetStylable(true);
	view->GetFont(&font);
	font.SetSize(font.Size() + 7.0f);
	font.SetFace(B_BOLD_FACE);
	view->SetFontAndColor(0, 12, &font);
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);	
	alert->Go();
}
