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
//	File Name:		DirectDriver.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//
//	Description:	BDirectWindow graphics module
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
#include "DirectDriver.h"
#include "ServerConfig.h"
#include "ServerCursor.h"
#include "ServerFont.h"
#include "FontFamily.h"
#include "LayerData.h"
#include "PNGDump.h"

#define DEBUG_DRIVER_MODULE

#ifdef DEBUG_DRIVER_MODULE
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

extern RGBColor workspace_default_color;

/* ---------------------------------------------------------------------------------------
	DW's NOTES:
	
	The current state of the driver still operates somewhat like ViewDriver in that it
	still uses the app_server to draw on the BBitmap. The advantage to this temporary
	state is that the driver can be quickly taken from not-at-all-working to works-but-
	still-needs-work. Code which uses the R5 server can be replaced with code which 
	directly manipulates the buffer one function at a time for an even greater speed boost.
	Simply not using DrawBitmap() to display the framebuffer will allow for a speed gain
	by a couple orders of magnitude
--------------------------------------------------------------------------------------- */

DirectDriver::DirectDriver(void)
{
	screenwin=NULL;
	framebuffer=new BBitmap(BRect(0,0,639,479),B_RGB32,true);
	drawview=new BView(framebuffer->Bounds(),"drawview",0,0);
	framebuffer->AddChild(drawview);

	BScreen screen;
	screen.GetMode(&fCurrentScreenMode);
	
	// We'll save this so that if we change the bit depth of the screen, we
	// can change it back when the driver is shut down.
	fSavedScreenMode=fCurrentScreenMode;
}

DirectDriver::~DirectDriver(void)
{
	Lock();
	delete framebuffer;
	screenwin->framebuffer=NULL;
	Unlock();
}

bool DirectDriver::Initialize(void)
{
	screenwin=new DDWindow(640,480,B_RGB32,this);
	return true;
}

void DirectDriver::Shutdown(void)
{
	screenwin->PostMessage(B_QUIT_REQUESTED);

	if(fSavedScreenMode.space!=fCurrentScreenMode.space)
		BScreen().SetMode(&fSavedScreenMode);
}

void DirectDriver::DrawBitmap(ServerBitmap *bmp, const BRect &src, const BRect &dest, const DrawData *d)
{
	if(!bmp || !d)
	{
		printf("CopyBitmap returned - not init or NULL bitmap\n");
		return;
	}
	
	SetDrawData(d);
	
	// Oh, wow, is this going to be slow. Gotta write the real DrawBitmap code sometime...
	
	BBitmap *mediator=new BBitmap(bmp->Bounds(),bmp->ColorSpace());
	memcpy(mediator->Bits(),bmp->Bits(),bmp->BitsLength());
	
	Lock();
	framebuffer->Lock();

	drawview->DrawBitmap(mediator,src,dest);
	drawview->Sync();
	
	framebuffer->Unlock();
	screenwin->rectpipe.PutRect(dest);
	Unlock();
	delete mediator;
}

void DirectDriver::InvertRect(const BRect &r)
{
	// Shamelessly stolen from AccelerantDriver.cpp
	
	Lock();
	switch (framebuffer->ColorSpace())
	{
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		case B_RGB32_LITTLE:
		case B_RGBA32_LITTLE:
		{
			uint16 width=r.IntegerWidth();
			uint16 height=r.IntegerHeight();
			uint32 *start=(uint32*)framebuffer->Bits();
			uint32 *index;
			start = (uint32 *)((uint8 *)start+(int32)r.top*framebuffer->BytesPerRow());
			start+=(int32)r.left;

			index = start;
			for(int32 i=0;i<height;i++)
			{
				// Are we inverting the right bits? Not sure about location of alpha
				for(int32 j=0; j<width; j++)
					index[j]^=0xFFFFFF00L;
				index = (uint32 *)((uint8 *)index+framebuffer->BytesPerRow());
			}
		}
		break;
		case B_RGB16_BIG:
		case B_RGB16_LITTLE:
		{
			uint16 width=r.IntegerWidth();
			uint16 height=r.IntegerHeight();
			uint16 *start=(uint16*)framebuffer->Bits();
			uint16 *index;
			start = (uint16 *)((uint8 *)start+(int32)r.top*framebuffer->BytesPerRow());
			start+=(int32)r.left;

			index = start;
			for(int32 i=0;i<height;i++)
			{
				for(int32 j=0; j<width; j++)
					index[j]^=0xFFFF;
				index = (uint16 *)((uint8 *)index+framebuffer->BytesPerRow());
			}
		}
		break;
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
		case B_RGB15_LITTLE:
		case B_RGBA15_LITTLE:
		{
			uint16 width=r.IntegerWidth();
			uint16 height=r.IntegerHeight();
			uint16 *start=(uint16*)framebuffer->Bits();
			uint16 *index;
			start = (uint16 *)((uint8 *)start+(int32)r.top*framebuffer->BytesPerRow());
			start+=(int32)r.left;

			index = start;
			for(int32 i=0;i<height;i++)
			{
				// TODO: Where is the alpha bit?
				for(int32 j=0; j<width; j++)
					index[j]^=0xFFFF;
				index = (uint16 *)((uint8 *)index+framebuffer->BytesPerRow());
			}
		}
		break;
		case B_CMAP8:
		case B_GRAY8:
		{
			uint16 width=r.IntegerWidth();
			uint16 height=r.IntegerHeight();
			uint8 *start=(uint8*)framebuffer->Bits();
			uint8 *index;
			start = (uint8 *)start+(int32)r.top*framebuffer->BytesPerRow();
			start+=(int32)r.left;

			index = start;
			for(int32 i=0;i<height;i++)
			{
				for(int32 j=0; j<width; j++)
					index[j]^=0xFF;
				index = (uint8 *)index+framebuffer->BytesPerRow();
			}
		}
		break;
		default:
		break;
	}
	Unlock();
}

void DirectDriver::StrokeLineArray(BPoint *pts, const int32 &numlines, const DrawData *d, RGBColor *colors)
{
	if( !numlines || !pts || !colors || !d)
		return;
	
	Lock();
	framebuffer->Lock();
	drawview->SetPenSize(d->pensize);
	drawview->SetDrawingMode(d->draw_mode);

	int32 ptindex=0;

	drawview->BeginLineArray(numlines);
	for(int32 i=0; i<numlines; i++)
	{
		BPoint pt1=pts[ptindex++];
		BPoint pt2=pts[ptindex++];
		rgb_color col=colors[i].GetColor32();
		
		drawview->AddLine(pt1,pt2,col);
		
	}
	drawview->EndLineArray();
	
	drawview->Sync();
	screenwin->rectpipe.PutRect(framebuffer->Bounds());
	framebuffer->Unlock();
	Unlock();
}

void DirectDriver::SetMode(const display_mode &mode)
{
	Lock();
	
	// Supports all modes all modes >= 640x480 and < supported resolutions in 
	// all supported color depths
	

	// Because we don't support modes with the same height and different widths, we can
	// use the height to determine what we're going to do.

	if(mode.virtual_height>fCurrentScreenMode.virtual_height)
	{
		STRACE(("DirectDriver::SetMode: height requested greater than current screen height\n"));
		Unlock();
		return;
	}

	switch(mode.virtual_height)
	{
		// These mode heights are kosher, so filter out any others
		case 400:
		case 480:
		case 600:
		case 768:
		case 864:
		case 1024:
		case 1200:
			break;
		
		default:
			Unlock();
			return;
	}
	
	// If we're running in a really crappy resolution, allow the user to set the mode
	// to full-screen non-exclusive mode.
	if(fCurrentScreenMode.virtual_height>480 &&
				mode.virtual_height==fCurrentScreenMode.virtual_height)
	{
		STRACE(("DirectDriver::SetMode: height requested equal to current screen height "
			"and greater than 640x480\n"));
		Unlock();
		return;
	}
	
	// We got this far, so apparently the mode requested is smaller than the current screen's
	// mode OR is one of our two exceptions, so now we check for supported bit depth.
	// Because we're directly accessing the framebuffer, changing bit depths for the driver
	// also means changing the user's actual screen bit depth. Realistically, we *could*
	// fake output and change to the user's current bit depth, but considering that (1) it
	// would be a *lot* more work, (2) this driver isn't the real thing, (3) there would
	// be a performance hit, and (4) I'm too lazy to mess with it, we'll change the screen's
	// depth, not make it default, and save the old ont so that we can change it back when
	// the driver is shut down.
	
/*	BScreen screen;
	display_mode candidate=mode;
	if(screen.ProposeMode(&candidate,&mode,&mode)!=B_OK)
	{
		STRACE(("DirectDriver::SetMode:ProposeMode failed\n"));
		return;
	}
*/	
	// It would seem that everything passed the error check, so actually set the mode
	screenwin->Lock();
	screenwin->Quit();
	delete framebuffer;

	framebuffer=new BBitmap(BRect(0,0,mode.virtual_width-1, mode.virtual_height-1),(color_space)mode.space,true);
	drawview=new BView(framebuffer->Bounds(),"drawview",0,0);
	framebuffer->AddChild(drawview);

	screenwin=new DDWindow(mode.virtual_width, mode.virtual_height,(color_space)mode.space,this);
	Unlock();
}

bool DirectDriver::DumpToFile(const char *path)
{
	Lock();
	SaveToPNG(path,framebuffer->Bounds(),framebuffer->ColorSpace(), 
			framebuffer->Bits(),framebuffer->BitsLength(),framebuffer->BytesPerRow());

	Unlock();
	return true;
}

status_t DirectDriver::SetDPMSMode(const uint32 &state)
{
	// This is a hack, but should do enough to be ok for our purposes
	return BScreen().SetDPMS(state);
}

uint32 DirectDriver::DPMSMode(void) const
{
	// This is a hack, but should do enough to be ok for our purposes
	return BScreen().DPMSState();
}

uint32 DirectDriver::DPMSCapabilities(void) const
{
	// This is a hack, but should do enough to be ok for our purposes
	return BScreen().DPMSCapabilites();
}

status_t DirectDriver::GetDeviceInfo(accelerant_device_info *info)
{
	if(!info)
		return B_ERROR;
	
	Lock();
	status_t error=BScreen().GetDeviceInfo(info);
	Unlock();
	
	return error;
}

status_t DirectDriver::GetModeList(display_mode **mode_list, uint32 *count)
{
	if(!mode_list || !count)
		return B_ERROR;
	
	Lock();
	status_t error=BScreen().GetModeList(mode_list,count);
	Unlock();
	
	return error;
}

status_t DirectDriver::GetPixelClockLimits(display_mode *mode, uint32 *low, uint32 *high)
{
	if(!mode || !low || !high)
		return B_ERROR;
	
	Lock();
	status_t error=BScreen().GetPixelClockLimits(mode,low,high);
	Unlock();
	
	return error;
}

status_t DirectDriver::GetTimingConstraints(display_timing_constraints *dtc)
{
	if(!dtc)
		return B_ERROR;
	
	Lock();
	status_t error=BScreen().GetTimingConstraints(dtc);
	Unlock();
	
	return error;
}

status_t DirectDriver::ProposeMode(display_mode *candidate, const display_mode *low, const display_mode *high)
{
	// This could get sticky here. Theoretically, we should support the subset of modes
	// which the hardware can display and is not fullscreen unless the mode is 640x480 and
	// the current screen mode is also 640x480.
	
	
	//TODO: Implement this properly or get a "gotcha" sometime down the road.
	return B_OK;
}

status_t DirectDriver::WaitForRetrace(bigtime_t timeout=B_INFINITE_TIMEOUT)
{
	// There shouldn't be a need for a Lock call on this one...
	return BScreen().WaitForRetrace(timeout);
}

void DirectDriver::FillSolidRect(const BRect &rect, const RGBColor &color)
{
	Lock();
	framebuffer->Lock();
	drawview->SetHighColor(color.GetColor32());
	drawview->FillRect(rect);
	drawview->Sync();
	screenwin->rectpipe.PutRect(rect);
	framebuffer->Unlock();
	Unlock();
}

void DirectDriver::FillPatternRect(const BRect &rect, const DrawData *d)
{
	if(!d)
		return;
	
	Lock();
	framebuffer->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->FillRect(rect,*((pattern*)d->patt.GetInt8()));
	drawview->Sync();
	screenwin->rectpipe.PutRect(rect);
	framebuffer->Unlock();
	Unlock();
}

void DirectDriver::StrokeSolidRect(const BRect &rect, const RGBColor &color)
{
	Lock();
	framebuffer->Lock();
	drawview->SetHighColor(color.GetColor32());
	drawview->StrokeRect(rect);
	drawview->Sync();
	screenwin->rectpipe.PutRect(rect);
	framebuffer->Unlock();
	Unlock();
}

void DirectDriver::StrokeSolidLine(int32 x1, int32 y1, int32 x2, int32 y2, const RGBColor &color)
{
	Lock();
	framebuffer->Lock();
	drawview->SetHighColor(color.GetColor32());
	drawview->StrokeLine(BPoint(x1,y1),BPoint(x2,y2));
	drawview->Sync();
	screenwin->rectpipe.PutRect(BRect(x1,y1,x2,y2));
	framebuffer->Unlock();
	Unlock();
}

void DirectDriver::StrokePatternLine(int32 x1, int32 y1, int32 x2, int32 y2, const DrawData *d)
{
	if(!d)
		return;
	
	Lock();
	framebuffer->Lock();
	drawview->SetHighColor(d->highcolor.GetColor32());
	drawview->SetLowColor(d->lowcolor.GetColor32());
	drawview->StrokeLine(BPoint(x1,y1),BPoint(x2,y2),*((pattern*)d->patt.GetInt8()));
	drawview->Sync();
	screenwin->rectpipe.PutRect(BRect(x1,y1,x2,y2));
	framebuffer->Unlock();
	Unlock();
}

void DirectDriver::CopyBitmap(ServerBitmap *bitmap, const BRect &source,const BRect &dest, const DrawData *d)
{
	if(!bitmap || !d)
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
	
	Lock();
	framebuffer->Lock();

	drawview->DrawBitmap(mediator,source,dest);
	drawview->Sync();
	
	framebuffer->Unlock();
	screenwin->rectpipe.PutRect(dest);
	Unlock();
	delete mediator;
}

void DirectDriver::SetDrawData(const DrawData *d, bool set_font_data)
{
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

void DirectDriver::CopyToBitmap(ServerBitmap *destbmp, const BRect &sourcerect)
{
	if(!destbmp)
	{
		printf("CopyToBitmap returned - not init or NULL bitmap\n");
		return;
	}
	
	if((destbmp->ColorSpace() & 0x000F) != (framebuffer->ColorSpace() & 0x000F))
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

void DirectDriver::ConstrainClippingRegion(BRegion *reg)
{
	Lock();
	framebuffer->Lock();

//	screenwin->view->ConstrainClippingRegion(reg);
	drawview->ConstrainClippingRegion(reg);

	framebuffer->Unlock();
	Unlock();
}
	
rgb_color DirectDriver::GetBlitColor(rgb_color src, rgb_color dest,DrawData *d, bool use_high)
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
		case B_OP_ALPHA:
		{
			//TODO: Implement
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

//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------


DDView::DDView(BRect bounds)
	: BView(bounds,"viewdriver_view",B_FOLLOW_ALL, B_WILL_DRAW),
	serverlink(0L)
{
	SetViewColor(B_TRANSPARENT_32_BIT);

#ifdef ENABLE_INPUT_SERVER_EMULATION
	serverlink.SetSendPort(find_port(SERVER_INPUT_PORT));
#endif

}

// These functions emulate the Input Server by sending the *exact* same kind of messages
// to the server's port. Being we're using a regular window, it would make little sense
// to do anything else.

void DDView::MouseDown(BPoint pt)
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

	serverlink.StartMessage(B_MOUSE_DOWN);
	serverlink.Attach(&time, sizeof(int64));
	serverlink.Attach(&pt.x,sizeof(float));
	serverlink.Attach(&pt.y,sizeof(float));
	serverlink.Attach(&mod, sizeof(uint32));
	serverlink.Attach(&buttons, sizeof(uint32));
	serverlink.Attach(&clicks, sizeof(uint32));
	serverlink.Flush();
#endif
}

void DDView::MouseMoved(BPoint pt, uint32 transit, const BMessage *msg)
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

	serverlink.StartMessage(B_MOUSE_MOVED);
	serverlink.Attach(&time,sizeof(int64));
	serverlink.Attach(&pt.x,sizeof(float));
	serverlink.Attach(&pt.y,sizeof(float));
	GetMouse(&p,&buttons);
	serverlink.Attach(&buttons,sizeof(int32));
	serverlink.Flush();
#endif
}

void DDView::MouseUp(BPoint pt)
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

	serverlink.StartMessage(B_MOUSE_UP);
	serverlink.Attach(&time, sizeof(int64));
	serverlink.Attach(&pt.x,sizeof(float));
	serverlink.Attach(&pt.y,sizeof(float));
	serverlink.Attach(&mod, sizeof(uint32));
	serverlink.Flush();
#endif
}

void DDView::MessageReceived(BMessage *msg)
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
			serverlink.StartMessage(B_MOUSE_WHEEL_CHANGED);
			serverlink.Attach(&time,sizeof(int64));
			serverlink.Attach(x);
			serverlink.Attach(y);
			serverlink.Flush();
			break;
		}
#endif
		default:
			BView::MessageReceived(msg);
			break;
	}
}

DDWindow::DDWindow(uint16 width, uint16 height, color_space space, DirectDriver *owner)
: BDirectWindow(BRect(0,0,width-1,height-1), "Haiku, Inc. App Server",B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_NOT_MOVABLE)
{
	AddChild(new DDView(Bounds()));
	
	fOwner=owner;
	fConnected=false;
	fConnectionDisabled=false;
	fClipList=NULL;
	fNumClipRects=0;
	fDirty=true;
	
	if(!owner)
		debugger("DDWindow requires a non-NULL owner.");
	
	framebuffer=owner->framebuffer;
	
	if(SupportsWindowMode())
	{
		BRect sframe(BScreen().Frame());
		screensize.left=(int32)sframe.left;
		screensize.right=(int32)sframe.right;
		screensize.top=(int32)sframe.top;
		screensize.bottom=(int32)sframe.bottom;
		
		MoveTo( (int32)(sframe.Width()-width)/2, (int32)(sframe.Height()-height)/2 );
		
	}
	else
	{
		SetFullScreen(true);
	}

	Show();
}

DDWindow::~DDWindow(void)
{
	int32 result;
	
	fConnectionDisabled=true;
	Hide();
//	Sync();
	wait_for_thread(fDrawThreadID,&result);
	free(fClipList);
}

void DDWindow::DirectConnected(direct_buffer_info *info)
{
	if(!fConnected && fConnectionDisabled)
		return;
	
	locker.Lock();
	
	switch(info->buffer_state & B_DIRECT_MODE_MASK)
	{
		case B_DIRECT_START:
		{
			fConnected=true;
			fDrawThreadID=spawn_thread(DrawingThread,"drawing_thread", B_NORMAL_PRIORITY,(void*)this);
			resume_thread(fDrawThreadID);
			// fall through to B_DIRECT_MODIFY case
		}
		case B_DIRECT_MODIFY:
		{
			fBits=(uint8*)info->bits;
			fRowBytes=info->bytes_per_row;
			fFormat=info->pixel_format;
			fBounds=info->window_bounds;

			if(fClipList)
			{
				free(fClipList);
				fClipList=NULL;
			}
			fNumClipRects=info->clip_list_count;
			fClipList=(clipping_rect*)malloc(fNumClipRects*sizeof(clipping_rect));
			
			if(fClipList)
			{
				memcpy(fClipList, info->clip_list, fNumClipRects*sizeof(clipping_rect));
				fDirty=true;
			}
			break;
		}
		case B_DIRECT_STOP:
		{
			fConnected=false;
			break;
		}
	}
	locker.Unlock();
}

bool DDWindow::QuitRequested(void)
{
	port_id serverport=find_port(SERVER_PORT_NAME);

	if(serverport!=B_NAME_NOT_FOUND)
		write_port(serverport,B_QUIT_REQUESTED,NULL,0);
	
	return true;
}

int32 DDWindow::DrawingThread(void *data)
{
	DDWindow *w;
	w=(DDWindow *)data;

	switch(w->fFormat)
	{
		case B_RGB32:
		case B_RGBA32:
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		{
			while(!w->fConnectionDisabled)
			{
				w->locker.Lock();
				if(w->fConnected)
				{
					if(w->fDirty)
					{
						int32 y,bytes_to_copy,height;
						uint8 *srcbits, *destbits;
						clipping_rect *clip;
						uint32 i;
						int32 winleft=(int32)w->Frame().left,wintop=(int32)w->Frame().top;
						
						for(i=0; i<w->fNumClipRects; i++)
						{
							clip=&(w->fClipList[i]);
							bytes_to_copy=((clip->right-clip->left)+1)<<2;
							height=(clip->bottom-clip->top)+1;
							
							destbits=w->fBits+ (clip->top*w->fRowBytes)+
													(clip->left<<2);

							srcbits=(uint8*)w->framebuffer->Bits()+
									((clip->top-wintop)*w->framebuffer->BytesPerRow())+
									( (clip->left-winleft)<<2);
							y=0;

							while(y<height)
							{
								memcpy(destbits, srcbits,bytes_to_copy);
								y++;
								destbits+=w->fRowBytes;
								srcbits+=w->framebuffer->BytesPerRow();
							}
						}
						w->fDirty=false;
					}

					if(!w->fConnected)
						break;

					// This will be true if the driver has changed the contents of
					// the framebuffer bitmap
					while(w->rectpipe.HasRects())
					{
						clipping_rect rect;
						int32 y,bytes_to_copy,height;
						uint8 *srcbits, *destbits;
						
						w->rectpipe.GetRect(&rect);
						bytes_to_copy=((rect.right-rect.left)+1)<<2;
						height=(rect.bottom-rect.top)+1;
						
						destbits=w->fBits+ (rect.top*w->fRowBytes)+
												(rect.left<<2);

						srcbits=(uint8*)w->framebuffer->Bits()+
								(rect.top*w->framebuffer->BytesPerRow())+
								(rect.left<<2);
						y=0;

						while(y<height)
						{
//							memcpy(destbits, srcbits,bytes_to_copy);
							y++;
							destbits+=w->fRowBytes;
							srcbits+=w->framebuffer->BytesPerRow();
						}
					}
				}
				w->locker.Unlock();
				snooze(16000);
			}
			break;
		}
		case B_GRAY8:
		case B_CMAP8:
		{
			while(!w->fConnectionDisabled)
			{
				w->locker.Lock();
				if(w->fConnected)
				{
					if(w->fDirty)
					{
						int32 y,width,height,adder;
						uint8 *p;
						clipping_rect *clip;
						uint32 i;
						
						adder=w->fRowBytes;
						for(i=0; i<w->fNumClipRects; i++)
						{
							clip=&(w->fClipList[i]);
							width=(clip->right-clip->left)+1;
							height=(clip->bottom-clip->top)+1;
							p=w->fBits+(clip->top*w->fRowBytes)+clip->left;
							y=0;
							while(y<height)
							{
								memset(p,0x00,width);
								y++;
								p+=adder;
							}
						}
						w->fDirty=false;
					}
				}
				w->locker.Unlock();
				snooze(16000);
			}
			break;
		}
		default:
			debugger("DirectDriver was given an unsupported color mode.");
			break;
	}
	return 0;
}

RectPipe::RectPipe(void)
{
	list.MakeEmpty();
}

RectPipe::~RectPipe(void)
{
	lock.Lock();
	clipping_rect *rect;
	for(int32 i=0; i<list.CountItems();i++)
	{
		rect=(clipping_rect*)list.ItemAt(i);
		delete rect;
	}
}

void RectPipe::PutRect(const BRect &rect)
{
	lock.Lock();
	has_rects=true;
	clipping_rect *clip=new clipping_rect;
	clip->left=(int32)rect.left;
	clip->right=(int32)rect.right;
	clip->top=(int32)rect.top;
	clip->bottom=(int32)rect.bottom;
	list.AddItem(clip);
	lock.Unlock();
}

void RectPipe::PutRect(const clipping_rect &rect)
{
	lock.Lock();
	has_rects=true;
	clipping_rect *clip=new clipping_rect;
	*clip=rect;
	list.AddItem(clip);
	lock.Unlock();
}

bool RectPipe::GetRect(clipping_rect *rect)
{
	lock.Lock();
	clipping_rect *clip=(clipping_rect*)list.RemoveItem(0L);
	if(clip)
	{
		*rect=*clip;
		delete clip;

		if(list.IsEmpty())
			has_rects=false;

		lock.Unlock();
		return true;
	}

	// If we returned NULL, it means that the list is empty
	has_rects=false;
	lock.Unlock();
	return false;
}

bool RectPipe::HasRects(void)
{
	lock.Lock();
	bool value=has_rects;
	lock.Unlock();
	return value;
}
