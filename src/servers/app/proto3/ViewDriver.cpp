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

#include <stdio.h>
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
#define VDWIN_CLEAR 'vwcl'
#define VDWIN_SAFEMODE 'vwsm'
#define VDWIN_RESET 'vwrs'
#define VDWIN_SETSCREEN 'vwss'
#define VDWIN_FILLRECT 'vwfr'
#define VDWIN_STROKERECT 'vwsr'
#define VDWIN_FILLELLIPSE 'vwfe'
#define VDWIN_STROKEELLIPSE 'vwse'
#define VDWIN_SHOWCURSOR	'vscr'
#define VDWIN_HIDECURSOR	'vhcr'
#define VDWIN_MOVECURSOR	'vmcr'

static uint8 pickcursor[] = {16,1,2,1,
0,0,64,0,160,0,88,0,36,0,34,0,17,0,8,128,
4,88,2,60,1,120,0,240,1,248,1,220,0,140,0,0,
0,0,64,0,224,0,120,0,60,0,62,0,31,0,15,128,
7,216,3,252,1,248,0,240,1,248,1,220,0,140,0,0
};
static uint8 crosscursor[] = {16,1,5,5,
14,0,4,0,4,0,4,0,128,32,241,224,128,32,4,0,
4,0,4,0,14,0,0,0,0,0,0,0,0,0,0,0,
14,0,4,0,4,0,4,0,128,32,245,224,128,32,4,0,
4,0,4,0,14,0,0,0,0,0,0,0,0,0,0,0
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
//	printf("VDView: app_server input port: %ld\n",serverlink->GetPort());

	hide_cursor=0;

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
	SetDrawingMode(B_OP_ALPHA);
	DrawBitmapAsync(cursor,cursor->Bounds(),cursorframe);
	SetDrawingMode(B_OP_COPY);
	Sync();
	
//	oldcursorframe.PrintToStream();
//	cursorframe.PrintToStream();
//	printf("\n");
}

void VDView::MouseDown(BPoint pt)
{
//	printf("VDView::MouseDown()\n");
//	serverlink->SetOpCode(B_MOUSE_DOWN);
//	serverlink->Flush();
}

void VDView::MouseMoved(BPoint pt, uint32 transit, const BMessage *msg)
{
	// This emulates an Input Server by sending the same kind of messages to the
	// server's port. Being we're using a regular window, it would make little sense
	// to do anything else.
	
	// Attach data:
	// 1) int64 - time of mouse click
	// 2) float - x coordinate of mouse click
	// 3) float - y coordinate of mouse click
	// 4) int32 - buttons down
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
}

void VDView::MouseUp(BPoint pt)
{
//	printf("VDView::MouseUp()\n");
//	serverlink->SetOpCode(B_MOUSE_UP);
//	serverlink->Flush();
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

VDWindow::VDWindow(void)
	: BWindow(BRect(100,60,740,540),"OBOS App Server, P3",B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
	view=new VDView(Bounds());
	AddChild(view);
	pick=new BCursor(pickcursor);
	cross=new BCursor(crosscursor);
}

VDWindow::~VDWindow(void)
{
	delete pick;
	delete cross;
}

void VDWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case VDWIN_CLEAR:
		{
//			printf("ViewDriver: Clear\n");
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
//			printf("ViewDriver: Reset\n");
			Reset();
			break;
		}
		case VDWIN_SAFEMODE:
		{
//			printf("ViewDriver: Safemode\n");
			SafeMode();
			break;
		}
		case VDWIN_FILLRECT:
		{
//			printf("ViewDriver: FillRect\n");
			BRect rect(0,0,0,0);
			rgb_color color;

			msg->FindRect("rect",&rect);
			msg->FindInt8("red",(int8 *)&color.red);
			msg->FindInt8("green",(int8 *)&color.green);
			msg->FindInt8("blue",(int8 *)&color.blue);

			view->viewbmp->Lock();
			view->viewbmp->AddChild(view->drawview);
			view->drawview->SetHighColor(color);
			view->drawview->FillRect(rect);
			view->viewbmp->RemoveChild(view->drawview);
			view->viewbmp->Unlock();
			break;
		}
		case VDWIN_STROKERECT:
		{
//			printf("ViewDriver: StrokeRect\n");
			BRect rect(0,0,0,0);
			rgb_color color;

			msg->FindRect("rect",&rect);
			msg->FindInt8("red",(int8 *)&color.red);
			msg->FindInt8("green",(int8 *)&color.green);
			msg->FindInt8("blue",(int8 *)&color.blue);

			view->viewbmp->Lock();
			view->viewbmp->AddChild(view->drawview);
			view->drawview->SetHighColor(color);
			view->drawview->StrokeRect(rect);
			view->viewbmp->RemoveChild(view->drawview);
			view->viewbmp->Unlock();
			break;
		}
		case VDWIN_FILLELLIPSE:
		{
//			printf("ViewDriver: FillEllipse\n");
			BRect rect(0,0,0,0);
			rgb_color color;

			msg->FindRect("rect",&rect);
			msg->FindInt8("red",(int8 *)&color.red);
			msg->FindInt8("green",(int8 *)&color.green);
			msg->FindInt8("blue",(int8 *)&color.blue);

			view->viewbmp->Lock();
			view->viewbmp->AddChild(view->drawview);
			view->drawview->SetHighColor(color);
			view->drawview->FillEllipse(rect);
			view->viewbmp->RemoveChild(view->drawview);
			view->viewbmp->Unlock();
			break;
		}
		case VDWIN_STROKEELLIPSE:
		{
//			printf("ViewDriver: StrokeEllipse\n");
			BRect rect(0,0,0,0);
			rgb_color color;

			msg->FindRect("rect",&rect);
			msg->FindInt8("red",(int8 *)&color.red);
			msg->FindInt8("green",(int8 *)&color.green);
			msg->FindInt8("blue",(int8 *)&color.blue);

			view->viewbmp->Lock();
			view->viewbmp->AddChild(view->drawview);
			view->drawview->SetHighColor(color);
			view->drawview->StrokeEllipse(rect);
			view->viewbmp->RemoveChild(view->drawview);
			view->viewbmp->Unlock();
			break;
		}
		case VDWIN_SHOWCURSOR:
		{
			if(view->hide_cursor>0)
				view->hide_cursor--;				
			break;
		}
		case VDWIN_HIDECURSOR:
		{
			view->hide_cursor++;
			break;
		}
		case VDWIN_MOVECURSOR:
		{
			view->oldcursorframe=view->cursorframe;
			
			int16 x,y;
			msg->FindInt16("x",&x);
			msg->FindInt16("y",&y);
			view->cursorframe.OffsetTo(x,y);
			view->Invalidate(view->oldcursorframe);
			break;
		}
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

void VDWindow::SafeMode(void)
{
	SetScreen(B_32_BIT_640x480);
}

void VDWindow::Reset(void)
{
	SafeMode();
	Clear(255,255,255);
}

void VDWindow::SetScreen(uint32 space)
{
	switch(space)
	{
		case B_32_BIT_640x480:
		case B_16_BIT_640x480:
		case B_8_BIT_640x480:
		{
			ResizeTo(640.0,480.0);
//printf("SetScreen(): 640x480\n");
			break;
		}
		case B_32_BIT_800x600:
		case B_16_BIT_800x600:
		case B_8_BIT_800x600:
		{
			ResizeTo(800.0,600.0);
//printf("SetScreen(): 800x600\n");
			break;
		}
		case B_32_BIT_1024x768:
		case B_16_BIT_1024x768:
		case B_8_BIT_1024x768:
		{
			ResizeTo(1024.0,768.0);
//printf("SetScreen(): 1024x768\n");
			break;
		}
		default:
			break;
	}
}

void VDWindow::Clear(uint8 red,uint8 green,uint8 blue)
{
//printf("Clear: (%d,%d,%d)\n",red,green,blue);
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
	locker->Unlock();
}

void ViewDriver::Shutdown(void)
{
	locker->Lock();
	screenwin->Lock();
	screenwin->Close();
	is_initialized=false;
	locker->Unlock();
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

void ViewDriver::StrokeRect(BRect rect,rgb_color color)
{
	locker->Lock();
	BMessage *msg=new BMessage(VDWIN_STROKERECT);
	msg->AddRect("rect",rect);
	msg->AddInt8("red",color.red);
	msg->AddInt8("green",color.green);
	msg->AddInt8("blue",color.blue);
	screenwin->PostMessage(msg);
	locker->Unlock();
}

void ViewDriver::FillRect(BRect rect, rgb_color color)
{
	locker->Lock();
	BMessage *msg=new BMessage(VDWIN_FILLRECT);
	msg->AddRect("rect",rect);
	msg->AddInt8("red",color.red);
	msg->AddInt8("green",color.green);
	msg->AddInt8("blue",color.blue);
	screenwin->PostMessage(msg);
	locker->Unlock();
}

void ViewDriver::StrokeEllipse(BRect rect,rgb_color color)
{
	locker->Lock();
	BMessage *msg=new BMessage(VDWIN_STROKEELLIPSE);
	msg->AddRect("rect",rect);
	msg->AddInt8("red",color.red);
	msg->AddInt8("green",color.green);
	msg->AddInt8("blue",color.blue);
	screenwin->PostMessage(msg);
	locker->Unlock();
}

void ViewDriver::FillEllipse(BRect rect,rgb_color color)
{
	locker->Lock();
	BMessage *msg=new BMessage(VDWIN_FILLELLIPSE);
	msg->AddRect("rect",rect);
	msg->AddInt8("red",color.red);
	msg->AddInt8("green",color.green);
	msg->AddInt8("blue",color.blue);
	screenwin->PostMessage(msg);
	locker->Unlock();
}

void ViewDriver::Blit(BPoint dest, ServerBitmap *sourcebmp, ServerBitmap *destbmp)
{
}

void ViewDriver::SetCursor(int32 value)
{
}

void ViewDriver::SetCursor(ServerCursor *cursor)
{
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

void ViewDriver::MoveCursorTo(int x, int y)
{
	locker->Lock();
	BMessage *msg=new BMessage(VDWIN_MOVECURSOR);
	msg->AddInt16("x",x);
	msg->AddInt16("y",y);
	screenwin->PostMessage(msg);
	locker->Unlock();
}
