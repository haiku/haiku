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
//	File Name:		ScreenDriver.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Gabe Yoder <gyoder@stny.rr.com>
//	Description:	BWindowScreen graphics module
//  
//------------------------------------------------------------------------------
#include "Angle.h"
#include "PatternHandler.h"
#include "ScreenDriver.h"
#include "ServerProtocol.h"
#include "ServerBitmap.h"
#include "ServerCursor.h"
#include "ServerConfig.h"
#include "SystemPalette.h"
#include "ColorUtils.h"
#include "PortLink.h"
#include "FontFamily.h"
#include "RGBColor.h"
#include "LayerData.h"
#include <View.h>
#include <stdio.h>
#include <string.h>
#include <String.h>
#include <math.h>

#define CLIP_X(a) ( (a < 0) ? 0 : ((a > fbuffer->gcinfo.width-1) ? \
			fbuffer->gcinfo.width-1 : a) )
#define CLIP_Y(a) ( (a < 0) ? 0 : ((a > fbuffer->gcinfo.height-1) ? \
			fbuffer->gcinfo.height-1 : a) )
#define CHECK_X(a) ( (a >= 0) || (a <= fbuffer->gcinfo.width-1) )
#define CHECK_Y(a) ( (a >= 0) || (a <= fbuffer->gcinfo.height-1) )

// TODO: add code to take advantage of HW acceleration

extern RGBColor workspace_default_color;	// defined in AppServer.cpp

//TODO: Remove the need for these
int64 patsolidhigh64=0xFFFFFFFFLL;
int8 *patsolidhigh=(int8*)patsolidhigh64;

void HLine_32Bit(graphics_card_info i, uint16 x, uint16 y, uint16 length, rgb_color col);
void HLine_16Bit(graphics_card_info i, uint16 x, uint16 y, uint16 length, uint16 col);
void HLine_16Bit(graphics_card_info i, uint16 x, uint16 y, uint16 length, uint8 col);

class LineCalc
{
public:
	LineCalc(const BPoint &pta, const BPoint &ptb);
	float GetX(float y);
	float GetY(float x);
	float Slope(void) { return slope; }
	float Offset(void) { return offset; }
private:
	float slope;
	float offset;
	BPoint start, end;
};

LineCalc::LineCalc(const BPoint &pta, const BPoint &ptb)
{
	start=pta;
	end=ptb;
	slope=(start.y-end.y)/(start.x-end.x);
	offset=start.y-(slope * start.x);
}

float LineCalc::GetX(float y)
{
	return ( (y-offset)/slope );
}

float LineCalc::GetY(float x)
{
	return ( (slope * x) + offset );
}

FrameBuffer::FrameBuffer(const char *title, uint32 space, status_t *st,bool debug)
	: BWindowScreen(title,space,st,debug)
{
	is_connected=false;
	port_id serverport=find_port(SERVER_INPUT_PORT);
	serverlink=new PortLink(serverport);
	mousepos.Set(0,0);
	buttons=0;

#ifdef ENABLE_INPUT_SERVER_EMULATION
	// View exists to poll for the mouse
	view=new BView(Bounds(),"view",0,0);
	AddChild(view);
	monitor_thread=spawn_thread(MouseMonitor,"mousemonitor",B_NORMAL_PRIORITY,this);
	resume_thread(monitor_thread);
#endif
}

FrameBuffer::~FrameBuffer(void)
{
	delete serverlink;
	kill_thread(monitor_thread);
}

void FrameBuffer::ScreenConnected(bool connected)
{
	is_connected=connected;
	if(connected)
	{
		// Cache the state just in case
		graphics_card_info *info=CardInfo();
		gcinfo=*info;

		// Add our spiffy HW acceleration support
//		graphics_card_hook gchook;
/*		_ae=(acquire_engine)CardHookAt(B_ACQUIRE_ENGINE);
		_re=(release_engine)CardHookAt(B_RELEASE_ENGINE);
		_s2sb=(screen_to_screen_blit)CardHookAt(B_SCREEN_TO_SCREEN_BLIT);
		_fspan=(fill_span)CardHookAt(B_FILL_SPAN);
		_ir=(invert_rectangle)CardHookAt(HWINVERT);

		printf("Hardware Acceleration capabilities:\n");
		if(_ae)
			printf("Acquire Engine\n");
		if(_re)
			printf("Release Engine\n");
		if(_s2sb)
			printf("Screen-To-Screen Blit\n");
		if(_fspan)
			printf("Fill Span\n");
		if(_ir)
			printf("Invert Rectangle\n");

		gchook=CardHookAt(HWLINE_32BIT);
		if(gchook)
			printf("32-big line\n");
		gchook=CardHookAt(HWBLIT);
		if(gchook)
			printf("Screen Blit\n");
*/
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
/*
#ifdef ENABLE_INPUT_SERVER_EMULATION
				case 0x57:	// up arrow
				{
					// Attach data:
					// 1) int64 - time of mouse click
					// 2) float - x coordinate of mouse click
					// 3) float - y coordinate of mouse click
					// 4) int32 - modifier keys down
					// 5) int32 - buttons down
					// 6) int32 - clicks
					
					
					if(mousepos.y<=0)
					{
						mousepos.y=0;
						break;
					}
					
					uint32 clicks=1; // can't get the # of clicks without a *lot* of extra work :(
					int64 time=(int64)real_time_clock();
					
					if(modifiers & B_SHIFT_KEY)
						mousepos.y--;
					else
					{
						mousepos.y-=10;
						if(mousepos.y<0)
							mousepos.y=0;
					}

					serverlink->SetOpCode(B_MOUSE_MOVED);
					serverlink->Attach(&time, sizeof(int64));
					serverlink->Attach(&mousepos.x,sizeof(float));
					serverlink->Attach(&mousepos.y,sizeof(float));
					serverlink->Attach(&servermods, sizeof(int32));
					serverlink->Attach(&buttons, sizeof(uint32));
					serverlink->Attach(&clicks, sizeof(uint32));
					serverlink->Flush();
					break;
				}
				case 0x62:	// down arrow
				{
					// Attach data:
					// 1) int64 - time of mouse click
					// 2) float - x coordinate of mouse click
					// 3) float - y coordinate of mouse click
					// 4) int32 - modifier keys down
					// 5) int32 - buttons down
					// 6) int32 - clicks
					
					
					if(mousepos.y>= (gcinfo.height-1))
					{
						mousepos.y=gcinfo.height-1;
						break;
					}
					
					uint32 clicks=1; // can't get the # of clicks without a *lot* of extra work :(
					int64 time=(int64)real_time_clock();
					
					if(modifiers & B_SHIFT_KEY)
						mousepos.y++;
					else
					{
						mousepos.y+=10;
						if(mousepos.y>gcinfo.height-1)
							mousepos.y=gcinfo.height-1;
					}

					serverlink->SetOpCode(B_MOUSE_MOVED);
					serverlink->Attach(&time, sizeof(int64));
					serverlink->Attach(&mousepos.x,sizeof(float));
					serverlink->Attach(&mousepos.y,sizeof(float));
					serverlink->Attach(&servermods, sizeof(int32));
					serverlink->Attach(&buttons, sizeof(uint32));
					serverlink->Attach(&clicks, sizeof(uint32));
					serverlink->Flush();
					break;
				}
				case 0x61:	// left arrow
				{
					// Attach data:
					// 1) int64 - time of mouse click
					// 2) float - x coordinate of mouse click
					// 3) float - y coordinate of mouse click
					// 4) int32 - modifier keys down
					// 5) int32 - buttons down
					// 6) int32 - clicks
					
					
					if(mousepos.x<=0)
					{
						mousepos.x=0;
						break;
					}
					
					uint32 clicks=1; // can't get the # of clicks without a *lot* of extra work :(
					int64 time=(int64)real_time_clock();
					
					if(modifiers & B_SHIFT_KEY)
						mousepos.x--;
					else
					{
						mousepos.x-=10;
						if(mousepos.x<0)
							mousepos.x=0;
					}

					serverlink->SetOpCode(B_MOUSE_MOVED);
					serverlink->Attach(&time, sizeof(int64));
					serverlink->Attach(&mousepos.x,sizeof(float));
					serverlink->Attach(&mousepos.y,sizeof(float));
					serverlink->Attach(&servermods, sizeof(int32));
					serverlink->Attach(&buttons, sizeof(uint32));
					serverlink->Attach(&clicks, sizeof(uint32));
					serverlink->Flush();
					break;
				}
				case 0x63:	// right arrow
				{
					// Attach data:
					// 1) int64 - time of mouse click
					// 2) float - x coordinate of mouse click
					// 3) float - y coordinate of mouse click
					// 4) int32 - modifier keys down
					// 5) int32 - buttons down
					// 6) int32 - clicks
					
					
					if(mousepos.x>= (gcinfo.width-1))
					{
						mousepos.x=gcinfo.width-1;
						break;
					}
					
					uint32 clicks=1; // can't get the # of clicks without a *lot* of extra work :(
					int64 time=(int64)real_time_clock();
					
					if(modifiers & B_SHIFT_KEY)
						mousepos.x++;
					else
					{
						mousepos.x+=10;
						if(mousepos.x>gcinfo.width-1)
							mousepos.x=gcinfo.width-1;
					}

					serverlink->SetOpCode(B_MOUSE_MOVED);
					serverlink->Attach(&time, sizeof(int64));
					serverlink->Attach(&mousepos.x,sizeof(float));
					serverlink->Attach(&mousepos.y,sizeof(float));
					serverlink->Attach(&servermods, sizeof(int32));
					serverlink->Attach(&buttons, sizeof(uint32));
					serverlink->Attach(&clicks, sizeof(uint32));
					serverlink->Flush();
					break;
				}
#endif	// end server emu code
*/				default:
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

/*		case B_MODIFIERS_CHANGED:
		{
			int32 modifiers;
			if(msg->FindInt32("modifiers",&modifiers)==B_OK)
			{

#ifdef ENABLE_INPUT_SERVER_EMULATION

				uint32 clicks=1; // can't get the # of clicks without a *lot* of extra work :(
				int64 time=(int64)real_time_clock();
				
				// we need to be able to shift click & such, so we will construct
				// a new modifiers value which excludes our mouse click keys
				int32 servermods=0;
				servermods|=modifiers & B_RIGHT_COMMAND_KEY;
				servermods|=modifiers & B_RIGHT_OPTION_KEY;
				servermods|=modifiers & B_RIGHT_CONTROL_KEY;
				servermods|=modifiers & B_SHIFT_KEY;
				
				if(modifiers & B_LEFT_OPTION_KEY)
				{
					buttons=B_PRIMARY_MOUSE_BUTTON;
					
					serverlink->SetOpCode(B_MOUSE_DOWN);
					serverlink->Attach(&time, sizeof(int64));
					serverlink->Attach(&mousepos.x,sizeof(float));
					serverlink->Attach(&mousepos.y,sizeof(float));
					serverlink->Attach(&servermods, sizeof(uint32));
					serverlink->Attach(&buttons, sizeof(uint32));
					serverlink->Attach(&clicks, sizeof(uint32));
					serverlink->Flush();
				}
				else
					if(modifiers & B_LEFT_CONTROL_KEY)
					{
						buttons=B_SECONDARY_MOUSE_BUTTON;

						serverlink->SetOpCode(B_MOUSE_DOWN);
						serverlink->Attach(&time, sizeof(int64));
						serverlink->Attach(&mousepos.x,sizeof(float));
						serverlink->Attach(&mousepos.y,sizeof(float));
						serverlink->Attach(&servermods, sizeof(uint32));
						serverlink->Attach(&buttons, sizeof(uint32));
						serverlink->Attach(&clicks, sizeof(uint32));
						serverlink->Flush();
					}
					else
					{
						// well, we got this far so none of the buttons
						// are down. set our mouse buttons to up and if this
						// is a change, send a B_MOUSE_UP message
						if(buttons>0)
						{
							serverlink->SetOpCode(B_MOUSE_UP);
							serverlink->Attach(&time, sizeof(int64));
							serverlink->Attach(&mousepos.x,sizeof(float));
							serverlink->Attach(&mousepos.y,sizeof(float));
							serverlink->Attach(&servermods, sizeof(uint32));
							serverlink->Flush();
						}
						buttons=0;
					}
#endif	// end server emu code
			}
			break;
		}
*/		default:
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

/*!
	\brief Sets up internal variables needed by all DisplayDriver subclasses
	
	Subclasses should follow DisplayDriver's lead and use this function mostly
	for initializing data members.
*/
ScreenDriver::ScreenDriver(void) : DisplayDriver()
{
	status_t st;
	fbuffer=new FrameBuffer("OBAppServer",B_8_BIT_640x480,&st,true);

	cursor=NULL;
	under_cursor=NULL;
	cursorframe.Set(0,0,0,0);
	_SetMode(B_8_BIT_640x480);
	_SetWidth(640);
	_SetHeight(480);
	_SetDepth(8);
	_SetBytesPerRow(fbuffer->FrameBufferInfo()->bytes_per_row);
}

/*!
	\brief Deletes the locking semaphore
	
	Subclasses should use the destructor mostly for freeing allocated heap space.
*/
ScreenDriver::~ScreenDriver(void)
{
	if(cursor)
		delete cursor;
	if(under_cursor)
		delete under_cursor;
}

/*!
	\brief Initializes the driver object.
	\return true if successful, false if not
	
	Initialize sets up the driver for display, including the initial clearing
	of the screen. If things do not go as they should, false should be returned.
*/
bool ScreenDriver::Initialize(void)
{
	fbuffer->Show();

	// wait 1 sec for the window to show...
	snooze(500000);

	if(fbuffer->IsConnected())
	{
		graphics_card_info *info=fbuffer->CardInfo();
		fbuffer->gcinfo=*info;
		
		// clear the frame buffer. Otherwise, we'll have garbage in it
		LayerData d;
		d.highcolor=workspace_default_color;
		for(int32 i=0; i<info->height; i++)
		{
			Line(BPoint(0,i),BPoint(info->width-1,i),&d,patsolidhigh);
//			HLine(0, info->width-1, i, workspace_default_color);
		}
	}
	else
		printf("Not connected in Initialize\n");

	// we start out without a cursor shown because otherwise we get glitches in the
	// upper left corner. init_desktop *always* sets a cursor, so this shouldn't be a problem
	return true;
}

/*!
	\brief Shuts down the driver's video subsystem
	
	Any work done by Initialize() should be undone here. Note that Shutdown() is
	called even if Initialize() was unsuccessful.
*/
void ScreenDriver::Shutdown(void)
{
	fbuffer->Lock();
	fbuffer->Quit();
}

/*!
	\brief Called for all BView::CopyBits calls
	\param src Source rectangle.
	\param dest Destination rectangle.
	
	Bounds checking must be done in this call. If the destination is not the same size 
	as the source, the source should be scaled to fit.
*/
void ScreenDriver::CopyBits(BRect src, BRect dest)
{
printf("ScreenDriver::CopyBits unimplemented\n");
}

/*!
	\brief Called for all BView::DrawBitmap calls
	\param bmp Bitmap to be drawn. It will always be non-NULL and valid. The color 
	space is not guaranteed to match.
	\param src Source rectangle
	\param dest Destination rectangle. Source will be scaled to fit if not the same size.
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	
	Bounds checking must be done in this call.
*/
void ScreenDriver::DrawBitmap(ServerBitmap *bitmap, BRect source, BRect dest, LayerData *d)
{
	_Lock();
	if(fbuffer->IsConnected())
	{
		// Scale bitmap here if source!=dest
	
		switch(fbuffer->gcinfo.bits_per_pixel)
		{	
			case 32:
			case 24:
			{
				// TODO: Implement
				printf("DrawBitmap: 32/24-bit unimplemented\n");
				break;
			}
			case 16:
			case 15:
			{
				// TODO: Implement
				printf("DrawBitmap: 16/15-bit unimplemented\n");
				break;
			}
			case 8:
			{
				// TODO: Implement
				printf("DrawBitmap: 8-bit unimplemented\n");
				break;
			}
			default:
			{
				break;
			}
		}
	}

	_Unlock();
}

/*!
	\brief Called for all BView::FillArc calls
	\param r Rectangle enclosing the entire arc
	\param angle Starting angle for the arc in degrees
	\param span Span of the arc in degrees. Ending angle = angle+span.
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the arc may end up
	being clipped.
*/
void ScreenDriver::FillArc(BRect r, float angle, float span, LayerData *d, int8 *pat)
{
}

/*!
	\brief Called for all BView::FillBezier calls.
	\param pts 4-element array of BPoints in the order of start, end, and then the two control
	points. 
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call.
*/
void ScreenDriver::FillBezier(BPoint *pts, LayerData *d, int8 *pat)
{
}

/*!
	\brief Called for all BView::FillEllipse calls
	\param r BRect enclosing the ellipse to be drawn.
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the ellipse may end up
	being clipped.
*/
void ScreenDriver::FillEllipse(BRect r, LayerData *ldata, int8 *pat)
{
// Ellipse code shamelessly stolen from the graphics library gd v2.0.1 and bolted on
// to support our API
	long d, b_sq, b_sq_4, b_sq_6;
	long a_sq, a_sq_4, a_sq_6;
	int x, y, switchem;
	long lsqrt (long);
	int pix, half, pstart;
	int32 thick = (int32)ldata->pensize;
	
	half = thick / 2;
	int32 w=int32(r.Width()/2),
		h=int32(r.Height()/2);
	int32 cx=(int32)r.left+w;
	int32 cy=(int32)r.top+h;
	
	d = 2 * (long) h *h + (long) w *w - 2 * (long) w *w * h;
	b_sq = (long) h *h;
	b_sq_4 = 4 * (long) h *h;
	b_sq_6 = 6 * (long) h *h;
	a_sq = (long) w *w;
	a_sq_4 = 4 * (long) w *w;
	a_sq_6 = 6 * (long) w *w;
	
	x = 0;
	y = -h;
//	switchem = a_sq / lsqrt (a_sq + b_sq);
	switchem = a_sq / (int32)sqrt(a_sq + b_sq);

	while (x <= switchem)
	{
		pstart = y - half;
		for (pix = pstart; pix < pstart + thick; pix++)
		{
			Line( BPoint(cx - x, cy + pix), BPoint(cx + x, cy + pix), ldata, patsolidhigh);
			Line( BPoint(cx - x, cy - pix), BPoint(cx + x, cy - pix), ldata, patsolidhigh);
		}
		if (d < 0)
			d += b_sq_4 * x++ + b_sq_6;
		else
			d += b_sq_4 * x++ + b_sq_6 + a_sq_4 * (++y);
	}
	
	/* Middlesplat!
	** Go a little further if the thickness is not nominal...
	*/
	if (thick > 1)
	{
		int xp = x;
		int yp = y;
		int dp = d;
		int thick2 = thick + 2;
		int half2 = half + 1;
		
		while (xp <= switchem + half)
		{
			pstart = yp - half2;
			for (pix = pstart; pix < pstart + thick2; pix++)
			{
				Line( BPoint(cx - xp, cy + pix), BPoint(cx + xp, cy + pix), ldata, patsolidhigh);
				Line( BPoint(cx - xp, cy - pix), BPoint(cx + xp, cy - pix), ldata, patsolidhigh);
			}
			if (dp < 0)
				dp += b_sq_4 * xp++ + b_sq_6;
			else
				dp += b_sq_4 * xp++ + b_sq_6 + a_sq_4 * (++yp);
		}
	}
	
	d += -2 * (long) b_sq + 2 * (long) a_sq - 2 * (long) b_sq *(x - 1) + 2 * (long) a_sq *(y - 1);
	
	while (y <= 0)
	{
		pstart = x - half;
		for (pix = pstart; pix < pstart + thick; pix++)
		{
			Line( BPoint(cx - pix, cy + y), BPoint(cx + pix, cy + y), ldata, patsolidhigh);
			Line( BPoint(cx - pix, cy - y), BPoint(cx + pix, cy - y), ldata, patsolidhigh);
		}
		
		if (d < 0)
			d += a_sq_4 * y++ + a_sq_6 + b_sq_4 * (++x);
		else
			d += a_sq_4 * y++ + a_sq_6;
	}
}

/*!
	\brief Called for all BView::FillRect calls
	\param r BRect to be filled. Guaranteed to be in the frame buffer's coordinate space
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

*/
void ScreenDriver::FillRect(BRect r, LayerData *d, int8 *pat)
{
	_Lock();
	if(fbuffer->IsConnected())
	{
	//	int32 width=rect.IntegerWidth();
		for(int32 i=(int32)r.top;i<=r.bottom;i++)
	//		HLine(fbuffer->gcinfo,(int32)rect.left,i,width,col);
			Line(BPoint(r.left,i),BPoint(r.right,i),d,pat);
	}
	_Unlock();
}

/*!
	\brief Called for all BView::FillRoundRect calls
	\param r The rectangle itself
	\param xrad X radius of the corner arcs
	\param yrad Y radius of the corner arcs
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the roundrect may end 
	up being clipped.
*/
void ScreenDriver::FillRoundRect(BRect r, float xrad, float yrad, LayerData *d, int8 *pat)
{
	printf("ScreenDriver::FillRoundRect unimplemented\n");
	StrokeRoundRect(r,xrad,yrad,d,pat);
}

/*!
	\brief Called for all BView::FillTriangle calls
	\param pts Array of 3 BPoints. Always non-NULL.
	\param r BRect enclosing the triangle. While it will definitely enclose the triangle,
	it may not be within the frame buffer's bounds.
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the triangle may end 
	up being clipped.
*/
void ScreenDriver::FillTriangle(BPoint *pts, BRect r, LayerData *d, int8 *pat)
{
	if(!pts || !d || !pat)
		return;

	_Lock();
	if(fbuffer->IsConnected())
	{
		BPoint first, second, third;

		// Sort points according to their y values
		if(pts[0].y < pts[1].y)
		{
			first=pts[0];
			second=pts[1];
		}
		else
		{
			first=pts[1];
			second=pts[0];
		}
		
		if(second.y<pts[2].y)
		{
			third=pts[2];
		}
		else
		{
			// second is lower than "third", so we must ensure that this third point
			// isn't higher than our first point
			third=second;
			if(first.y<pts[2].y)
				second=pts[2];
			else
			{
				second=first;
				first=pts[2];
			}
		}
		
		// Now that the points are sorted, check to see if they all have the same
		// y value
		if(first.y==second.y && second.y==third.y)
		{
			BPoint start,end;
			start.y=first.y;
			end.y=first.y;
			start.x=MIN(first.x,MIN(second.x,third.x));
			end.x=MAX(first.x,MAX(second.x,third.x));
			Line(start,end, d, pat);
			_Unlock();
			return;
		}

		int32 i;

		// Special case #1: first and second in the same row
		if(first.y==second.y)
		{
			LineCalc lineA(first, third);
			LineCalc lineB(second, third);
			
			Line(first, second,d,pat);
			for(i=int32(first.y+1);i<third.y;i++)
				Line( BPoint(lineA.GetX(i),i), BPoint(lineB.GetX(i),i),d,pat);
			_Unlock();
			return;
		}
		
		// Special case #2: second and third in the same row
		if(second.y==third.y)
		{
			LineCalc lineA(first, second);
			LineCalc lineB(first, third);
			
			Line(second, third,d,pat);
			for(i=int32(first.y+1);i<third.y;i++)
				Line( BPoint(lineA.GetX(i),i), BPoint(lineB.GetX(i),i),d,pat);
			_Unlock();
			return;
		}
		
		// Normal case.
		// Calculate the y deltas for the two lines and we set the maximum for the
		// first loop to the lesser of the two so that we can change lines.
		int32 dy1=int32(second.y-first.y),
			dy2=int32(third.y-first.y),
			max;
		max=int32(first.y+MIN(dy1,dy2));
		
		LineCalc lineA(first, second);
		LineCalc lineB(first, third);
		LineCalc lineC(second, third);
		
		for(i=int32(first.y+1);i<max;i++)
			Line( BPoint(lineA.GetX(i),i), BPoint(lineB.GetX(i),i),d,pat);

		for(i=max; i<third.y; i++)
			Line( BPoint(lineC.GetX(i),i), BPoint(lineB.GetX(i),i),d,pat);
		
	}
	_Unlock();
}

void ScreenDriver::SetThickPixel(int x, int y, int thick, RGBColor col)
{
	switch(fbuffer->gcinfo.bits_per_pixel)
	{
		case 32:
		case 24:
			SetThickPixel32(x,y,thick,col.GetColor32());
			break;
		case 16:
		case 15:
			SetThickPixel16(x,y,thick,col.GetColor16());
			break;
		case 8:
			SetThickPixel8(x,y,thick,col.GetColor8());
			break;
		default:
			printf("Unknown pixel depth %d in SetThickPixel\n",fbuffer->gcinfo.bits_per_pixel);
			break;
	}
}

void ScreenDriver::SetThickPixel32(int x, int y, int thick, rgb_color col)
{
	// Code courtesy of YNOP's SecondDriver
	union
	{
		uint8 bytes[4];
		uint32 word;
	}c1;
	
	c1.bytes[0]=col.blue; 
	c1.bytes[1]=col.green; 
	c1.bytes[2]=col.red; 
	c1.bytes[3]=col.alpha; 

	/*
	uint32 *bits=(uint32*)fbuffer->gcinfo.frame_buffer;
	*(bits + x + (y*fbuffer->gcinfo.width))=c1.word;
	*/
	uint32 *bits=(uint32*)fbuffer->gcinfo.frame_buffer+(x-thick/2)+(y-thick/2)*fbuffer->gcinfo.width;
	int i,j;
	for (i=0; i<thick; i++)
	{
		for (j=0; j<thick; j++)
		{
			*(bits+j)=c1.word;
		}
		bits += fbuffer->gcinfo.width;
	}
}

void ScreenDriver::SetThickPixel16(int x, int y, int thick, uint16 col)
{
// TODO: Implement
printf("SetThickPixel16 unimplemented\n");

}

void ScreenDriver::SetThickPixel8(int x, int y, int thick, uint8 col)
{
	// When the DisplayDriver API changes, we'll use the uint8 highcolor. Until then,
	// we'll use *pattern
  /*
	uint8 *bits=(uint8*)fbuffer->gcinfo.frame_buffer;
	*(bits + x + (y*fbuffer->gcinfo.bytes_per_row))=col;
	*/

	uint8 *bits=(uint8*)fbuffer->gcinfo.frame_buffer+(x-thick/2)+(y-thick/2)*fbuffer->gcinfo.bytes_per_row;
	int i,j;
	for (i=0; i<thick; i++)
	{
		for (j=0; j<thick; j++)
		{
			*(bits+j)=col;
		}
		bits += fbuffer->gcinfo.bytes_per_row;
	}
}

void ScreenDriver::SetPixel(int x, int y, RGBColor col)
{
	switch(fbuffer->gcinfo.bits_per_pixel)
	{
		case 32:
		case 24:
			SetPixel32(x,y,col.GetColor32());
			break;
		case 16:
		case 15:
			SetPixel16(x,y,col.GetColor16());
			break;
		case 8:
			SetPixel8(x,y,col.GetColor8());
			break;
		default:
			break;
	}
}

void ScreenDriver::SetPixel32(int x, int y, rgb_color col)
{
	// Code courtesy of YNOP's SecondDriver
	union
	{
		uint8 bytes[4];
		uint32 word;
	}c1;
	
	c1.bytes[0]=col.blue; 
	c1.bytes[1]=col.green; 
	c1.bytes[2]=col.red; 
	c1.bytes[3]=col.alpha; 

	uint32 *bits=(uint32*)fbuffer->gcinfo.frame_buffer;
	*(bits + x + (y*fbuffer->gcinfo.width))=c1.word;
}

void ScreenDriver::SetPixel16(int x, int y, uint16 col)
{
// TODO: Implement
printf("SetPixel16 unimplemented\n");

}

void ScreenDriver::SetPixel8(int x, int y, uint8 col)
{
	// When the DisplayDriver API changes, we'll use the uint8 highcolor. Until then,
	// we'll use *pattern
	uint8 *bits=(uint8*)fbuffer->gcinfo.frame_buffer;
	*(bits + x + (y*fbuffer->gcinfo.bytes_per_row))=col;

}

/*!
	\brief Sets the screen mode to specified resolution and color depth.
	\param mode constant as defined in GraphicsDefs.h
	
	Subclasses must include calls to _SetDepth, _SetHeight, _SetWidth, and _SetMode
	to update the state variables kept internally by the DisplayDriver class.
*/
void ScreenDriver::SetMode(int32 space)
{
	_Lock();
	if(fbuffer->IsConnected())
	{
		fbuffer->SetSpace(space);
		
		// We have changed the frame buffer info, so update the cached info
		graphics_card_info *info=fbuffer->CardInfo();
		fbuffer->gcinfo=*info;
		
		// clear the frame buffer. Otherwise, we'll have garbage in it
		LayerData d;
		d.highcolor=workspace_default_color;
		for(int32 i=0; i<info->height; i++)
		{
			Line(BPoint(0,i),BPoint(info->width-1,i),&d,patsolidhigh);
//			HLine(0,info->width-1,i,workspace_default_color);
		}
		_SetMode(space);
		frame_buffer_info fbi=*fbuffer->FrameBufferInfo();
		_SetBytesPerRow(fbi.bytes_per_row);
		_SetWidth(fbi.display_width);
		_SetHeight(fbi.display_height);
		_SetDepth(fbi.bits_per_pixel);
	}
	_Unlock();
}


/*!
	\brief Called for all BView::StrokeArc calls
	\param r Rectangle enclosing the entire arc
	\param angle Starting angle for the arc in degrees
	\param span Span of the arc in degrees. Ending angle = angle+span.
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the arc may end up
	being clipped.
*/
void ScreenDriver::StrokeArc(BRect r, float angle, float span, LayerData *d, int8 *pat)
{
// TODO: Add pattern support
	float xc = (r.left+r.right)/2;
	float yc = (r.top+r.bottom)/2;
	float rx = r.Width()/2;
	float ry = r.Height()/2;
	int Rx2 = ROUND(rx*rx);
	int Ry2 = ROUND(ry*ry);
	int twoRx2 = 2*Rx2;
	int twoRy2 = 2*Ry2;
	int p;
	int x=0;
	int y = (int)ry;
	int px = 0;
	int py = twoRx2 * y;
	int startx, endx;
	int startQuad, endQuad;
	bool useQuad1, useQuad2, useQuad3, useQuad4;
	bool shortspan = false;
	int thick = (int)d->pensize;

	// Watch out for bozos giving us whacko spans
	if ( (span >= 360) || (span <= -360) )
	{
	  StrokeEllipse(r,d,pat);
	  return;
	}

	if ( span > 0 )
	{
		startQuad = (int)(angle/90)%4+1;
		endQuad = (int)((angle+span)/90)%4+1;
		startx = ROUND(.5*r.Width()*fabs(cos(angle*M_PI/180)));
		endx = ROUND(.5*r.Width()*fabs(cos((angle+span)*M_PI/180)));
	}
	else
	{
		endQuad = (int)(angle/90)%4+1;
		startQuad = (int)((angle+span)/90)%4+1;
		endx = ROUND(.5*r.Width()*fabs(cos(angle*M_PI/180)));
		startx = ROUND(.5*r.Width()*fabs(cos((angle+span)*M_PI/180)));
	}

	if ( startQuad != endQuad )
	{
		useQuad1 = (endQuad > 1) && (startQuad > endQuad);
		useQuad2 = ((startQuad == 1) && (endQuad > 2)) || ((startQuad > endQuad) && (endQuad > 2));
		useQuad3 = ((startQuad < 3) && (endQuad == 4)) || ((startQuad < 3) && (endQuad < startQuad));
		useQuad4 = (startQuad < 4) && (startQuad > endQuad);
	}
	else
	{
		if ( (span < 90) && (span > -90) )
		{
			useQuad1 = false;
			useQuad2 = false;
			useQuad3 = false;
			useQuad4 = false;
			shortspan = true;
		}
		else
		{
			useQuad1 = (startQuad != 1);
			useQuad2 = (startQuad != 2);
			useQuad3 = (startQuad != 3);
			useQuad4 = (startQuad != 4);
		}
	}

	if ( useQuad1 || 
	     (!shortspan && (((startQuad == 1) && (x <= startx)) || ((endQuad == 1) && (x >= endx)))) || 
	     (shortspan && (startQuad == 1) && (x <= startx) && (x >= endx)) ) 
		SetThickPixel(xc+x,yc-y,thick,d->highcolor);
	if ( useQuad2 || 
	     (!shortspan && (((startQuad == 2) && (x >= startx)) || ((endQuad == 2) && (x <= endx)))) || 
	     (shortspan && (startQuad == 2) && (x >= startx) && (x <= endx)) ) 
		SetThickPixel(xc-x,yc-y,thick,d->highcolor);
	if ( useQuad3 || 
	     (!shortspan && (((startQuad == 3) && (x <= startx)) || ((endQuad == 3) && (x >= endx)))) || 
	     (shortspan && (startQuad == 3) && (x <= startx) && (x >= endx)) ) 
		SetThickPixel(xc-x,yc+y,thick,d->highcolor);
	if ( useQuad4 || 
	     (!shortspan && (((startQuad == 4) && (x >= startx)) || ((endQuad == 4) && (x <= endx)))) || 
	     (shortspan && (startQuad == 4) && (x >= startx) && (x <= endx)) ) 
		SetThickPixel(xc+x,yc+y,thick,d->highcolor);

	p = ROUND (Ry2 - (Rx2 * ry) + (.25 * Rx2));
	while (px < py)
	{
		x++;
		px += twoRy2;
		if ( p < 0 )
			p += Ry2 + px;
		else
		{
			y--;
			py -= twoRx2;
			p += Ry2 + px - py;
		}

		if ( useQuad1 || 
		     (!shortspan && (((startQuad == 1) && (x <= startx)) || ((endQuad == 1) && (x >= endx)))) || 
		     (shortspan && (startQuad == 1) && (x <= startx) && (x >= endx)) ) 
			SetThickPixel(xc+x,yc-y,thick,d->highcolor);
		if ( useQuad2 || 
		     (!shortspan && (((startQuad == 2) && (x >= startx)) || ((endQuad == 2) && (x <= endx)))) || 
		     (shortspan && (startQuad == 2) && (x >= startx) && (x <= endx)) ) 
			SetThickPixel(xc-x,yc-y,thick,d->highcolor);
		if ( useQuad3 || 
		     (!shortspan && (((startQuad == 3) && (x <= startx)) || ((endQuad == 3) && (x >= endx)))) || 
		     (shortspan && (startQuad == 3) && (x <= startx) && (x >= endx)) ) 
			SetThickPixel(xc-x,yc+y,thick,d->highcolor);
		if ( useQuad4 || 
		     (!shortspan && (((startQuad == 4) && (x >= startx)) || ((endQuad == 4) && (x <= endx)))) || 
		     (shortspan && (startQuad == 4) && (x >= startx) && (x <= endx)) ) 
			SetThickPixel(xc+x,yc+y,thick,d->highcolor);
	}

	p = ROUND(Ry2*(x+.5)*(x+.5) + Rx2*(y-1)*(y-1) - Rx2*Ry2);
	while (y>0)
	{
		y--;
		py -= twoRx2;
		if (p>0)
			p += Rx2 - py;
		else
		{
			x++;
			px += twoRy2;
			p += Rx2 - py +px;
		}

		if ( useQuad1 || 
		     (!shortspan && (((startQuad == 1) && (x <= startx)) || ((endQuad == 1) && (x >= endx)))) || 
		     (shortspan && (startQuad == 1) && (x <= startx) && (x >= endx)) ) 
			SetThickPixel(xc+x,yc-y,thick,d->highcolor);
		if ( useQuad2 || 
		     (!shortspan && (((startQuad == 2) && (x >= startx)) || ((endQuad == 2) && (x <= endx)))) || 
		     (shortspan && (startQuad == 2) && (x >= startx) && (x <= endx)) ) 
			SetThickPixel(xc-x,yc-y,thick,d->highcolor);
		if ( useQuad3 || 
		     (!shortspan && (((startQuad == 3) && (x <= startx)) || ((endQuad == 3) && (x >= endx)))) || 
		     (shortspan && (startQuad == 3) && (x <= startx) && (x >= endx)) ) 
			SetThickPixel(xc-x,yc+y,thick,d->highcolor);
		if ( useQuad4 || 
		     (!shortspan && (((startQuad == 4) && (x >= startx)) || ((endQuad == 4) && (x <= endx)))) || 
		     (shortspan && (startQuad == 4) && (x >= startx) && (x <= endx)) ) 
			SetThickPixel(xc+x,yc+y,thick,d->highcolor);
	}
}

/*!
	\brief Called for all BView::StrokeBezier calls.
	\param pts 4-element array of BPoints in the order of start, end, and then the two control
	points. 
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call.
*/
void ScreenDriver::StrokeBezier(BPoint *pts, LayerData *d, int8 *pat)
{
// TODO: Add pattern support
	double Ax, Bx, Cx, Dx;
	double Ay, By, Cy, Dy;
	int x, y;
	int lastx=-1, lasty=-1;
	double t;
	double dt = .001;
	double dt2, dt3;
	double X, Y, dx, ddx, dddx, dy, ddy, dddy;

	Ax = -pts[0].x + 3*pts[1].x - 3*pts[2].x + pts[3].x;
	Bx = 3*pts[0].x - 6*pts[1].x + 3*pts[2].x;
	Cx = -3*pts[0].x + 3*pts[1].x;
	Dx = pts[0].x;

	Ay = -pts[0].y + 3*pts[1].y - 3*pts[2].y + pts[3].y;
	By = 3*pts[0].y - 6*pts[1].y + 3*pts[2].y;
	Cy = -3*pts[0].y + 3*pts[1].y;
	Dy = pts[0].y;
	
	dt2 = dt * dt;
	dt3 = dt2 * dt;
	X = Dx;
	dx = Ax*dt3 + Bx*dt2 + Cx*dt;
	ddx = 6*Ax*dt3 + 2*Bx*dt2;
	dddx = 6*Ax*dt3;
	Y = Dy;
	dy = Ay*dt3 + By*dt2 + Cy*dt;
	ddy = 6*Ay*dt3 + 2*By*dt2;
	dddy = 6*Ay*dt3;

	lastx = -1;
	lasty = -1;

	for (t=0; t<=1; t+=dt)
	{
		x = ROUND(X);
		y = ROUND(Y);
		if ( (x!=lastx) || (y!=lasty) )
			SetThickPixel(x,y,d->pensize,d->highcolor);
		lastx = x;
		lasty = y;

		X += dx;
		dx += ddx;
		ddx += dddx;
		Y += dy;
		dy += ddy;
		ddy += dddy;
	}
}

/*!
	\brief Called for all BView::StrokeEllipse calls
	\param r BRect enclosing the ellipse to be drawn.
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the ellipse may end up
	being clipped.
*/
void ScreenDriver::StrokeEllipse(BRect r, LayerData *ldata, int8 *pat)
{
// Ellipse code shamelessly stolen from the graphics library gd v2.0.1 and bolted on
// to support our API
	long d, b_sq, b_sq_4, b_sq_6;
	long a_sq, a_sq_4, a_sq_6;
	int x, y, switchem;
	long lsqrt (long);
	int pix, half, pstart;
	int32 thick = (int32)ldata->pensize;
	
	half = thick / 2;
	int32 w=int32(r.Width()/2),
		h=int32(r.Height()/2);
	int32 cx=(int32)r.left+w;
	int32 cy=(int32)r.top+h;
	
	d = 2 * (long) h *h + (long) w *w - 2 * (long) w *w * h;
	b_sq = (long) h *h;
	b_sq_4 = 4 * (long) h *h;
	b_sq_6 = 6 * (long) h *h;
	a_sq = (long) w *w;
	a_sq_4 = 4 * (long) w *w;
	a_sq_6 = 6 * (long) w *w;
	
	x = 0;
	y = -h;
//	switchem = a_sq / lsqrt (a_sq + b_sq);
	switchem = a_sq / (int32)sqrt(a_sq + b_sq);

	while (x <= switchem)
	{
		pstart = y - half;
		for (pix = pstart; pix < pstart + thick; pix++)
		{
			SetPixel(cx + x, cy + pix, ldata->highcolor);
			SetPixel(cx - x, cy + pix, ldata->highcolor);
			SetPixel(cx + x, cy - pix, ldata->highcolor);
			SetPixel(cx - x, cy - pix, ldata->highcolor);
		}
		if (d < 0)
			d += b_sq_4 * x++ + b_sq_6;
		else
			d += b_sq_4 * x++ + b_sq_6 + a_sq_4 * (++y);
	}
	
	/* Middlesplat!
	** Go a little further if the thickness is not nominal...
	*/
	if (thick > 1)
	{
		int xp = x;
		int yp = y;
		int dp = d;
		int thick2 = thick + 2;
		int half2 = half + 1;
		
		while (xp <= switchem + half)
		{
			pstart = yp - half2;
			for (pix = pstart; pix < pstart + thick2; pix++)
			{
				SetPixel(cx + xp, cy + pix, ldata->highcolor);
				SetPixel(cx - xp, cy + pix, ldata->highcolor);
				SetPixel(cx + xp, cy - pix, ldata->highcolor);
				SetPixel(cx - xp, cy - pix, ldata->highcolor);
			}
			if (dp < 0)
				dp += b_sq_4 * xp++ + b_sq_6;
			else
				dp += b_sq_4 * xp++ + b_sq_6 + a_sq_4 * (++yp);
		}
	}
	
	d += -2 * (long) b_sq + 2 * (long) a_sq - 2 * (long) b_sq *(x - 1) + 2 * (long) a_sq *(y - 1);
	
	while (y <= 0)
	{
		pstart = x - half;
		for (pix = pstart; pix < pstart + thick; pix++)
		{
			SetPixel (cx + pix, cy + y, ldata->highcolor);
			SetPixel (cx - pix, cy + y, ldata->highcolor);
			SetPixel (cx + pix, cy - y, ldata->highcolor);
			SetPixel (cx - pix, cy - y, ldata->highcolor);
		}
		
		if (d < 0)
			d += a_sq_4 * y++ + a_sq_6 + b_sq_4 * (++x);
		else
			d += a_sq_4 * y++ + a_sq_6;
	}

}

/*!
	\brief Draws a line. Really.
	\param start Starting point
	\param end Ending point
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.
	
	The endpoints themselves are guaranteed to be in bounds, but clipping for lines with
	a thickness greater than 1 will need to be done.
*/
void ScreenDriver::StrokeLine(BPoint start, BPoint end, LayerData *d, int8 *pat)
{
	_Lock();
	if(fbuffer->IsConnected())
	{
		// Courtesy YNOP's SecondDriver with minor changes by DW
		int oct=0;
		int xoff=(int32)end.x;
		int yoff=(int32)end.y; 
		int32 x2=(int32)start.x-xoff;
		int32 y2=(int32)start.y-yoff; 
		int32 x1=0;
		int32 y1=0;
		if(y2<0){ y2=-y2; oct+=4; }//bit2=1
		if(x2<0){ x2=-x2; oct+=2;}//bit1=1
		if(x2<y2){ int t=x2; x2=y2; y2=t; oct+=1;}//bit0=1
		int x=x1,
			y=y1,
			sum=x2-x1,
			Dx=2*(x2-x1),
			Dy=2*(y2-y1);
		for(int i=0; i <= x2-x1; i++)
		{
			switch(oct)
			{
				case 0:SetPixel(( x)+xoff,( y)+yoff,d->highcolor);break;
				case 1:SetPixel(( y)+xoff,( x)+yoff,d->highcolor);break;
	 			case 3:SetPixel((-y)+xoff,( x)+yoff,d->highcolor);break;
				case 2:SetPixel((-x)+xoff,( y)+yoff,d->highcolor);break;
				case 6:SetPixel((-x)+xoff,(-y)+yoff,d->highcolor);break;
				case 7:SetPixel((-y)+xoff,(-x)+yoff,d->highcolor);break;
				case 5:SetPixel(( y)+xoff,(-x)+yoff,d->highcolor);break;
				case 4:SetPixel(( x)+xoff,(-y)+yoff,d->highcolor);break;
			}
			x++;
			sum-=Dy;
			if(sum < 0)
			{
				y++;
				sum += Dx;
			}
		}
	}
	_Unlock();
}

/*!
	\brief Called for all BView::StrokePolygon calls
	\param ptlist Array of BPoints defining the polygon.
	\param numpts Number of points in the BPoint array.
	\param rect Rectangle which contains the polygon
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	The points in the array are not guaranteed to be within the framebuffer's 
	coordinate range.
*/
void ScreenDriver::StrokePolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, int8 *pat, bool is_closed=true)
{
	_Lock();
	if(fbuffer->IsConnected())
	{
		for(int32 i=0; i<(numpts-1); i++)
			Line(ptlist[i],ptlist[i+1],d,pat);
	
		if(is_closed)
			Line(ptlist[numpts-1],ptlist[0],d,pat);
	}
	_Unlock();
}

/*!
	\brief Called for all BView::StrokeRect calls
	\param r BRect to be filled. Guaranteed to be in the frame buffer's coordinate space
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

*/
void ScreenDriver::StrokeRect(BRect r, LayerData *d, int8 *pat)
{
	_Lock();
	if(fbuffer->IsConnected())
	{
		Line(r.LeftTop(),r.RightTop(),d,pat);
		Line(r.RightTop(),r.RightBottom(),d,pat);
		Line(r.RightBottom(),r.LeftBottom(),d,pat);
		Line(r.LeftTop(),r.LeftBottom(),d,pat);
	}
	_Unlock();
}

/*!
	\brief Called for all BView::StrokeRoundRect calls
	\param r The rect itself
	\param xrad X radius of the corner arcs
	\param yrad Y radius of the corner arcs
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the roundrect may end 
	up being clipped.
*/
void ScreenDriver::StrokeRoundRect(BRect r, float xrad, float yrad, LayerData *d, int8 *pat)
{
	float hLeft, hRight;
	float vTop, vBottom;
	float bLeft, bRight, bTop, bBottom;
	_Lock();
	PatternHandler pattern(pat);
	pattern.SetColors(d->highcolor, d->lowcolor);

	hLeft = r.left + xrad;
	hRight = r.right - xrad;
	vTop = r.top + yrad;
	vBottom = r.bottom - yrad;
	bLeft = hLeft + xrad;
	bRight = hRight -xrad;
	bTop = vTop + yrad;
	bBottom = vBottom - yrad;
	StrokeArc(BRect(bRight, r.top, r.right, bTop), 0, 90, d, pat);
	Line(BPoint(hLeft,r.top), BPoint(hRight, r.top), d, pat);
	
	StrokeArc(BRect(r.left,r.top,bLeft,bTop), 90, 90, d, pat);
	Line(BPoint(r.left,vTop),BPoint(r.left,vBottom),d,pat);

	StrokeArc(BRect(r.left,bBottom,bLeft,r.bottom), 180, 90, d, pat);
	Line(BPoint(hLeft,r.bottom), BPoint(hRight, r.bottom), d, pat);

	StrokeArc(BRect(bRight,bBottom,r.right,r.bottom), 270, 90, d, pat);
	StrokeLine(BPoint(r.right,vBottom),BPoint(r.right,vTop),d,pat);
	_Unlock();
}

/*!
	\brief Called for all BView::StrokeTriangle calls
	\param pts Array of 3 BPoints. Always non-NULL.
	\param r BRect enclosing the triangle. While it will definitely enclose the triangle,
	it may not be within the frame buffer's bounds.
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
	\param pat 8-byte array containing the pattern to use. Always non-NULL.

	Bounds checking must be done in this call because only part of the triangle may end 
	up being clipped.
*/
void ScreenDriver::StrokeTriangle(BPoint *pts, BRect r, LayerData *d, int8 *pat)
{
	_Lock();
	if(fbuffer->IsConnected())
	{
		Line(pts[0],pts[1],d,pat);
		Line(pts[1],pts[2],d,pat);
		Line(pts[2],pts[0],d,pat);
	}
	_Unlock();
}

void ScreenDriver::SetPixelPattern(int x, int y, uint8 *pattern, uint8 patternindex)
{
	// This function is designed to add pattern support to this thing. Should be
	// inlined later to add speed lost in the multiple function calls.
	if(patternindex>32)
		return;

	if(fbuffer->IsConnected())
	{
//		uint64 *p64=(uint64*)pattern;

		// check for transparency in mask. If transparent, we can quit here
		
//		bool transparent_bit=
//			( *p64 & ~((uint64)2 << (32-patternindex)))?true:false;

//		bool highcolor_bit=
//			( *p64 & ~((uint64)2 << (64-patternindex)))?true:false;
			
		switch(fbuffer->gcinfo.bits_per_pixel)
		{	
			case 32:
			case 24:
			{
				
				break;
			}
			case 16:
			case 15:
			{
				break;
			}
			case 8:
			{
				break;
			}
			default:
			{
				break;
			}
		}
	}
}

void ScreenDriver::Line(BPoint start, BPoint end, LayerData *d, int8 *pat)
{
	// Internal function which is called from within other functions
	
	// Courtesy YNOP's SecondDriver with minor changes by DW
	int oct=0;
	int xoff=(int32)end.x;
	int yoff=(int32)end.y; 
	int32 x2=(int32)start.x-xoff;
	int32 y2=(int32)start.y-yoff; 
	int32 x1=0;
	int32 y1=0;
	if(y2<0){ y2=-y2; oct+=4; }//bit2=1
	if(x2<0){ x2=-x2; oct+=2;}//bit1=1
	if(x2<y2){ int t=x2; x2=y2; y2=t; oct+=1;}//bit0=1
	int x=x1,
		y=y1,
		sum=x2-x1,
		Dx=2*(x2-x1),
		Dy=2*(y2-y1);
	for(int i=0; i <= x2-x1; i++)
	{
		switch(oct)
		{
			case 0:SetPixel(( x)+xoff,( y)+yoff,d->highcolor);break;
			case 1:SetPixel(( y)+xoff,( x)+yoff,d->highcolor);break;
 			case 3:SetPixel((-y)+xoff,( x)+yoff,d->highcolor);break;
			case 2:SetPixel((-x)+xoff,( y)+yoff,d->highcolor);break;
			case 6:SetPixel((-x)+xoff,(-y)+yoff,d->highcolor);break;
			case 7:SetPixel((-y)+xoff,(-x)+yoff,d->highcolor);break;
			case 5:SetPixel(( y)+xoff,(-x)+yoff,d->highcolor);break;
			case 4:SetPixel(( x)+xoff,(-y)+yoff,d->highcolor);break;
		}
		x++;
		sum-=Dy;
		if(sum < 0)
		{
			y++;
			sum += Dx;
		}
	}
}

void ScreenDriver::HideCursor(void)
{
	_Lock();
	if(fbuffer->IsConnected())
	{
		if(!IsCursorHidden())
			BlitBitmap(under_cursor,under_cursor->Bounds(),cursorframe, B_OP_COPY);
		DisplayDriver::HideCursor();

	}
	_Unlock();
}

/*!
	\brief Moves the cursor to the given point.

	The coordinates passed to MoveCursorTo are guaranteed to be within the frame buffer's
	range, but the cursor data itself will need to be clipped. A check to see if the 
	cursor is obscured should be made and if so, a call to _SetCursorObscured(false) 
	should be made the cursor in addition to displaying at the passed coordinates.
*/
void ScreenDriver::MoveCursorTo(float x, float y)
{
	if(!under_cursor)
		return;
	_Lock();
	if(!IsCursorHidden())
		BlitBitmap(under_cursor,under_cursor->Bounds(),cursorframe, B_OP_COPY);

	cursorframe.OffsetTo(x,y);
	ExtractToBitmap(under_cursor,under_cursor->Bounds(),cursorframe);
		
	if(!IsCursorHidden())
		BlitBitmap(cursor,cursor->Bounds(),cursorframe, B_OP_OVER);
	
	_Unlock();
}

/*!
	\brief Shows the cursor.
	
	Show calls are not nestable, unlike that of the BApplication class. Subclasses should
	call _SetCursorHidden(false) somewhere within this function to ensure that data is
	maintained accurately. Subclasses must call DisplayDriver::ShowCursor at some point
	to ensure proper state tracking.
*/
void ScreenDriver::ShowCursor(void)
{
	_Lock();
	if(fbuffer->IsConnected())
	{
		if(IsCursorHidden())
			BlitBitmap(cursor,cursor->Bounds(),cursorframe, B_OP_OVER);
		DisplayDriver::ShowCursor();
	}
	_Unlock();
}

/*!
	\brief Obscures the cursor.
	
	Obscure calls are not nestable. Subclasses should call DisplayDriver::ObscureCursor
	somewhere within this function to ensure that data is maintained accurately. A check
	will be made by the system before the next MoveCursorTo call to show the cursor if
	it is obscured.
*/
void ScreenDriver::ObscureCursor(void)
{
	_Lock();
	if(!IsCursorHidden() && fbuffer->IsConnected())
		BlitBitmap(under_cursor,under_cursor->Bounds(),cursorframe, B_OP_COPY);
	DisplayDriver::ObscureCursor();
	_Unlock();
}

/*!
	\brief Changes the cursor.
	\param cursor The new cursor. Guaranteed to be non-NULL.
	
	The driver does not take ownership of the given cursor. Subclasses should make
	a copy of the cursor passed to it. The default version of this function hides the
	cursory, replaces it, and shows the cursor if previously visible.
*/
void ScreenDriver::SetCursor(ServerCursor *csr)
{
	if(!csr)
		return;
		
	_Lock();

	// erase old if visible
	if(!IsCursorHidden() && under_cursor)
		BlitBitmap(under_cursor,under_cursor->Bounds(),cursorframe, B_OP_COPY);

	if(cursor)
		delete cursor;
	if(under_cursor)
		delete under_cursor;
	
	cursor=new ServerCursor(csr);
	under_cursor=new ServerCursor(csr);
	
	cursorframe.right=cursorframe.left+csr->Bounds().Width();
	cursorframe.bottom=cursorframe.top+csr->Bounds().Height();
	
	ExtractToBitmap(under_cursor,under_cursor->Bounds(),cursorframe);
	
	if(!IsCursorHidden())
		BlitBitmap(cursor,cursor->Bounds(),cursorframe, B_OP_OVER);
	
	_Unlock();
}

void ScreenDriver::HLine(int32 x1, int32 x2, int32 y, RGBColor color)
{
// TODO: make this work with HW Acceleration, if possible
#ifndef DISABLE_HARDWARE_ACCELERATION
	// Internal function called from others in the driver
	if(fbuffer->_fspan)
	{
		uint16 ptarray[3];
		ptarray[0]=(uint16)x1;
		ptarray[1]=(uint16)x2;
		ptarray[2]=(uint16)y;
		rgb_color col=color.GetColor32();
		fbuffer->_ae(B_2D_ACCELERATION,100000,&fbuffer->_stoken,&fbuffer->_et);
		fbuffer->_fspan(fbuffer->_et,*((uint32*)&col),ptarray,1);
		fbuffer->_re(fbuffer->_et,&fbuffer->_stoken);
		return;
	}
#endif
	// TODO: Implement and substitute Line() calls with HLine calls as appropriate
	// elsewhere in the driver
	
	switch(fbuffer->gcinfo.bits_per_pixel)
	{
		case 32:
		case 24:
			break;
		case 16:
		case 15:
			break;
		default:
			break;
	}
}

void HLine_32Bit(graphics_card_info i, uint16 x, uint16 y, uint16 length, rgb_color col)
{
// TODO: Finish
printf("HLine32() Unfinished\n");
	uint32 c=0,*pcolor,bytes;
	
	// apparently width=bytes per row
	uint32 bpr=i.width;
	
	// ARGB order
	c|=col.alpha * 0x01000000;
	c|=col.red * 0x010000;
	c|=col.green * 0x0100;
	c|=col.blue;

	bytes=(uint32(x+length)>bpr)?length-((x+length)-bpr):length;

	pcolor=(uint32*)i.frame_buffer;
	pcolor+=(y*bpr)+(x*4);
	for(int32 i=0;i<length;i++)
		pcolor[i]=c;
}

void HLine_16Bit(graphics_card_info i, uint16 x, uint16 y, uint16 length, uint16 col)
{
// TODO: Implement
printf("HLine16() Unimplemented\n");
}

void HLine_8Bit(graphics_card_info i, uint16 x, uint16 y, uint16 length, uint8 col)
{
	size_t bytes;
	uint32 bpr=i.width;
	if( (bpr%2)!=0 )
		bpr++;

	uint8 *pcolor=(uint8*)i.frame_buffer;
	
	pcolor+=(y*bpr)+x;

	bytes=(uint32(x+length)>bpr)?length-((x+length)-bpr):length;

	memset(pcolor,col,bytes);
}

// This function is intended to eventually take care of most of the heavy lifting for
// DrawBitmap in 32-bit mode, with others coming later. Right now, it is *just* used for
// the 
void ScreenDriver::BlitBitmap(ServerBitmap *sourcebmp,BRect sourcerect, BRect destrect, drawing_mode mode=B_OP_COPY)
{
	// Another internal function called from other functions.
	
	if(!sourcebmp)
		return;

	if(sourcebmp->BitsPerPixel() != fbuffer->gcinfo.bits_per_pixel)
		return;

	uint8 colorspace_size=sourcebmp->BitsPerPixel()/8;
	// First, clip source rect to destination
	if(sourcerect.Width() > destrect.Width())
		sourcerect.right=sourcerect.left+destrect.Width();
	

	if(sourcerect.Height() > destrect.Height())
		sourcerect.bottom=sourcerect.top+destrect.Height();
	

	// Second, check rectangle bounds against their own bitmaps
	BRect work_rect;

	work_rect=sourcebmp->Bounds();
	
	if( !(work_rect.Contains(sourcerect)) )
	{	// something in selection must be clipped
		if(sourcerect.left < 0)
			sourcerect.left = 0;
		if(sourcerect.right > work_rect.right)
			sourcerect.right = work_rect.right;
		if(sourcerect.top < 0)
			sourcerect.top = 0;
		if(sourcerect.bottom > work_rect.bottom)
			sourcerect.bottom = work_rect.bottom;
	}

	work_rect.Set(0,0,fbuffer->gcinfo.width-1,fbuffer->gcinfo.height-1);

	// Check to see if we actually need to copy anything
	if( (destrect.right<work_rect.left) || (destrect.left>work_rect.right) ||
			(destrect.bottom<work_rect.top) || (destrect.top>work_rect.bottom) )
		return;

	// something in selection must be clipped
	if(destrect.left < 0)
		destrect.left = 0;
	if(destrect.right > work_rect.right)
		destrect.right = work_rect.right;
	if(destrect.top < 0)
		destrect.top = 0;
	if(destrect.bottom > work_rect.bottom)
		destrect.bottom = work_rect.bottom;

	// Set pointers to the actual data
	uint8 *src_bits  = (uint8*) sourcebmp->Bits();	
	uint8 *dest_bits = (uint8*) fbuffer->gcinfo.frame_buffer;

	// Get row widths for offset looping
	uint32 src_width  = uint32 (sourcebmp->BytesPerRow());
	uint32 dest_width = uint32 (fbuffer->gcinfo.bytes_per_row);

	// Offset bitmap pointers to proper spot in each bitmap
	src_bits += uint32 ( (sourcerect.top  * src_width)  + (sourcerect.left  * colorspace_size) );
	dest_bits += uint32 ( (destrect.top * dest_width) + (destrect.left * colorspace_size) );

	uint32 line_length = uint32 ((destrect.right - destrect.left+1)*colorspace_size);
	uint32 lines = uint32 (destrect.bottom-destrect.top+1);

	switch(mode)
	{
		case B_OP_OVER:
		{
//			uint32 srow_pixels=src_width>>2;
			uint32 srow_pixels=((destrect.IntegerWidth()>=sourcerect.IntegerWidth())?src_width:destrect.IntegerWidth()+1)>>2;
			uint8 *srow_index, *drow_index;
			
			
			// This could later be optimized to use uint32's for faster copying
			for (uint32 pos_y=0; pos_y!=lines; pos_y++)
			{
				
				srow_index=src_bits;
				drow_index=dest_bits;
				
				for(uint32 pos_x=0; pos_x!=srow_pixels;pos_x++)
				{
					// 32-bit RGBA32 mode byte order is BGRA
					if(srow_index[3]>127)
					{
						*drow_index=*srow_index; drow_index++; srow_index++;
						*drow_index=*srow_index; drow_index++; srow_index++;
						*drow_index=*srow_index; drow_index++; srow_index++;
						// we don't copy the alpha channel
						drow_index++; srow_index++;
					}
					else
					{
						srow_index+=4;
						drow_index+=4;
					}
				}
		
				// Increment offsets
		   		src_bits += src_width;
		   		dest_bits += dest_width;
			}
			break;
		}
		default:	// B_OP_COPY
		{
			for (uint32 pos_y = 0; pos_y != lines; pos_y++)
			{
				memcpy(dest_bits,src_bits,line_length);
		
				// Increment offsets
		   		src_bits += src_width;
		   		dest_bits += dest_width;
			}
			break;
		}
	}
}

void ScreenDriver::ExtractToBitmap(ServerBitmap *destbmp,BRect destrect, BRect sourcerect)
{
	// Another internal function called from other functions. Extracts data from
	// the framebuffer to a target ServerBitmap

	if(!destbmp)
		return;

	if(destbmp->BitsPerPixel() != fbuffer->gcinfo.bits_per_pixel)
		return;

	uint8 colorspace_size=destbmp->BitsPerPixel()/8;
	// First, clip source rect to destination
	if(sourcerect.Width() > destrect.Width())
		sourcerect.right=sourcerect.left+destrect.Width();
	

	if(sourcerect.Height() > destrect.Height())
		sourcerect.bottom=sourcerect.top+destrect.Height();
	

	// Second, check rectangle bounds against their own bitmaps
	BRect work_rect;

	work_rect.Set(	destbmp->Bounds().left,
					destbmp->Bounds().top,
					destbmp->Bounds().right,
					destbmp->Bounds().bottom	);
	if( !(work_rect.Contains(destrect)) )
	{	// something in selection must be clipped
		if(destrect.left < 0)
			destrect.left = 0;
		if(destrect.right > work_rect.right)
			destrect.right = work_rect.right;
		if(destrect.top < 0)
			destrect.top = 0;
		if(destrect.bottom > work_rect.bottom)
			destrect.bottom = work_rect.bottom;
	}

	work_rect.Set(	0,0,fbuffer->gcinfo.width-1,fbuffer->gcinfo.height-1);

	if( !(work_rect.Contains(sourcerect)) )
	{	// something in selection must be clipped
		if(sourcerect.left < 0)
			sourcerect.left = 0;
		if(sourcerect.right > work_rect.right)
			sourcerect.right = work_rect.right;
		if(sourcerect.top < 0)
			sourcerect.top = 0;
		if(sourcerect.bottom > work_rect.bottom)
			sourcerect.bottom = work_rect.bottom;
	}

	// Set pointers to the actual data
	uint8 *dest_bits  = (uint8*) destbmp->Bits();	
	uint8 *src_bits = (uint8*) fbuffer->gcinfo.frame_buffer;

	// Get row widths for offset looping
	uint32 dest_width  = uint32 (destbmp->BytesPerRow());
	uint32 src_width = uint32 (fbuffer->gcinfo.bytes_per_row);

	// Offset bitmap pointers to proper spot in each bitmap
	src_bits += uint32 ( (sourcerect.top  * src_width)  + (sourcerect.left  * colorspace_size) );
	dest_bits += uint32 ( (destrect.top * dest_width) + (destrect.left * colorspace_size) );

	uint32 line_length = uint32 ((destrect.right - destrect.left+1)*colorspace_size);
	uint32 lines = uint32 (destrect.bottom-destrect.top+1);

	for (uint32 pos_y = 0; pos_y != lines; pos_y++)
	{ 
		memcpy(dest_bits,src_bits,line_length);

		// Increment offsets
   		src_bits += src_width;
   		dest_bits += dest_width;
	}
}

void ScreenDriver::InvertRect(BRect r)
{
	_Lock();
	if(fbuffer->IsConnected())
	{
		if(r.top<0 || r.left<0 || 
			r.right>fbuffer->gcinfo.width-1 || r.bottom>fbuffer->gcinfo.height-1)
		{
			_Unlock();
			return;
		}
		
		switch(fbuffer->gcinfo.bits_per_pixel)
		{
			case 32:
			case 24:
			{
				uint16 width=r.IntegerWidth(), height=r.IntegerHeight();
				uint32 *start=(uint32*)fbuffer->gcinfo.frame_buffer, *index;
				start+=int32(r.top)*fbuffer->gcinfo.width;
				start+=int32(r.left);
				
				for(int32 i=0;i<height;i++)
				{
					index=start + (i*fbuffer->gcinfo.width);
					for(int32 j=0; j<width; j++)
						index[j]^=0xFFFFFF00L;
				}
				break;
			}
			case 16:
				// TODO: Implement
				printf("ScreenDriver::16-bit mode unimplemented\n");
				break;
			case 15:
				// TODO: Implement
				printf("ScreenDriver::15-bit mode unimplemented\n");
				break;
			case 8:
				// TODO: Implement
				printf("ScreenDriver::8-bit mode unimplemented\n");
				break;
			default:
				break;
		}

	}
	_Unlock();
}


float ScreenDriver::StringWidth(const char *string, int32 length, LayerData *d)
{
	if(!string || !d || !d->font)
		return 0.0;
	_Lock();

	ServerFont *font=d->font;
	FontStyle *style=font->Style();

	if(!style)
	{
		_Unlock();
		return 0.0;
	}

	FT_Face face;
	FT_GlyphSlot slot;
	FT_UInt glyph_index, previous=0;
	FT_Vector pen,delta;
	int16 error=0;
	int32 strlength,i;
	float returnval;

	error=FT_New_Face(ftlib, style->GetPath(), 0, &face);
	if(error)
	{
		_Unlock();
		return 0.0;
	}

	slot=face->glyph;

	bool use_kerning=FT_HAS_KERNING(face) && font->Spacing()==B_STRING_SPACING;
	
	error=FT_Set_Char_Size(face, 0,int32(font->Size())*64,72,72);
	if(error)
	{
		_Unlock();
		return 0.0;
	}

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

	FT_Done_Face(face);

	returnval=pen.x>>6;
	_Unlock();
	return returnval;
}

float ScreenDriver::StringHeight(const char *string, int32 length, LayerData *d)
{
	if(!string || !d || !d->font)
		return 0.0;
	_Lock();

	ServerFont *font=d->font;
	FontStyle *style=font->Style();

	if(!style)
	{
		_Unlock();
		return 0.0;
	}

	FT_Face face;
	FT_GlyphSlot slot;
	int16 error=0;
	int32 strlength,i;
	float returnval=0.0,ascent=0.0,descent=0.0;

	error=FT_New_Face(ftlib, style->GetPath(), 0, &face);
	if(error)
	{
		_Unlock();
		return 0.0;
	}

	slot=face->glyph;
	
	error=FT_Set_Char_Size(face, 0,int32(font->Size())*64,72,72);
	if(error)
	{
		_Unlock();
		return 0.0;
	}

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
	_Unlock();

	FT_Done_Face(face);

	returnval=ascent+descent;
	_Unlock();
	return returnval;
}

/*!
	\brief Utilizes the font engine to draw a string to the frame buffer
	\param string String to be drawn. Always non-NULL.
	\param length Number of characters in the string to draw. Always greater than 0. If greater
	than the number of characters in the string, draw the entire string.
	\param pt Point at which the baseline starts. Characters are to be drawn 1 pixel above
	this for backwards compatibility. While the point itself is guaranteed to be inside
	the frame buffers coordinate range, the clipping of each individual glyph must be
	performed by the driver itself.
	\param d Data structure containing any other data necessary for the call. Always non-NULL.
*/
void ScreenDriver::DrawString(const char *string, int32 length, BPoint pt, LayerData *d, escapement_delta *edelta=NULL)
{
	if(!string || !d || !d->font)
		return;

	_Lock();

	pt.y--;	// because of Be's backward compatibility hack

	ServerFont *font=d->font;
	FontStyle *style=font->Style();

	if(!style)
	{
		_Unlock();
		return;
	}

	FT_Face face;
	FT_GlyphSlot slot;
	FT_Matrix rmatrix,smatrix;
	FT_UInt glyph_index, previous=0;
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
	{
		printf("Couldn't create face object\n");
		_Unlock();
		return;
	}

	slot=face->glyph;

	bool use_kerning=FT_HAS_KERNING(face) && font->Spacing()==B_STRING_SPACING;
	
	error=FT_Set_Char_Size(face, 0,int32(font->Size())*64,72,72);
	if(error)
	{
		_Unlock();
		return;
	}

	// if we do any transformation, we do a call to FT_Set_Transform() here
	
	// First, rotate
	rmatrix.xx = (FT_Fixed)( rotation.Cosine()*0x10000); 
	rmatrix.xy = (FT_Fixed)( rotation.Sine()*0x10000); 
	rmatrix.yx = (FT_Fixed)(-rotation.Sine()*0x10000); 
	rmatrix.yy = (FT_Fixed)( rotation.Cosine()*0x10000); 
	
	// Next, shear
	smatrix.xx = (FT_Fixed)(0x10000); 
	smatrix.xy = (FT_Fixed)(-shear.Cosine()*0x10000); 
	smatrix.yx = (FT_Fixed)(0); 
	smatrix.yy = (FT_Fixed)(0x10000); 

	//FT_Matrix_Multiply(&rmatrix,&smatrix);
	FT_Matrix_Multiply(&smatrix,&rmatrix);
	
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
		//FT_Set_Transform(face,&smatrix,&pen);
		FT_Set_Transform(face,&rmatrix,&pen);

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
		else
			printf("Couldn't load character %c\n", string[i]);

		// increment pen position
		pen.x+=slot->advance.x;
		pen.y+=slot->advance.y;
		previous=glyph_index;
	}
	FT_Done_Face(face);
	_Unlock();
}

void ScreenDriver::BlitMono2RGB32(FT_Bitmap *src, BPoint pt, LayerData *d)
{
	rgb_color color=d->highcolor.GetColor32();
	
	// pointers to the top left corner of the area to be copied in each bitmap
	uint8 *srcbuffer, *destbuffer;
	
	// index pointers which are incremented during the course of the blit
	uint8 *srcindex, *destindex, *rowptr, value;
	
	// increment values for the index pointers
	int32 srcinc=src->pitch, destinc=fbuffer->gcinfo.bytes_per_row;
	
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

	if(y+srcheight>fbuffer->gcinfo.height)
	{
		if(y>pt.y)
			y--;
		srcheight-=(y+srcheight-1)-fbuffer->gcinfo.height;
	}

	if(x+srcwidth>fbuffer->gcinfo.width)
	{
		if(x>pt.x)
			x--;
		srcwidth-=(x+srcwidth-1)-fbuffer->gcinfo.width;
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
	destbuffer=(uint8*)fbuffer->gcinfo.frame_buffer+int32( (pt.y*fbuffer->gcinfo.bytes_per_row)+(pt.x*4) );

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
		destinc=fbuffer->gcinfo.bytes_per_row,
		srcwidth=src->width,
		srcheight=src->rows,
		incval=0;
	
	int16 i,j;
	
	// starting point in source bitmap
	srcbuffer=(uint8*)src->buffer;

	// starting point in destination bitmap
//	destbuffer=(uint8*)dest->Bits()+(y*dest->BytesPerRow()+(x*4));
	destbuffer=(uint8*)fbuffer->gcinfo.frame_buffer+(y*fbuffer->gcinfo.bytes_per_row+(x*4));


	if(y<0)
	{
		if(y<pt.y)
			y++;
		
		incval=0-y;
		
		srcbuffer+=incval * srcinc;
		srcheight-=incval;
		destbuffer+=incval * destinc;
	}

	if(y+srcheight>fbuffer->gcinfo.height)
	{
		if(y>pt.y)
			y--;
		srcheight-=(y+srcheight-1)-fbuffer->gcinfo.height;
	}

	if(x+srcwidth>fbuffer->gcinfo.width)
	{
		if(x>pt.x)
			x--;
		srcwidth-=(x+srcwidth-1)-fbuffer->gcinfo.width;
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

rgb_color ScreenDriver::GetBlitColor(rgb_color src, rgb_color dest, LayerData *d, bool use_high=true)
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
	_Unlock();
	return returncolor;
}
