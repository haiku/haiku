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

SRect preview_bounds(0,0,200,200);;

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
	view=new PVView(preview_bounds.Rect());
	drawview=new BView(preview_bounds.Rect(),"drawview",B_FOLLOW_ALL, 0);
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

void PreviewDriver::CopyBits(SRect src, SRect dest)
{
	view->viewbmp->Lock();
	drawview->CopyBits(src.Rect(),dest.Rect());
	drawview->Sync();
	view->Invalidate(dest.Rect());
	view->viewbmp->Unlock();
}

void PreviewDriver::DrawBitmap(ServerBitmap *bmp, SRect src, SRect dest)
{
}

void PreviewDriver::DrawChar(char c, SPoint pt)
{
	view->viewbmp->Lock();
	drawview->DrawChar(c,pt.Point());
	drawview->Sync();
	view->Invalidate();
	view->viewbmp->Unlock();
}

/*
void DrawPicture(SPicture *pic, SPoint pt)
{
}

void DrawString(const char *string, int32 length, SPoint pt, escapement_delta *delta=NULL)
{
}
*/

void PreviewDriver::InvertRect(SRect r)
{
	view->viewbmp->Lock();
	drawview->InvertRect(r.Rect());
	drawview->Sync();
	view->Invalidate(r.Rect());
	view->viewbmp->Unlock();
}

void PreviewDriver::StrokeBezier(SPoint *pts, LayerData *d, int8 *pat)
{
	BPoint points[4];
	for(uint8 i=0;i<4;i++)
		points[i]=pts[i].Point();

	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->StrokeBezier(points,*((pattern *)pat));
	drawview->Sync();
	view->Invalidate();
	view->viewbmp->Unlock();
}

void PreviewDriver::FillBezier(SPoint *pts, LayerData *d, int8 *pat)
{
	BPoint points[4];
	for(uint8 i=0;i<4;i++)
		points[i]=pts[i].Point();

	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->StrokeBezier(points,*((pattern *)pat));
	drawview->Sync();
	view->Invalidate();
	view->viewbmp->Unlock();
}

void PreviewDriver::StrokeEllipse(SRect r, LayerData *d, int8 *pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->StrokeEllipse(r.Rect(),*((pattern *)pat));
	drawview->Sync();
	view->Invalidate(r.Rect());
	view->viewbmp->Unlock();
}

void PreviewDriver::FillEllipse(SRect r, LayerData *d, int8 *pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->FillEllipse(r.Rect(),*((pattern *)pat));
	drawview->Sync();
	view->Invalidate(r.Rect());
	view->viewbmp->Unlock();
}

void PreviewDriver::StrokeArc(SRect r, float angle, float span, LayerData *d, int8 *pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->StrokeArc(r.Rect(),angle,span,*((pattern *)pat));
	drawview->Sync();
	view->Invalidate(r.Rect());
	view->viewbmp->Unlock();
}

void PreviewDriver::FillArc(SRect r, float angle, float span, LayerData *d, int8 *pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->FillArc(r.Rect(),angle,span,*((pattern *)pat));
	drawview->Sync();
	view->Invalidate(r.Rect());
	view->viewbmp->Unlock();
}

void PreviewDriver::StrokeLine(SPoint start, SPoint end, LayerData *d, int8 *pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->StrokeLine(start.Point(),end.Point(),*((pattern *)pat));
	drawview->Sync();
	view->Invalidate();
	view->viewbmp->Unlock();
}

void PreviewDriver::StrokePolygon(SPoint *ptlist, int32 numpts, SRect rect, LayerData *d, int8 *pat, bool is_closed=true)
{
}

void PreviewDriver::FillPolygon(SPoint *ptlist, int32 numpts, SRect rect, LayerData *d, int8 *pat)
{
}

void PreviewDriver::StrokeRect(SRect r, LayerData *d, int8 *pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->StrokeRect(r.Rect(),*((pattern *)pat));
	drawview->Sync();
	view->Invalidate(r.Rect());
	view->viewbmp->Unlock();
}

void PreviewDriver::FillRect(SRect r, LayerData *d, int8 *pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->FillRect(r.Rect(),*((pattern *)pat));
	drawview->Sync();
	view->Invalidate(r.Rect());
	view->viewbmp->Unlock();
}

void PreviewDriver::StrokeRoundRect(SRect r, float xrad, float yrad, LayerData *d, int8 *pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->StrokeRoundRect(r.Rect(),xrad,yrad,*((pattern *)pat));
	drawview->Sync();
	view->Invalidate(r.Rect());
	view->viewbmp->Unlock();
}

void PreviewDriver::FillRoundRect(SRect r, float xrad, float yrad, LayerData *d, int8 *pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->FillRoundRect(r.Rect(),xrad,yrad,*((pattern *)pat));
	drawview->Sync();
	view->Invalidate(r.Rect());
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

void PreviewDriver::StrokeTriangle(SPoint *pts, SRect r, LayerData *d, int8 *pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->StrokeTriangle(pts[0].Point(),pts[1].Point(),
		pts[2].Point(),r.Rect(),*((pattern *)pat));
	drawview->Sync();
	view->Invalidate(r.Rect());
	view->viewbmp->Unlock();
}

void PreviewDriver::FillTriangle(SPoint *pts, SRect r, LayerData *d, int8 *pat)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->FillTriangle(pts[0].Point(),pts[1].Point(),
		pts[2].Point(),r.Rect(),*((pattern *)pat));
	drawview->Sync();
	view->Invalidate(r.Rect());
	view->viewbmp->Unlock();
}

void PreviewDriver::StrokeLineArray(SPoint *pts, int32 numlines, RGBColor *colors, LayerData *d)
{
	view->viewbmp->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetPenSize(d->pensize);
	drawview->BeginLineArray(numlines);
	for(int32 i=0; i<numlines; i++)
		drawview->AddLine(pts[2*i].Point(),pts[(2*i)+1].Point(),colors[i].GetColor32());
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
