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
#include "PreviewDriver.h"
#include "LayerData.h"
#include "ColorSet.h"

BRect preview_bounds(0,0,150,150);;

PVView::PVView(BRect bounds)
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
	view=new PVView(preview_bounds);
	drawview=new BView(preview_bounds,"drawview",B_FOLLOW_ALL, 0);
}

PreviewDriver::~PreviewDriver(void)
{
	delete view;
	delete drawview;
}

bool PreviewDriver::Initialize(void)
{
	view->viewbmp->Lock();
	view->viewbmp->AddChild(drawview);
	view->viewbmp->Unlock();
	return true;
}

void PreviewDriver::Shutdown(void)
{
	view->viewbmp->Lock();
	view->viewbmp->RemoveChild(drawview);
	view->viewbmp->Unlock();
}

void PreviewDriver::CopyBits(BRect src, BRect dest)
{
	view->viewbmp->Lock();
	drawview->CopyBits(src,dest);
	drawview->Sync();
	view->Invalidate(dest);
	view->viewbmp->Unlock();
}

void PreviewDriver::DrawBitmap(ServerBitmap *bmp, BRect src, BRect dest, LayerData *d)
{
}

void PreviewDriver::DrawChar(char c, BPoint pt)
{
	view->viewbmp->Lock();
	drawview->DrawChar(c,pt);
	drawview->Sync();
	view->Invalidate();
	view->viewbmp->Unlock();
}

/*
void DrawPicture(SPicture *pic, BPoint pt)
{
}
*/
void PreviewDriver::DrawString(const char *string, int32 length, BPoint pt, LayerData *d, escapement_delta *delta=NULL)
{
	if(!d)
		return;
	BRect r;
	
	view->viewbmp->Lock();

	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	pt.y--;
	drawview->DrawString(string,length,pt,delta);
	drawview->Sync();
	
	// calculate the invalid rectangle
	font_height fh;
	BFont font;
	drawview->GetFont(&font);
	drawview->GetFontHeight(&fh);
	r.left=pt.x;
	r.right=pt.x+font.StringWidth(string);
	r.top=pt.y-fh.ascent;
	r.bottom=pt.y+fh.descent;
	view->Invalidate(r);
	
	view->viewbmp->Unlock();
}


void PreviewDriver::InvertRect(BRect r)
{
	view->viewbmp->Lock();
	drawview->InvertRect(r);
	drawview->Sync();
	view->Invalidate(r);
	view->viewbmp->Unlock();
}

void PreviewDriver::StrokeBezier(BPoint *pts, LayerData *d, const Pattern &pat)
{
	BPoint points[4];
	for(uint8 i=0;i<4;i++)
		points[i]=pts[i];

	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->StrokeBezier(points,*((pattern *)pat.GetInt8()));
	drawview->Sync();
	view->Invalidate();
	view->viewbmp->Unlock();
}

void PreviewDriver::FillBezier(BPoint *pts, LayerData *d, const Pattern &pat)
{
	BPoint points[4];
	for(uint8 i=0;i<4;i++)
		points[i]=pts[i];

	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->StrokeBezier(points,*((pattern *)pat.GetInt8()));
	drawview->Sync();
	view->Invalidate();
	view->viewbmp->Unlock();
}

void PreviewDriver::StrokeEllipse(BRect r, LayerData *d, const Pattern &pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->StrokeEllipse(r,*((pattern *)pat.GetInt8()));
	drawview->Sync();
	view->Invalidate(r);
	view->viewbmp->Unlock();
}

void PreviewDriver::FillEllipse(BRect r, LayerData *d, const Pattern &pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->FillEllipse(r,*((pattern *)pat.GetInt8()));
	drawview->Sync();
	view->Invalidate(r);
	view->viewbmp->Unlock();
}

void PreviewDriver::StrokeArc(BRect r, float angle, float span, LayerData *d, const Pattern &pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->StrokeArc(r,angle,span,*((pattern *)pat.GetInt8()));
	drawview->Sync();
	view->Invalidate(r);
	view->viewbmp->Unlock();
}

void PreviewDriver::FillArc(BRect r, float angle, float span, LayerData *d, const Pattern &pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->FillArc(r,angle,span,*((pattern *)pat.GetInt8()));
	drawview->Sync();
	view->Invalidate(r);
	view->viewbmp->Unlock();
}

void PreviewDriver::StrokeLine(BPoint start, BPoint end, LayerData *d, const Pattern &pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->StrokeLine(start,end,*((pattern *)pat.GetInt8()));
	drawview->Sync();
	view->Invalidate();
	view->viewbmp->Unlock();
}

void PreviewDriver::StrokePolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, const Pattern &pat, bool is_closed=true)
{
}

void PreviewDriver::FillPolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, const Pattern &pat)
{
}

void PreviewDriver::StrokeRect(BRect r, LayerData *d, const Pattern &pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->StrokeRect(r,*((pattern *)pat.GetInt8()));
	drawview->Sync();
	view->Invalidate(r);
	view->viewbmp->Unlock();
}

void PreviewDriver::FillRect(BRect r, LayerData *d, const Pattern &pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->FillRect(r,*((pattern *)pat.GetInt8()));
	drawview->Sync();
	view->Invalidate(r);
	view->viewbmp->Unlock();
}

void PreviewDriver::StrokeRoundRect(BRect r, float xrad, float yrad, LayerData *d, const Pattern &pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->StrokeRoundRect(r,xrad,yrad,*((pattern *)pat.GetInt8()));
	drawview->Sync();
	view->Invalidate(r);
	view->viewbmp->Unlock();
}

void PreviewDriver::FillRoundRect(BRect r, float xrad, float yrad, LayerData *d, const Pattern &pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->FillRoundRect(r,xrad,yrad,*((pattern *)pat.GetInt8()));
	drawview->Sync();
	view->Invalidate(r);
	view->viewbmp->Unlock();
}

/*
void StrokeShape(SShape *sh, LayerData *d, const Pattern &pat)
{
}

void FillShape(SShape *sh, LayerData *d, const Pattern &pat)
{
}
*/

void PreviewDriver::StrokeTriangle(BPoint *pts, BRect r, LayerData *d, const Pattern &pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->StrokeTriangle(pts[0],pts[1],
		pts[2],r,*((pattern *)pat.GetInt8()));
	drawview->Sync();
	view->Invalidate(r);
	view->viewbmp->Unlock();
}

void PreviewDriver::FillTriangle(BPoint *pts, BRect r, LayerData *d, const Pattern &pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->FillTriangle(pts[0],pts[1],
		pts[2],r,*((pattern *)pat.GetInt8()));
	drawview->Sync();
	view->Invalidate(r);
	view->viewbmp->Unlock();
}

void PreviewDriver::StrokeLineArray(BPoint *pts, int32 numlines, RGBColor *colors, LayerData *d)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->BeginLineArray(numlines);
	for(int32 i=0; i<numlines; i++)
		drawview->AddLine(pts[2*i],pts[(2*i)+1],colors[i].GetColor32());
	drawview->EndLineArray();
	drawview->Sync();
	view->Invalidate();
	view->viewbmp->Unlock();
}

void PreviewDriver::SetMode(int32 mode)
{
}

bool PreviewDriver::DumpToFile(const char *path)
{
	return false;
}

float PreviewDriver::StringWidth(const char *string, int32 length, LayerData *d)
{
	view->viewbmp->Lock();
	float returnval=drawview->StringWidth(string,length);
	view->viewbmp->Unlock();
	return returnval;
}

void PreviewDriver::GetTruncatedStrings( const char **instrings, int32 stringcount, uint32 mode, float maxwidth, char **outstrings)
{
	view->viewbmp->Lock();
	be_plain_font->GetTruncatedStrings(instrings,stringcount,mode,maxwidth,outstrings);
	view->viewbmp->Unlock();
}
