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
#include <Bitmap.h>
#include <OS.h>
#include "PortLink.h"
#include "ServerProtocol.h"
#include "ServerBitmap.h"
#include "ViewDriver.h"
#include "ServerCursor.h"
#include "DebugTools.h"

// Message defines for internal use only
enum
{
VDWIN_CLEAR=100,
VDWIN_SAFEMODE,
VDWIN_RESET,
VDWIN_SETSCREEN,

VDWIN_FILLARC,
VDWIN_STROKEARC,
VDWIN_FILLRECT,
VDWIN_STROKERECT,
VDWIN_FILLROUNDRECT,
VDWIN_STROKEROUNDRECT,
VDWIN_FILLELLIPSE,
VDWIN_STROKEELLIPSE,
VDWIN_FILLBEZIER,
VDWIN_STROKEBEZIER,
VDWIN_FILLTRIANGLE,
VDWIN_STROKETRIANGLE,
VDWIN_STROKELINE,
VDWIN_DRAWSTRING,

VDWIN_SETPENSIZE,
VDWIN_MOVEPENTO,
VDWIN_SHOWCURSOR,
VDWIN_HIDECURSOR,
VDWIN_OBSCURECURSOR,
VDWIN_MOVECURSOR,
VDWIN_SETCURSOR,
VDWIN_SETHIGHCOLOR,
VDWIN_SETLOWCOLOR
};

VDView::VDView(BRect bounds)
	: BView(bounds,"viewdriver_view",B_FOLLOW_ALL, B_WILL_DRAW)
{
	viewbmp=new BBitmap(bounds,B_RGB32,true);
	drawview=new BView(viewbmp->Bounds(),"drawview",B_FOLLOW_ALL, B_WILL_DRAW);

	// This link for sending mouse messages to the OBAppServer.
	// This is only to take the place of the Input Server. I suppose I could write
	// an addon filter to be more like the Input Server, but then I wouldn't be working
	// on this thing! :P
	serverlink=new PortLink(find_port(SERVER_INPUT_PORT));

#ifdef DEBUG_DRIVER_MODULE
	printf("VDView: app_server input port: %ld\n",serverlink->GetPort());
#endif

	hide_cursor=0;
	obscure_cursor=false;

	// Create a cursor which isn't just a box
	cursor=new BBitmap(BRect(0,0,20,20),B_RGBA32,true);
	BView *v=new BView(cursor->Bounds(),"v", B_FOLLOW_NONE, B_WILL_DRAW);

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
	delete viewbmp;
	delete serverlink;
	delete cursor;
}

void VDView::AttachedToWindow(void)
{
//	printf("VDView::AttachedToWindow()\n");
}

void VDView::Draw(BRect rect)
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

void VDView::SetMode(int16 width, int16 height, uint8 bpp)
{
	// This function resets the emulated video buffer
	color_space s;
	
	switch(bpp)
	{
		case 16:
			s=B_RGBA15;
			break;
		case 8:
			s=B_CMAP8;
			break;
		default:
			s=B_RGBA32;
			break;
	}
	delete viewbmp;
	viewbmp=new BBitmap(BRect(0,0,width-1,height-1),s,true);
	drawview->ResizeTo(width-1,height-1);
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver::SetMode(%d x %d x %d)\n",width,height,bpp);
#endif
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

VDWindow::VDWindow(void)
	: BWindow(BRect(100,60,740,540),"OBOS App Server, P5",B_TITLED_WINDOW,
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
		case VDWIN_CLEAR:
		{
			int8 red=0,green=0,blue=0;
			msg->FindInt8("red",&red);
			msg->FindInt8("green",&green);
			msg->FindInt8("blue",&blue);
			Clear(red,green,blue);
			break;
		}
		case VDWIN_SETSCREEN:
		{
			// debug printing done in SetScreen()
			int32 space=0;
			msg->FindInt32("screenmode",&space);
			SetScreen(space);
			break;
		}
		case VDWIN_RESET:
		{
			Reset();
			break;
		}
		case VDWIN_SAFEMODE:
		{
			SafeMode();
			break;
		}
		case VDWIN_DRAWSTRING:
		{
			char *string;
			BPoint point;
			uint16 length;
			msg->FindString("string",(const char **)&string);
			msg->FindInt16("length",(int16*)&length);
			msg->FindPoint("point",&point);

			view->viewbmp->Lock();
			view->viewbmp->AddChild(view->drawview);
			view->drawview->SetDrawingMode(B_OP_ALPHA);
			view->drawview->DrawString(string,length,point);
			view->drawview->SetDrawingMode(B_OP_COPY);
			view->viewbmp->RemoveChild(view->drawview);
			view->viewbmp->Unlock();
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver::DrawString: ");
printf("String: %s\n",string);
printf("Length: %d\n",length);
printf("Point: "); point.PrintToStream();
#endif
			// Until I can figure out a better way, if I even bother...
			view->Invalidate();
		}
		case VDWIN_FILLARC:
		{
			float cx,cy,rx,ry,angle,span;
			int64 temp64;

			msg->FindFloat("cx",&cx);
			msg->FindFloat("cy",&cy);
			msg->FindFloat("rx",&rx);
			msg->FindFloat("ry",&ry);
			msg->FindFloat("angle",&angle);
			msg->FindFloat("span",&span);
			if(msg->FindInt64("pattern",&temp64)!=B_OK)
				temp64=*((int64*)&B_SOLID_HIGH);

			view->viewbmp->Lock();
			view->viewbmp->AddChild(view->drawview);
			view->drawview->FillArc(BPoint(cx,cy),rx,ry,angle,span,*((pattern *)&temp64));
			view->viewbmp->RemoveChild(view->drawview);
			view->viewbmp->Unlock();
			view->Invalidate(BRect(cx-rx,cy-ry,cx+rx,cy+ry));
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver::FillArc: ");
printf("Center: %f,%f\n",cx,cy);
printf("Radii: %f,%f\n",rx,ry);
printf("Angle, Span: %f,%f\n",angle, span);
printf("pattern: {%llx}\n",temp64);
#endif
			break;
		}
		case VDWIN_STROKEARC:
		{
			float cx,cy,rx,ry,angle,span;
			int64 temp64;

			msg->FindFloat("cx",&cx);
			msg->FindFloat("cy",&cy);
			msg->FindFloat("rx",&rx);
			msg->FindFloat("ry",&ry);
			msg->FindFloat("angle",&angle);
			msg->FindFloat("span",&span);
			if(msg->FindInt64("pattern",&temp64)!=B_OK)
				temp64=*((int64*)&B_SOLID_HIGH);

			view->viewbmp->Lock();
			view->viewbmp->AddChild(view->drawview);
			view->drawview->StrokeArc(BPoint(cx,cy),rx,ry,angle,span,*((pattern *)&temp64));
			view->viewbmp->RemoveChild(view->drawview);
			view->viewbmp->Unlock();
			view->Invalidate(BRect(cx-rx,cy-ry,cx+rx,cy+ry));
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: StrokeArc: ");
printf("Center: %f,%f\n",cx,cy);
printf("Radii: %f,%f\n",rx,ry);
printf("Angle, Span: %f,%f\n",angle, span);
printf("pattern: {%llx}\n",temp64);
#endif
			break;
		}
		case VDWIN_FILLBEZIER:
		{
			BPoint points[4];
			int64 temp64;
			msg->FindPoint("p1",points);
			msg->FindPoint("p2",&(points[1]));
			msg->FindPoint("p3",&(points[2]));
			msg->FindPoint("p4",&(points[3]));
			if(msg->FindInt64("pattern",&temp64)!=B_OK)
				temp64=*((int64*)&B_SOLID_HIGH);

			view->viewbmp->Lock();
			view->viewbmp->AddChild(view->drawview);
			view->drawview->FillBezier(points,*((pattern *)&temp64));
			view->viewbmp->RemoveChild(view->drawview);
			view->viewbmp->Unlock();
			
			// Invalidate the whole view until I can figure out how to calculate
			// a rect which encompasses the line
			view->Invalidate();
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: FillBezier:\n");
printf("Point 1: "); points[0].PrintToStream();
printf("Point 2: "); points[1].PrintToStream();
printf("Point 3: "); points[2].PrintToStream();
printf("Point 4: "); points[3].PrintToStream();
printf("pattern: {%llx}\n",temp64);
#endif
			break;
		}
		case VDWIN_STROKEBEZIER:
		{
			BPoint points[4];
			int64 temp64;
			msg->FindPoint("p1",points);
			msg->FindPoint("p2",&(points[1]));
			msg->FindPoint("p3",&(points[2]));
			msg->FindPoint("p4",&(points[3]));
			if(msg->FindInt64("pattern",&temp64)!=B_OK)
				temp64=*((int64*)&B_SOLID_HIGH);

			view->viewbmp->Lock();
			view->viewbmp->AddChild(view->drawview);
			view->drawview->StrokeBezier(points,*((pattern *)&temp64));
			view->viewbmp->RemoveChild(view->drawview);
			view->viewbmp->Unlock();

			// Invalidate the whole view until I can figure out how to calculate
			// a rect which encompasses the line
			view->Invalidate();
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: StrokeBezier:\n");
printf("Point 1: "); points[0].PrintToStream();
printf("Point 2: "); points[1].PrintToStream();
printf("Point 3: "); points[2].PrintToStream();
printf("Point 4: "); points[3].PrintToStream();
printf("pattern: {%llx}\n",temp64);
#endif
			break;
		}
		case VDWIN_FILLRECT:
		{
			BRect rect(0,0,0,0);
			int64 temp64;

			msg->FindRect("rect",&rect);
			if(msg->FindInt64("pattern",&temp64)==B_NAME_NOT_FOUND)
			{
				// Pattern will not exist if we are using the internal
				// API which specifies a color and a rectangle
				rgb_color highcolor=view->drawview->HighColor();
				rgb_color rectcolor;
				msg->FindInt8("red",(int8*)&(rectcolor.red));
				msg->FindInt8("green",(int8*)&(rectcolor.green));
				msg->FindInt8("blue",(int8*)&(rectcolor.blue));
				msg->FindInt8("alpha",(int8*)&(rectcolor.alpha));

				view->viewbmp->Lock();
				view->viewbmp->AddChild(view->drawview);
				view->drawview->SetHighColor(rectcolor);
				view->drawview->FillRect(rect,B_SOLID_HIGH);
				view->drawview->SetHighColor(highcolor);
				view->viewbmp->RemoveChild(view->drawview);
				view->viewbmp->Unlock();
				view->Invalidate(rect);
#ifdef DEBUG_DRIVER_MODULE
printf("FillRect: "); rect.PrintToStream();
printf("color (%d,%d,%d,%d)\n",rectcolor.red,rectcolor.green,rectcolor.blue,rectcolor.alpha);
#endif
				break;
			}

			view->viewbmp->Lock();
			view->viewbmp->AddChild(view->drawview);
			view->drawview->FillRect(rect,*((pattern *)&temp64));
			view->viewbmp->RemoveChild(view->drawview);
			view->viewbmp->Unlock();
			view->Invalidate(rect);
#ifdef DEBUG_DRIVER_MODULE
printf("FillRect: "); rect.PrintToStream();
printf("pattern: {%llxd}\n",temp64);
#endif
			break;
		}
		case VDWIN_STROKERECT:
		{
			BRect rect(0,0,0,0);
			int64 temp64;

			msg->FindRect("rect",&rect);
			
			if(msg->FindInt64("pattern",&temp64)==B_NAME_NOT_FOUND)
			{
				// Pattern will not exist if we are using the internal
				// API which specifies a color and a rectangle
				rgb_color highcolor=view->drawview->HighColor();
				rgb_color rectcolor;
				msg->FindInt8("red",(int8*)&(rectcolor.red));
				msg->FindInt8("green",(int8*)&(rectcolor.green));
				msg->FindInt8("blue",(int8*)&(rectcolor.blue));
				msg->FindInt8("alpha",(int8*)&(rectcolor.alpha));

				view->viewbmp->Lock();
				view->viewbmp->AddChild(view->drawview);
				view->drawview->SetHighColor(rectcolor);
				view->drawview->StrokeRect(rect,B_SOLID_HIGH);
				view->drawview->SetHighColor(highcolor);
				view->viewbmp->RemoveChild(view->drawview);
				view->viewbmp->Unlock();
				view->Invalidate(rect);
#ifdef DEBUG_DRIVER_MODULE
printf("StrokeRect: "); rect.PrintToStream();
printf("color (%d,%d,%d,%d)\n",rectcolor.red,rectcolor.green,rectcolor.blue,rectcolor.alpha);
#endif
				break;
			}

			view->viewbmp->Lock();
			view->viewbmp->AddChild(view->drawview);
			view->drawview->StrokeRect(rect,*((pattern *)&temp64));
			view->viewbmp->RemoveChild(view->drawview);
			view->viewbmp->Unlock();
			view->Invalidate(rect);
#ifdef DEBUG_DRIVER_MODULE
printf("StrokeRect: "); rect.PrintToStream();
printf("pattern: {%llx}\n",temp64);
#endif
			break;
		}
		case VDWIN_FILLELLIPSE:
		{
			float cx,cy,rx,ry;
			int64 temp64;

			msg->FindFloat("cx",&cx);
			msg->FindFloat("cy",&cy);
			msg->FindFloat("rx",&rx);
			msg->FindFloat("ry",&ry);
			if(msg->FindInt64("pattern",&temp64)!=B_OK)
				temp64=*((int64*)&B_SOLID_HIGH);

			view->viewbmp->Lock();
			view->viewbmp->AddChild(view->drawview);
			view->drawview->FillEllipse(BPoint(cx,cy),rx,ry,*((pattern *)&temp64));
			view->viewbmp->RemoveChild(view->drawview);
			view->viewbmp->Unlock();
			view->Invalidate(BRect(cx-rx,cy-ry,cx+rx,cy+ry));
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: FillEllipse: ");
printf("Center: %f,%f\n",cx,cy);
printf("Radii: %f,%f\n",rx,ry);
printf("pattern: {%llx}\n",temp64);
#endif
			break;
		}
		case VDWIN_STROKEELLIPSE:
		{
			float cx,cy,rx,ry;
			int64 temp64;

			msg->FindFloat("cx",&cx);
			msg->FindFloat("cy",&cy);
			msg->FindFloat("rx",&rx);
			msg->FindFloat("ry",&ry);
			if(msg->FindInt64("pattern",&temp64)!=B_OK)
				temp64=*((int64*)&B_SOLID_HIGH);

			view->viewbmp->Lock();
			view->viewbmp->AddChild(view->drawview);
			view->drawview->StrokeEllipse(BPoint(cx,cy),rx,ry,*((pattern *)&temp64));
			view->viewbmp->RemoveChild(view->drawview);
			view->viewbmp->Unlock();
			view->Invalidate(BRect(cx-rx,cy-ry,cx+rx,cy+ry));
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: StrokeEllipse: ");
printf("Center: %f,%f\n",cx,cy);
printf("Radii: %f,%f\n",rx,ry);
printf("pattern: {%llx}\n",temp64);
#endif
			break;
		}
		case VDWIN_FILLROUNDRECT:
		{
			BRect rect(0,0,0,0);
			int64 temp64;
			float x,y;

			msg->FindRect("rect",&rect);
			msg->FindFloat("x",&x);
			msg->FindFloat("y",&y);
			if(msg->FindInt64("pattern",&temp64)!=B_OK)
				temp64=*((int64*)&B_SOLID_HIGH);

			view->viewbmp->Lock();
			view->viewbmp->AddChild(view->drawview);
			view->drawview->FillRoundRect(rect,x,y,*((pattern *)&temp64));
			view->viewbmp->RemoveChild(view->drawview);
			view->viewbmp->Unlock();
			view->Invalidate(rect);
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: FillRoundRect: "); rect.PrintToStream();
printf("Radii: %f,%f\n",x,y);
printf("pattern: {%llx}\n",temp64);
#endif
			break;
		}
		case VDWIN_STROKEROUNDRECT:
		{
			BRect rect(0,0,0,0);
			int64 temp64;
			float x,y;

			msg->FindRect("rect",&rect);
			msg->FindFloat("x",&x);
			msg->FindFloat("y",&y);
			if(msg->FindInt64("pattern",&temp64)!=B_OK)
				temp64=*((int64*)&B_SOLID_HIGH);

			view->viewbmp->Lock();
			view->viewbmp->AddChild(view->drawview);
			view->drawview->StrokeRoundRect(rect,x,y,*((pattern *)&temp64));
			view->viewbmp->RemoveChild(view->drawview);
			view->viewbmp->Unlock();
			view->Invalidate(rect);
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: StrokeRoundRect: "); rect.PrintToStream();
printf("Radii: %f,%f\n",x,y);
printf("pattern: {%llx}\n",temp64);
#endif
			break;
		}
		case VDWIN_FILLTRIANGLE:
		{
			BPoint pt1,pt2,pt3;
			BRect invalid;
			int64 temp64;
			msg->FindPoint("first",&pt1);
			msg->FindPoint("second",&pt2);
			msg->FindPoint("third",&pt3);
			msg->FindRect("rect",&invalid);
			if(msg->FindInt64("pattern",&temp64)!=B_OK)
				temp64=*((int64*)&B_SOLID_HIGH);

			view->viewbmp->Lock();
			view->viewbmp->AddChild(view->drawview);
			view->drawview->FillTriangle(pt1,pt2,pt3,*((pattern *)&temp64));
			view->viewbmp->RemoveChild(view->drawview);
			view->viewbmp->Unlock();

			view->Invalidate(invalid);
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: FillTriangle:\n");
printf("Point 1: "); pt1.PrintToStream();
printf("Point 2: "); pt2.PrintToStream();
printf("Point 3: "); pt3.PrintToStream();
printf("Rect: "); invalid.PrintToStream();
printf("pattern: {%llx}\n",temp64);
#endif
			break;
		}
		case VDWIN_STROKETRIANGLE:
		{
			BPoint pt1,pt2,pt3;
			BRect invalid;
			int64 temp64;
			msg->FindPoint("first",&pt1);
			msg->FindPoint("second",&pt2);
			msg->FindPoint("third",&pt3);
			msg->FindRect("rect",&invalid);
			if(msg->FindInt64("pattern",&temp64)!=B_OK)
				temp64=*((int64*)&B_SOLID_HIGH);

			view->viewbmp->Lock();
			view->viewbmp->AddChild(view->drawview);
			view->drawview->StrokeTriangle(pt1,pt2,pt3,*((pattern *)&temp64));
			view->viewbmp->RemoveChild(view->drawview);
			view->viewbmp->Unlock();

			view->Invalidate(invalid);
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: StrokeTriangle:\n");
printf("Point 1: "); pt1.PrintToStream();
printf("Point 2: "); pt2.PrintToStream();
printf("Point 3: "); pt3.PrintToStream();
printf("Rect: "); invalid.PrintToStream();
printf("pattern: {%llx}\n",temp64);
#endif
			break;
		}
		case VDWIN_STROKELINE:
		{
			BPoint pt(0,0),pt2(0,0);
			int64 temp64;
			rgb_color tcol, oldcol;

			msg->FindPoint("from",&pt);
			msg->FindPoint("to",&pt2);
			if(msg->FindInt64("pattern",&temp64)!=B_OK)
				temp64=*((int64*)&B_SOLID_HIGH);

			view->viewbmp->Lock();
			view->viewbmp->AddChild(view->drawview);

			if(msg->FindInt8("red",(int8*)&tcol.red)!=B_NAME_NOT_FOUND)
			{
				msg->FindInt8("green",(int8*)&tcol.green);
				msg->FindInt8("blue",(int8*)&tcol.blue);
				msg->FindInt8("alpha",(int8*)&tcol.alpha);
				oldcol=view->drawview->HighColor();
				view->drawview->SetHighColor(tcol);
				view->drawview->StrokeLine(pt,pt2,*((pattern *)&temp64));
				view->drawview->SetHighColor(oldcol);
			}
			else
				view->drawview->StrokeLine(pt,pt2,*((pattern *)&temp64));
			view->viewbmp->RemoveChild(view->drawview);
			view->viewbmp->Unlock();

			// Because BRect requires (pt1,pt2) where pt1=LeftTop(),
			// we have to jump through some hoops

			BRect invalid(MIN(pt2.x,pt.x), MIN(pt2.y,pt.y),
				MAX(pt2.x,pt.x), MAX(pt2.y,pt.y)  );
			invalid.InsetBy((view->drawview->PenSize()/2)*-1,(view->drawview->PenSize()/2)*-1);
			view->Invalidate(invalid);
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: StrokeLine(%f,%f)\n",pt.x,pt.y);
printf("pattern: {%llx}\n",temp64);
#endif
			break;
		}
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
		case VDWIN_SETHIGHCOLOR:
		{
			uint8 r=0,g=0,b=0,a=0;
			msg->FindInt8("red",(int8*)&r);
			msg->FindInt8("green",(int8*)&g);
			msg->FindInt8("blue",(int8*)&b);
			msg->FindInt8("alpha",(int8*)&a);
			view->drawview->SetHighColor(r,g,b,a);
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: SetHighColor(%d,%d,%d,%d)\n",r,g,b,a);
#endif
			break;
		}
		case VDWIN_SETLOWCOLOR:
		{
			uint8 r=0,g=0,b=0,a=0;
			msg->FindInt8("red",(int8*)&r);
			msg->FindInt8("green",(int8*)&g);
			msg->FindInt8("blue",(int8*)&b);
			msg->FindInt8("alpha",(int8*)&a);
			view->drawview->SetLowColor(r,g,b,a);
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: SetLowColor(%d,%d,%d,%d)\n",r,g,b,a);
#endif
			break;
		}
		case VDWIN_SETPENSIZE:
		{
			float pensize;
			msg->FindFloat("size",&pensize);
			view->drawview->SetPenSize(pensize);
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: SetPenSize(%f)\n",pensize);
#endif
			break;
		}
		case VDWIN_MOVEPENTO:
		{
			BPoint pt;
			msg->FindPoint("point",&pt);
			view->drawview->MovePenTo(pt);
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: MovePenTo(%f,%f)\n",pt.x,pt.y);
#endif
			break;
		}
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

void VDWindow::SafeMode(void)
{
	SetScreen(B_8_BIT_640x480);
}

void VDWindow::Reset(void)
{
	SafeMode();
	Clear(255,255,255);
}

void VDWindow::SetScreen(uint32 space)
{
	int16 w=640,h=480;
	uint8 bpp=0;
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
	ResizeTo(w-1,h-1);

	switch(space)
	{
		case B_32_BIT_640x480:
		case B_32_BIT_800x600:
		case B_32_BIT_1024x768:
			bpp=32;
			break;
		case B_16_BIT_640x480:
		case B_16_BIT_800x600:
		case B_16_BIT_1024x768:
			bpp=16;
			break;
		case B_8_BIT_640x480:
		case B_8_BIT_800x600:
		case B_8_BIT_1024x768:
			bpp=8;
			break;
		default:
			break;
	}
	view->SetMode(w,h,bpp);
}

void VDWindow::Clear(uint8 red,uint8 green,uint8 blue)
{
#ifdef DEBUG_DRIVER_MODULE
printf("ViewDriver:: Clear(%d,%d,%d)\n",red,green,blue);
#endif
	view->viewbmp->Lock();
	view->viewbmp->AddChild(view->drawview);
	view->drawview->SetHighColor(red,green,blue);
	view->drawview->FillRect(view->drawview->Bounds());
	view->viewbmp->RemoveChild(view->drawview);
	view->viewbmp->Unlock();
	view->Invalidate();
}

bool VDWindow::QuitRequested(void)
{
	port_id serverport=find_port(SERVER_PORT_NAME);
	if(serverport!=B_NAME_NOT_FOUND)
	{
		write_port(serverport,B_QUIT_REQUESTED,NULL,0);
	}
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

/*
	Posting messages to the rendering window helps run right around a whole
	migraine's worth of multithreaded code
*/

ViewDriver::ViewDriver(void)
{
	screenwin=new VDWindow();
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
	locker->Lock();
	screenwin->Show();
	is_initialized=true;
	SetPenSize(1.0);
	MovePenTo(BPoint(0,0));
	locker->Unlock();
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
	locker->Lock();
	screenwin->PostMessage(VDWIN_SAFEMODE);
	locker->Unlock();
}

void ViewDriver::Reset(void)
{
	locker->Lock();
	screenwin->PostMessage(VDWIN_RESET);
	locker->Unlock();
}

void ViewDriver::SetScreen(uint32 space)
{
	locker->Lock();
	BMessage *msg=new BMessage(VDWIN_SETSCREEN);
	msg->AddInt32("screenmode",space);
	screenwin->PostMessage(msg);
	locker->Unlock();
}

void ViewDriver::Clear(uint8 red, uint8 green, uint8 blue)
{
	locker->Lock();
	BMessage *msg=new BMessage(VDWIN_CLEAR);
	msg->AddInt8("red",red);
	msg->AddInt8("green",green);
	msg->AddInt8("blue",blue);
	screenwin->PostMessage(msg);
	locker->Unlock();
}

void ViewDriver::Clear(rgb_color col)
{
	Clear(col.red,col.green,col.blue);
}

int32 ViewDriver::GetHeight(void)
{	// Gets the height of the current mode
	return 0;
}

int32 ViewDriver::GetWidth(void)
{	// Gets the width of the current mode
	return 0;
}

int ViewDriver::GetDepth(void)
{	// Gets the color depth of the current mode
	return 0;
}

void ViewDriver::Blit(BRect src, BRect dest)
{
}

void ViewDriver::DrawBitmap(ServerBitmap *bitmap)
{
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
	locker->Lock();
	BMessage *msg=new BMessage(VDWIN_DRAWSTRING);
	msg->AddString("string",string);
	msg->AddInt16("length",length);
	msg->AddPoint("point",point);
	screenwin->PostMessage(msg);
	locker->Unlock();
}

void ViewDriver::FillArc(int centerx, int centery, int xradius, int yradius, float angle, float span, uint8 *pattern)
{
	locker->Lock();
	BMessage *msg=new BMessage(VDWIN_FILLARC);
	msg->AddFloat("cx",centerx);
	msg->AddFloat("cy",centery);
	msg->AddFloat("rx",xradius);
	msg->AddFloat("ry",yradius);
	msg->AddFloat("angle",angle);
	msg->AddFloat("span",span);

	// Have to actually copy the pattern data because a sort of race condition
	// Using an int64 to make for an easy copy.
	if(pattern)
	{
		int64 pat=*((int64*)pattern);
		msg->AddInt64("pattern",pat);
	}

	screenwin->PostMessage(msg);
	locker->Unlock();
}

void ViewDriver::FillBezier(BPoint *points, uint8 *pattern)
{
	locker->Lock();

	BMessage *msg=new BMessage(VDWIN_FILLBEZIER);
	msg->AddPoint("p1",points[0]);
	msg->AddPoint("p2",points[1]);
	msg->AddPoint("p3",points[2]);
	msg->AddPoint("p4",points[3]);
	if(pattern)
	{
		int64 pat=*((int64*)pattern);
		msg->AddInt64("pattern",pat);
	}
	
	screenwin->PostMessage(msg);
	locker->Unlock();
}

void ViewDriver::FillEllipse(float centerx, float centery, float x_radius, float y_radius,uint8 *pattern)
{
	locker->Lock();
	BMessage *msg=new BMessage(VDWIN_FILLELLIPSE);
	msg->AddFloat("cx",centerx);
	msg->AddFloat("cy",centery);
	msg->AddFloat("rx",x_radius);
	msg->AddFloat("ry",y_radius);

	// Have to actually copy the pattern data because a sort of race condition
	// Using an int64 to make for an easy copy.
	if(pattern)
	{
		int64 pat=*((int64*)pattern);
		msg->AddInt64("pattern",pat);
	}

	screenwin->PostMessage(msg);
	locker->Unlock();
}

void ViewDriver::FillPolygon(int *x, int *y, int numpoints, bool is_closed)
{
}

void ViewDriver::FillRect(BRect rect, uint8 *pattern)
{
	locker->Lock();
	BMessage *msg=new BMessage(VDWIN_FILLRECT);
	msg->AddRect("rect",rect);

	// Have to actually copy the pattern data because a sort of race condition
	// Using an int64 to make for an easy copy.
	if(pattern)
	{
		int64 pat=*((int64*)pattern);
		msg->AddInt64("pattern",pat);
	}
	screenwin->PostMessage(msg);
	locker->Unlock();
}

void ViewDriver::FillRect(BRect rect, rgb_color col)
{
	locker->Lock();
	BMessage *msg=new BMessage(VDWIN_FILLRECT);
	msg->AddRect("rect",rect);

	msg->AddInt8("red",col.red);
	msg->AddInt8("green",col.green);
	msg->AddInt8("blue",col.blue);
	msg->AddInt8("alpha",col.alpha);
	screenwin->PostMessage(msg);
	locker->Unlock();
}

void ViewDriver::FillRegion(BRegion *region)
{
}

void ViewDriver::FillRoundRect(BRect rect,float xradius, float yradius, uint8 *pattern)
{
	locker->Lock();
	BMessage *msg=new BMessage(VDWIN_FILLROUNDRECT);
	msg->AddRect("rect",rect);
	msg->AddFloat("x",xradius);
	msg->AddFloat("y",yradius);

	// Have to actually copy the pattern data because a sort of race condition
	// Using an int64 to make for an easy copy.
	if(pattern)
	{
		int64 pat=*((int64*)pattern);
		msg->AddInt64("pattern",pat);
	}

	screenwin->PostMessage(msg);
	locker->Unlock();
}

void ViewDriver::FillShape(BShape *shape)
{
}

void ViewDriver::FillTriangle(BPoint first, BPoint second, BPoint third, BRect rect, uint8 *pattern)
{
	locker->Lock();
	BMessage *msg=new BMessage(VDWIN_FILLTRIANGLE);
	msg->AddPoint("first",first);
	msg->AddPoint("second",second);
	msg->AddPoint("third",third);
	msg->AddRect("rect",rect);

	// Have to actually copy the pattern data because a sort of race condition
	// Using an int64 to make for an easy copy.
	if(pattern)
	{
		int64 pat=*((int64*)pattern);
		msg->AddInt64("pattern",pat);
	}

	screenwin->PostMessage(msg);
	locker->Unlock();
}

void ViewDriver::HideCursor(void)
{
	locker->Lock();
	hide_cursor++;
	screenwin->PostMessage(VDWIN_HIDECURSOR);
	locker->Unlock();
}

bool ViewDriver::IsCursorHidden(void)
{
	locker->Lock();
	bool value=(hide_cursor>0)?true:false;
	locker->Unlock();
	return value;
}

void ViewDriver::ObscureCursor(void)
{	// Hides cursor until mouse is moved
	locker->Lock();
	screenwin->PostMessage(VDWIN_OBSCURECURSOR);
	locker->Unlock();
}

void ViewDriver::MoveCursorTo(float x, float y)
{
	locker->Lock();
	BMessage *msg=new BMessage(VDWIN_MOVECURSOR);
	msg->AddFloat("x",x);
	msg->AddFloat("y",y);
	screenwin->PostMessage(msg);
	locker->Unlock();
}

void ViewDriver::MovePenTo(BPoint pt)
{	// Moves the graphics pen to this position

	locker->Lock();
	penpos=pt;

	BMessage *msg=new BMessage(VDWIN_MOVEPENTO);
	msg->AddPoint("point",penpos);
	screenwin->PostMessage(msg);
	locker->Unlock();
}

BPoint ViewDriver::PenPosition(void)
{
	return penpos;
}

float ViewDriver::PenSize(void)
{
	return pensize;
}

void ViewDriver::SetCursor(int32 value)
{
}

void ViewDriver::SetCursor(ServerCursor *cursor)
{
	// We are given a ServerCursor, but the data is really a ServerBitmap.
	// Copy it and fire it off to the view driver's window for translation
	locker->Lock();
	BMessage *msg=new BMessage(VDWIN_SETCURSOR);
	msg->AddPointer("SCursor",cursor);
	screenwin->PostMessage(msg);	// Much faster than copying data. :)
	locker->Unlock();
}

void ViewDriver::SetPenSize(float size)
{
	locker->Lock();
	pensize=size;

	BMessage *msg=new BMessage(VDWIN_SETPENSIZE);
	msg->AddFloat("size",pensize);
	screenwin->PostMessage(msg);
	locker->Unlock();
}

void ViewDriver::SetHighColor(uint8 r,uint8 g,uint8 b,uint8 a=255)
{
	locker->Lock();
	highcol.red=r;
	highcol.green=g;
	highcol.blue=b;
	highcol.alpha=a;

	BMessage *msg=new BMessage(VDWIN_SETHIGHCOLOR);
	msg->AddInt8("red",r);
	msg->AddInt8("green",g);
	msg->AddInt8("blue",b);
	msg->AddInt8("alpha",a);
	screenwin->PostMessage(msg);
	locker->Unlock();
}

void ViewDriver::SetLowColor(uint8 r,uint8 g,uint8 b,uint8 a=255)
{
	locker->Lock();
	lowcol.red=r;
	lowcol.green=g;
	lowcol.blue=b;
	lowcol.alpha=a;

	BMessage *msg=new BMessage(VDWIN_SETLOWCOLOR);
	msg->AddInt8("red",r);
	msg->AddInt8("green",g);
	msg->AddInt8("blue",b);
	msg->AddInt8("alpha",a);
	screenwin->PostMessage(msg);
	locker->Unlock();
}

void ViewDriver::SetPixel(int x, int y, uint8 *pattern)
{	// Internal function utilized by other functions to draw to the buffer
	// Will eventually be an inline function
}

void ViewDriver::ShowCursor(void)
{
	locker->Lock();
	if(hide_cursor>0)
	{
		hide_cursor--;
		screenwin->PostMessage(VDWIN_SHOWCURSOR);
	}
	locker->Unlock();
}

void ViewDriver::StrokeArc(int centerx, int centery, int xradius, int yradius, float angle, float span, uint8 *pattern)
{
	locker->Lock();
	BMessage *msg=new BMessage(VDWIN_STROKEARC);
	msg->AddFloat("cx",centerx);
	msg->AddFloat("cy",centery);
	msg->AddFloat("rx",xradius);
	msg->AddFloat("ry",yradius);
	msg->AddFloat("angle",angle);
	msg->AddFloat("span",span);

	// Have to actually copy the pattern data because a sort of race condition
	// Using an int64 to make for an easy copy.
	if(pattern)
	{
		int64 pat=*((int64*)pattern);
		msg->AddInt64("pattern",pat);
	}

	screenwin->PostMessage(msg);
	locker->Unlock();
}

void ViewDriver::StrokeBezier(BPoint *points, uint8 *pattern)
{
	locker->Lock();

	BMessage *msg=new BMessage(VDWIN_STROKEBEZIER);
	msg->AddPoint("p1",points[0]);
	msg->AddPoint("p2",points[1]);
	msg->AddPoint("p3",points[2]);
	msg->AddPoint("p4",points[3]);
	if(pattern)
	{
		int64 pat=*((int64*)pattern);
		msg->AddInt64("pattern",pat);
	}
	
	screenwin->PostMessage(msg);
	locker->Unlock();
}

void ViewDriver::StrokeEllipse(float centerx, float centery, float x_radius, float y_radius,uint8 *pattern)
{
	locker->Lock();
	BMessage *msg=new BMessage(VDWIN_STROKEELLIPSE);
	msg->AddFloat("cx",centerx);
	msg->AddFloat("cy",centery);
	msg->AddFloat("rx",x_radius);
	msg->AddFloat("ry",y_radius);

	// Have to actually copy the pattern data because a sort of race condition
	// Using an int64 to make for an easy copy.
	if(pattern)
	{
		int64 pat=*((int64*)pattern);
		msg->AddInt64("pattern",pat);
	}

	screenwin->PostMessage(msg);
	locker->Unlock();
}

void ViewDriver::StrokeLine(BPoint point, uint8 *pattern)
{
	locker->Lock();
	BMessage *msg=new BMessage(VDWIN_STROKELINE);
	msg->AddPoint("from",penpos);
	msg->AddPoint("to",point);

	// Have to actually copy the pattern data because a sort of race condition
	// Using an int64 to make for an easy copy.
	if(pattern)
	{
		int64 pat=*((int64*)pattern);
		msg->AddInt64("pattern",pat);
	}

	screenwin->PostMessage(msg);
	locker->Unlock();
}

void ViewDriver::StrokeLine(BPoint pt1, BPoint pt2, rgb_color col)
{
	locker->Lock();
	BMessage *msg=new BMessage(VDWIN_STROKELINE);
	msg->AddPoint("from",pt1);
	msg->AddPoint("to",pt2);
	msg->AddInt8("red",col.red);
	msg->AddInt8("green",col.green);
	msg->AddInt8("blue",col.blue);
	msg->AddInt8("alpha",col.alpha);

	screenwin->PostMessage(msg);
	locker->Unlock();
}

void ViewDriver::StrokePolygon(int *x, int *y, int numpoints, bool is_closed)
{
}

void ViewDriver::StrokeRect(BRect rect,uint8 *pattern)
{
	locker->Lock();
	BMessage *msg=new BMessage(VDWIN_STROKERECT);
	msg->AddRect("rect",rect);

	// Have to actually copy the pattern data because a sort of race condition
	// Using an int64 to make for an easy copy.
	if(pattern)
	{
		int64 pat=*((int64*)pattern);
		msg->AddInt64("pattern",pat);
	}

	screenwin->PostMessage(msg);
	locker->Unlock();
}

void ViewDriver::StrokeRect(BRect rect,rgb_color col)
{
	locker->Lock();
	BMessage *msg=new BMessage(VDWIN_STROKERECT);
	msg->AddRect("rect",rect);

	msg->AddInt8("red",col.red);
	msg->AddInt8("green",col.green);
	msg->AddInt8("blue",col.blue);
	msg->AddInt8("alpha",col.alpha);
	screenwin->PostMessage(msg);
	locker->Unlock();
}

void ViewDriver::StrokeRoundRect(BRect rect,float xradius, float yradius, uint8 *pattern)
{
	locker->Lock();
	BMessage *msg=new BMessage(VDWIN_STROKEROUNDRECT);
	msg->AddRect("rect",rect);
	msg->AddFloat("x",xradius);
	msg->AddFloat("y",yradius);

	// Have to actually copy the pattern data because a sort of race condition
	// Using an int64 to make for an easy copy.
	if(pattern)
	{
		int64 pat=*((int64*)pattern);
		msg->AddInt64("pattern",pat);
	}

	screenwin->PostMessage(msg);
	locker->Unlock();
}

void ViewDriver::StrokeShape(BShape *shape)
{
}

void ViewDriver::StrokeTriangle(BPoint first, BPoint second, BPoint third, BRect rect, uint8 *pattern)
{
	locker->Lock();
	BMessage *msg=new BMessage(VDWIN_STROKETRIANGLE);
	msg->AddPoint("first",first);
	msg->AddPoint("second",second);
	msg->AddPoint("third",third);
	msg->AddRect("rect",rect);

	// Have to actually copy the pattern data because a sort of race condition
	// Using an int64 to make for an easy copy.
	if(pattern)
	{
		int64 pat=*((int64*)pattern);
		msg->AddInt64("pattern",pat);
	}

	screenwin->PostMessage(msg);
	locker->Unlock();
}

#ifdef DEBUG_DRIVER_MODULE
#undef DEBUG_DRIVER_MODULE
#endif