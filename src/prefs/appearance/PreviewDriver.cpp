/*
	PreviewDriver:
		Module based on proto6's ViewDriver for the purpose of Decorator previews.
		
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
#include <GraphicsDefs.h>
#include "PortLink.h"
#include "PreviewDriver.h"
#include "LayerData.h"
#include "ColorSet.h"

BRect preview_bounds(0,0,200,200);;

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

void DrawString(const char *string, int32 length, BPoint pt, escapement_delta *delta=NULL)
{
}
*/

void PreviewDriver::InvertRect(BRect r)
{
	view->viewbmp->Lock();
	drawview->InvertRect(r);
	drawview->Sync();
	view->Invalidate(r);
	view->viewbmp->Unlock();
}

void PreviewDriver::StrokeBezier(BPoint *pts, LayerData *d, int8 *pat)
{
	BPoint points[4];
	for(uint8 i=0;i<4;i++)
		points[i]=pts[i];

	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->StrokeBezier(points,*((pattern *)pat));
	drawview->Sync();
	view->Invalidate();
	view->viewbmp->Unlock();
}

void PreviewDriver::FillBezier(BPoint *pts, LayerData *d, int8 *pat)
{
	BPoint points[4];
	for(uint8 i=0;i<4;i++)
		points[i]=pts[i];

	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->StrokeBezier(points,*((pattern *)pat));
	drawview->Sync();
	view->Invalidate();
	view->viewbmp->Unlock();
}

void PreviewDriver::StrokeEllipse(BRect r, LayerData *d, int8 *pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->StrokeEllipse(r,*((pattern *)pat));
	drawview->Sync();
	view->Invalidate(r);
	view->viewbmp->Unlock();
}

void PreviewDriver::FillEllipse(BRect r, LayerData *d, int8 *pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->FillEllipse(r,*((pattern *)pat));
	drawview->Sync();
	view->Invalidate(r);
	view->viewbmp->Unlock();
}

void PreviewDriver::StrokeArc(BRect r, float angle, float span, LayerData *d, int8 *pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->StrokeArc(r,angle,span,*((pattern *)pat));
	drawview->Sync();
	view->Invalidate(r);
	view->viewbmp->Unlock();
}

void PreviewDriver::FillArc(BRect r, float angle, float span, LayerData *d, int8 *pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->FillArc(r,angle,span,*((pattern *)pat));
	drawview->Sync();
	view->Invalidate(r);
	view->viewbmp->Unlock();
}

void PreviewDriver::StrokeLine(BPoint start, BPoint end, LayerData *d, int8 *pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->StrokeLine(start,end,*((pattern *)pat));
	drawview->Sync();
	view->Invalidate();
	view->viewbmp->Unlock();
}

void PreviewDriver::StrokePolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, int8 *pat, bool is_closed=true)
{
}

void PreviewDriver::FillPolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, int8 *pat)
{
}

void PreviewDriver::StrokeRect(BRect r, LayerData *d, int8 *pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->StrokeRect(r,*((pattern *)pat));
	drawview->Sync();
	view->Invalidate(r);
	view->viewbmp->Unlock();
}

void PreviewDriver::FillRect(BRect r, LayerData *d, int8 *pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->FillRect(r,*((pattern *)pat));
	drawview->Sync();
	view->Invalidate(r);
	view->viewbmp->Unlock();
}

void PreviewDriver::StrokeRoundRect(BRect r, float xrad, float yrad, LayerData *d, int8 *pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->StrokeRoundRect(r,xrad,yrad,*((pattern *)pat));
	drawview->Sync();
	view->Invalidate(r);
	view->viewbmp->Unlock();
}

void PreviewDriver::FillRoundRect(BRect r, float xrad, float yrad, LayerData *d, int8 *pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->FillRoundRect(r,xrad,yrad,*((pattern *)pat));
	drawview->Sync();
	view->Invalidate(r);
	view->viewbmp->Unlock();
}

/*
void StrokeShape(SShape *sh, LayerData *d, int8 *pat)
{
}

void FillShape(SShape *sh, LayerData *d, int8 *pat)
{
}
*/

void PreviewDriver::StrokeTriangle(BPoint *pts, BRect r, LayerData *d, int8 *pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->StrokeTriangle(pts[0],pts[1],
		pts[2],r,*((pattern *)pat));
	drawview->Sync();
	view->Invalidate(r);
	view->viewbmp->Unlock();
}

void PreviewDriver::FillTriangle(BPoint *pts, BRect r, LayerData *d, int8 *pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->FillTriangle(pts[0],pts[1],
		pts[2],r,*((pattern *)pat));
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
