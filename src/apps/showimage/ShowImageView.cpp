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
#include <Bitmap.h>
#include <Message.h>
#include <ScrollBar.h>
#include <StopWatch.h>
#include <Alert.h>
#include <MenuBar.h>
#include <MenuItem.h>

#include "ShowImageConstants.h"
#include "ShowImageView.h"

ShowImageView::ShowImageView(BRect rect, const char *name, uint32 resizingMode,
	uint32 flags)
	: BView(rect, name, resizingMode, flags)
{
	fpbitmap = NULL;
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

ShowImageView::~ShowImageView()
{
	delete fpbitmap;
	fpbitmap = NULL;
}

void
ShowImageView::SetBitmap(BBitmap *pbitmap)
{
	fpbitmap = pbitmap;
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

