/*
	ViewDriver:
		First, slowest, and easiest driver class in the app_server which is designed
		to utilize the BeOS graphics functions to cut out a lot of junk in getting the
		drawing infrastructure in this server.
		
		The concept is to have VDView::Draw() draw a bitmap, which is a "frame buffer" 
		of sorts, utilize a second view to write to it. This cuts out
		the most problems with having a crapload of code to get just right without
		having to write a bunch of unit tests
		
		Components:		3 classes, VDView, VDWindow, and ViewDriver
		
		ViewDriver - a wrapper class which mostly posts messages to the VDWindow
		VDWindow - does most of the work.
		VDView - doesn't do all that much except display the rendered bitmap
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
#include <Font.h>

#include "PortLink.h"
#include "ServerProtocol.h"
#include "ServerBitmap.h"
#include "ViewDriver.h"
#include "ServerCursor.h"
#include "ServerFont.h"
#include "FontFamily.h"
//#include "DebugTools.h"
#include "LayerData.h"

enum
{
VDWIN_CLEAR=100,

VDWIN_SHOWCURSOR,
VDWIN_HIDECURSOR,
VDWIN_OBSCURECURSOR,
VDWIN_MOVECURSOR,
VDWIN_SETCURSOR,
};

ViewDriver *viewdriver_global;

#ifdef DEBUG_DRIVER_MODULE

void DumpLayerData(LayerData *ld)
{
	if(ld)
	{
		printf("Layer Data: \n");
		printf("\tPen Size: %f\n"
				"\tPen Location: (%f,%f)\n"
				"\tDraw Mode: %d\n"
				"\tScale: %f\n",
				ld->pensize,
				ld->penlocation.x, ld->penlocation.y,
				ld->draw_mode,
				ld->scale);
		ld->highcolor.PrintToStream();
		ld->lowcolor.PrintToStream();
	}
	else
	{
		printf("Layer Data: NULL\n");
	}
}

#endif

VDView::VDView(BRect bounds)
	: BView(bounds,"viewdriver_view",B_FOLLOW_ALL, B_WILL_DRAW)
{
	SetViewColor(B_TRANSPARENT_32_BIT);
	viewbmp=new BBitmap(bounds,B_RGB32,true);

	// This link for sending mouse messages to the OBAppServer.
	// This is only to take the place of the Input Server. I suppose I could write
	// an addon filter to be more like the Input Server, but then I wouldn't be working
	// on this thing! :P
	serverlink=new PortLink(find_port(SERVER_INPUT_PORT));

#ifdef DEBUG_DRIVER_MODULE
	printf("VDView: app_server input port: %ld\n",serverlink->GetPort());
#endif

	// Create a cursor which isn't just a box
	cursor=new BBitmap(BRect(0,0,20,20),B_RGBA32,true);
	BView *v=new BView(cursor->Bounds(),"v", B_FOLLOW_NONE, B_WILL_DRAW);
	hide_cursor=0;

	cursor->Lock();
	cursor->AddChild(v);

	v->SetHighColor(255,255,255,0);
	v->FillRect(cursor->Bounds());
	v->SetHighColor(255,0,0,255);
	v->FillTriangle(cursor->Bounds().LeftTop(),cursor->Bounds().RightTop(),cursor->Bounds().LeftBottom());

	cursor->RemoveChild(v);
	cursor->Unlock();
	
	cursorframe=cursor->Bounds();
	oldcursorframe=cursor->Bounds();
}

VDView::~VDView(void)
{
	delete serverlink;
	delete viewbmp;
	delete cursor;
}

void VDView::AttachedToWindow(void)
{
}

void VDView::Draw(BRect rect)
{
	if(viewbmp)
	{
		DrawBitmapAsync(viewbmp,oldcursorframe,oldcursorframe);
		DrawBitmapAsync(viewbmp,rect,rect);
	
		if(hide_cursor==0 && obscure_cursor==false)
		{
			SetDrawingMode(B_OP_ALPHA);
			DrawBitmapAsync(cursor,cursor->Bounds(),cursorframe);
			SetDrawingMode(B_OP_COPY);
		}
		Sync();
	}
}

// These functions emulate the Input Server by sending the *exact* same kind of messages
// to the server's port. Being we're using a regular window, it would make little sense
// to do anything else.

void VDView::MouseDown(BPoint pt)
{
#ifdef DEBUG_SERVER_EMU
printf("ViewDriver::MouseDown\n");
#endif
	// Attach data:
	// 1) int64 - time of mouse click
	// 2) float - x coordinate of mouse click
	// 3) float - y coordinate of mouse click
	// 4) int32 - modifier keys down
	// 5) int32 - buttons down
	// 6) int32 - clicks
#ifndef DISABLE_SERVER_EMU
	BPoint p;

	uint32 buttons,
		mod=modifiers(),
		clicks=1;		// can't get the # of clicks without a *lot* of extra work :(

	int64 time=(int64)real_time_clock();

	GetMouse(&p,&buttons);

	serverlink->SetOpCode(B_MOUSE_DOWN);
	serverlink->Attach(&time, sizeof(int64));
	serverlink->Attach(&pt.x,sizeof(float));
	serverlink->Attach(&pt.y,sizeof(float));
	serverlink->Attach(&mod, sizeof(uint32));
	serverlink->Attach(&buttons, sizeof(uint32));
	serverlink->Attach(&clicks, sizeof(uint32));
	serverlink->Flush();
#endif
}

void VDView::MouseMoved(BPoint pt, uint32 transit, const BMessage *msg)
{
	// Attach data:
	// 1) int64 - time of mouse click
	// 2) float - x coordinate of mouse click
	// 3) float - y coordinate of mouse click
	// 4) int32 - buttons down
#ifndef DISABLE_SERVER_EMU
	BPoint p;
	uint32 buttons;
	int64 time=(int64)real_time_clock();

	serverlink->SetOpCode(B_MOUSE_MOVED);
	serverlink->Attach(&time,sizeof(int64));
	serverlink->Attach(&pt.x,sizeof(float));
	serverlink->Attach(&pt.y,sizeof(float));
	GetMouse(&p,&buttons);
	serverlink->Attach(&buttons,sizeof(int32));
	serverlink->Flush();
#endif
}

void VDView::MouseUp(BPoint pt)
{
#ifdef DEBUG_SERVER_EMU
printf("ViewDriver::MouseUp\n");
#endif
	// Attach data:
	// 1) int64 - time of mouse click
	// 2) float - x coordinate of mouse click
	// 3) float - y coordinate of mouse click
	// 4) int32 - modifier keys down
#ifndef DISABLE_SERVER_EMU
	BPoint p;

	uint32 buttons,
		mod=modifiers();

	int64 time=(int64)real_time_clock();

	GetMouse(&p,&buttons);

	serverlink->SetOpCode(B_MOUSE_UP);
	serverlink->Attach(&time, sizeof(int64));
	serverlink->Attach(&pt.x,sizeof(float));
	serverlink->Attach(&pt.y,sizeof(float));
	serverlink->Attach(&mod, sizeof(uint32));
	serverlink->Flush();
#endif
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

VDWindow::VDWindow(void)
	: BWindow(BRect(100,60,740,540),"OBOS App Server, P7",B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
	view=new VDView(Bounds());
	AddChild(view);
}

VDWindow::~VDWindow(void)
{
}

void VDWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case VDWIN_SHOWCURSOR:
		{
			if(view->hide_cursor>0)
			{
				view->hide_cursor--;
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: ShowCursor() - still hidden\n");
#endif
			}
			if(view->hide_cursor==0)
			{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: ShowCursor() - cursor shown\n");
#endif
				view->Invalidate(view->cursorframe);
			}
			break;
		}
		case VDWIN_HIDECURSOR:
		{
			view->hide_cursor++;
			if(view->hide_cursor==1)
			{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: HideCursor() - cursor was hidden\n");
#endif
				view->Invalidate(view->cursorframe);
			}
			else
			{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: HideCursor()\n");
#endif
			}
			break;
		}
		case VDWIN_OBSCURECURSOR:
		{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: ObscureCursor()\n");
#endif
			view->obscure_cursor=true;
			view->Invalidate(view->cursorframe);
			break;
		}
		case VDWIN_MOVECURSOR:
		{
			float x,y;
			msg->FindFloat("x",&x);
			msg->FindFloat("y",&y);

			// this was changed because an extra message was
			// sent even though the mouse was never moved
			if(view->cursorframe.left!=x || view->cursorframe.top!=y)
			{
				if(view->obscure_cursor)
					view->obscure_cursor=false;
	
				view->oldcursorframe=view->cursorframe;
				view->cursorframe.OffsetTo(x,y);
			}

			if(view->hide_cursor==0)
				view->Invalidate(view->oldcursorframe);
			break;
		}
		case VDWIN_SETCURSOR:
		{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: SetCursor()\n");
#endif
			ServerCursor *cdata;
			msg->FindPointer("SCursor",(void**)&cdata);

			if(cdata!=NULL)
			{
				BBitmap *bmp=new BBitmap(cdata->Bounds(), B_RGBA32);

				// Copy the server bitmap in the cursor to a BBitmap
				uint8	*sbmppos=(uint8*)cdata->Buffer(),
						*bbmppos=(uint8*)bmp->Bits();

				int32 bytes=cdata->BytesPerLine(),
					bbytes=bmp->BytesPerRow();

				for(int i=0;i<cdata->Height();i++)
					memcpy(bbmppos+(i*bbytes), sbmppos+(i*bytes), bytes);

				// Replace the bitmap
				delete view->cursor;
				view->cursor=bmp;
				view->Invalidate(view->cursorframe);
				break;
			}
			break;
		}
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

bool VDWindow::QuitRequested(void)
{
	port_id serverport=find_port(SERVER_PORT_NAME);

	if(serverport!=B_NAME_NOT_FOUND)
		write_port(serverport,B_QUIT_REQUESTED,NULL,0);

	return true;
}

void VDWindow::WindowActivated(bool active)
{
	// This is just to hide the regular system cursor so we can see our own
	if(active)
		be_app->HideCursor();
	else
		be_app->ShowCursor();
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

ViewDriver::ViewDriver(void)
{
	viewdriver_global=this;
	screenwin=new VDWindow();
	framebuffer=screenwin->view->viewbmp;
	serverlink=screenwin->view->serverlink;
	hide_cursor=0;
}

ViewDriver::~ViewDriver(void)
{
	if(is_initialized)
	{
		screenwin->Lock();
		screenwin->Quit();
		screenwin=NULL;
	}
}

bool ViewDriver::Initialize(void)
{
	Lock();
	drawview=new BView(framebuffer->Bounds(),"drawview",B_FOLLOW_ALL, B_WILL_DRAW);
	framebuffer->AddChild(drawview);

	hide_cursor=0;
	obscure_cursor=false;

	is_initialized=true;

	// We can afford to call the above functions without locking
	// because the window is locked until Show() is first called
	screenwin->Show();
	Unlock();
	return true;
}

void ViewDriver::Shutdown(void)
{
	Lock();
	is_initialized=false;
	Unlock();
}

void ViewDriver::SetMode(int32 space)
{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: SetMode(%ld)\n",space);
#endif
	screenwin->Lock();
	int16 w=640,h=480;
	color_space s;

	switch(space)
	{
		case B_32_BIT_800x600:
		case B_16_BIT_800x600:
		case B_8_BIT_800x600:
		{
			w=800; h=600;
			break;
		}
		case B_32_BIT_1024x768:
		case B_16_BIT_1024x768:
		case B_8_BIT_1024x768:
		{
			w=1024; h=768;
			break;
		}
		default:
			break;
	}
	screenwin->ResizeTo(w-1,h-1);

	switch(space)
	{
		case B_32_BIT_640x480:
		case B_32_BIT_800x600:
		case B_32_BIT_1024x768:
			s=B_RGBA32;
			break;
		case B_16_BIT_640x480:
		case B_16_BIT_800x600:
		case B_16_BIT_1024x768:
			s=B_RGBA15;
			break;
		case B_8_BIT_640x480:
		case B_8_BIT_800x600:
		case B_8_BIT_1024x768:
			s=B_CMAP8;
			break;
		default:
			break;
	}
	
	
	delete framebuffer;
	
	screenwin->view->viewbmp=new BBitmap(BRect(0,0,w-1,h-1),s,true);
	framebuffer=screenwin->view->viewbmp;
	drawview=new BView(framebuffer->Bounds(),"drawview",B_FOLLOW_ALL, B_WILL_DRAW);
	framebuffer->AddChild(drawview);

	framebuffer->Lock();
	drawview->SetHighColor(80,85,152);
	drawview->FillRect(drawview->Bounds());
	drawview->Sync();
	framebuffer->Unlock();
	screenwin->view->Invalidate();
	screenwin->Unlock();
}

void ViewDriver::CopyBits(BRect src, BRect dest)
{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: CopyBits()\n"); src.PrintToStream(); dest.PrintToStream();
#endif
	screenwin->Lock();
	framebuffer->Lock();
	drawview->CopyBits(src,dest);
	drawview->Sync();
	screenwin->view->Invalidate(dest);
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::DrawBitmap(ServerBitmap *bitmap, BRect src, BRect dest)
{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: DrawBitmap unimplemented()\n");
#endif
}

void ViewDriver::DrawChar(char c, BPoint pt, LayerData *d)
{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: DrawChar(%c, BPoint(%f,%f))\n",c,pt.x,pt.y);
#endif
	char str[2];
	str[0]=c;
	str[1]='\0';
	DrawString(str, 1, pt, d);
}

void ViewDriver::DrawString(const char *string, int32 length, BPoint pt, LayerData *d, escapement_delta *delta=NULL)
{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: DrawString(\"%s\",%ld,BPoint(%f,%f))\n",string,length,pt.x,pt.y);
#endif
	if(!d)
		return;
	BRect r;
	
	screenwin->Lock();
	framebuffer->Lock();
	
	SetLayerData(d,true);	// set all layer data and additionally set the font-related data

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
	screenwin->view->Invalidate(r);
	
	framebuffer->Unlock();
	screenwin->Unlock();
	
}
/*
bool ViewDriver::DumpToFile(const char *path)
{
	return false;
}
*/
void ViewDriver::FillArc(BRect r, float angle, float span, LayerData *d, int8 *pat)
{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: FillArc()\n");
#endif
	if(!pat || !d)
		return;
	screenwin->Lock();
	framebuffer->Lock();
	SetLayerData(d);
	drawview->FillArc(r,angle,span,*((pattern*)pat));
	drawview->Sync();
	screenwin->view->Invalidate(r);
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::FillBezier(BPoint *pts, LayerData *d, int8 *pat)
{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: FillBezier()\n");
#endif
	if(!pat || !pts)
		return;
	screenwin->Lock();
	framebuffer->Lock();
	SetLayerData(d);
	drawview->FillBezier(pts,*((pattern*)pat));
	drawview->Sync();
	
	// Invalidate the whole view until I get around to adding in the invalid rect calc code
	screenwin->view->Invalidate();
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::FillEllipse(BRect r, LayerData *d, int8 *pat)
{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: FillEllipse()\n");
#endif
	if(!pat || !d)
		return;
	screenwin->Lock();
	framebuffer->Lock();
	SetLayerData(d);
	drawview->FillEllipse(r,*((pattern*)pat));
	drawview->Sync();
	screenwin->view->Invalidate(r);
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::FillPolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, int8 *pat)
{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: FillPolygon unimplemented\n");
#endif
	if(!pat || !ptlist)
		return;
}

void ViewDriver::FillRect(BRect r, LayerData *d, int8 *pat)
{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: FillRect()\n");
#endif
	if(!pat || !d)
		return;
	screenwin->Lock();
	framebuffer->Lock();
	SetLayerData(d);
	drawview->FillRect(r,*((pattern*)pat));
	drawview->Sync();
	screenwin->view->Invalidate(r);
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::FillRoundRect(BRect r, float xrad, float yrad, LayerData *d, int8 *pat)
{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: FillRoundRect()\n");
#endif
	if(!pat || !d)
		return;
	screenwin->Lock();
	framebuffer->Lock();
	SetLayerData(d);
	drawview->FillRoundRect(r,xrad,yrad,*((pattern*)pat));
	drawview->Sync();
	screenwin->view->Invalidate(r);
	framebuffer->Unlock();
	screenwin->Unlock();

}

void ViewDriver::FillTriangle(BPoint *pts, BRect r, LayerData *d, int8 *pat)
{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: FillTriangle()\n");
#endif
	if(!pat || !pts)
		return;
	screenwin->Lock();
	framebuffer->Lock();
	BPoint first=pts[0],second=pts[1],third=pts[2];
	SetLayerData(d);
	drawview->FillTriangle(first,second,third,r,*((pattern*)pat));
	drawview->Sync();
	screenwin->view->Invalidate(r);
	framebuffer->Unlock();
	screenwin->Unlock();

}

void ViewDriver::HideCursor(void)
{
	screenwin->Lock();
	Lock();

	hide_cursor++;
	screenwin->PostMessage(VDWIN_HIDECURSOR);

	Unlock();
	screenwin->Unlock();
}

void ViewDriver::InvertRect(BRect r)
{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: InvertRect()\n");
#endif
	screenwin->Lock();
	framebuffer->Lock();
	drawview->InvertRect(r);
	drawview->Sync();
	screenwin->view->Invalidate(r);
	framebuffer->Unlock();
	screenwin->Unlock();
}

bool ViewDriver::IsCursorHidden(void)
{
	screenwin->Lock();
	bool value=(hide_cursor>0)?true:false;
	screenwin->Unlock();
	return value;
}

void ViewDriver::ObscureCursor(void)
{
	screenwin->Lock();
	screenwin->PostMessage(VDWIN_OBSCURECURSOR);
	screenwin->Unlock();
}

void ViewDriver::MoveCursorTo(float x, float y)
{
	screenwin->Lock();
	BMessage *msg=new BMessage(VDWIN_MOVECURSOR);
	msg->AddFloat("x",x);
	msg->AddFloat("y",y);
	screenwin->PostMessage(msg);
	screenwin->Unlock();
}

void ViewDriver::SetCursor(ServerCursor *cursor)
{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: SetCursor(%p)\n",cursor);
#endif
	if(cursor!=NULL)
	{
		screenwin->Lock();
		BBitmap *bmp=new BBitmap(cursor->Bounds(),B_RGBA32);
	
		// Copy the server bitmap in the cursor to a BBitmap
		uint8	*sbmppos=(uint8*)cursor->Buffer(),
				*bbmppos=(uint8*)bmp->Bits();
	
		int32 bytes=cursor->BytesPerLine(),
			bbytes=bmp->BytesPerRow();
	
		for(int i=0;i<cursor->Height();i++)
			memcpy(bbmppos+(i*bbytes), sbmppos+(i*bytes), bytes);
	
		// Replace the bitmap
		delete screenwin->view->cursor;
		screenwin->view->cursor=bmp;
		screenwin->view->Invalidate(screenwin->view->cursorframe);
		screenwin->Unlock();
	}
}

void ViewDriver::ShowCursor(void)
{
	screenwin->Lock();
	if(hide_cursor>0)
	{
		hide_cursor--;
		screenwin->PostMessage(VDWIN_SHOWCURSOR);
	}
	screenwin->Unlock();

}

void ViewDriver::StrokeArc(BRect r, float angle, float span, LayerData *d, int8 *pat)
{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: StrokeArc()\n");
#endif
	if(!pat || !d)
		return;
	screenwin->Lock();
	framebuffer->Lock();
	SetLayerData(d);
	drawview->StrokeArc(r,angle,span,*((pattern*)pat));
	drawview->Sync();
	screenwin->view->Invalidate(r);
	framebuffer->Unlock();
	screenwin->Unlock();

}

void ViewDriver::StrokeBezier(BPoint *pts, LayerData *d, int8 *pat)
{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: StrokeBezier()\n");
#endif
	if(!pat || !pts)
		return;
	screenwin->Lock();
	framebuffer->Lock();
	SetLayerData(d);
	drawview->StrokeBezier(pts,*((pattern*)pat));
	drawview->Sync();
	
	// Invalidate the whole view until I get around to adding in the invalid rect calc code
	screenwin->view->Invalidate();
	framebuffer->Unlock();
	screenwin->Unlock();

}

void ViewDriver::StrokeEllipse(BRect r, LayerData *d, int8 *pat)
{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: StrokeEllipse()\n");
#endif
	if(!pat || !d)
		return;
	screenwin->Lock();
	framebuffer->Lock();
	SetLayerData(d);
	drawview->StrokeEllipse(r,*((pattern*)pat));
	drawview->Sync();
	screenwin->view->Invalidate(r);
	framebuffer->Unlock();
	screenwin->Unlock();

}

void ViewDriver::StrokeLine(BPoint start, BPoint end, LayerData *d, int8 *pat)
{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: StrokeLine()\n");
#endif
	if(!pat || !d)
		return;
	screenwin->Lock();
	framebuffer->Lock();
	SetLayerData(d);
	drawview->StrokeLine(start,end,*((pattern*)pat));
	drawview->Sync();
	screenwin->view->Invalidate(BRect(start,end));
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::StrokeLineArray(BPoint *pts, int32 numlines, RGBColor *colors, LayerData *d)
{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: StrokeLineArray unimplemented\n");
#endif
}

void ViewDriver::StrokePolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, int8 *pat, bool is_closed=true)
{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: StrokePolygon()\n");
#endif
	if(!pat || !ptlist)
		return;
	screenwin->Lock();
	framebuffer->Lock();

	BRegion invalid;

	SetLayerData(d);
	drawview->BeginLineArray(numpts+2);
	for(int i=1;i<numpts;i++)
	{
		drawview->AddLine(ptlist[i-1],ptlist[i],d->highcolor.GetColor32());
		invalid.Include(BRect(ptlist[i-1],ptlist[i]));
	}

	if(is_closed)
	{
		drawview->AddLine(ptlist[numpts-1],ptlist[0],d->highcolor.GetColor32());
		invalid.Include(BRect(ptlist[numpts-1],ptlist[0]));
	}
	drawview->EndLineArray();

	drawview->Sync();
	screenwin->view->Invalidate(invalid.Frame());
	framebuffer->Unlock();
	screenwin->Unlock();

}

void ViewDriver::StrokeRect(BRect r, LayerData *d, int8 *pat)
{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: StrokeRect()\n");
#endif
	if(!pat || !d)
		return;
	screenwin->Lock();
	framebuffer->Lock();
	SetLayerData(d);
	drawview->StrokeRect(r,*((pattern*)pat));
	drawview->Sync();
	screenwin->view->Invalidate(r);
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::StrokeRoundRect(BRect r, float xrad, float yrad, LayerData *d, int8 *pat)
{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: StrokeRoundRect()\n");
#endif
	if(!pat || !d)
		return;
	screenwin->Lock();
	framebuffer->Lock();
	SetLayerData(d);
	drawview->StrokeRoundRect(r,xrad,yrad,*((pattern*)pat));
	drawview->Sync();
	screenwin->view->Invalidate(r);
	framebuffer->Unlock();
	screenwin->Unlock();

}

void ViewDriver::StrokeTriangle(BPoint *pts, BRect r, LayerData *d, int8 *pat)
{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: StrokeTriangle()\n");
#endif
	if(!pat || !pts || !d)
		return;
	screenwin->Lock();
	framebuffer->Lock();
	BPoint first=pts[0],second=pts[1],third=pts[2];
	SetLayerData(d);
	drawview->StrokeTriangle(first,second,third,r,*((pattern*)pat));
	drawview->Sync();
	screenwin->view->Invalidate(r);
	framebuffer->Unlock();
	screenwin->Unlock();

}

void ViewDriver::SetLayerData(LayerData *d, bool set_font_data=false)
{
	if(!d)
		return;

	drawview->SetPenSize(d->pensize);
	drawview->SetDrawingMode(d->draw_mode);
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetScale(d->scale);
	drawview->MovePenTo(d->penlocation);
	if(set_font_data)
	{
		BFont font;
		ServerFont *sf=d->font;

		if(!sf)
		{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver::SetLayerData was passed a NULL ServerFont\n");
#endif
			return;
		}
		FontStyle *style=d->font->Style();

		if(!style)
		{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver::SetLayerData font had a NULL style\n");
#endif
			return;
		}
		
		FontFamily *family=(FontFamily *)style->Family();
		if(!family)
		{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver::SetLayerData font had a NULL family\n");
#endif
		}
		font.SetFamilyAndStyle((font_family)family->GetName(),style->Style());
		font.SetFlags(sf->Flags());
		font.SetEncoding(sf->Encoding());
		font.SetSize(sf->Size());
		font.SetRotation(sf->Rotation());
		font.SetShear(sf->Shear());
		font.SetSpacing(sf->Spacing());
		drawview->SetFont(&font);
	}
}
