//------------------------------------------------------------------------------
//
// Copyright 2002-2005, Haiku, Inc.
// Distributed under the terms of the MIT License.
//
//
//	File Name:		ViewHWInterface.cpp
//	Authors:		DarkWyrm <bpmagic@columbus.rr.com>
//					Stephan Aßmus <superstippi@gmx.de>
//	Description:	BView/BWindow combination HWInterface implementation
//  
//------------------------------------------------------------------------------

#include <stdio.h>

#include <Bitmap.h>
#include <Cursor.h>
#include <Locker.h>
#include <Message.h>
#include <MessageRunner.h>
#include <Region.h>
#include <Screen.h>
#include <String.h>
#include <View.h>
#include <Window.h>

#include "fake_input_server.h"

#include "BitmapBuffer.h"
#include "PortLink.h"
#include "ServerConfig.h"
#include "ServerProtocol.h"
#include "UpdateQueue.h"

#include "ViewHWInterface.h"

#ifdef DEBUG_DRIVER_MODULE
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

const unsigned char kEmptyCursor[] = { 16, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

enum {
	MSG_UPDATE = 'updt',
};

class CardView : public BView {
 public:
								CardView(BRect bounds);
	virtual						~CardView();

	virtual	void				AttachedToWindow();
	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
										   const BMessage* dragMessage);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MessageReceived(BMessage* message);

								// CardView
			void				SetBitmap(const BBitmap* bimtap);

	inline	BPortLink*			ServerLink() const
									{ return fServerLink; }

private:
			BPortLink*			fServerLink;
			const BBitmap*		fBitmap;

/*	int hide_cursor;
	BBitmap *cursor;
	
	BRect cursorframe, oldcursorframe;
	bool obscure_cursor;*/
};

class CardWindow : public BWindow {
 public:
								CardWindow(BRect frame);
	virtual						~CardWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

								// CardWindow
			void				SetBitmap(const BBitmap* bitmap);
			void				Invalidate(const BRect& area);

 private:	
			CardView*			fView;
			BMessageRunner*		fUpdateRunner;
			BRegion				fUpdateRegion;
			BLocker				fUpdateLock;
};


//extern RGBColor workspace_default_color;

CardView::CardView(BRect bounds)
	: BView(bounds, "graphics card view", B_FOLLOW_ALL, B_WILL_DRAW),
	  fServerLink(NULL),
	  fBitmap(NULL)
{
	SetViewColor(B_TRANSPARENT_32_BIT);

	// This link for sending mouse messages to the Haiku app_server.
	// This is only to take the place of the input_server. 
	port_id serverInputPort = create_port(200, SERVER_INPUT_PORT);
	if (serverInputPort == B_NO_MORE_PORTS) {
		debugger("ViewHWInterface: out of ports\n");
		return;
	}
	fServerLink = new BPortLink(serverInputPort);

	// Create a cursor which isn't just a box
/*	cursor=new BBitmap(BRect(0,0,20,20),B_RGBA32,true);
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
	oldcursorframe=cursor->Bounds();*/
}

CardView::~CardView()
{
	delete fServerLink;
/*	delete cursor;*/
}

// AttachedToWindow
void
CardView::AttachedToWindow()
{
}

// Draw
void
CardView::Draw(BRect updateRect)
{
	if (fBitmap) {
		DrawBitmapAsync(fBitmap, updateRect, updateRect);
	}
/*	if (viewbmp) {
		DrawBitmapAsync(viewbmp,oldcursorframe,oldcursorframe);
		DrawBitmapAsync(viewbmp,rect,rect);
	
		if (hide_cursor == 0 && obscure_cursor == false) {
			SetDrawingMode(B_OP_ALPHA);
			DrawBitmapAsync(cursor,cursor->Bounds(), cursorframe);
			SetDrawingMode(B_OP_COPY);
		}
	}*/
}

// These functions emulate the Input Server by sending the *exact* same kind of messages
// to the server's port. Being we're using a regular window, it would make little sense
// to do anything else.

// MouseDown
void
CardView::MouseDown(BPoint pt)
{
#ifdef ENABLE_INPUT_SERVER_EMULATION
	send_mouse_down(fServerLink, pt, Window()->CurrentMessage());
#endif
}

// MouseMoved
void
CardView::MouseMoved(BPoint pt, uint32 transit, const BMessage* dragMessage)
{
	if (!Bounds().Contains(pt))
		return;
// TODO: no cursor support yet

	// A bug in R5 prevents this call from having an effect if
	// called elsewhere, and calling it here works, if we're lucky :-)
//	BCursor cursor(kEmptyCursor);
//	SetViewCursor(&cursor, true);

#ifdef ENABLE_INPUT_SERVER_EMULATION
	send_mouse_moved(fServerLink, pt, Window()->CurrentMessage());
#endif
}

// MouseUp
void
CardView::MouseUp(BPoint pt)
{
#ifdef ENABLE_INPUT_SERVER_EMULATION
	send_mouse_up(fServerLink, pt, Window()->CurrentMessage());
#endif
}

// MessageReceived
void
CardView::MessageReceived(BMessage* message)
{
	switch(message->what) {
		default:
			BView::MessageReceived(message);
			break;
	}
}

// SetBitmap
void
CardView::SetBitmap(const BBitmap* bitmap)
{
	if (bitmap != fBitmap) {
		fBitmap = bitmap;

		if (Parent())
			Invalidate();
	}
}

// constructor
CardWindow::CardWindow(BRect frame)
	: BWindow(frame, "Haiku App Server", B_TITLED_WINDOW,
			  B_NOT_ZOOMABLE | B_NOT_RESIZABLE),
	  fUpdateRunner(NULL),
	  fUpdateRegion(),
	  fUpdateLock("update lock")
{
	fView = new CardView(Bounds());
	AddChild(fView);
}

// destructor
CardWindow::~CardWindow()
{
	delete fUpdateRunner;
}

void CardWindow::MessageReceived(BMessage *msg)
{
STRACE("CardWindow::MessageReceived()\n");
	switch (msg->what) {
		case MSG_UPDATE:
STRACE("MSG_UPDATE\n");
			// invalidate all areas in the view that need redrawing
			if (fUpdateLock.LockWithTimeout(0LL) >= B_OK) {
/*				int32 count = fUpdateRegion.CountRects();
				for (int32 i = 0; i < count; i++) {
					fView->Invalidate(fUpdateRegion.RectAt(i));
				}*/
				BRect frame = fUpdateRegion.Frame();
				if (frame.IsValid()) {
					fView->Invalidate(frame);
				}
				fUpdateRegion.MakeEmpty();
				fUpdateLock.Unlock();
			} else {
				// see you next time
			}
			break;
		default:
#ifdef ENABLE_INPUT_SERVER_EMULATION
			if (!handle_message(fView->ServerLink(), msg))
#endif
				BWindow::MessageReceived(msg);
			break;
	}
STRACE("CardWindow::MessageReceived() - exit\n");
}

// QuitRequested
bool
CardWindow::QuitRequested()
{
	port_id serverport = find_port(SERVER_PORT_NAME);

	if (serverport >= 0) {
		BPortLink link(serverport);
		link.StartMessage(B_QUIT_REQUESTED);
		link.Flush();
	} else
		printf("ERROR: couldn't find the app_server's main port!");
	
	// we don't quit on ourself, we let us be Quit()!
	return false;
}

// SetBitmap
void
CardWindow::SetBitmap(const BBitmap* bitmap)
{
	fView->SetBitmap(bitmap);
}

// Invalidate
void
CardWindow::Invalidate(const BRect& frame)
{
	if (fUpdateLock.Lock()) {
		if (!fUpdateRunner) {
			BMessage message(MSG_UPDATE);
			fUpdateRunner = new BMessageRunner(BMessenger(this, this), &message, 20000);
		}
		fUpdateRegion.Include(frame);
		fUpdateLock.Unlock();
	}
}


// constructor
ViewHWInterface::ViewHWInterface()
	: HWInterface(),
	  fBackBuffer(NULL),
	  fFrontBuffer(NULL),
	  fWindow(NULL),
	  fUpdateExecutor(new UpdateQueue(this))
{
	fDisplayMode.virtual_width = 640;
	fDisplayMode.virtual_height = 480;
	fDisplayMode.space = B_RGBA32;
}

// destructor
ViewHWInterface::~ViewHWInterface()
{
	fWindow->Lock();
	fWindow->Quit();

	delete fBackBuffer;
	delete fFrontBuffer;
}

// Initialize
status_t
ViewHWInterface::Initialize()
{
	return B_OK;
}

// Shutdown
status_t
ViewHWInterface::Shutdown()
{
	return B_OK;
}

// SetMode
status_t
ViewHWInterface::SetMode(const display_mode &mode)
{
	status_t ret = B_OK;
	// prevent from doing the unnecessary
	if (fDisplayMode.virtual_width == mode.virtual_width &&
		fDisplayMode.virtual_height == mode.virtual_height &&
		fDisplayMode.space == mode.space) {
		return ret;
	}
	// TODO: check if the mode is valid even (ie complies to the modes we said we would support)
	// or else ret = B_BAD_VALUE

	// take on settings
	fDisplayMode.virtual_width = mode.virtual_width;
	fDisplayMode.virtual_height = mode.virtual_height;
	fDisplayMode.space = mode.space;

// left over code
//	hide_cursor=0;

	BRect frame(0.0, 0.0,
				fDisplayMode.virtual_width - 1,
				fDisplayMode.virtual_height - 1);

	// create the window if we don't have one already
	if (!fWindow) {
		fWindow = new CardWindow(frame.OffsetToCopy(BPoint(50.0, 50.0)));

		// fire up the window thread but don't show it on screen yet
		fWindow->Hide();
		fWindow->Show();
	}

	if (fWindow->Lock()) {
		// just to be save
		fWindow->SetBitmap(NULL);

		// free and reallocate the bitmaps while the window is locked, 
		// so that the view does not accidentally draw a freed bitmap
		delete fBackBuffer;
		delete fFrontBuffer;

		BBitmap* backBitmap = new BBitmap(frame, 0, (color_space)fDisplayMode.space);
		BBitmap* frontBitmap = new BBitmap(frame, 0, (color_space)fDisplayMode.space);

		fBackBuffer = new BitmapBuffer(backBitmap);
		fFrontBuffer = new BitmapBuffer(frontBitmap);

		status_t err = fBackBuffer->InitCheck();
		if (err < B_OK) {
			delete fBackBuffer;
			fBackBuffer = NULL;
			ret = err;
		}

		err = fFrontBuffer->InitCheck();
		if (err < B_OK) {
			delete fFrontBuffer;
			fFrontBuffer = NULL;
			ret = err;
		}

		if (ret >= B_OK) {
			// clear out buffers, alpha is 255 this way
			// TODO: maybe this should handle different color spaces in different ways
			memset(backBitmap->Bits(), 255, backBitmap->BitsLength());
			memset(frontBitmap->Bits(), 255, frontBitmap->BitsLength());
	
			// change the window size and update the bitmap used for drawing
			fWindow->ResizeTo(frame.Width(), frame.Height());
			fWindow->SetBitmap(fFrontBuffer->Bitmap());
		}

		// window is hidden when this function is called the first time
		if (fWindow->IsHidden())
			fWindow->Show();

		fWindow->Unlock();
	} else {
		ret = B_ERROR;
	}
	return ret;
}

// GetDeviceInfo
status_t
ViewHWInterface::GetDeviceInfo(accelerant_device_info *info)
{
//	if(!info || !is_initialized)
//		return B_ERROR;

	// We really don't have to provide anything here because this is strictly
	// a software-only driver, but we'll have some fun, anyway.
	
	info->version=100;
	sprintf(info->name,"Haiku, Inc. ViewHWInterface");
	sprintf(info->chipset,"Haiku, Inc. Chipset");
	sprintf(info->serial_no,"3.14159265358979323846");
	info->memory=134217728;	// 128 MB, not that we really have that much. :)
	info->dac_speed=0xFFFFFFFF;	// *heh*
	
	return B_OK;
}

// GetModeList
status_t
ViewHWInterface::GetModeList(display_mode **modes, uint32 *count)
{
//	if(!count || !is_initialized)
//		return B_ERROR;

//	screenwin->Lock();
	
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
//	screenwin->Unlock();
	
	return B_OK;
}

status_t
ViewHWInterface::GetPixelClockLimits(display_mode *mode, uint32 *low, uint32 *high)
{
	return B_ERROR;
}

status_t
ViewHWInterface::GetTimingConstraints(display_timing_constraints *dtc)
{
	return B_ERROR;
}

status_t
ViewHWInterface::ProposeMode(display_mode *candidate, const display_mode *low, const display_mode *high)
{
	// We should be able to get away with this because we're not dealing with any
	// specific hardware. This is a Good Thing(TM) because we can support any hardware
	// we wish within reasonable expectaions and programmer laziness. :P
	return B_OK;
}

// SetDPMSMode
status_t
ViewHWInterface::SetDPMSMode(const uint32 &state)
{
	return BScreen().SetDPMS(state);
}

// DPMSMode
uint32
ViewHWInterface::DPMSMode() const
{
	return BScreen().DPMSState();
}

// DPMSCapabilities
uint32
ViewHWInterface::DPMSCapabilities() const
{
	return BScreen().DPMSCapabilites();
}

// WaitForRetrace
status_t
ViewHWInterface::WaitForRetrace(bigtime_t timeout = B_INFINITE_TIMEOUT)
{
	// Locking shouldn't be necessary here - R5 should handle this for us. :)
	BScreen screen;
	return screen.WaitForRetrace(timeout);
}

// FrontBuffer
RenderingBuffer*
ViewHWInterface::FrontBuffer() const
{
	return fFrontBuffer;
}

// BackBuffer
RenderingBuffer*
ViewHWInterface::BackBuffer() const
{
	return fBackBuffer;
}

// Invalidate
status_t
ViewHWInterface::Invalidate(const BRect& frame)
{
	return CopyBackToFront(frame);;

// TODO: get this working, the locking in the DisplayDriverPainter needs
// to be based on locking this object, which essentially means the access
// to the back buffer is locked, or more precise the access to the invalid
// region scheduled to be copied to the front buffer
//	fUpdateExecutor->AddRect(frame);
//	return B_OK;
}

// CopyBackToFront
status_t
ViewHWInterface::CopyBackToFront(const BRect& frame)
{
	if (!fBackBuffer || !fFrontBuffer)
		return B_NO_INIT;

	// we need to mess with the area, but it is const
	BRect area(frame);

	if (area.IsValid() && area.Intersects(fFrontBuffer->Bitmap()->Bounds())) {
// if we couldn't be sure the bitmaps had the same size:
//					   && area.Intersects(fBackBuffer->Bitmap()->Bounds())) {

		const BBitmap* from = fBackBuffer->Bitmap();
		const BBitmap* into = fFrontBuffer->Bitmap();

		// make sure we don't copy out of bounds
		area = from->Bounds() & area;
//		area = into->Bounds() & area;

		uint32 srcBPR = from->BytesPerRow();
		uint32 dstBPR = into->BytesPerRow();
		uint8* dst = (uint8*)into->Bits();
		uint8* src = (uint8*)from->Bits();

		// convert to integer coordinates
		int32 x = (int32)floorf(area.left);
		int32 y = (int32)floorf(area.top);
		int32 right = (int32)ceilf(area.right);
		int32 bottom = (int32)ceilf(area.bottom);

		int32 pixelSize = 0;
		color_space dstFormat = into->ColorSpace();
		// NOTE: there is no check if the bitmaps
		// are of the same format, because they are
		if (dstFormat == B_RGBA32 || dstFormat == B_RGB32) {
			pixelSize = 4;
		} else if (dstFormat == B_RGB24) {
			pixelSize = 3;
		} else if (dstFormat == B_RGB15 || dstFormat == B_RGB16) {
			pixelSize = 2;
		} else if (dstFormat == B_GRAY8 || dstFormat == B_CMAP8) {
			pixelSize = 1;
		}
		int32 bytes = (right - x + 1) * pixelSize;

		if (bytes > 0) {
			// offset pointers to left top of area
			dst += y * dstBPR + x * pixelSize;
			src += y * srcBPR + x * pixelSize;
			// copy
			for (; y <= bottom; y++) {
				memcpy(dst, src, bytes);
				dst += dstBPR;
				src += srcBPR;
			}
		}
		// update the region on screen
		fWindow->Invalidate(area);
	}

	return B_OK;
}


/*void ViewHWInterface::CopyBitmap(ServerBitmap *bitmap, const BRect &source, const BRect &dest, const DrawData *d)
{
	if(!is_initialized || !bitmap || !d)
	{
		printf("CopyBitmap returned - not init or NULL bitmap\n");
		return;
	}
	
	// DON't set draw data here! your existing clipping region will be deleted
//	SetDrawData(d);
	
	// Oh, wow, is this going to be slow. Then again, ViewHWInterface was never meant to be very fast. It could
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

void ViewHWInterface::CopyToBitmap(ServerBitmap *destbmp, const BRect &sourcerect)
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

void ViewHWInterface::ConstrainClippingRegion(BRegion *reg)
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

bool ViewHWInterface::AcquireBuffer(FBBitmap *bmp)
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

void ViewHWInterface::ReleaseBuffer()
{
	if(!is_initialized)
		return;
	framebuffer->Unlock();
	screenwin->Unlock();
}

void ViewHWInterface::Invalidate(const BRect &r)
{
	if(!is_initialized)
		return;
	
	screenwin->Lock();
	screenwin->view->Draw(r);
	screenwin->Unlock();
}
*/
