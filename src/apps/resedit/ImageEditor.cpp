/*
 * Copyright (c) 2005-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@gmail.com>
 */
#include "BitmapView.h"
#include "InternalEditors.h"
#include "ResourceData.h"

#include <DataIO.h>
#include <TranslationUtils.h>

#define M_IMAGE_CHANGED 'imch'

ImageEditor::ImageEditor(const BRect &frame, ResourceData *data, BHandler *owner)
  :	Editor(frame, data, owner)
{
	SetFlags(Flags()  | B_NOT_RESIZABLE | B_NOT_ZOOMABLE);
	SetTitle(data->GetName());
	
	// Set up the image and the viewer for such
	BMemoryIO memio(data->GetData(), data->GetLength());
	fImage = BTranslationUtils::GetBitmap(&memio);
	
	BRect imgsize;
	if (fImage)
		imgsize = ScaleRectToFit(fImage->Bounds(), BRect(0, 0, 200, 200));
	else
		imgsize.Set(0, 0, 200, 200);
	
	BView *back = new BView(Bounds(), "back", B_FOLLOW_ALL, B_WILL_DRAW);
	back->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(back);
	
	BRect r;
	
	float labelwidth = be_plain_font->StringWidth("ID: ");
	float strwidth = be_plain_font->StringWidth("(attr) ");
	
	font_height fh;
	be_plain_font->GetHeight(&fh);
	float strheight = fh.ascent + fh.descent + fh.leading + 5;
	
	fIDBox = new BTextControl(BRect(10, 10, 10 + (strwidth + labelwidth) + 15,
									10 + strheight),
							  "id", "ID: ", data->GetIDString(), NULL);
	fIDBox->SetDivider(labelwidth + 5);
	back->AddChild(fIDBox);
	
	r = fIDBox->Frame();
	r.OffsetBy(r.Width() + 10, 0);
	r.right = Bounds().right - 10;
	fNameBox = new BTextControl(r, "name", "Name: ", data->GetName(), NULL,
								B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fNameBox->SetDivider(be_plain_font->StringWidth("Name: ") + 5);
	back->AddChild(fNameBox);
	
	r = imgsize;
	
	r.OffsetTo( (Bounds().Width() - r.Width()) / 2, fNameBox->Frame().bottom + 10);
	fImageView = new BitmapView(r, "imageview", new BMessage(M_IMAGE_CHANGED), fImage,
								NULL, B_PLAIN_BORDER, B_FOLLOW_NONE,
								B_WILL_DRAW | B_FRAME_EVENTS);
	back->AddChild(fImageView);
	fImageView->SetConstrainDrops(false);
	
	// No limit on bitmap size
	fImageView->SetMaxBitmapSize(0, 0);
	fImageView->SetBitmapRemovable(false);
	
	fOK = new BButton(BRect(10, 10, 11, 11), "ok", "Cancel", new BMessage(M_UPDATE_RESOURCE),
					  B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	fOK->ResizeToPreferred();
	fOK->SetLabel("OK");
	fOK->MakeDefault(true);
	back->AddChild(fOK);
	
	fOK->MoveTo(Bounds().right - fOK->Bounds().Width() - 10,
				Bounds().bottom - fOK->Bounds().Height() - 10);
	r = fOK->Frame();
	
	r.OffsetBy(-r.Width() - 10, 0);
	fCancel = new BButton(r, "cancel", "Cancel", new BMessage(B_QUIT_REQUESTED),
					  B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	back->AddChild(fCancel);
	
	ResizeTo(MAX(fImageView->Frame().right, fNameBox->Frame().right) + 20,
			fImageView->Frame().bottom + fOK->Frame().Height() + 20);
	
	SetSizeLimits(Bounds().right, 30000, Bounds().bottom, 30000);
}


ImageEditor::~ImageEditor(void)
{
	delete fImage;
}


void
ImageEditor::MessageReceived(BMessage *msg)
{
	if (msg->WasDropped()) {
		fImageView->MessageReceived(msg);
	} else if (msg->what == M_IMAGE_CHANGED) {
		fImage = fImageView->GetBitmap();		
		BRect r = ScaleRectToFit(fImage->Bounds(), BRect(0, 0, 200, 200));
		fImageView->ResizeTo(r.Width(), r.Height());
		ResizeTo(MAX(fImageView->Frame().right, fNameBox->Frame().right) + 20,
				fImageView->Frame().bottom + fOK->Frame().Height() + 20);
	} else
		BWindow::MessageReceived(msg);
}


void
ImageEditor::FrameResized(float w, float h)
{
	fImageView->MoveTo( (w - fImageView->Bounds().Width()) / 2,
						fNameBox->Frame().bottom + 10);
}
