//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		PreviewDriver.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Class to display decorators from regular BApplications
//  
//------------------------------------------------------------------------------

/*
	PreviewDriver:
		Module based on app_server's ViewDriver for the purpose of Decorator previews.
		
		The concept is to have a view draw a bitmap, which is a "frame buffer" 
		of sorts, utilize a second view to write to it. This cuts out
		the most problems with having a crapload of code to get just right without
		having to write a bunch of unit tests
		
		Components:		3 classes, VDView, VDWindow, and ViewDriver
		
		PreviewDriver - a wrapper class which mostly posts messages to the VDWindow
		PDView - doesn't do all that much except display the rendered bitmap
*/

//#define DEBUG_DRIVER_MODULE
//#define DEBUG_SERVER_EMU

//#define DISABLE_SERVER_EMU

#include <stdio.h>
#include <iostream.h>
#include <Message.h>
#include <Region.h>
#include <Bitmap.h>
#include <OS.h>
#include <Font.h>
#include <GraphicsDefs.h>
#include "PortLink.h"
#include "PatternHandler.h"
#include "PreviewDriver.h"
#include "LayerData.h"
#include "ColorSet.h"
#include <ft2build.h>
#include FT_FREETYPE_H

BRect preview_bounds(0,0,150,150);;

PVView::PVView(const BRect &bounds)
	: BView(bounds,"previewdriver_view",B_FOLLOW_NONE, B_WILL_DRAW)
{
	viewbmp=new BBitmap(bounds,B_RGB32,true);
}

PVView::~PVView(void)
{
	delete viewbmp;
}

void PVView::AttachedToWindow(void)
{
	win=Window();
}

void PVView::DetachedFromWindow(void)
{
	win=NULL;
}

void PVView::Draw(BRect rect)
{
	if(viewbmp)
	{
		DrawBitmapAsync(viewbmp,rect,rect);
		Sync();
	}
}

PreviewDriver::PreviewDriver(void)
{
	fView=new PVView(preview_bounds);
	fDrawView=new BView(preview_bounds,"drawview",B_FOLLOW_ALL, 0);
}

PreviewDriver::~PreviewDriver(void)
{
	delete fView;
	delete fDrawView;
}

bool PreviewDriver::Initialize(void)
{
	fView->viewbmp->Lock();
	fView->viewbmp->AddChild(fDrawView);
	fView->viewbmp->Unlock();
	return true;
}

void PreviewDriver::Shutdown(void)
{
	fView->viewbmp->Lock();
	fView->viewbmp->RemoveChild(fDrawView);
	fView->viewbmp->Unlock();
}

void PreviewDriver::InvertRect(const BRect &r)
{
	fView->viewbmp->Lock();
	fDrawView->InvertRect(r);
	fDrawView->Sync();
	fView->Invalidate(r);
	fView->viewbmp->Unlock();
}

void PreviewDriver::StrokeLineArray(const int32 &numlines, const LineArrayData *linedata,
	 const DrawData *d)
{
	if(!linedata || !d)
		return;
	
	fView->viewbmp->Lock();
	fDrawView->SetHighColor(d->HighColor().GetColor32());
	fDrawView->SetLowColor(d->LowColor().GetColor32());
	fDrawView->SetPenSize(d->PenSize());
	fDrawView->BeginLineArray(numlines);
	for(int32 i=0; i<numlines; i++)
	{
		const LineArrayData *data=&(linedata[i]);
		fDrawView->AddLine(data->pt1,data->pt2,data->color);
	}
	fDrawView->EndLineArray();
	fDrawView->Sync();
	fView->Invalidate();
	fView->viewbmp->Unlock();
}

bool PreviewDriver::Lock(bigtime_t timeout=B_INFINITE_TIMEOUT)
{
	return true;
}

void PreviewDriver::Unlock(void)
{
}

float
PreviewDriver::StringWidth(const char *string, int32 length, const DrawData *d)
{
	SetDrawData(d,true);
	fView->viewbmp->Lock();
	float value=fDrawView->StringWidth(string,length);
	fView->viewbmp->Unlock();
	
	return value;
}

float
PreviewDriver::StringWidth(const char *string,int32 length, const ServerFont &font)
{
	DrawData d;
	d.SetFont(font);
	return StringWidth(string,length,&d);
}

void
PreviewDriver::SetDrawData(const DrawData *d, bool set_font_data)
{
	if(!d)
		return;
	
	fView->viewbmp->Lock();
	
	BRegion reg=*d->ClippingRegion();
	fDrawView->ConstrainClippingRegion(&reg);
	
	fDrawView->SetPenSize(d->PenSize());
	fDrawView->SetDrawingMode(d->GetDrawingMode());
	
	fDrawView->SetHighColor(d->HighColor().GetColor32());
	fDrawView->SetLowColor(d->LowColor().GetColor32());
	
	fDrawView->SetScale(d->Scale());
	fDrawView->MovePenTo(d->PenLocation());
	
	if(set_font_data)
	{
		BFont font;
		const ServerFont *sf=&(d->Font());

		if(!sf)
			return;

		font.SetFamilyAndStyle(sf->GetFamily(),sf->GetStyle());
		font.SetFlags(sf->Flags());
		font.SetEncoding(sf->Encoding());
		font.SetSize(sf->Size());
		font.SetRotation(sf->Rotation());
		font.SetShear(sf->Shear());
		font.SetSpacing(sf->Spacing());
		fDrawView->SetFont(&font);
	}
	
	fView->viewbmp->Unlock();
}

float
PreviewDriver::StringHeight(const char *string,int32 length,
							const DrawData *d)
{
	SetDrawData(d,true);
	
	fView->viewbmp->Lock();
	
	font_height fh;
	
	fDrawView->GetFontHeight(&fh);
	
	float value=fh.ascent+fh.descent+fh.leading;
	
	fView->viewbmp->Unlock();
	
	return value;
}
