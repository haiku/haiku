//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
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
//	File Name:		ViewDriver.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	BView/BWindow combination graphics module
//  
//------------------------------------------------------------------------------

#include <stdio.h>
#include <iostream>
#include <Message.h>
#include <Region.h>
#include <Bitmap.h>
#include <OS.h>
#include <GraphicsDefs.h>
#include <Font.h>
#include <BitmapStream.h>
#include <File.h>
#include <Entry.h>
#include <Screen.h>
#include <TranslatorRoster.h>

#include "Angle.h"
#include "PortLink.h"
#include "RectUtils.h"
#include "ServerProtocol.h"
#include "ServerBitmap.h"
#include "ViewDriver.h"
#include "ServerConfig.h"
#include "ServerCursor.h"
#include "ServerFont.h"
#include "FontFamily.h"
#include "LayerData.h"
#include "PNGDump.h"

#ifdef DEBUG_DRIVER_MODULE
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

enum
{
VDWIN_CLEAR=100,

VDWIN_SHOWCURSOR,
VDWIN_HIDECURSOR,
VDWIN_OBSCURECURSOR,
VDWIN_MOVECURSOR,
VDWIN_SETCURSOR,
};

extern RGBColor workspace_default_color;

bool is_initialized = false;
BPoint		offset(100,50);

VDView::VDView(BRect bounds)
	: BView(bounds,"viewdriver_view",B_FOLLOW_ALL, B_WILL_DRAW)
{
	SetViewColor(B_TRANSPARENT_32_BIT);
	viewbmp=new BBitmap(bounds,B_RGBA32,true);

	// This link for sending mouse messages to the OBAppServer.
	// This is only to take the place of the Input Server. 
	serverlink = new BPortLink(find_port(SERVER_INPUT_PORT));

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
	delete cursor;

	viewbmp->Lock();
	delete viewbmp;
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
	// Attach data:
	// 1) int64 - time of mouse click
	// 2) float - x coordinate of mouse click
	// 3) float - y coordinate of mouse click
	// 4) int32 - modifier keys down
	// 5) int32 - buttons down
	// 6) int32 - clicks
#ifdef ENABLE_INPUT_SERVER_EMULATION
	BPoint p;

	uint32 buttons,
		mod=modifiers(),
		clicks=1;		// can't get the # of clicks without a *lot* of extra work :(

	int64 time=(int64)real_time_clock();

	GetMouse(&p,&buttons);

	serverlink->StartMessage(B_MOUSE_DOWN);
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
#ifdef ENABLE_INPUT_SERVER_EMULATION
	BPoint p;
	uint32 buttons;
	int64 time=(int64)real_time_clock();

	serverlink->StartMessage(B_MOUSE_MOVED);
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
	// Attach data:
	// 1) int64 - time of mouse click
	// 2) float - x coordinate of mouse click
	// 3) float - y coordinate of mouse click
	// 4) int32 - modifier keys down
#ifdef ENABLE_INPUT_SERVER_EMULATION
	BPoint p;

	uint32 buttons,
		mod=modifiers();

	int64 time=(int64)real_time_clock();

	GetMouse(&p,&buttons);

	serverlink->StartMessage(B_MOUSE_UP);
	serverlink->Attach(&time, sizeof(int64));
	serverlink->Attach(&pt.x,sizeof(float));
	serverlink->Attach(&pt.y,sizeof(float));
	serverlink->Attach(&mod, sizeof(uint32));
	serverlink->Flush();
#endif
}

void VDView::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
#ifdef ENABLE_INPUT_SERVER_EMULATION
		case B_MOUSE_WHEEL_CHANGED:
		{
			float x,y;
			msg->FindFloat("be:wheel_delta_x",&x);
			msg->FindFloat("be:wheel_delta_y",&y);
			int64 time=real_time_clock();
			serverlink->StartMessage(B_MOUSE_WHEEL_CHANGED);
			serverlink->Attach(&time,sizeof(int64));
			serverlink->Attach(x);
			serverlink->Attach(y);
			serverlink->Flush();
			break;
		}
#endif
		default:
			BView::MessageReceived(msg);
			break;
	}
}

VDWindow::VDWindow(BRect frame)
	: BWindow(frame, "Haiku App Server", B_TITLED_WINDOW,
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
		case B_KEY_DOWN:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 raw key code (scancode)
			// 3) int32 modifier-independent ASCII code for the character
			// 4) int32 repeat count
			// 5) int32 modifiers
			// 6) int8[3] UTF-8 data generated
			// 7) int8 number of bytes to follow containing the 
			//		generated string
			// 8) Character string generated by the keystroke
			// 10) int8[16] state of all keys
			bigtime_t systime;
			int32 scancode, asciicode,repeatcount,modifiers;
			int8 utf8data[3];
			BString string;
			int8 keyarray[16];

			systime=(int64)real_time_clock();
			msg->FindInt32("key",&scancode);
			msg->FindInt32("be:key_repeat",&repeatcount);
			msg->FindInt32("modifiers",&modifiers);
			msg->FindInt32("raw_char",&asciicode);
			msg->FindInt8("byte",0,utf8data);
			msg->FindInt8("byte",1,utf8data+1);
			msg->FindInt8("byte",2,utf8data+2);
			msg->FindString("bytes",&string);
			for(int8 i=0;i<15;i++)
				msg->FindInt8("states",i,&keyarray[i]);
			view->serverlink->StartMessage(B_KEY_DOWN);
			view->serverlink->Attach(&systime,sizeof(bigtime_t));
			view->serverlink->Attach(scancode);
			view->serverlink->Attach(asciicode);
			view->serverlink->Attach(repeatcount);
			view->serverlink->Attach(modifiers);
			view->serverlink->Attach(utf8data,sizeof(int8)*3);
			view->serverlink->Attach(string.Length()+1);
			view->serverlink->Attach(string.String());
			view->serverlink->Attach(keyarray,sizeof(int8)*16);
			view->serverlink->Flush();
			break;
		}
		case B_KEY_UP:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 raw key code (scancode)
			// 3) int32 modifier-independent ASCII code for the character
			// 4) int32 modifiers
			// 5) int8[3] UTF-8 data generated
			// 6) int8 number of bytes to follow containing the 
			//		generated string
			// 7) Character string generated by the keystroke
			// 8) int8[16] state of all keys
			bigtime_t systime;
			int32 scancode, asciicode,modifiers;
			int8 utf8data[3];
			BString string;
			int8 keyarray[16];

			systime=(int64)real_time_clock();
			msg->FindInt32("key",&scancode);
			msg->FindInt32("raw_char",&asciicode);
			msg->FindInt32("modifiers",&modifiers);
			msg->FindInt8("byte",0,utf8data);
			msg->FindInt8("byte",1,utf8data+1);
			msg->FindInt8("byte",2,utf8data+2);
			msg->FindString("bytes",&string);
			for(int8 i=0;i<15;i++)
				msg->FindInt8("states",i,&keyarray[i]);
			view->serverlink->StartMessage(B_KEY_UP);
			view->serverlink->Attach(&systime,sizeof(bigtime_t));
			view->serverlink->Attach(scancode);
			view->serverlink->Attach(asciicode);
			view->serverlink->Attach(modifiers);
			view->serverlink->Attach(utf8data,sizeof(int8)*3);
			view->serverlink->Attach(string.Length()+1);
			view->serverlink->Attach(string.String());
			view->serverlink->Attach(keyarray,sizeof(int8)*16);
			view->serverlink->Flush();
			break;
		}
		case B_UNMAPPED_KEY_DOWN:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 raw key code (scancode)
			// 3) int32 modifiers
			// 4) int8[16] state of all keys
			bigtime_t systime;
			int32 scancode,modifiers;
			int8 keyarray[16];

			systime=(int64)real_time_clock();
			msg->FindInt32("key",&scancode);
			msg->FindInt32("modifiers",&modifiers);
			for(int8 i=0;i<15;i++)
				msg->FindInt8("states",i,&keyarray[i]);
			view->serverlink->StartMessage(B_UNMAPPED_KEY_DOWN);
			view->serverlink->Attach(&systime,sizeof(bigtime_t));
			view->serverlink->Attach(scancode);
			view->serverlink->Attach(modifiers);
			view->serverlink->Attach(keyarray,sizeof(int8)*16);
			view->serverlink->Flush();
			break;
		}
		case B_UNMAPPED_KEY_UP:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 raw key code (scancode)
			// 3) int32 modifiers
			// 4) int8[16] state of all keys
			bigtime_t systime;
			int32 scancode,modifiers;
			int8 keyarray[16];

			systime=(int64)real_time_clock();
			msg->FindInt32("key",&scancode);
			msg->FindInt32("modifiers",&modifiers);
			for(int8 i=0;i<15;i++)
				msg->FindInt8("states",i,&keyarray[i]);
			view->serverlink->StartMessage(B_UNMAPPED_KEY_UP);
			view->serverlink->Attach(&systime,sizeof(bigtime_t));
			view->serverlink->Attach(scancode);
			view->serverlink->Attach(modifiers);
			view->serverlink->Attach(keyarray,sizeof(int8)*16);
			view->serverlink->Flush();
			break;
		}
		case B_MODIFIERS_CHANGED:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 modifiers
			// 3) int32 old modifiers
			// 4) int8 state of all keys
			bigtime_t systime;
			int32 scancode,modifiers,oldmodifiers;
			int8 keyarray[16];

			systime=(int64)real_time_clock();
			msg->FindInt32("key",&scancode);
			msg->FindInt32("modifiers",&modifiers);
			msg->FindInt32("be:old_modifiers",&oldmodifiers);
			for(int8 i=0;i<15;i++)
				msg->FindInt8("states",i,&keyarray[i]);
			view->serverlink->StartMessage(B_MODIFIERS_CHANGED);
			view->serverlink->Attach(&systime,sizeof(bigtime_t));
			view->serverlink->Attach(scancode);
			view->serverlink->Attach(modifiers);
			view->serverlink->Attach(oldmodifiers);
			view->serverlink->Attach(keyarray,sizeof(int8)*16);
			view->serverlink->Flush();
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

	if(serverport>=0)
	{
		BPortLink link(serverport);
		link.StartMessage(B_QUIT_REQUESTED);
		link.Flush();
	}
	else
		printf("ERROR: couldn't find the app_server's main port!");
	
	// We actually have to perform the internal equivalent of calling Shutdown
	// to eliminate a race condition. If a ServerWindow attempts to call a driver
	// function after the window has closed but before the AppServer instance
	// has called Shutdown, we end up with a crash inside one of the graphics
	// methods
	is_initialized=false;
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
	fDisplayMode.virtual_width=640;
	fDisplayMode.virtual_height=480;
	fDisplayMode.space=B_RGBA32;

	screenwin=new VDWindow(BRect(0,0,fDisplayMode.virtual_width-1,fDisplayMode.virtual_height-1).OffsetToCopy(offset));
	framebuffer=screenwin->view->viewbmp;
	serverlink=screenwin->view->serverlink;
	hide_cursor=0;
	
	framebuffer->Lock();
	drawview=new BView(framebuffer->Bounds(),"drawview",B_FOLLOW_ALL, B_WILL_DRAW);
	framebuffer->AddChild(drawview);
	framebuffer->Unlock();
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

	// the screen should start black
	framebuffer->Lock();
	rgb_color	c; c.red = 0; c.blue = 0; c.green = 0; c.alpha = 255;
	drawview->SetHighColor(c);
	drawview->FillRect(drawview->Bounds());
	drawview->Sync();
	framebuffer->Unlock();

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

void ViewDriver::SetMode(const display_mode &mode)
{
	if(!is_initialized)
		return;

	if (fDisplayMode.virtual_width==mode.virtual_width &&
		fDisplayMode.virtual_height==mode.virtual_height &&
		fDisplayMode.space==mode.space)
	{
		return;
	}

	screenwin->Lock();
	
	BBitmap *tempbmp=new BBitmap(BRect(0,0,mode.virtual_width-1, mode.virtual_height-1),
		(color_space)mode.space,true);

	if(!tempbmp)
	{
		screenwin->Unlock();
		delete tempbmp;
		return;
	}
	
	if(!tempbmp->IsValid())
	{
		screenwin->Unlock();
		delete tempbmp;
		return;
	}
	
	delete framebuffer;

	screenwin->ResizeTo(mode.virtual_width-1, mode.virtual_height-1);

	screenwin->view->viewbmp=tempbmp;
	framebuffer=screenwin->view->viewbmp;

	drawview=new BView(framebuffer->Bounds(),"drawview",B_FOLLOW_ALL, B_WILL_DRAW);
	framebuffer->AddChild(drawview);

	// the screen should start black
	framebuffer->Lock();
	rgb_color	c; c.red = 0; c.blue = 0; c.green = 0; c.alpha = 255;
	drawview->SetHighColor(c);//workspace_default_color.GetColor32());
	drawview->FillRect(drawview->Bounds());
	drawview->Sync();
	framebuffer->Unlock();

	screenwin->view->Invalidate();
	screenwin->Unlock();
}


void ViewDriver::DrawBitmap(ServerBitmap *bitmap, const BRect &src, const BRect &dest, const DrawData *d)
{
	if(!is_initialized)
		return;
		
STRACE(("ViewDriver:: DrawBitmap unimplemented()\n"));
}

bool ViewDriver::DumpToFile(const char *path)
{
	if(!is_initialized)
		return false;
		
	// Dump to PNG
	Lock();
	SaveToPNG(path,framebuffer->Bounds(),framebuffer->ColorSpace(), 
			framebuffer->Bits(),framebuffer->BitsLength(),framebuffer->BytesPerRow());

	Unlock();
	return true;
}


/*!
	\brief Draws a series of lines - optimized for speed
	\param pts Array of BPoints pairs
	\param numlines Number of lines to be drawn
	\param pensize The thickness of the lines
	\param colors Array of colors for each respective line
*/
void ViewDriver::StrokeLineArray(const int32 &numlines, const LineArrayData *linedata, 
	const DrawData *d)
{
	if(!is_initialized)
		return;
		
	if( !numlines || !linedata || !d)
		return;
	
	const LineArrayData *data;
	
	Lock();
	screenwin->Lock();
	framebuffer->Lock();
	
	drawview->SetPenSize(d->pensize);
	drawview->SetDrawingMode(d->draw_mode);

	drawview->BeginLineArray(numlines);
	for(int32 i=0; i<numlines; i++)
	{
		data=(const LineArrayData *)&linedata[i];
		drawview->AddLine(data->pt1,data->pt2,data->color);
	}
	drawview->EndLineArray();
	
	drawview->Sync();
	screenwin->view->Invalidate();

	framebuffer->Unlock();
	screenwin->Unlock();
	Unlock();
}


void ViewDriver::InvertRect(const BRect &r)
{
	if(!is_initialized)
		return;
		
	screenwin->Lock();
	framebuffer->Lock();
	drawview->InvertRect(r);
	drawview->Sync();
	screenwin->view->Invalidate(r);
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::FillSolidRect(const BRect &rect, const RGBColor &color)
{
	if(!is_initialized)
		return;
		
	screenwin->Lock();
	framebuffer->Lock();
	drawview->SetHighColor(color.GetColor32());
	drawview->FillRect(rect);
	drawview->Sync();
	screenwin->view->Invalidate(rect);
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::FillPatternRect(const BRect &rect, const DrawData *d)
{
	if(!d)
		return;
	
	if(!is_initialized)
		return;

	screenwin->Lock();
	framebuffer->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->FillRect(rect,*((pattern*)d->patt.GetInt8()));
	drawview->Sync();
	screenwin->view->Invalidate(rect);
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::StrokePatternLine(int32 x1, int32 y1, int32 x2, int32 y2, const DrawData *d)
{
	if(!d)
		return;
	
	if(!is_initialized)
		return;

	screenwin->Lock();
	framebuffer->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->StrokeLine(BPoint(x1,y1),BPoint(x2,y2),*((pattern*)d->patt.GetInt8()));
	drawview->Sync();
	screenwin->view->Invalidate(BRect(x1, y1, x2, y2));
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::StrokeSolidLine(int32 x1, int32 y1, int32 x2, int32 y2, const RGBColor &color)
{
	if(!is_initialized)
		return;

	screenwin->Lock();
	framebuffer->Lock();
	drawview->SetHighColor(color.GetColor32());
	drawview->StrokeLine(BPoint(x1,y1),BPoint(x2,y2));
	drawview->Sync();
	screenwin->view->Invalidate(BRect(x1,y1,x2,y2));
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::StrokeSolidRect(const BRect &rect, const RGBColor &color)
{
	if(!is_initialized)
		return;

	screenwin->Lock();
	framebuffer->Lock();
	drawview->SetHighColor(color.GetColor32());
	drawview->StrokeRect(rect);
	drawview->Sync();
	screenwin->view->Invalidate(rect);
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::SetDrawData(const DrawData *d, bool set_font_data)
{
	if(!is_initialized)
		return;
		
	if(!d)
		return;

	bool unlock=false;
	if(!framebuffer->IsLocked())
	{
		framebuffer->Lock();
		unlock=true;
	}
	
	drawview->SetPenSize(d->pensize);
	drawview->SetDrawingMode(d->draw_mode);
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->SetScale(d->scale);
	drawview->MovePenTo(d->penlocation);
	if(set_font_data)
	{
		BFont font;
		const ServerFont *sf=&(d->font);

		if(!sf)
			return;

		FontStyle *style=d->font.Style();

		if(!style)
			return;
		
		FontFamily *family=(FontFamily *)style->Family();
		if(!family)
			return;
		
		font_family fontfamily;
		strcpy(fontfamily,family->Name());
		font.SetFamilyAndStyle(fontfamily,style->Name());
		font.SetFlags(sf->Flags());
		font.SetEncoding(sf->Encoding());
		font.SetSize(sf->Size());
		font.SetRotation(sf->Rotation());
		font.SetShear(sf->Shear());
		font.SetSpacing(sf->Spacing());
		drawview->SetFont(&font);
	}
	if(unlock)
		framebuffer->Unlock();
}

rgb_color ViewDriver::GetBlitColor(rgb_color src, rgb_color dest, DrawData *d, bool use_high)
{
	rgb_color returncolor={0,0,0,0};

	int16 value;
	if(!d || !is_initialized)
		return returncolor;
	
	switch(d->draw_mode)
	{
		case B_OP_COPY:
		{
			return src;
		}
		case B_OP_ADD:
		{
			value=src.red+dest.red;
			returncolor.red=(value>255)?255:value;

			value=src.green+dest.green;
			returncolor.green=(value>255)?255:value;

			value=src.blue+dest.blue;
			returncolor.blue=(value>255)?255:value;
			return returncolor;
		}
		case B_OP_SUBTRACT:
		{
			value=src.red-dest.red;
			returncolor.red=(value<0)?0:value;

			value=src.green-dest.green;
			returncolor.green=(value<0)?0:value;

			value=src.blue-dest.blue;
			returncolor.blue=(value<0)?0:value;
			return returncolor;
		}
		case B_OP_BLEND:
		{
			value=int16(src.red+dest.red)>>1;
			returncolor.red=value;

			value=int16(src.green+dest.green)>>1;
			returncolor.green=value;

			value=int16(src.blue+dest.blue)>>1;
			returncolor.blue=value;
			return returncolor;
		}
		case B_OP_MIN:
		{
			
			return ( uint16(src.red+src.blue+src.green) > 
				uint16(dest.red+dest.blue+dest.green) )?dest:src;
		}
		case B_OP_MAX:
		{
			return ( uint16(src.red+src.blue+src.green) < 
				uint16(dest.red+dest.blue+dest.green) )?dest:src;
		}
		case B_OP_OVER:
		{
			return (use_high && src.alpha>127)?src:dest;
		}
		case B_OP_INVERT:
		{
			returncolor.red=dest.red ^ 255;
			returncolor.green=dest.green ^ 255;
			returncolor.blue=dest.blue ^ 255;
			return (use_high && src.alpha>127)?returncolor:dest;
		}
		// This is a pain in the arse to implement, so I'm saving it for the real
		// server
		case B_OP_ALPHA:
		{
			return src;
		}
		case B_OP_ERASE:
		{
			// This one's tricky. 
			return (use_high && src.alpha>127)?d->lowcolor.GetColor32():dest;
		}
		case B_OP_SELECT:
		{
			// This one's tricky, too. We are passed a color in src. If it's the layer's
			// high color or low color, we check for a swap.
			if(d->highcolor==src)
				return (use_high && d->highcolor==dest)?d->lowcolor.GetColor32():dest;
			
			if(d->lowcolor==src)
				return (use_high && d->lowcolor==dest)?d->highcolor.GetColor32():dest;

			return dest;
		}
		default:
		{
			break;
		}
	}
	return returncolor;
}

status_t ViewDriver::SetDPMSMode(const uint32 &state)
{
	if(!is_initialized)
		return B_ERROR;
		
	// NOTE: Originally, this was a to-do item to implement software DPMS,
	// but this driver will not be the official testing driver, so implementing
	// this stuff the way it was intended -- blanking the server's screen but not
	// the physical monitor -- is moot, but we will support blanking the
	// actual monitor if it is supported.
	return BScreen().SetDPMS(state);
}

uint32 ViewDriver::DPMSMode(void) const
{
	// See note for SetDPMSMode if there are questions
	return BScreen().DPMSState();
}

uint32 ViewDriver::DPMSCapabilities(void) const
{
	// See note for SetDPMSMode if there are questions
	return BScreen().DPMSCapabilites();
}

status_t ViewDriver::GetDeviceInfo(accelerant_device_info *info)
{
	if(!info || !is_initialized)
		return B_ERROR;

	// We really don't have to provide anything here because this is strictly
	// a software-only driver, but we'll have some fun, anyway.
	
	info->version=100;
	sprintf(info->name,"Haiku, Inc. ViewDriver");
	sprintf(info->chipset,"Haiku, Inc. Chipset");
	sprintf(info->serial_no,"3.14159265358979323846");
	info->memory=134217728;	// 128 MB, not that we really have that much. :)
	info->dac_speed=0xFFFFFFFF;	// *heh*
	
	return B_OK;
}

status_t ViewDriver::GetModeList(display_mode **modes, uint32 *count)
{
	if(!count || !is_initialized)
		return B_ERROR;

	screenwin->Lock();
	
	// DEPRECATED:
	// NOTE: Originally, I was going to figure out good timing values to be 
	// returned in each of the modes supported, but I won't bother, being this
	// won't be used much longer anyway. 
	
	*modes=new display_mode[13];
	*count=13;

	modes[0]->virtual_width=640;
	modes[0]->virtual_width=480;
	modes[0]->space=B_CMAP8;	
	modes[1]->virtual_width=640;
	modes[1]->virtual_width=480;
	modes[1]->space=B_RGB16;
	modes[2]->virtual_width=640;
	modes[2]->virtual_width=480;
	modes[2]->space=B_RGB32;
	modes[3]->virtual_width=640;
	modes[3]->virtual_width=480;
	modes[3]->space=B_RGBA32;	

	modes[4]->virtual_width=800;
	modes[4]->virtual_width=600;
	modes[4]->space=B_CMAP8;
	modes[5]->virtual_width=800;
	modes[5]->virtual_width=600;
	modes[5]->space=B_RGB16;	
	modes[6]->virtual_width=800;
	modes[6]->virtual_width=600;
	modes[6]->space=B_RGB32;	

	modes[7]->virtual_width=1024;
	modes[7]->virtual_width=768;
	modes[7]->space=B_CMAP8;;
	modes[8]->virtual_width=1024;
	modes[8]->virtual_width=768;
	modes[8]->space=B_RGB16;	
	modes[9]->virtual_width=1024;
	modes[9]->virtual_width=768;
	modes[9]->space=B_RGB32;	

	modes[10]->virtual_width=1152;
	modes[10]->virtual_width=864;
	modes[10]->space=B_CMAP8;	
	modes[11]->virtual_width=1152;
	modes[11]->virtual_width=864;
	modes[11]->space=B_RGB16;	
	modes[12]->virtual_width=1152;
	modes[12]->virtual_width=864;
	modes[12]->space=B_RGB32;	
	
	for(int32 i=0; i<13; i++)
	{
		modes[i]->h_display_start=0;
		modes[i]->v_display_start=0;
		modes[i]->flags=B_PARALLEL_ACCESS;
	}
	screenwin->Unlock();
	
	return B_OK;
}

status_t ViewDriver::GetPixelClockLimits(display_mode *mode, uint32 *low, uint32 *high)
{
	if(!is_initialized)
		return B_ERROR;
		
	return B_ERROR;
}

status_t ViewDriver::GetTimingConstraints(display_timing_constraints *dtc)
{
	if(!is_initialized)
		return B_ERROR;
		
	return B_ERROR;
}

status_t ViewDriver::ProposeMode(display_mode *candidate, const display_mode *low, const display_mode *high)
{
	if(!is_initialized)
		return B_ERROR;
		
	// We should be able to get away with this because we're not dealing with any
	// specific hardware. This is a Good Thing(TM) because we can support any hardware
	// we wish within reasonable expectaions and programmer laziness. :P
	return B_OK;
}

status_t ViewDriver::WaitForRetrace(bigtime_t timeout=B_INFINITE_TIMEOUT)
{
	if(!is_initialized)
		return B_ERROR;
		
	// Locking shouldn't be necessary here - R5 should handle this for us. :)
	BScreen screen;
	return screen.WaitForRetrace(timeout);
}

void ViewDriver::CopyBitmap(ServerBitmap *bitmap, const BRect &source, const BRect &dest, const DrawData *d)
{
	if(!is_initialized || !bitmap || !d)
	{
		printf("CopyBitmap returned - not init or NULL bitmap\n");
		return;
	}
	
	SetDrawData(d);
	
	// Oh, wow, is this going to be slow. Then again, ViewDriver was never meant to be very fast. It could
	// be made significantly faster by directly copying from the source to the destination, but that would
	// require implementing a lot of code. Eventually, this should be replaced, but for now, using
	// DrawBitmap will at least work with a minimum of effort.
	
	BBitmap *mediator=new BBitmap(bitmap->Bounds(),bitmap->ColorSpace());
	memcpy(mediator->Bits(),bitmap->Bits(),bitmap->BitsLength());
	
	screenwin->Lock();
	framebuffer->Lock();

	drawview->DrawBitmap(mediator,source,dest);
	drawview->Sync();
	screenwin->view->Invalidate(dest);
	
	framebuffer->Unlock();
	screenwin->Unlock();
	delete mediator;
}

void ViewDriver::CopyToBitmap(ServerBitmap *destbmp, const BRect &sourcerect)
{
	if(!is_initialized || !destbmp)
	{
		printf("CopyToBitmap returned - not init or NULL bitmap\n");
		return;
	}
	
	if(((uint32)destbmp->ColorSpace() & 0x000F) != (fDisplayMode.space & 0x000F))
	{
		printf("CopyToBitmap returned - unequal buffer pixel depth\n");
		return;
	}
	
	BRect destrect(destbmp->Bounds()), source(sourcerect);
	
	uint8 colorspace_size=destbmp->BitsPerPixel()/8;
	
	// First, clip source rect to destination
	if(source.Width() > destrect.Width())
		source.right=source.left+destrect.Width();
	
	if(source.Height() > destrect.Height())
		source.bottom=source.top+destrect.Height();
	

	// Second, check rectangle bounds against their own bitmaps
	BRect work_rect(destbmp->Bounds());
	
	if( !(work_rect.Contains(destrect)) )
	{
		// something in selection must be clipped
		if(destrect.left < 0)
			destrect.left = 0;
		if(destrect.right > work_rect.right)
			destrect.right = work_rect.right;
		if(destrect.top < 0)
			destrect.top = 0;
		if(destrect.bottom > work_rect.bottom)
			destrect.bottom = work_rect.bottom;
	}

	work_rect.Set(0,0,fDisplayMode.virtual_width-1,fDisplayMode.virtual_height-1);

	if(!work_rect.Contains(sourcerect))
		return;

	if( !(work_rect.Contains(source)) )
	{
		// something in selection must be clipped
		if(source.left < 0)
			source.left = 0;
		if(source.right > work_rect.right)
			source.right = work_rect.right;
		if(source.top < 0)
			source.top = 0;
		if(source.bottom > work_rect.bottom)
			source.bottom = work_rect.bottom;
	}

	// Set pointers to the actual data
	uint8 *dest_bits  = (uint8*) destbmp->Bits();	
	uint8 *src_bits = (uint8*) framebuffer->Bits();

	// Get row widths for offset looping
	uint32 dest_width  = uint32 (destbmp->BytesPerRow());
	uint32 src_width = uint32 (framebuffer->BytesPerRow());

	// Offset bitmap pointers to proper spot in each bitmap
	src_bits += uint32 ( (source.top  * src_width)  + (source.left  * colorspace_size) );
	dest_bits += uint32 ( (destrect.top * dest_width) + (destrect.left * colorspace_size) );
	
	
	uint32 line_length = uint32 ((destrect.right - destrect.left+1)*colorspace_size);
	uint32 lines = uint32 (source.bottom-source.top+1);

	for (uint32 pos_y=0; pos_y<lines; pos_y++)
	{
		memcpy(dest_bits,src_bits,line_length);

		// Increment offsets
 		src_bits += src_width;
 		dest_bits += dest_width;
	}

}

void ViewDriver::ConstrainClippingRegion(BRegion *reg)
{
	if(!is_initialized)
	{
		printf("ConstrainClippingRegion returned - not init\n");
		return;
	}

	screenwin->Lock();
	framebuffer->Lock();

//	screenwin->view->ConstrainClippingRegion(reg);
	drawview->ConstrainClippingRegion(reg);

	framebuffer->Unlock();
	screenwin->Unlock();
}

bool ViewDriver::AcquireBuffer(FBBitmap *bmp)
{
	if(!bmp || !is_initialized)
		return false;
	
	screenwin->Lock();
	framebuffer->Lock();

	bmp->SetBytesPerRow(framebuffer->BytesPerRow());
	bmp->SetSpace(framebuffer->ColorSpace());
	bmp->SetSize(framebuffer->Bounds().IntegerWidth(), framebuffer->Bounds().IntegerHeight());
	bmp->SetBuffer(framebuffer->Bits());
	bmp->SetBitsPerPixel(framebuffer->ColorSpace(),framebuffer->BytesPerRow());

	return true;
}

void ViewDriver::ReleaseBuffer(void)
{
	if(!is_initialized)
		return;
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewDriver::Invalidate(const BRect &r)
{
	if(!is_initialized)
		return;
	
	screenwin->Lock();
	screenwin->view->Draw(r);
	screenwin->Unlock();
}
