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
//	File Name:		ScreenDriver.cpp
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
#include <TranslatorRoster.h>

#include "Angle.h"
#include "PortLink.h"
#include "ServerProtocol.h"
#include "ServerBitmap.h"
#include "ScreenDriver.h"
#include "ServerConfig.h"
#include "ServerCursor.h"
#include "ServerFont.h"
#include "FontFamily.h"
#include "LayerData.h"
#include "PNGDump.h"
#include "PatternHandler.h"

#ifdef DEBUG_DRIVER_MODULE
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

enum
{
SDWIN_CLEAR=100,

SDWIN_SHOWCURSOR,
SDWIN_HIDECURSOR,
SDWIN_OBSCURECURSOR,
SDWIN_MOVECURSOR,
SDWIN_SETCURSOR,
};

extern RGBColor workspace_default_color;

FrameBuffer::FrameBuffer(const char *title, uint32 space, status_t *st,bool debug)
	: BWindowScreen(title,space,st,debug)
{
	is_connected=false;
	port_id serverport=find_port(SERVER_INPUT_PORT);
	serverlink=new PortLink(serverport);
	mousepos.Set(0,0);
	buttons=0;
	viewbmp=new BBitmap(BRect(0,0,639,479),B_CMAP8,true);

	// View exists to poll for the mouse
	view=new BView(Bounds(),"view",0,0);
	AddChild(view);
	view->GetMouse(&mousepos,&buttons);
	
	invalid=new BRegion(Bounds());
	invalidflag=0;

#ifdef ENABLE_INPUT_SERVER_EMULATION
	monitor_thread=spawn_thread(MouseMonitor,"mousemonitor",B_NORMAL_PRIORITY,this);
	resume_thread(monitor_thread);
#endif

	copy_thread=spawn_thread(CopyThread,"copymonitor",B_NORMAL_PRIORITY,this);
	resume_thread(copy_thread);
}

FrameBuffer::~FrameBuffer(void)
{
	kill_thread(copy_thread);
	kill_thread(monitor_thread);
	delete viewbmp;
	delete invalid;
	delete serverlink;
}

void FrameBuffer::ScreenConnected(bool connected)
{
	is_connected=connected;
	if(connected)
	{
		// Cache the state just in case
		graphics_card_info *info=CardInfo();
		gcinfo=*info;
	}
}

void FrameBuffer::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case B_KEY_DOWN:
		{
			int32 key,modifiers;
			msg->FindInt32("key",&key);
			msg->FindInt32("modifiers",&modifiers);

			int32 servermods=0;
			servermods|=modifiers & B_RIGHT_COMMAND_KEY;
			servermods|=modifiers & B_RIGHT_OPTION_KEY;
			servermods|=modifiers & B_RIGHT_CONTROL_KEY;
			servermods|=modifiers & B_SHIFT_KEY;
			
			switch(key)
			{
				case 0x47:	// Enter key
				{
					port_id serverport=find_port(SERVER_PORT_NAME);
					write_port(serverport,B_QUIT_REQUESTED,NULL,0);
					break;
				}
				default:
					break;
			}
		}

#ifdef ENABLE_INPUT_SERVER_EMULATION
		case B_MOUSE_WHEEL_CHANGED:
		{
			float x,y;
			msg->FindFloat("be:wheel_delta_x",&x);
			msg->FindFloat("be:wheel_delta_y",&y);
			int64 time=real_time_clock();
			serverlink->SetOpCode(B_MOUSE_WHEEL_CHANGED);
			serverlink->Attach(&time,sizeof(int64));
			serverlink->Attach(x);
			serverlink->Attach(y);
			serverlink->Flush();
			break;
		}
#endif
		default:
			BWindowScreen::MessageReceived(msg);
	}
}

bool FrameBuffer::QuitRequested(void)
{
	port_id serverport=find_port(SERVER_PORT_NAME);

	if(serverport!=B_NAME_NOT_FOUND)
		write_port(serverport,B_QUIT_REQUESTED,NULL,0);

	return true;
}

int32 FrameBuffer::CopyThread(void *data)
{
	FrameBuffer *fb=(FrameBuffer*)data;

	while(1)
	{
		if(fb->IsConnected())
		{
			if(fb->invalidflag)
			{
				fb->Lock();
				fb->viewbmp->Lock();
				
				if(fb->invalid)
				{
					int8 *start_ptr;
					clipping_rect r;
					for(int32 i=0; i<fb->invalid->CountRects(); i++)
					{
						// Copy from viewbmp to framebuffer
						start_ptr=(int8*)fb->viewbmp->Bits();
						r=fb->invalid->RectAtInt(i);
						memcpy(fb->gcinfo.frame_buffer,start_ptr,fb->viewbmp->BitsLength());
					}
				}
				
				fb->viewbmp->Unlock();
				fb->Unlock();
			}
		}
	}
	
	return 0;
}

int32 FrameBuffer::MouseMonitor(void *data)
{
	FrameBuffer *fb=(FrameBuffer*)data;
	BPoint mousepos(0,0),oldpos(0,0);
	uint32 buttons=0, oldbuttons=0;
	uint32 clicks=1;		 // TODO: add multiclick support
	uint32 mods;
	int64 time;
	
	fb->Lock();
	PortLink *link=new PortLink(fb->serverlink->GetPort());
	fb->view->GetMouse(&mousepos,&buttons);
	oldpos=mousepos;
	oldbuttons=buttons;
	fb->Unlock();

	while(1)
	{
		if(fb->IsConnected())
		{
			// Get the mouse position
			fb->Lock();
			fb->view->GetMouse(&mousepos,&buttons);
			fb->Unlock();
			
			// Check for changes and post messages as necessary
			
			// Mouse button change?
			if(buttons!=oldbuttons)
			{
				time=(int64)real_time_clock();
				mods=modifiers();
				if(oldbuttons==0)
				{
					// MouseDown
					link->SetOpCode(B_MOUSE_DOWN);
					link->Attach(&time, sizeof(int64));
					link->Attach(&mousepos.x,sizeof(float));
					link->Attach(&mousepos.y,sizeof(float));
					link->Attach(&mods, sizeof(uint32));
					link->Attach(&buttons, sizeof(uint32));
					link->Attach(&clicks, sizeof(uint32));
					link->Flush();
				}
				else
				{
					// MouseUp
					link->SetOpCode(B_MOUSE_UP);
					link->Attach(&time, sizeof(int64));
					link->Attach(&mousepos.x,sizeof(float));
					link->Attach(&mousepos.y,sizeof(float));
					link->Attach(&mods, sizeof(uint32));
					link->Flush();
				}
				oldbuttons=buttons;
			}
			
			// Mouse Position change?
			if( (mousepos.x!=oldpos.x) || (mousepos.y!=oldpos.y))
			{
				time=(int64)real_time_clock();
				mods=modifiers();

				// B_MOUSE_MOVED
				link->SetOpCode(B_MOUSE_MOVED);
				link->Attach(&time, sizeof(int64));
				link->Attach(&mousepos.x,sizeof(float));
				link->Attach(&mousepos.y,sizeof(float));
				link->Attach(&mods, sizeof(int32));
				link->Attach(&buttons, sizeof(uint32));
				link->Attach(&clicks, sizeof(uint32));
				link->Flush();
				oldpos=mousepos;
			}
			
			// Mouse wheel support messages are actually sent to BWindowScreens,
			// so we handle that in MessageReceived
		}
		snooze(150);
	}

	delete link;
}

void FrameBuffer::Invalidate(const BRect &r)
{
	if(invalid)
		invalid->Include(r);
	else
		invalid=new BRegion(r);
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

ScreenDriver::ScreenDriver(void)
{
	status_t st;
	screenwin=new FrameBuffer("OBAppServer",B_8_BIT_640x480,&st,false);
	
	framebuffer=screenwin->viewbmp;
	serverlink=screenwin->serverlink;
	hide_cursor=0;
	_SetWidth(640);
	_SetHeight(480);
	_SetDepth(8);
	_SetMode(B_8_BIT_640x480);
	_SetBytesPerRow(framebuffer->BytesPerRow());
}

ScreenDriver::~ScreenDriver(void)
{
	if(is_initialized)
	{
		screenwin->Lock();
		screenwin->Quit();
		screenwin=NULL;
	}
}

bool ScreenDriver::Initialize(void)
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

void ScreenDriver::Shutdown(void)
{
	Lock();
	screenwin->Disconnect();
	is_initialized=false;
	Unlock();
}

void ScreenDriver::SetMode(const display_mode &mode)
{
	screenwin->Lock();
	int16 w=mode.virtual_width,h=mode.virtual_height;
	color_space s=(color_space)mode.space;

	screenwin->ResizeTo(w-1,h-1);

	// Clear the invalid flag so that there is no danger of a crash	
	while(screenwin->invalidflag>0)
		atomic_add(&screenwin->invalidflag,-1);
		
	delete framebuffer;

	// don't forget to update the internal vars!
	_SetWidth(w);
	_SetHeight(h);
	_SetMode(s);

	screenwin->SetSpace((uint32)s);

	screenwin->viewbmp=new BBitmap(screenwin->Bounds(),s,true);
	framebuffer=screenwin->viewbmp;
	drawview=new BView(framebuffer->Bounds(),"drawview",B_FOLLOW_ALL, B_WILL_DRAW);
	framebuffer->AddChild(drawview);

	framebuffer->Lock();
	drawview->SetHighColor(workspace_default_color.GetColor32());
	drawview->FillRect(drawview->Bounds());
	drawview->Sync();
	framebuffer->Unlock();

	screenwin->Invalidate(framebuffer->Bounds());

	_SetBytesPerRow(framebuffer->BytesPerRow());
	atomic_add(&screenwin->invalidflag,1);
	screenwin->Unlock();
}

void ScreenDriver::SetMode(int32 space)
{
	screenwin->Lock();
	int16 w=640,h=480;
	color_space s=B_CMAP8;

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
			_SetDepth(32);
			break;
		case B_16_BIT_640x480:
		case B_16_BIT_800x600:
		case B_16_BIT_1024x768:
			s=B_RGBA15;
			_SetDepth(15);
			break;
		case B_8_BIT_640x480:
		case B_8_BIT_800x600:
		case B_8_BIT_1024x768:
			s=B_CMAP8;
			_SetDepth(8);
			break;
		default:
			_SetDepth(8);
			break;
	}

	// Clear the invalid flag so that there is no danger of a crash	
	while(screenwin->invalidflag>0)
		atomic_add(&screenwin->invalidflag,-1);
		
	delete framebuffer;

	// don't forget to update the internal vars!
	_SetWidth(w);
	_SetHeight(h);
	_SetMode(space);

	screenwin->SetSpace((uint32)space);

	screenwin->viewbmp=new BBitmap(screenwin->Bounds(),s,true);
	framebuffer=screenwin->viewbmp;
	drawview=new BView(framebuffer->Bounds(),"drawview",B_FOLLOW_ALL, B_WILL_DRAW);
	framebuffer->AddChild(drawview);

	framebuffer->Lock();
	drawview->SetHighColor(workspace_default_color.GetColor32());
	drawview->FillRect(drawview->Bounds());
	drawview->Sync();
	framebuffer->Unlock();

	screenwin->Invalidate(framebuffer->Bounds());

	_SetBytesPerRow(framebuffer->BytesPerRow());
	atomic_add(&screenwin->invalidflag,1);
	screenwin->Unlock();
}

void ScreenDriver::CopyBits(BRect src, BRect dest)
{
	screenwin->Lock();
	framebuffer->Lock();
	drawview->CopyBits(src,dest);
	drawview->Sync();
	screenwin->view->Invalidate(src);
	screenwin->view->Invalidate(dest);
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ScreenDriver::DrawBitmap(ServerBitmap *bitmap, BRect src, BRect dest)
{
STRACE(("ScreenDriver:: DrawBitmap unimplemented()\n"));
}

void ScreenDriver::DrawChar(char c, BPoint pt, LayerData *d)
{
	char str[2];
	str[0]=c;
	str[1]='\0';
	DrawString(str, 1, pt, d);
}
/*
void ScreenDriver::DrawString(const char *string, int32 length, BPoint pt, LayerData *d, escapement_delta *delta=NULL)
{
STRACE(("ScreenDriver:: DrawString(\"%s\",%ld,BPoint(%f,%f))\n",string,length,pt.x,pt.y));
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
*/

bool ScreenDriver::DumpToFile(const char *path)
{
	// Dump to PNG
	Lock();
	SaveToPNG(path,framebuffer->Bounds(),framebuffer->ColorSpace(), 
			framebuffer->Bits(),framebuffer->BitsLength(),framebuffer->BytesPerRow());

	Unlock();
	return true;
}

void ScreenDriver::FillArc(BRect r, float angle, float span, LayerData *d, const Pattern &pat)
{
	if(!d)
		return;
	screenwin->Lock();
	framebuffer->Lock();
	SetLayerData(d);
	drawview->FillArc(r,angle,span,*((pattern*)pat.GetInt8()));
	drawview->Sync();
	screenwin->view->Invalidate(r);
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ScreenDriver::FillBezier(BPoint *pts, LayerData *d, const Pattern &pat)
{
	if(!pts)
		return;
	screenwin->Lock();
	framebuffer->Lock();
	SetLayerData(d);
	drawview->FillBezier(pts,*((pattern*)pat.GetInt8()));
	drawview->Sync();
	
	// Invalidate the whole view until I get around to adding in the invalid rect calc code
	screenwin->view->Invalidate();
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ScreenDriver::FillEllipse(BRect r, LayerData *d, const Pattern &pat)
{
	if(!d)
		return;
	screenwin->Lock();
	framebuffer->Lock();
	SetLayerData(d);
	drawview->FillEllipse(r,*((pattern*)pat.GetInt8()));
	drawview->Sync();
	screenwin->view->Invalidate(r);
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ScreenDriver::FillPolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, const Pattern &pat)
{
	if(!d)
		return;
	screenwin->Lock();
	framebuffer->Lock();
	SetLayerData(d);
	drawview->FillPolygon(ptlist,numpts,rect,*((pattern*)pat.GetInt8()));
	drawview->Sync();
	screenwin->view->Invalidate(rect);
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ScreenDriver::FillRect(BRect r, LayerData *d, const Pattern &pat)
{
	if(!d)
		return;
	screenwin->Lock();
	framebuffer->Lock();
	SetLayerData(d);
	drawview->FillRect(r,*((pattern*)pat.GetInt8()));
	drawview->Sync();
	screenwin->view->Invalidate(r);
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ScreenDriver::FillRoundRect(BRect r, float xrad, float yrad, LayerData *d, const Pattern &pat)
{
	if(!d)
		return;
	screenwin->Lock();
	framebuffer->Lock();
	SetLayerData(d);
	drawview->FillRoundRect(r,xrad,yrad,*((pattern*)pat.GetInt8()));
	drawview->Sync();
	screenwin->view->Invalidate(r);
	framebuffer->Unlock();
	screenwin->Unlock();

}

void ScreenDriver::FillTriangle(BPoint *pts, BRect r, LayerData *d, const Pattern &pat)
{
	if(!pts)
		return;
	screenwin->Lock();
	framebuffer->Lock();
	BPoint first=pts[0],second=pts[1],third=pts[2];
	SetLayerData(d);
	drawview->FillTriangle(first,second,third,r,*((pattern*)pat.GetInt8()));
	drawview->Sync();
	screenwin->view->Invalidate(r);
	framebuffer->Unlock();
	screenwin->Unlock();

}

void ScreenDriver::HideCursor(void)
{
	screenwin->Lock();
	Lock();

	hide_cursor++;
	screenwin->PostMessage(SDWIN_HIDECURSOR);

	Unlock();
	screenwin->Unlock();
}

void ScreenDriver::InvertRect(BRect r)
{
	screenwin->Lock();
	framebuffer->Lock();
	drawview->InvertRect(r);
	drawview->Sync();
	screenwin->view->Invalidate(r);
	framebuffer->Unlock();
	screenwin->Unlock();
}

bool ScreenDriver::IsCursorHidden(void)
{
	screenwin->Lock();
	bool value=(hide_cursor>0)?true:false;
	screenwin->Unlock();
	return value;
}

void ScreenDriver::ObscureCursor(void)
{
	screenwin->Lock();
	screenwin->PostMessage(SDWIN_OBSCURECURSOR);
	screenwin->Unlock();
}

void ScreenDriver::MoveCursorTo(float x, float y)
{
	screenwin->Lock();
	BMessage *msg=new BMessage(SDWIN_MOVECURSOR);
	msg->AddFloat("x",x);
	msg->AddFloat("y",y);
	screenwin->PostMessage(msg);
	screenwin->Unlock();
}

void ScreenDriver::SetCursor(ServerCursor *cursor)
{
printf("SetCursor unimplemented\n");
/*
	if(cursor!=NULL)
	{
		screenwin->Lock();
		BBitmap *bmp=new BBitmap(cursor->Bounds(),B_RGBA32);
	
		// Copy the server bitmap in the cursor to a BBitmap
		uint8	*sbmppos=(uint8*)cursor->Bits(),
				*bbmppos=(uint8*)bmp->Bits();
	
		int32 bytes=cursor->BytesPerRow(),
			bbytes=bmp->BytesPerRow();
	
		for(int i=0;i<=cursor->Bounds().IntegerHeight();i++)
			memcpy(bbmppos+(i*bbytes), sbmppos+(i*bytes), bytes);

		// Replace the bitmap
		delete screenwin->cursor;
		screenwin->cursor=bmp;
		screenwin->Invalidate(screenwin->view->cursorframe);
		screenwin->Unlock();
	}
*/
}

void ScreenDriver::ShowCursor(void)
{
	screenwin->Lock();
	if(hide_cursor>0)
	{
		hide_cursor--;
		screenwin->PostMessage(SDWIN_SHOWCURSOR);
	}
	screenwin->Unlock();

}

void ScreenDriver::StrokeArc(BRect r, float angle, float span, LayerData *d, const Pattern &pat)
{
	if(!d)
		return;
	screenwin->Lock();
	framebuffer->Lock();
	SetLayerData(d);
	drawview->StrokeArc(r,angle,span,*((pattern*)pat.GetInt8()));
	drawview->Sync();
	screenwin->view->Invalidate(r);
	framebuffer->Unlock();
	screenwin->Unlock();

}

void ScreenDriver::StrokeBezier(BPoint *pts, LayerData *d, const Pattern &pat)
{
	if(!pts)
		return;
	screenwin->Lock();
	framebuffer->Lock();
	SetLayerData(d);
	drawview->StrokeBezier(pts,*((pattern*)pat.GetInt8()));
	drawview->Sync();
	
	// Invalidate the whole view until I get around to adding in the invalid rect calc code
	screenwin->view->Invalidate();
	framebuffer->Unlock();
	screenwin->Unlock();

}

void ScreenDriver::StrokeEllipse(BRect r, LayerData *d, const Pattern &pat)
{
	if(!d)
		return;
	screenwin->Lock();
	framebuffer->Lock();
	SetLayerData(d);
	drawview->StrokeEllipse(r,*((pattern*)pat.GetInt8()));
	drawview->Sync();
	screenwin->view->Invalidate(r);
	framebuffer->Unlock();
	screenwin->Unlock();

}

void ScreenDriver::StrokeLine(BPoint start, BPoint end, LayerData *d, const Pattern &pat)
{
	if(!d)
		return;
	screenwin->Lock();
	framebuffer->Lock();
	SetLayerData(d);
	drawview->StrokeLine(start,end,*((pattern*)pat.GetInt8()));
	drawview->Sync();
	screenwin->view->Invalidate(BRect(start,end));
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ScreenDriver::StrokeLineArray(BPoint *pts, int32 numlines, RGBColor *colors, LayerData *d)
{
STRACE(("ScreenDriver:: StrokeLineArray unimplemented\n"));

}

void ScreenDriver::StrokePolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, const Pattern &pat, bool is_closed)
{
	if(!ptlist)
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

void ScreenDriver::StrokeRect(BRect r, LayerData *d, const Pattern &pat)
{
	if(!d)
		return;
	screenwin->Lock();
	framebuffer->Lock();
	SetLayerData(d);
	drawview->StrokeRect(r,*((pattern*)pat.GetInt8()));
	drawview->Sync();
	screenwin->view->Invalidate(r);
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ScreenDriver::StrokeRoundRect(BRect r, float xrad, float yrad, LayerData *d, const Pattern &pat)
{
	if(!d)
		return;
	screenwin->Lock();
	framebuffer->Lock();
	SetLayerData(d);
	drawview->StrokeRoundRect(r,xrad,yrad,*((pattern*)pat.GetInt8()));
	drawview->Sync();
	screenwin->view->Invalidate(r);
	framebuffer->Unlock();
	screenwin->Unlock();

}

void ScreenDriver::StrokeTriangle(BPoint *pts, BRect r, LayerData *d, const Pattern &pat)
{
	if(!pts || !d)
		return;
	screenwin->Lock();
	framebuffer->Lock();
	BPoint first=pts[0],second=pts[1],third=pts[2];
	SetLayerData(d);
	drawview->StrokeTriangle(first,second,third,r,*((pattern*)pat.GetInt8()));
	drawview->Sync();
	screenwin->view->Invalidate(r);
	framebuffer->Unlock();
	screenwin->Unlock();

}

void ScreenDriver::SetLayerData(LayerData *d, bool set_font_data)
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
		ServerFont *sf=&(d->font);

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
}

float ScreenDriver::StringWidth(const char *string, int32 length, LayerData *d)
{
	if(!string || !d)
		return 0.0;
	screenwin->Lock();

	ServerFont *font=&(d->font);
	FontStyle *style=font->Style();

	if(!style)
		return 0.0;

	FT_Face face;
	FT_GlyphSlot slot;
	FT_UInt glyph_index=0, previous=0;
	FT_Vector pen,delta;
	int16 error=0;
	int32 strlength,i;
	float returnval;

	error=FT_New_Face(ftlib, style->GetPath(), 0, &face);
	if(error)
		return 0.0;

	slot=face->glyph;

	bool use_kerning=FT_HAS_KERNING(face) && font->Spacing()==B_STRING_SPACING;
	
	error=FT_Set_Char_Size(face, 0,int32(font->Size())*64,72,72);
	if(error)
		return 0.0;

	// set the pen position in 26.6 cartesian space coordinates
	pen.x=0;
	
	slot=face->glyph;
	
	strlength=strlen(string);
	if(length<strlength)
		strlength=length;

	for(i=0;i<strlength;i++)
	{
		// get kerning and move pen
		if(use_kerning && previous && glyph_index)
		{
			FT_Get_Kerning(face, previous, glyph_index,ft_kerning_default, &delta);
			pen.x+=delta.x;
		}

		error=FT_Load_Char(face,string[i],FT_LOAD_MONOCHROME);

		// increment pen position
		pen.x+=slot->advance.x;
		previous=glyph_index;
	}
	screenwin->Unlock();

	FT_Done_Face(face);

	returnval=pen.x>>6;
	return returnval;
}

float ScreenDriver::StringHeight(const char *string, int32 length, LayerData *d)
{
	if(!string || !d)
		return 0.0;
	screenwin->Lock();

	ServerFont *font=&(d->font);
	FontStyle *style=font->Style();

	if(!style)
		return 0.0;

	FT_Face face;
	FT_GlyphSlot slot;
	int16 error=0;
	int32 strlength,i;
	float returnval=0.0,ascent=0.0,descent=0.0;

	error=FT_New_Face(ftlib, style->GetPath(), 0, &face);
	if(error)
		return 0.0;

	slot=face->glyph;
	
	error=FT_Set_Char_Size(face, 0,int32(font->Size())*64,72,72);
	if(error)
		return 0.0;

	slot=face->glyph;
	
	strlength=strlen(string);
	if(length<strlength)
		strlength=length;

	for(i=0;i<strlength;i++)
	{
		FT_Load_Char(face,string[i],FT_LOAD_RENDER);
		if(slot->metrics.horiBearingY<slot->metrics.height)
			descent=MAX((slot->metrics.height-slot->metrics.horiBearingY)>>6,descent);
		else
			ascent=MAX(slot->bitmap.rows,ascent);
	}
	screenwin->Unlock();

	FT_Done_Face(face);

	returnval=ascent+descent;
	return returnval;
}

void ScreenDriver::DrawString(const char *string, int32 length, BPoint pt, LayerData *d, escapement_delta *edelta)
{
	if(!string || !d)
		return;
	screenwin->Lock();

	pt.y--;	// because of Be's backward compatibility hack

	ServerFont *font=&(d->font);
	FontStyle *style=font->Style();

	if(!style)
		return;

	FT_Face face;
	FT_GlyphSlot slot;
	FT_Matrix rmatrix,smatrix;
	FT_UInt glyph_index=0, previous=0;
	FT_Vector pen,delta,space,nonspace;
	int16 error=0;
	int32 strlength,i;
	Angle rotation(font->Rotation()), shear(font->Shear());

	bool antialias=( (font->Size()<18 && font->Flags()& B_DISABLE_ANTIALIASING==0)
		|| font->Flags()& B_FORCE_ANTIALIASING)?true:false;

	// Originally, I thought to do this shear checking here, but it really should be
	// done in BFont::SetShear()
	float shearangle=shear.Value();
	if(shearangle>135)
		shearangle=135;
	if(shearangle<45)
		shearangle=45;

	if(shearangle>90)
		shear=90+((180-shearangle)*2);
	else
		shear=90-(90-shearangle)*2;
	
	error=FT_New_Face(ftlib, style->GetPath(), 0, &face);
	if(error)
		return;

	slot=face->glyph;

	bool use_kerning=FT_HAS_KERNING(face) && font->Spacing()==B_STRING_SPACING;
	
	error=FT_Set_Char_Size(face, 0,int32(font->Size())*64,72,72);
	if(error)
		return;

	// if we do any transformation, we do a call to FT_Set_Transform() here
	
	// First, rotate
	rmatrix.xx = (FT_Fixed)( rotation.Cosine()*0x10000); 
	rmatrix.xy = (FT_Fixed)(-rotation.Sine()*0x10000); 
	rmatrix.yx = (FT_Fixed)( rotation.Sine()*0x10000); 
	rmatrix.yy = (FT_Fixed)( rotation.Cosine()*0x10000); 
	
	// Next, shear
	smatrix.xx = (FT_Fixed)(0x10000); 
	smatrix.xy = (FT_Fixed)(-shear.Cosine()*0x10000); 
	smatrix.yx = (FT_Fixed)(0); 
	smatrix.yy = (FT_Fixed)(0x10000); 

	FT_Matrix_Multiply(&rmatrix,&smatrix);
	
	// Set up the increment value for escapement padding
	space.x=int32(d->edelta.space * rotation.Cosine()*64);
	space.y=int32(d->edelta.space * rotation.Sine()*64);
	nonspace.x=int32(d->edelta.nonspace * rotation.Cosine()*64);
	nonspace.y=int32(d->edelta.nonspace * rotation.Sine()*64);
	
	// set the pen position in 26.6 cartesian space coordinates
	pen.x=(int32)pt.x * 64;
	pen.y=(int32)pt.y * 64;
	
	slot=face->glyph;

	
	strlength=strlen(string);
	if(length<strlength)
		strlength=length;

	for(i=0;i<strlength;i++)
	{
		FT_Set_Transform(face,&smatrix,&pen);

		// Handle escapement padding option
		if((uint8)string[i]<=0x20)
		{
			pen.x+=space.x;
			pen.y+=space.y;
		}
		else
		{
			pen.x+=nonspace.x;
			pen.y+=nonspace.y;
		}

	
		// get kerning and move pen
		if(use_kerning && previous && glyph_index)
		{
			FT_Get_Kerning(face, previous, glyph_index,ft_kerning_default, &delta);
			pen.x+=delta.x;
			pen.y+=delta.y;
		}

		error=FT_Load_Char(face,string[i],
			((antialias)?FT_LOAD_RENDER:FT_LOAD_RENDER | FT_LOAD_MONOCHROME) );

		if(!error)
		{
			if(antialias)
				BlitGray2RGB32(&slot->bitmap,
					BPoint(slot->bitmap_left,pt.y-(slot->bitmap_top-pt.y)), d);
			else
				BlitMono2RGB32(&slot->bitmap,
					BPoint(slot->bitmap_left,pt.y-(slot->bitmap_top-pt.y)), d);
		}

		// increment pen position
		pen.x+=slot->advance.x;
		pen.y+=slot->advance.y;
		previous=glyph_index;
	}

	// TODO: implement properly
	// calculate the invalid rectangle
	BRect r;
	r.left=MIN(pt.x,pen.x>>6);
	r.right=MAX(pt.x,pen.x>>6);
	r.top=pt.y-face->height;
	r.bottom=pt.y+face->height;

	screenwin->view->Invalidate(r);
	screenwin->Unlock();

	FT_Done_Face(face);
}

void ScreenDriver::BlitMono2RGB32(FT_Bitmap *src, BPoint pt, LayerData *d)
{
	rgb_color color=d->highcolor.GetColor32();
	
	// pointers to the top left corner of the area to be copied in each bitmap
	uint8 *srcbuffer, *destbuffer;
	
	// index pointers which are incremented during the course of the blit
	uint8 *srcindex, *destindex, *rowptr, value;
	
	// increment values for the index pointers
	int32 srcinc=src->pitch, destinc=framebuffer->BytesPerRow();
	
	int16 i,j,k, srcwidth=src->pitch, srcheight=src->rows;
	int32 x=(int32)pt.x,y=(int32)pt.y;
	
	// starting point in source bitmap
	srcbuffer=(uint8*)src->buffer;

	if(y<0)
	{
		if(y<pt.y)
			y++;
		srcbuffer+=srcinc * (0-y);
		srcheight-=srcinc;
		destbuffer+=destinc * (0-y);
	}

	if(y+srcheight>framebuffer->Bounds().IntegerHeight())
	{
		if(y>pt.y)
			y--;
		srcheight-=(y+srcheight-1)-framebuffer->Bounds().IntegerHeight();
	}

	if(x+srcwidth>framebuffer->Bounds().IntegerWidth())
	{
		if(x>pt.x)
			x--;
		srcwidth-=(x+srcwidth-1)-framebuffer->Bounds().IntegerWidth();
	}
	
	if(x<0)
	{
		if(x<pt.x)
			x++;
		srcbuffer+=(0-x)>>3;
		srcwidth-=0-x;
		destbuffer+=(0-x)*4;
	}
	
	// starting point in destination bitmap
	destbuffer=(uint8*)framebuffer->Bits()+int32( (pt.y*framebuffer->BytesPerRow())+(pt.x*4) );

	srcindex=srcbuffer;
	destindex=destbuffer;

	for(i=0; i<srcheight; i++)
	{
		rowptr=destindex;		

		for(j=0;j<srcwidth;j++)
		{
			for(k=0; k<8; k++)
			{
				value=*(srcindex+j) & (1 << (7-k));
				if(value)
				{
					rowptr[0]=color.blue;
					rowptr[1]=color.green;
					rowptr[2]=color.red;
					rowptr[3]=color.alpha;
				}

				rowptr+=4;
			}

		}
		
		srcindex+=srcinc;
		destindex+=destinc;
	}

}

void ScreenDriver::BlitGray2RGB32(FT_Bitmap *src, BPoint pt, LayerData *d)
{
	// pointers to the top left corner of the area to be copied in each bitmap
	uint8 *srcbuffer=NULL, *destbuffer=NULL;
	
	// index pointers which are incremented during the course of the blit
	uint8 *srcindex=NULL, *destindex=NULL, *rowptr=NULL;
	
	rgb_color highcolor=d->highcolor.GetColor32(), lowcolor=d->lowcolor.GetColor32();	float rstep,gstep,bstep,astep;

	rstep=float(highcolor.red-lowcolor.red)/255.0;
	gstep=float(highcolor.green-lowcolor.green)/255.0;
	bstep=float(highcolor.blue-lowcolor.blue)/255.0;
	astep=float(highcolor.alpha-lowcolor.alpha)/255.0;
	
	// increment values for the index pointers
	int32 x=(int32)pt.x,
		y=(int32)pt.y,
		srcinc=src->pitch,
//		destinc=dest->BytesPerRow(),
		destinc=framebuffer->BytesPerRow(),
		srcwidth=src->width,
		srcheight=src->rows,
		incval=0;
	
	int16 i,j;
	
	// starting point in source bitmap
	srcbuffer=(uint8*)src->buffer;

	// starting point in destination bitmap
//	destbuffer=(uint8*)dest->Bits()+(y*dest->BytesPerRow()+(x*4));
	destbuffer=(uint8*)framebuffer->Bits()+(y*framebuffer->BytesPerRow()+(x*4));


	if(y<0)
	{
		if(y<pt.y)
			y++;
		
		incval=0-y;
		
		srcbuffer+=incval * srcinc;
		srcheight-=incval;
		destbuffer+=incval * destinc;
	}

	if(y+srcheight>framebuffer->Bounds().IntegerHeight())
	{
		if(y>pt.y)
			y--;
		srcheight-=(y+srcheight-1)-framebuffer->Bounds().IntegerHeight();
	}

	if(x+srcwidth>framebuffer->Bounds().IntegerWidth())
	{
		if(x>pt.x)
			x--;
		srcwidth-=(x+srcwidth-1)-framebuffer->Bounds().IntegerWidth();
	}
	
	if(x<0)
	{
		if(x<pt.x)
			x++;
		incval=0-x;
		srcbuffer+=incval;
		srcwidth-=incval;
		destbuffer+=incval*4;
	}

	int32 value;

	srcindex=srcbuffer;
	destindex=destbuffer;

	for(i=0; i<srcheight; i++)
	{
		rowptr=destindex;		

		for(j=0;j<srcwidth;j++)
		{
			value=*(srcindex+j) ^ 255;

			if(value!=255)
			{
				if(d->draw_mode==B_OP_COPY)
				{
					rowptr[0]=uint8(highcolor.blue-(value*bstep));
					rowptr[1]=uint8(highcolor.green-(value*gstep));
					rowptr[2]=uint8(highcolor.red-(value*rstep));
					rowptr[3]=255;
				}
				else
					if(d->draw_mode==B_OP_OVER)
					{
						if(highcolor.alpha>127)
						{
							rowptr[0]=uint8(highcolor.blue-(value*(float(highcolor.blue-rowptr[0])/255.0)));
							rowptr[1]=uint8(highcolor.green-(value*(float(highcolor.green-rowptr[1])/255.0)));
							rowptr[2]=uint8(highcolor.red-(value*(float(highcolor.red-rowptr[2])/255.0)));
							rowptr[3]=255;
						}
					}
			}
			rowptr+=4;

		}
		
		srcindex+=srcinc;
		destindex+=destinc;
	}
}

rgb_color ScreenDriver::GetBlitColor(rgb_color src, rgb_color dest, LayerData *d, bool use_high)
{
	rgb_color returncolor={0,0,0,0};
	int16 value;
	if(!d)
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
