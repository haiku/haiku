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

#define DEBUG_DRIVER_MODULE
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
#include "ServerProtocol.h"
#include "ServerBitmap.h"
#include "ViewDriver.h"
#include "ServerCursor.h"
#include "DebugTools.h"

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

VDView::VDView(BRect bounds)
	: BView(bounds,"viewdriver_view",B_FOLLOW_ALL, B_WILL_DRAW)
{
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
printf("ViewDriver::MouseDown\t");
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
printf("ViewDriver::MouseUp\t");
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
	: BWindow(BRect(100,60,740,540),"OBOS App Server, P6",B_TITLED_WINDOW,
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
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: HideCursor()\n");
#endif
			if(view->hide_cursor==1)
			{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: HideCursor() - cursor was hidden\n");
#endif
				view->Invalidate(view->cursorframe);
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
				BBitmap *bmp=new BBitmap(BRect(0,0,cdata->width-1,cdata->height-1),
					cdata->cspace);

				// Copy the server bitmap in the cursor to a BBitmap
				uint8	*sbmppos=(uint8*)cdata->bitmap->Buffer(),
						*bbmppos=(uint8*)bmp->Bits();

				int32 bytes=cdata->bitmap->bytesperline,
					bbytes=bmp->BytesPerRow();

				for(int i=0;i<cdata->height;i++)
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
	}
}

void ViewDriver::Initialize(void)
{
	drawview=new BView(framebuffer->Bounds(),"drawview",B_FOLLOW_ALL, B_WILL_DRAW);
	framebuffer->AddChild(drawview);

	hide_cursor=0;
	obscure_cursor=false;

	SetPenSize(1.0);
	MovePenTo(BPoint(0,0));
	is_initialized=true;

	// We can afford to call the above functions without locking
	// because the window is locked until Show() is first called
	screenwin->Show();
}

void ViewDriver::Shutdown(void)
{
	locker->Lock();
	is_initialized=false;
	locker->Unlock();
}

bool ViewDriver::IsInitialized(void)
{
	return is_initialized;
}

void ViewDriver::SafeMode(void)
{
	SetScreen(B_8_BIT_640x480);
}

void ViewDriver::Reset(void)
{
	SafeMode();
	Clear(255,255,255);
}

void ViewDriver::SetScreen(uint32 space)
{
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

	screenwin->Unlock();
}

void ViewDriver::Clear(uint8 red, uint8 green, uint8 blue)
{
	screenwin->Lock();
	framebuffer->Lock();
	drawview->SetHighColor(red,green,blue);
	drawview->FillRect(drawview->Bounds());
	drawview->Sync();
	screenwin->view->Invalidate();
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::Clear(rgb_color col)
{
	screenwin->Lock();
	framebuffer->Lock();
	drawview->SetHighColor(col);
	drawview->FillRect(drawview->Bounds());
	drawview->Sync();
	screenwin->view->Invalidate();
	framebuffer->Unlock();
	screenwin->Unlock();
}

int32 ViewDriver::GetHeight(void)
{
	screenwin->Lock();
	int32 h=bufferheight;
	screenwin->Unlock();

	return h;
}

int32 ViewDriver::GetWidth(void)
{
	screenwin->Lock();
	int32 w=bufferwidth;
	screenwin->Unlock();

	return w;
}

int ViewDriver::GetDepth(void)
{
	screenwin->Lock();
	int d=bufferdepth;
	screenwin->Unlock();

	return d;
}

void ViewDriver::AddLine(BPoint pt1, BPoint pt2, rgb_color col)
{
	drawview->AddLine(pt1,pt2,col);
	laregion.Include(BRect(pt1,pt2));
}

void ViewDriver::BeginLineArray(int32 count)
{
	// NOTE: the unlocking is done in EndLineArray()!
	screenwin->Lock();
	framebuffer->Lock();
	laregion.MakeEmpty();
	drawview->BeginLineArray(count);
}

void ViewDriver::Blit(BRect src, BRect dest)
{
	screenwin->Lock();
	framebuffer->Lock();
	drawview->CopyBits(src,dest);
	drawview->Sync();
	screenwin->view->Invalidate(dest);
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::DrawBitmap(ServerBitmap *bitmap)
{
}

void ViewDriver::DrawLineArray(int32 count,BPoint *start, BPoint *end, rgb_color *color)
{
	screenwin->Lock();
	framebuffer->Lock();

	BRegion invalid;
	drawview->BeginLineArray(count+1);
	for(int32 i=0;i<count;i++)
	{
		drawview->AddLine(start[i],end[i],color[i]);
		invalid.Include(BRect(start[i],end[i]));
	}
	drawview->EndLineArray();
	drawview->Sync();
	screenwin->view->Invalidate(invalid.Frame());
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::DrawChar(char c, BPoint point)
{
	char string[2];
	string[0]=c;
	string[1]='\0';
	DrawString(string,2,point);
}

void ViewDriver::DrawString(char *string, int length, BPoint point)
{
	screenwin->Lock();
	framebuffer->Lock();
	drawview->DrawString(string,length,point);
	font_height fheight;
	drawview->GetFontHeight(&fheight);
	BRect invalid(point,BPoint(point.x+drawview->StringWidth(string,length),
		fheight.ascent+5));
	drawview->Sync();
	screenwin->view->Invalidate(invalid);
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::EndLineArray(void)
{
	// NOTE: the locking is done in BeginLineArray()!
	drawview->EndLineArray();
	drawview->Sync();
	screenwin->view->Invalidate(laregion.Frame());
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::FillArc(int centerx, int centery, int xradius, int yradius, float angle, float span, uint8 *pat)
{
	screenwin->Lock();
	framebuffer->Lock();
	if(pat!=NULL)
		drawview->FillArc(BPoint(centerx,centery),xradius,yradius,angle,span,*((pattern*)pat));
	else
		drawview->FillArc(BPoint(centerx,centery),xradius,yradius,angle,span);
	drawview->Sync();
	screenwin->view->Invalidate(BRect(centerx-xradius,centery-yradius,
		centerx+xradius,centery+yradius));
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::FillBezier(BPoint *points, uint8 *pat)
{
	screenwin->Lock();
	framebuffer->Lock();
	if(pat!=NULL)
		drawview->FillBezier(points,*((pattern*)pat));
	else
		drawview->FillBezier(points);
	drawview->Sync();
	
	// Invalidate the whole view until I get around to adding in the invalid rect calc code
	screenwin->view->Invalidate();
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::FillEllipse(float centerx, float centery, float x_radius, float y_radius,uint8 *pat)
{
	screenwin->Lock();
	framebuffer->Lock();
	if(pat!=NULL)
		drawview->FillEllipse(BPoint(centerx,centery),x_radius,y_radius,*((pattern*)pat));
	else
		drawview->FillEllipse(BPoint(centerx,centery),x_radius,y_radius);
	drawview->Sync();
	screenwin->view->Invalidate(BRect(centerx-x_radius,centery-y_radius,
		centerx+x_radius,centery+y_radius));
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::FillPolygon(int *x, int *y, int numpoints, bool is_closed)
{
}

void ViewDriver::FillRect(BRect rect, uint8 *pat)
{
	screenwin->Lock();
	framebuffer->Lock();
	if(pat!=NULL)
		drawview->FillRect(rect,*((pattern*)pat));
	else
		drawview->FillRect(rect);
	drawview->Sync();
	screenwin->view->Invalidate(rect);
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::FillRect(BRect rect, rgb_color col)
{
	screenwin->Lock();
	framebuffer->Lock();
	drawview->SetHighColor(col);
	drawview->FillRect(rect);
	drawview->SetHighColor(highcolor);
	drawview->Sync();
	screenwin->view->Invalidate(rect);
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::FillRegion(BRegion *region)
{
}

void ViewDriver::FillRoundRect(BRect rect,float xradius, float yradius, uint8 *pat)
{
	screenwin->Lock();
	framebuffer->Lock();
	if(pat!=NULL)
		drawview->FillRoundRect(rect,xradius,yradius,*((pattern*)pat));
	else
		drawview->FillRoundRect(rect,xradius,yradius);
	drawview->Sync();
	screenwin->view->Invalidate(rect);
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::FillShape(BShape *shape)
{
}

void ViewDriver::FillTriangle(BPoint first, BPoint second, BPoint third, BRect rect, uint8 *pat)
{
	screenwin->Lock();
	framebuffer->Lock();
	if(pat!=NULL)
		drawview->FillTriangle(first,second,third,rect,*((pattern*)pat));
	else
		drawview->FillTriangle(first,second,third,rect);
	drawview->Sync();
	screenwin->view->Invalidate(rect);
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::FillTriangle(BPoint first, BPoint second, BPoint third, BRect rect, rgb_color col)
{
	screenwin->Lock();
	framebuffer->Lock();
	drawview->SetHighColor(col);
	drawview->FillTriangle(first,second,third,rect);
	drawview->SetHighColor(highcolor);
	drawview->Sync();
	screenwin->view->Invalidate(rect);
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::HideCursor(void)
{
	screenwin->Lock();
	locker->Lock();

	hide_cursor++;
	screenwin->PostMessage(VDWIN_HIDECURSOR);

	locker->Unlock();
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

void ViewDriver::MovePenTo(BPoint pt)
{
	screenwin->Lock();
	framebuffer->Lock();
	drawview->MovePenTo(pt);
	penpos=pt;
	framebuffer->Unlock();
	screenwin->Unlock();
}

BPoint ViewDriver::PenPosition(void)
{
	return penpos;
}

float ViewDriver::PenSize(void)
{
	return pensize;
}

void ViewDriver::SetCursor(ServerCursor *cursor)
{
	if(cursor!=NULL)
	{
		screenwin->Lock();
		BBitmap *bmp=new BBitmap(BRect(0,0,cursor->width-1,cursor->height-1),
			cursor->cspace);
	
		// Copy the server bitmap in the cursor to a BBitmap
		uint8	*sbmppos=(uint8*)cursor->bitmap->Buffer(),
				*bbmppos=(uint8*)bmp->Bits();
	
		int32 bytes=cursor->bitmap->bytesperline,
			bbytes=bmp->BytesPerRow();
	
		for(int i=0;i<cursor->height;i++)
			memcpy(bbmppos+(i*bbytes), sbmppos+(i*bytes), bytes);
	
		// Replace the bitmap
		delete screenwin->view->cursor;
		screenwin->view->cursor=bmp;
		screenwin->view->Invalidate(screenwin->view->cursorframe);
		screenwin->Unlock();
	}
}

void ViewDriver::SetPenSize(float size)
{
	screenwin->Lock();
	framebuffer->Lock();
	pensize=size;
	drawview->SetPenSize(size);
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::SetHighColor(uint8 r,uint8 g,uint8 b,uint8 a=255)
{
	screenwin->Lock();
	framebuffer->Lock();
	drawview->SetHighColor(r,g,b,a);
	highcolor.red=r;
	highcolor.green=g;
	highcolor.blue=b;
	highcolor.alpha=a;
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::SetHighColor(rgb_color col)
{
	screenwin->Lock();
	framebuffer->Lock();
	drawview->SetHighColor(col);
	highcolor=col;
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::SetLowColor(uint8 r,uint8 g,uint8 b,uint8 a=255)
{
	screenwin->Lock();
	framebuffer->Lock();
	drawview->SetLowColor(r,g,b,a);
	lowcolor.red=r;
	lowcolor.green=g;
	lowcolor.blue=b;
	lowcolor.alpha=a;
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::SetLowColor(rgb_color col)
{
	screenwin->Lock();
	framebuffer->Lock();
	drawview->SetLowColor(col);
	lowcolor=col;
	framebuffer->Unlock();
	screenwin->Unlock();
}

drawing_mode ViewDriver::GetDrawingMode(void)
{
	drawing_mode dm;
	
	screenwin->Lock();
	framebuffer->Lock();
	dm=drawview->DrawingMode();
	framebuffer->Unlock();
	screenwin->Unlock();
	return dm;
}

void ViewDriver::SetDrawingMode(drawing_mode dmode)
{
	screenwin->Lock();
	framebuffer->Lock();
	drawview->SetDrawingMode(dmode);
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::SetPixel(int x, int y, uint8 *pattern)
{
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

float ViewDriver::StringWidth(const char *string, int32 length)
{
	return drawview->StringWidth(string,length);
}

void ViewDriver::StrokeArc(int centerx, int centery, int xradius, int yradius, float angle, float span, uint8 *pat)
{
	screenwin->Lock();
	framebuffer->Lock();
	if(pat!=NULL)
		drawview->StrokeArc(BPoint(centerx,centery),xradius,yradius,angle,span,*((pattern*)pat));
	else
		drawview->StrokeArc(BPoint(centerx,centery),xradius,yradius,angle,span);
	drawview->Sync();
	screenwin->view->Invalidate(BRect(centerx-xradius,centery-yradius,
		centerx+xradius,centery+yradius));
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::StrokeBezier(BPoint *points, uint8 *pat)
{
	screenwin->Lock();
	framebuffer->Lock();
	if(pat!=NULL)
		drawview->StrokeBezier(points,*((pattern*)pat));
	else
		drawview->StrokeBezier(points);
	drawview->Sync();
	
	// Invalidate the whole view until I get around to adding in the invalid rect calc code
	screenwin->view->Invalidate();
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::StrokeEllipse(float centerx, float centery, float x_radius, float y_radius,uint8 *pat)
{
	screenwin->Lock();
	framebuffer->Lock();
	if(pat!=NULL)
		drawview->StrokeEllipse(BPoint(centerx,centery),x_radius,y_radius,*((pattern*)pat));
	else
		drawview->StrokeEllipse(BPoint(centerx,centery),x_radius,y_radius);
	drawview->Sync();
	screenwin->view->Invalidate(BRect(centerx-x_radius,centery-y_radius,
		centerx+x_radius,centery+y_radius));
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::StrokeLine(BPoint point, uint8 *pat)
{
	screenwin->Lock();
	framebuffer->Lock();
	if(pat!=NULL)
		drawview->StrokeLine(point,*((pattern*)pat));
	else
		drawview->StrokeLine(point);
	drawview->Sync();
	screenwin->view->Invalidate(BRect(penpos,point));
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::StrokeLine(BPoint pt1, BPoint pt2, rgb_color col)
{
	screenwin->Lock();
	framebuffer->Lock();
	drawview->SetHighColor(col);
	drawview->StrokeLine(pt1,pt2);
	drawview->SetHighColor(highcolor);
	drawview->Sync();
	screenwin->view->Invalidate(BRect(pt1,pt2));
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::StrokePolygon(int *x, int *y, int numpoints, bool is_closed)
{
	screenwin->Lock();
	framebuffer->Lock();

	BRegion invalid;

	drawview->BeginLineArray(numpoints+2);
	BPoint start,end;
	for(int i=1;i<numpoints;i++)
	{
		start.Set(x[i-1],y[i-1]);
		end.Set(x[i],y[i]);
		drawview->AddLine(start,end,highcolor);
		invalid.Include(BRect(start,end));
	}

	if(is_closed)
	{
		start.Set(x[numpoints-1],y[numpoints-1]);
		end.Set(x[0],y[0]);
		drawview->AddLine(start,end,highcolor);
		invalid.Include(BRect(start,end));
	}
	drawview->EndLineArray();

	drawview->Sync();
	screenwin->view->Invalidate(invalid.Frame());
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::StrokeRect(BRect rect, uint8 *pat)
{
	screenwin->Lock();
	framebuffer->Lock();
	if(pat!=NULL)
		drawview->StrokeRect(rect,*((pattern*)pat));
	else
		drawview->StrokeRect(rect);
	drawview->Sync();
	screenwin->view->Invalidate(rect);
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::StrokeRect(BRect rect, rgb_color col)
{
	screenwin->Lock();
	framebuffer->Lock();
	drawview->SetHighColor(col);
	drawview->StrokeRect(rect);
	drawview->SetHighColor(highcolor);
	drawview->Sync();
	screenwin->view->Invalidate(rect);
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::StrokeRoundRect(BRect rect,float xradius, float yradius, uint8 *pat)
{
	screenwin->Lock();
	framebuffer->Lock();
	if(pat!=NULL)
		drawview->StrokeRoundRect(rect,xradius,yradius,*((pattern*)pat));
	else
		drawview->StrokeRoundRect(rect,xradius,yradius);
	drawview->Sync();
	screenwin->view->Invalidate(rect);
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::StrokeShape(BShape *shape)
{
}

void ViewDriver::StrokeTriangle(BPoint first, BPoint second, BPoint third, BRect rect, uint8 *pat)
{
	screenwin->Lock();
	framebuffer->Lock();
	if(pat!=NULL)
		drawview->StrokeTriangle(first,second,third,rect,*((pattern*)pat));
	else
		drawview->StrokeTriangle(first,second,third,rect);
	drawview->Sync();
	screenwin->view->Invalidate(rect);
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::StrokeTriangle(BPoint first, BPoint second, BPoint third, BRect rect, rgb_color col)
{
	screenwin->Lock();
	framebuffer->Lock();
	drawview->SetHighColor(col);
	drawview->StrokeTriangle(first,second,third,rect);
	drawview->SetHighColor(highcolor);
	drawview->Sync();
	screenwin->view->Invalidate(rect);
	framebuffer->Unlock();
	screenwin->Unlock();
}
