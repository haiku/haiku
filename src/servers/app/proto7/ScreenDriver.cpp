/*
	ScreenDriver.cpp
		Replacement class for the ViewDriver which utilizes a BWindowScreen for ease
		of testing without requiring a second video card for SecondDriver and also
		without the limitations (mostly speed) of ViewDriver.
		
		Note that unlike ViewDriver, this graphics module does NOT emulate
		the Input Server. The Router input server filter is required for use.
		
		The concept is pretty close to the retooled ViewDriver, where each module
		call locks a couple BLockers and draws to the buffer.
		
		Components:
			ScreenDriver: actual driver module
			FrameBuffer: BWindowScreen derivative which provides the module access
				to the video card
			Internal functions for doing graphics on the buffer
*/
#include "ScreenDriver.h"
#include "ServerProtocol.h"
#include "ServerBitmap.h"
#include "SystemPalette.h"
#include "ColorUtils.h"
#include "PortLink.h"
#include "RGBColor.h"
#include "DebugTools.h"
#include "LayerData.h"
#include "LineCalc.h"
#include <View.h>
#include <stdio.h>
#include <string.h>
#include <String.h>

//#define DEBUG_DRIVER
#define DEBUG_SERVER_EMU

//#define DISABLE_SERVER_EMU

// Define this if you want to use the spacebar to launch the server prototype's
// test application
//#define LAUNCH_TESTAPP

#ifdef LAUNCH_TESTAPP
#include <Roster.h>
#include <Entry.h>
#endif

void HLine_32Bit(graphics_card_info i, uint16 x, uint16 y, uint16 length, rgb_color col);
void HLine_16Bit(graphics_card_info i, uint16 x, uint16 y, uint16 length, uint16 col);
void HLine_16Bit(graphics_card_info i, uint16 x, uint16 y, uint16 length, uint8 col);

FrameBuffer::FrameBuffer(const char *title, uint32 space, status_t *st,bool debug)
	: BWindowScreen(title,space,st,debug)
{
	is_connected=false;
	port_id serverport=find_port(SERVER_INPUT_PORT);
	serverlink=new PortLink(serverport);
	mousepos.Set(0,0);
#ifdef DEBUG_SERVER_EMU
printf("ScreenDriver:: app_server input port: %ld\n",serverlink->GetPort());
#endif
}

FrameBuffer::~FrameBuffer(void)
{
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

			switch(key)
			{
				case 0x47:	// Enter key
				{
					port_id serverport=find_port(SERVER_PORT_NAME);
					write_port(serverport,B_QUIT_REQUESTED,NULL,0);
					break;
				}
#ifdef LAUNCH_TESTAPP
				// use spacebar to launch test app
				case 0x5e:
				{
					printf("Launching test app\n");
					status_t value;
					BEntry entry("/boot/home/Desktop/openbeos/sources/proto6/OBApplication/OBApplication");
					entry_ref ref;
					entry.GetRef(&ref);
					value=be_roster->Launch(&ref);
	
					BString str=TranslateStatusToString(value);
					printf("Launch return code: %s",str.String());
					printf("\n");
					break;
				}
#endif

#ifndef DISABLE_SERVER_EMU
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
					serverlink->Attach(&modifiers, sizeof(uint32));
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
					serverlink->Attach(&modifiers, sizeof(uint32));
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
					serverlink->Attach(&modifiers, sizeof(uint32));
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
					serverlink->Attach(&modifiers, sizeof(uint32));
					serverlink->Attach(&buttons, sizeof(uint32));
					serverlink->Attach(&clicks, sizeof(uint32));
					serverlink->Flush();
					break;
				}
#endif	// end server emu code
				default:
					break;
			}

		}
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

ScreenDriver::ScreenDriver(void)
{
	status_t st;
	fbuffer=new FrameBuffer("OBAppServer",B_8_BIT_640x480,&st,true);

	drawmode=DRAW_COPY;	

	// We start without a cursor
	cursor=NULL;
	under_cursor=NULL;

	cursorframe.Set(0,0,0,0);
	oldcursorframe.Set(0,0,0,0);
}

ScreenDriver::~ScreenDriver(void)
{
	if(cursor)
		delete cursor;
	if(under_cursor)
		delete under_cursor;
}

bool ScreenDriver::Initialize(void)
{
	fbuffer->Show();

	// draw on the module's default cursor here to make it look preety
	
	// wait 1 sec for the window to show...
	snooze(500000);
	return true;
}

void ScreenDriver::Shutdown(void)
{
	fbuffer->Lock();
	fbuffer->Quit();
}

void ScreenDriver::DrawBitmap(ServerBitmap *bitmap, BRect source, BRect dest)
{
	Lock();
#ifdef DEBUG_DRIVER
printf("ScreenDriver::DrawBitmap(*, (%f,%f,%f,%f),(%f,%f,%f,%f) )\n",
	source.left,source.top,source.right,source.bottom,
	dest.left,dest.top,dest.right,dest.bottom);
#endif
	if(fbuffer->IsConnected())
	{
		// Scale bitmap here if source!=dest
	
		switch(fbuffer->gcinfo.bits_per_pixel)
		{	
			case 32:
			case 24:
			{
				printf("DrawBitmap: 32/24-bit unimplemented\n");
				break;
			}
			case 16:
			case 15:
			{
				printf("DrawBitmap: 16/15-bit unimplemented\n");
				break;
			}
			case 8:
			{
				printf("DrawBitmap: 8-bit unimplemented\n");
				break;
			}
			default:
			{
#ifdef DEBUG_DRIVER
printf("Clear: unknown bit depth %u\n",fbuffer->gcinfo.bits_per_pixel);
#endif
				break;
			}
		}
	}

	Unlock();
}

// Ellipse code shamelessly stolen from the graphics library gd v2.0.1 and bolted on
// to support our API
void ScreenDriver::FillEllipse(BRect r, LayerData *ldata, int8 *pat)
{
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
			Line( BPoint(cx - x, cy + pix), BPoint(cx + x, cy + pix), ldata, solidhigh);
			Line( BPoint(cx - x, cy - pix), BPoint(cx + x, cy - pix), ldata, solidhigh);
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
				Line( BPoint(cx - xp, cy + pix), BPoint(cx + xp, cy + pix), ldata, solidhigh);
				Line( BPoint(cx - xp, cy - pix), BPoint(cx + xp, cy - pix), ldata, solidhigh);
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
			Line( BPoint(cx - pix, cy + y), BPoint(cx + pix, cy + y), ldata, solidhigh);
			Line( BPoint(cx - pix, cy - y), BPoint(cx + pix, cy - y), ldata, solidhigh);
		}
		
		if (d < 0)
			d += a_sq_4 * y++ + a_sq_6 + b_sq_4 * (++x);
		else
			d += a_sq_4 * y++ + a_sq_6;
	}
}

void ScreenDriver::FillRect(BRect r, LayerData *d, int8 *pat)
{
#ifdef DEBUG_DRIVER
printf("ScreenDriver::FillRect( (%f,%f,%f,%f)\n",r.left,r.top,r.right,r.bottom);
#endif
	Lock();
	if(fbuffer->IsConnected())
	{
	//	int32 width=rect.IntegerWidth();
		for(int32 i=(int32)r.top;i<=r.bottom;i++)
	//		HLine(fbuffer->gcinfo,(int32)rect.left,i,width,col);
			Line(BPoint(r.left,i),BPoint(r.right,i),d,pat);
	}
	Unlock();
}

void ScreenDriver::FillRoundRect(BRect r, float xrad, float yrad, LayerData *d, int8 *pat)
{
#ifdef DEBUG_DRIVER
printf("ScreenDriver::FillRoundRect( (%f,%f,%f,%f), %llx) ---->Unimplemented<----\n",r.left,r.top,
	r.right,r.bottom,*((uint64*)pat));
#endif
	FillRect(r,d,pat);
}

void ScreenDriver::FillTriangle(BPoint *pts, BRect r, LayerData *d, int8 *pat)
{
#ifdef DEBUG_DRIVER
printf("ScreenDriver::FillTriangle(%llx): --->Currently Unimplemented<---\n",*((uint64*)pat));
printf("\tFirst: (%f,%f)\n\tSecond: (%f,%f)\n\tThird: (%f,%f)\n",pts[0].x,pts[0].y,
	pts[1].x,pts[1].y,pts[2].x,pts[2].y);
r.PrintToStream();
#endif
	if(!pts || !d || !pat)
		return;

	Lock();
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
			Unlock();
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
			Unlock();
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
			Unlock();
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
	Unlock();
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
			printf("Unknown pixel depth %d in SetPixel\n",fbuffer->gcinfo.bits_per_pixel);
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

#ifdef DEBUG_DRIVER
printf("SetPixel16 unimplemented\n");
#endif

}

void ScreenDriver::SetPixel8(int x, int y, uint8 col)
{
	// When the DisplayDriver API changes, we'll use the uint8 highcolor. Until then,
	// we'll use *pattern
	uint8 *bits=(uint8*)fbuffer->gcinfo.frame_buffer;
	*(bits + x + (y*fbuffer->gcinfo.bytes_per_row))=col;

}

void ScreenDriver::SetMode(int32 space)
{
#ifdef DEBUG_DRIVER
printf("ScreenDriver::SetScreen(%lu}\n",space);
#endif
	Lock();
	if(fbuffer->IsConnected())
	{
		fbuffer->SetSpace(space);
		
		// We have changed the frame buffer info, so update the cached info
		graphics_card_info *info=fbuffer->CardInfo();
		fbuffer->gcinfo=*info;
		
		// clear the frame buffer. Otherwise, we'll have garbage in it
		LayerData d;
		d.highcolor.SetColor(80,85,152);
		for(int32 i=0; i<info->height; i++)
		{
			Line(BPoint(0,i),BPoint(info->width-1,i),&d,solidhigh);
		}
	}
	Unlock();
}

// Ellipse code shamelessly stolen from the graphics library gd v2.0.1 and bolted on
// to support our API
void ScreenDriver::StrokeEllipse(BRect r, LayerData *ldata, int8 *pat)
{
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

void ScreenDriver::StrokeLine(BPoint start, BPoint end, LayerData *d, int8 *pat)
{
#ifdef DEBUG_DRIVER
printf("ScreenDriver::StrokeLine( (%f,%f), %llx}\n",start.x,start.y,*((uint64*)pat));
#endif
	Lock();
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
	Unlock();
}

void ScreenDriver::StrokePolygon(BPoint *ptlist, int32 numpts, BRect rect, LayerData *d, int8 *pat, bool is_closed=true)
{
#ifdef DEBUG_DRIVER
printf("ScreenDriver::StrokePolygon( is_closed=%s)\n",(is_closed==true)?"true":"false");
for(int debug=0;debug<numpts; debug++)
	printf("Point %d: (%f,%f)\n",debug,ptlist[debug].x,ptlist[debug].y);
#endif
	Lock();
	if(fbuffer->IsConnected())
	{
		for(int32 i=0; i<(numpts-1); i++)
			Line(ptlist[i],ptlist[i+1],d,pat);
	
		if(is_closed)
			Line(ptlist[numpts-1],ptlist[0],d,pat);
	}
	Unlock();
}

void ScreenDriver::StrokeRect(BRect r, LayerData *d, int8 *pat)
{
#ifdef DEBUG_DRIVER
printf("ScreenDriver::StrokeRect( (%f,%f,%f,%f), %llx)\n",r.left,r.top,
	r.right,r.bottom,*((uint64*)pat));
#endif
	Lock();
	if(fbuffer->IsConnected())
	{
		Line(r.LeftTop(),r.RightTop(),d,pat);
		Line(r.RightTop(),r.RightBottom(),d,pat);
		Line(r.RightBottom(),r.LeftBottom(),d,pat);
		Line(r.LeftTop(),r.LeftBottom(),d,pat);
	}
	Unlock();
}

void ScreenDriver::StrokeRoundRect(BRect r, float xrad, float yrad, LayerData *d, int8 *pat)
{
#ifdef DEBUG_DRIVER
printf("ScreenDriver::StrokeRoundRect( (%f,%f,%f,%f), %llx) ---->Unimplemented<----\n",r.left,r.top,
	r.right,r.bottom,*((uint64*)pat));
#endif
	StrokeRect(r,d,pat);
}

void ScreenDriver::StrokeTriangle(BPoint *pts, BRect r, LayerData *d, int8 *pat)
{
#ifdef DEBUG_DRIVER
printf("ScreenDriver::StrokeTriangle(%llx): --->Currently Unimplemented<---\n",*((uint64*)pat));
printf("\tFirst: (%f,%f)\n\tSecond: (%f,%f)\n\tThird: (%f,%f)\n",pts[0].x,pts[0].y,
	pts[1].x,pts[1].y,pts[2].x,pts[2].y);
r.PrintToStream();
#endif
	Lock();
	if(fbuffer->IsConnected())
	{
		Line(pts[0],pts[1],d,pat);
		Line(pts[1],pts[2],d,pat);
		Line(pts[2],pts[0],d,pat);
	}
	Unlock();
}

void ScreenDriver::SetPixelPattern(int x, int y, uint8 *pattern, uint8 patternindex)
{
	// This function is designed to add pattern support to this thing. Should be
	// inlined later to add speed lost in the multiple function calls.
#ifdef DEBUG_DRIVER
printf("ScreenDriver::SetPixelPattern(%u,%u,%llx,%u)\n",x,y,*((int64*)pattern),patternindex);
#endif
	if(patternindex>32)
		return;

	if(fbuffer->IsConnected())
	{
		uint64 *p64=(uint64*)pattern;

		// check for transparency in mask. If transparent, we can quit here
		
		bool transparent_bit=
			( *p64 & ~((uint64)2 << (32-patternindex)))?true:false;
printf("SetPixelPattern: pattern=0x%llx\n",*p64);
printf("SetPixelPattern: bit=0x%llx\n",((uint64)2 << (32-patternindex)));
printf("SetPixelPattern: bit mask=0x%llx\n",~((uint64)2 << (32-patternindex)));
printf("SetPixelPattern: bit value=%s\n",transparent_bit?"true":"false");

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
#ifdef DEBUG_DRIVER
printf("SetPixelPattern: unknown bit depth %u\n",fbuffer->gcinfo.bits_per_pixel);
#endif
				break;
			}
		}
	}
	else
	{
#ifdef DEBUG_DRIVER
printf("SetPixelPattern: driver is disconnected\n");
#endif
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
#ifdef DEBUG_DRIVER
printf("ScreenDriver::HideCursor\n");
#endif
	Lock();
	if(fbuffer->IsConnected())
	{
		SetCursorHidden(true);
		if(CursorStateChanged())
			BlitBitmap(under_cursor,under_cursor->Bounds(),oldcursorframe);

	}
	Unlock();
}

void ScreenDriver::MoveCursorTo(float x, float y)
{
#ifdef DEBUG_DRIVER
printf("ScreenDriver::MoveCursorTo(%f,%f)\n",x,y);
#endif
	Lock();
	if(!IsCursorHidden() && cursor)
		BlitBitmap(under_cursor,under_cursor->Bounds(),cursorframe);

	oldcursorframe=cursorframe;

	// TODO: save cursorframe to under_cursor
	
	if(!IsCursorHidden() && cursor)
		BlitBitmap(cursor,cursor->Bounds(),cursorframe);
	
	Unlock();
}

void ScreenDriver::ShowCursor(void)
{
#ifdef DEBUG_DRIVER
printf("ScreenDriver::ShowCursor\n");
#endif
	Lock();
	if(fbuffer->IsConnected())
	{
		hide_cursor--;
		if(CursorStateChanged() && cursor)
			BlitBitmap(cursor,cursor->Bounds(),cursorframe);
	}
	Unlock();
}

void ScreenDriver::ObscureCursor(void)
{
#ifdef DEBUG_DRIVER
printf("ScreenDriver::ObscureCursor\n");
#endif
	Lock();
	SetCursorObscured(true);
	if(!IsCursorHidden() && fbuffer->IsConnected() && under_cursor)
		BlitBitmap(under_cursor,under_cursor->Bounds(),oldcursorframe);
	Unlock();
}
/*
void ScreenDriver::SetCursor(ServerBitmap *csr)
{
#ifdef DEBUG_DRIVER
printf("ScreenDriver::SetCursor\n");
#endif
	if(!csr)
		return;
		
	Lock();

	// erase old if visible
	if(!IsCursorHidden() && under_cursor)
		BlitBitmap(under_cursor,under_cursor->Bounds(),oldcursorframe);

	if(cursor)
		delete cursor;
	if(under_cursor)
		delete under_cursor;
	
	cursor=new ServerBitmap(csr);
	under_cursor=new ServerBitmap(csr);
	
	cursorframe.right=cursorframe.left+csr->Bounds().Width();
	cursorframe.bottom=cursorframe.top+csr->Bounds().Height();
	oldcursorframe=cursorframe;
	
	// TODO: Save to oldcursorframe area to under_cursor
	
	if(!IsCursorHidden() && cursor)
		BlitBitmap(cursor,cursor->Bounds(),cursorframe);
	
	Unlock();
}
*/
void ScreenDriver::HLine(int32 x1, int32 x2, int32 y, RGBColor color)
{
	// Internal function called from others in the driver
	
	// TODO: Implment and substitute Line() calls with HLine calls as appropriate
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
#ifdef DEBUG_DRIVER
printf("HLine() Unfinished\n");
#endif
	uint32 c=0,*pcolor,bytes;
	
	// apparently width=bytes per row
	uint32 bpr=i.width;
	
	// ARGB order
	c|=col.alpha * 0x01000000;
	c|=col.red * 0x010000;
	c|=col.green * 0x0100;
	c|=col.blue;

#ifdef DEBUG_DRIVER
printf("HLine color value: 0x%lx\n",c);
#endif
	bytes=(uint32(x+length)>bpr)?length-((x+length)-bpr):length;

	pcolor=(uint32*)i.frame_buffer;
	pcolor+=(y*bpr)+(x*4);
	for(int32 i=0;i<length;i++)
		pcolor[i]=c;
}

void HLine_16Bit(graphics_card_info i, uint16 x, uint16 y, uint16 length, uint16 col)
{
#ifdef DEBUG_DRIVER
printf("HLine() Unimplemented\n");
#endif
}

void HLine_8Bit(graphics_card_info i, uint16 x, uint16 y, uint16 length, uint8 col)
{
#ifdef DEBUG_DRIVER
printf("HLine(%u,%u,length %u,%u)\n",x,y,length,col);
#endif
	size_t bytes;
	uint32 bpr=i.width;
	if( (bpr%2)!=0 )
		bpr++;

	uint8 *pcolor=(uint8*)i.frame_buffer;
	
	pcolor+=(y*bpr)+x;

	bytes=(uint32(x+length)>bpr)?length-((x+length)-bpr):length;

	memset(pcolor,col,bytes);
}

void ScreenDriver::BlitBitmap(ServerBitmap *sourcebmp,BRect sourcerect, BRect destrect)
{
	// Another internal function called from other functions.
	
	if(!sourcebmp)
	{
#ifdef DEBUG_DRIVER
printf("ScreenDriver::BlitBitmap(): NULL source bitmap\n");
#endif
		return;
	}

	if(sourcebmp->bpp != fbuffer->gcinfo.bits_per_pixel)
	{
#ifdef DEBUG_DRIVER
printf("ScreenDriver::BlitBitmap(): Incompatible bitmap pixel depth\n");
#endif
		return;
	}

	uint8 colorspace_size=sourcebmp->BitsPerPixel();
	// First, clip source rect to destination
	if(sourcerect.Width() > destrect.Width())
		sourcerect.right=sourcerect.left+destrect.Width();
	

	if(sourcerect.Height() > destrect.Height())
		sourcerect.bottom=sourcerect.top+destrect.Height();
	

	// Second, check rectangle bounds against their own bitmaps
	BRect work_rect;

	work_rect.Set(	sourcebmp->Bounds().left,
					sourcebmp->Bounds().top,
					sourcebmp->Bounds().right,
					sourcebmp->Bounds().bottom	);
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

	work_rect.Set(	0,0,fbuffer->gcinfo.width-1,fbuffer->gcinfo.height-1);

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

	// Set pointers to the actual data
	uint8 *src_bits  = (uint8*) sourcebmp->Bits();	
	uint8 *dest_bits = (uint8*) fbuffer->gcinfo.frame_buffer;

	// Get row widths for offset looping
	uint32 src_width  = uint32 (sourcebmp->BytesPerRow());
	uint32 dest_width = uint32 (fbuffer->gcinfo.bytes_per_row);

	// Offset bitmap pointers to proper spot in each bitmap
	src_bits += uint32 ( (sourcerect.top  * src_width)  + (sourcerect.left  * colorspace_size) );
	dest_bits += uint32 ( (destrect.top * dest_width) + (destrect.left * colorspace_size) );

	uint32 line_length = uint32 ((destrect.right - destrect.left)*colorspace_size);
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
	Lock();
	if(fbuffer->IsConnected())
	{
		if(r.top<0 || r.left<0 || 
			r.right>fbuffer->gcinfo.width-1 || r.bottom>fbuffer->gcinfo.height-1)
		{
			Unlock();
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
				printf("ScreenDriver::16-bit mode unimplemented\n");
				break;
			case 15:
				printf("ScreenDriver::15-bit mode unimplemented\n");
				break;
			case 8:
				printf("ScreenDriver::8-bit mode unimplemented\n");
				break;
			default:
				break;
		}

	}
	Unlock();
}
