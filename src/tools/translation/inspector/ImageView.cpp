/*****************************************************************************/
// ImageView
// Written by Michael Wilber, OBOS Translation Kit Team
//
// ImageView.cpp
//
// BView class for showing images.  Images can be dropped on this view
// from the tracker and images viewed in this view can be dragged
// to the tracker to be saved.
//
//
// Copyright (c) 2003 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/


#include "ImageView.h"
#include "Constants.h"
#include "StatusCheck.h"
#include <TranslationUtils.h>
#include <TranslatorRoster.h>
#include <BitmapStream.h>
#include <Entry.h>
#include <Path.h>
#include <Directory.h>
#include <File.h>
#include <Node.h>
#include <NodeInfo.h>
#include <MenuBar.h>
#include <Screen.h>
#include <ScrollBar.h>
#include <Alert.h>
#include <Font.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

ImageView::ImageView(BRect rect, const char *name)
	: BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS)
{
	fpbitmap = NULL;
	fpmsgFromTracker = NULL;
	fbdragFromTracker = false;
	
	SetViewColor(255, 255, 255);
	SetHighColor(255, 255, 255);
}

ImageView::~ImageView()
{
	delete fpbitmap;
	fpbitmap = NULL;
}

void
ImageView::AttachedToWindow()
{
	AdjustScrollBars();
}

void
ImageView::Draw(BRect rect)
{
	// Force view setting changes to be local
	// to this function
	PushState();
	
	// calculate highlight rectangle
	BRect bounds = Bounds();
	bounds.right--;
	bounds.bottom--;
	
	if (HasImage())
		DrawBitmap(fpbitmap, BPoint(0, 0));
	else if (!fbdragFromTracker) {
		SetHighColor(255, 255, 255);
		SetPenSize(2.0);
		StrokeRect(bounds);
	}
	
	if (fbdragFromTracker) {
		SetHighColor(0, 0, 229);
		SetPenSize(2.0);
		StrokeRect(bounds);
	}
	
	PopState();
}

void
ImageView::FrameResized(float width, float height)
{
	AdjustScrollBars();
	
	// If there is a bitmap, draw it.
	// If not, invalidate the entire view so that
	// the advice text will be displayed
	if (HasImage())
		ReDraw();
	else
		Invalidate();
}

void
ImageView::MouseDown(BPoint point)
{
	if (!HasImage())
		return;

	// Tell BeOS to setup a Drag/Drop operation
	BMessage msg(B_SIMPLE_DATA);
	msg.AddInt32("be:actions", B_COPY_TARGET);
	msg.AddString("be:filetypes", "application/octet-stream");
	msg.AddString("be:types", "application/octet-stream");
	msg.AddString("be:clip_name", "Bitmap");

	DragMessage(&msg, Bounds());
}

void
ImageView::MouseMoved(BPoint point, uint32 state, const BMessage *pmsg)
{
	if (state == B_EXITED_VIEW) {
		// If the cursor was dragged over,
		// then off of this view, turn off
		// drag/drop border highlighting and
		// forget the last drag/drop BMessage
		// from the tracker
		fbdragFromTracker = false;
		fpmsgFromTracker = NULL;
		ReDraw();
		return;
	}
	
	if (!pmsg || pmsg->what != B_SIMPLE_DATA)
		// if there is no message (nothing being dragged)
		// or the message contains uninteresting data,
		// exit this function
		return;
		
	if (pmsg != fpmsgFromTracker) {
		fpmsgFromTracker = pmsg;
		
		entry_ref ref;
		if (pmsg->FindRef("refs", 0, &ref) != B_OK)
			return;
			
		BNode node(&ref);
		if (node.InitCheck() != B_OK)
			return;
		BNodeInfo nodeinfo(&node);
		if (nodeinfo.InitCheck() != B_OK)
			return;
		char mime[B_MIME_TYPE_LENGTH];
		if (nodeinfo.GetType(mime) != B_OK)
			return;

		if (strstr(mime, "image/") != NULL) {
			fbdragFromTracker = true;
			ReDraw();
		}
	}
}

void
ImageView::MouseUp(BPoint point)
{
}

void
ImageView::MessageReceived(BMessage *pmsg)
{
	switch (pmsg->what) {
		case B_COPY_TARGET:
			SaveImageAtDropLocation(pmsg);
			break;
		
		default:
			BView::MessageReceived(pmsg);
			break;
	}
}

void
ImageView::SaveImageAtDropLocation(BMessage *pmsg)
{
	// Find the location and name of the drop and
	// write the image file there
	
	BBitmapStream stream(fpbitmap);
	
	StatusCheck chk;
		// throw an exception if this is assigned 
		// anything other than B_OK
	try {
		entry_ref dirref;
		chk = pmsg->FindRef("directory", &dirref);
		const char *filename;
		chk = pmsg->FindString("name", &filename);
		
		BDirectory dir(&dirref);
		BFile file(&dir, filename, B_WRITE_ONLY | B_CREATE_FILE);
		chk = file.InitCheck();
		
		BTranslatorRoster *proster = BTranslatorRoster::Default();
		chk = proster->Translate(&stream, NULL, NULL, &file, B_TGA_FORMAT);

	} catch (StatusNotOKException) {
		BAlert *palert = new BAlert(NULL,
			"Sorry, unable to write the image file.", "OK");
		palert->Go();
	}
	
	stream.DetachBitmap(&fpbitmap);
}

void
ImageView::AdjustScrollBars()
{
	BRect rctview = Bounds(), rctbitmap(0, 0, 0, 0);
	if (HasImage())
		rctbitmap = fpbitmap->Bounds();
	
	float prop, range;
	BScrollBar *psb = ScrollBar(B_HORIZONTAL);
	if (psb) {
		range = rctbitmap.Width() - rctview.Width();
		if (range < 0) range = 0;
		prop = rctview.Width() / rctbitmap.Width();
		if (prop > 1.0f) prop = 1.0f;
		psb->SetRange(0, range);
		psb->SetProportion(prop);
		psb->SetSteps(10, 100);
	}
	psb = ScrollBar(B_VERTICAL);
	if (psb) {
		range = rctbitmap.Height() - rctview.Height();
		if (range < 0) range = 0;
		prop = rctview.Height() / rctbitmap.Height();
		if (prop > 1.0f) prop = 1.0f;
		psb->SetRange(0, range);
		psb->SetProportion(prop);
		psb->SetSteps(10, 100);
	}
}

void
ImageView::SetImage(BMessage *pmsg)
{
	// Replace current image with the image
	// specified in the given BMessage
	
	fbdragFromTracker = false;
	entry_ref ref;
	if (pmsg->FindRef("refs", &ref) != B_OK)
		return;
	
	BBitmap *pbitmap = BTranslationUtils::GetBitmap(&ref);
	if (pbitmap) {
		delete fpbitmap;
		fpbitmap = pbitmap;
		
		// Set the name of the Window to reflect the file name
		BWindow *pwin = Window();
		BEntry entry(&ref);
		if (entry.InitCheck() == B_OK) {
			BPath path(&entry);
			if (path.InitCheck() == B_OK)
				pwin->SetTitle(path.Leaf());
			else
				pwin->SetTitle(IMAGEWINDOW_TITLE);
		} else
			pwin->SetTitle(IMAGEWINDOW_TITLE);
		
		// Resize parent window and set size limits to 
		// reflect the size of the new bitmap
		float width, height;
		BMenuBar *pbar = pwin->KeyMenuBar();
		width = fpbitmap->Bounds().Width() + B_V_SCROLL_BAR_WIDTH;
		height = fpbitmap->Bounds().Height() +
			pbar->Bounds().Height() + B_H_SCROLL_BAR_HEIGHT + 1;
			
		BScreen *pscreen = new BScreen(pwin);
		BRect rctscreen = pscreen->Frame();
		if (width > rctscreen.Width())
			width = rctscreen.Width();
		if (height > rctscreen.Height())
			height = rctscreen.Height();
		pwin->SetSizeLimits(B_V_SCROLL_BAR_WIDTH * 4, width,
			pbar->Bounds().Height() + (B_H_SCROLL_BAR_HEIGHT * 4) + 1, height);
		pwin->SetZoomLimits(width, height);
		
		AdjustScrollBars();
		
		pwin->Zoom();
			// Perform all of the hard work of resizing the
			// window while taking into account the size of
			// the screen, the tab and borders of the window
			//
			// HACK: Works well enough, but not exactly how I would like
		
		// repaint view
		FillRect(Bounds());
		ReDraw();
	} else {
		BAlert *palert = new BAlert(NULL,
			"Sorry, unable to load the image.", "OK");
		palert->Go();
	}
}

