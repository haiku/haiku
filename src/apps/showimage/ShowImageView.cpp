/*****************************************************************************/
// ShowImageView
// Written by Fernando Francisco de Oliveira, Michael Wilber
//
// ShowImageView.cpp
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

#include <stdio.h>
#include <Message.h>
#include <ScrollBar.h>
#include <StopWatch.h>
#include <Alert.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <File.h>
#include <Bitmap.h>
#include <TranslatorRoster.h>
#include <BitmapStream.h>
#include <Rect.h>

#include "ShowImageConstants.h"
#include "ShowImageView.h"

#define BORDER_WIDTH 16
#define BORDER_HEIGHT 16
#define PEN_SIZE 1.0f

ShowImageView::ShowImageView(BRect rect, const char *name, uint32 resizingMode,
	uint32 flags)
	: BView(rect, name, resizingMode, flags)
{
	fpbitmap = NULL;
	fdocumentIndex = 1;
	fdocumentCount = 1;
	fbhasSelection = false;
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetHighColor(0, 0, 0);
	SetPenSize(PEN_SIZE);
}

ShowImageView::~ShowImageView()
{
	delete fpbitmap;
	fpbitmap = NULL;
}

void
ShowImageView::SetImage(const entry_ref *pref)
{
	ClearViewBitmap();
	delete fpbitmap;
	fpbitmap = NULL;
	
	entry_ref ref;
	if (!pref)
		ref = fcurrentRef;
	else
		ref = *pref;

	BTranslatorRoster *proster = BTranslatorRoster::Default();
	if (!proster)
		return;
	BFile file(&ref, B_READ_ONLY);
	translator_info info;
	memset(&info, 0, sizeof(translator_info));
	BMessage ioExtension;
	if (ref != fcurrentRef)
		// if new image, reset to first document
		fdocumentIndex = 1;
	if (ioExtension.AddInt32("/documentIndex", fdocumentIndex) != B_OK)
		return;
	if (proster->Identify(&file, &ioExtension, &info, 0, NULL,
		B_TRANSLATOR_BITMAP) != B_OK)
		return;
	
	// Translate image data and create a new ShowImage window
	BBitmapStream outstream;
	if (proster->Translate(&file, &info, &ioExtension, &outstream,
		B_TRANSLATOR_BITMAP) != B_OK)
		return;
	if (outstream.DetachBitmap(&fpbitmap) != B_OK)
		return;
	fcurrentRef = ref;
	
	// get the number of documents (pages) if it has been supplied
	int32 documentCount = 0;
	if (ioExtension.FindInt32("/documentCount", &documentCount) == B_OK &&
		documentCount > 0)
		fdocumentCount = documentCount;
	else
		fdocumentCount = 1;
		
	// send message to parent about new image
	BMessage msg(MSG_UPDATE_STATUS);
	msg.AddString("status", info.name);
	BMessenger msgr(Window());
	msgr.SendMessage(&msg);
	
	SetViewBitmap(fpbitmap, fpbitmap->Bounds(),
		BRect(BORDER_WIDTH - PEN_SIZE, BORDER_HEIGHT - PEN_SIZE,
			fpbitmap->Bounds().Width() + BORDER_WIDTH + PEN_SIZE,
			fpbitmap->Bounds().Height() + BORDER_HEIGHT + PEN_SIZE),
		B_FOLLOW_TOP | B_FOLLOW_LEFT, 0
	);

	FixupScrollBars();
	Invalidate();
}

BBitmap *
ShowImageView::GetBitmap()
{
	return fpbitmap;
}

void
ShowImageView::AttachedToWindow()
{
	FixupScrollBars();
}

void
ShowImageView::Draw(BRect updateRect)
{
	if (fpbitmap) {
		// Draw black rectangle around image
		StrokeRect(
			BRect(BORDER_WIDTH - PEN_SIZE,
				BORDER_HEIGHT - PEN_SIZE,
				fpbitmap->Bounds().Width() + BORDER_WIDTH + PEN_SIZE,
				fpbitmap->Bounds().Height() + BORDER_HEIGHT + PEN_SIZE));
				
		if (fbhasSelection)
			DrawInvertBox(fselectionRect);
	}
}

void
ShowImageView::DrawInvertBox(BRect &rect)
{
	drawing_mode oldmode = DrawingMode();
	SetDrawingMode(B_OP_INVERT);
	StrokeRect(rect);
	SetDrawingMode(oldmode);
}

void
ShowImageView::FrameResized(float /* width */, float /* height */)
{
	FixupScrollBars();
}

void
ShowImageView::ConstrainToImage(BPoint &point)
{
	point.ConstrainTo(
		BRect(
			BORDER_WIDTH, BORDER_HEIGHT,
			fpbitmap->Bounds().Width() + BORDER_WIDTH,
			fpbitmap->Bounds().Height() + BORDER_HEIGHT
		)
	);
}

void
ShowImageView::MouseDown(BPoint point)
{
	// TODO: Need to handle the case where user clicks
	// inside an existing selection and starts a drag operation
	
	if (fbhasSelection)
		DrawInvertBox(fselectionRect);
	
	fbhasSelection = false;
	
	if (!fpbitmap) {
		// Can't select anything if there is no image
		Invalidate();
		return;
	}

	BPoint firstPoint, secondPoint;
	firstPoint = point;
	ConstrainToImage(firstPoint);
	secondPoint = firstPoint;
		// just to be safe
	
	BRect curSel, lastSel;

	BMessage *pmsg = Window()->CurrentMessage();
	uint32 buttons = static_cast<uint32>(pmsg->FindInt32("buttons"));
	bool bfirst = true;
	// While any combination of mouse buttons is down,
	// allow the user to size their selection. When all
	// mouse buttons are up, the loop ends, and the selection
	// has been made.
	while (buttons) {
		ConstrainToImage(secondPoint);
		curSel = BRect(firstPoint, secondPoint);
		if (!bfirst && curSel != lastSel)
			DrawInvertBox(lastSel);
		if (curSel != lastSel)
			DrawInvertBox(curSel);
				
		snooze(20 * 1000);
		lastSel = curSel;
		GetMouse(&secondPoint, &buttons);
		bfirst = false;
	}

	if (curSel.IntegerWidth() < 2 && curSel.IntegerHeight() < 2) {
		// The user must select at least 2 pixels
		fbhasSelection = false;
		Invalidate();
	} else {
		fselectionRect = curSel;
		fbhasSelection = true;
	}
}

void
ShowImageView::MouseMoved(BPoint point, uint32 state, const BMessage *pmsg)
{
}

void
ShowImageView::MouseUp(BPoint point)
{
}

void
ShowImageView::MessageReceived(BMessage *pmsg)
{
	switch (pmsg->what) {
		default:
			BView::MessageReceived(pmsg);
			break;
	}
}

void
ShowImageView::FixupScrollBars()
{
	BRect rctview = Bounds(), rctbitmap(0, 0, 0, 0);
	if (fpbitmap)
		rctbitmap = fpbitmap->Bounds();
	
	float prop, range;
	BScrollBar *psb = ScrollBar(B_HORIZONTAL);
	if (psb) {
		range = rctbitmap.Width() + (BORDER_WIDTH * 2) - rctview.Width();
		if (range < 0) range = 0;
		prop = rctview.Width() / (rctbitmap.Width() + (BORDER_WIDTH * 2));
		if (prop > 1.0f) prop = 1.0f;
		psb->SetRange(0, range);
		psb->SetProportion(prop);
		psb->SetSteps(10, 100);
	}
	
	psb = ScrollBar(B_VERTICAL);
	if (psb) {
		range = rctbitmap.Height() + (BORDER_HEIGHT * 2) - rctview.Height();
		if (range < 0) range = 0;
		prop = rctview.Height() / (rctbitmap.Height() + (BORDER_HEIGHT * 2));
		if (prop > 1.0f) prop = 1.0f;
		psb->SetRange(0, range);
		psb->SetProportion(prop);
		psb->SetSteps(10, 100);
	}
}

int32
ShowImageView::CurrentPage()
{
	return fdocumentIndex;
}

int32
ShowImageView::PageCount()
{
	return fdocumentCount;
}

void
ShowImageView::FirstPage()
{
	if (fdocumentIndex != 1) {
		fdocumentIndex = 1;
		SetImage(NULL);
	}
}

void
ShowImageView::LastPage()
{
	if (fdocumentIndex != fdocumentCount) {
		fdocumentIndex = fdocumentCount;
		SetImage(NULL);
	}
}

void
ShowImageView::NextPage()
{
	if (fdocumentIndex < fdocumentCount) {
		fdocumentIndex++;
		SetImage(NULL);
	}
}

void
ShowImageView::PrevPage()
{
	if (fdocumentIndex > 1) {
		fdocumentIndex--;
		SetImage(NULL);
	}
}
