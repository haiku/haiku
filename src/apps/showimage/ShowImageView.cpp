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

#include "ShowImageConstants.h"
#include "ShowImageView.h"

ShowImageView::ShowImageView(BRect rect, const char *name, uint32 resizingMode,
	uint32 flags)
	: BView(rect, name, resizingMode, flags)
{
	fpbitmap = NULL;
	fdocumentIndex = 1;
	fdocumentCount = 1;
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

ShowImageView::~ShowImageView()
{
	delete fpbitmap;
	fpbitmap = NULL;
}

void
ShowImageView::SetImage(const entry_ref *pref)
{
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
	BString str = info.name;
	str << " (page " << fdocumentIndex << " of " << fdocumentCount << ")";
	BMessage msg(MSG_UPDATE_STATUS);
	msg.AddString("status", str);
	BMessenger msgr(Window());
	msgr.SendMessage(&msg);
		
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
	if (fpbitmap)
		DrawBitmap(fpbitmap, updateRect, updateRect);
}

void
ShowImageView::FrameResized(float /* width */, float /* height */)
{
	FixupScrollBars();
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
	if (!fpbitmap)
		return;

	BRect vBds = Bounds(), bBds = fpbitmap->Bounds();
	float prop;
	float range;
	
	BScrollBar *psb = ScrollBar(B_HORIZONTAL);
	if (psb) {
		range = bBds.Width() - vBds.Width();
		if (range < 0) range = 0;
		prop = vBds.Width() / bBds.Width();
		if (prop > 1.0f) prop = 1.0f;
		psb->SetRange(0, range);
		psb->SetProportion(prop);
		psb->SetSteps(10, 100);
	}
	psb = ScrollBar(B_VERTICAL);
	if (psb) {
		range = bBds.Height() - vBds.Height();
		if (range < 0) range = 0;
		prop = vBds.Height() / bBds.Height();
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
