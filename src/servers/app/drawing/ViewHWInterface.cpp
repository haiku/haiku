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
#include "ServerCursor.h"
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

// string_for_color_space
const char*
string_for_color_space(color_space format)
{
	const char* name = "<unkown format>";
	switch (format) {
		case B_RGB32:
			name = "B_RGB32";
			break;
		case B_RGBA32:
			name = "B_RGBA32";
			break;
		case B_RGB32_BIG:
			name = "B_RGB32_BIG";
			break;
		case B_RGBA32_BIG:
			name = "B_RGBA32_BIG";
			break;
		case B_RGB24:
			name = "B_RGB24";
			break;
		case B_RGB24_BIG:
			name = "B_RGB24_BIG";
			break;
		case B_CMAP8:
			name = "B_CMAP8";
			break;
		case B_GRAY8:
			name = "B_GRAY8";
			break;
		case B_GRAY1:
			name = "B_GRAY1";
			break;
		default:
			break;
	}
	return name;
}

//#define INPUTSERVER_TEST_MODE 1

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

	inline	BPrivate::PortLink*	ServerLink() const
									{ return fServerLink; }

	void						ForwardMessage();

private:
			port_id			fInputPort;
			BPrivate::PortLink*	fServerLink;
			const BBitmap*		fBitmap;
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

#ifndef INPUTSERVER_TEST_MODE
	// This link for sending mouse messages to the Haiku app_server.
	// This is only to take the place of the input_server. 
	port_id input_port = find_port(SERVER_INPUT_PORT);
	fServerLink = new BPrivate::PortLink(input_port);
#else
	fInputPort = create_port(100, "ViewInputDevice");
#endif

}

CardView::~CardView()
{
	delete fServerLink;
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
}

// These functions emulate the Input Server by sending the *exact* same kind of messages
// to the server's port. Being we're using a regular window, it would make little sense
// to do anything else.

void
CardView::ForwardMessage()
{
	BMessage *message = Window()->CurrentMessage();
	size_t length = message->FlattenedSize();
	char stream[length];

	if ( message->Flatten(stream, length) == B_OK) {
		write_port(fInputPort, 0, stream, length);
	}
}


// MouseDown
void
CardView::MouseDown(BPoint pt)
{
#ifdef ENABLE_INPUT_SERVER_EMULATION
#ifndef INPUTSERVER_TEST_MODE
	send_mouse_down(fServerLink, pt, Window()->CurrentMessage());
#else
	ForwardMessage();
#endif
#endif
}

// MouseMoved
void
CardView::MouseMoved(BPoint pt, uint32 transit, const BMessage* dragMessage)
{
	if (!Bounds().Contains(pt))
		return;

	// A bug in R5 prevents this call from having an effect if
	// called elsewhere, and calling it here works, if we're lucky :-)
	BCursor cursor(kEmptyCursor);
	SetViewCursor(&cursor, true);

#ifdef ENABLE_INPUT_SERVER_EMULATION
#ifndef INPUTSERVER_TEST_MODE
	send_mouse_moved(fServerLink, pt, Window()->CurrentMessage());
#else
	ForwardMessage();
#endif
#endif
}

// MouseUp
void
CardView::MouseUp(BPoint pt)
{
#ifdef ENABLE_INPUT_SERVER_EMULATION
#ifndef INPUTSERVER_TEST_MODE
	send_mouse_up(fServerLink, pt, Window()->CurrentMessage());
#else
	ForwardMessage();
#endif
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
			if (fUpdateLock.LockWithTimeout(2000LL) >= B_OK) {
/*				int32 count = fUpdateRegion.CountRects();
				for (int32 i = 0; i < count; i++) {
					fView->Invalidate(fUpdateRegion.RectAt(i));
				}*/
				BRect frame = fUpdateRegion.Frame();
				if (frame.IsValid()) {
//					fView->Invalidate(frame);
					fView->Invalidate();
				}
				fUpdateRegion.MakeEmpty();
				fUpdateLock.Unlock();
			} else {
				// see you next time
			}
			break;
		default:
#ifdef ENABLE_INPUT_SERVER_EMULATION
#ifndef INPUTSERVER_TEST_MODE
			if (!handle_message(fView->ServerLink(), msg))
#else
			fView->ForwardMessage();
#endif
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
		BPrivate::PortLink link(serverport);
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
	  fWindow(NULL)
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
	if (fBackBuffer && fFrontBuffer
		&& fDisplayMode.virtual_width == mode.virtual_width
		&& fDisplayMode.virtual_height == mode.virtual_height
		&& fDisplayMode.space == mode.space)
		return ret;

	// check if we support the mode

	display_mode *modes;
	uint32 modeCount, i;
	if (GetModeList(&modes, &modeCount) != B_OK)
		return B_NO_MEMORY;

	for (i = 0; i < modeCount; i++) {
		// we only care for the bare minimum
		if (modes[i].virtual_width == mode.virtual_width
			&& modes[i].virtual_height == mode.virtual_height
			&& modes[i].space == mode.space) {
			// take over settings
			fDisplayMode = modes[i];
			break;
		}
	}

	delete[] modes;

	if (i == modeCount)
		return B_BAD_VALUE;

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
		fBackBuffer = NULL;
		delete fFrontBuffer;

		// NOTE: backbuffer is always B_RGBA32, this simplifies the
		// drawing backend implementation tremendously for the time
		// being. The color space conversion is handled in CopyBackToFront()

		// TODO: Above not true anymore for single buffered mode!!!
		// -> fall back to double buffer for fDisplayMode.space != B_RGB32
		// as intermediate solution...
		bool doubleBuffered = HWInterface::IsDoubleBuffered();
		if ((color_space)fDisplayMode.space != B_RGB32 &&
			(color_space)fDisplayMode.space != B_RGBA32)
			doubleBuffered = true;

		BBitmap* frontBitmap = new BBitmap(frame, 0, (color_space)fDisplayMode.space);
		fFrontBuffer = new BitmapBuffer(frontBitmap);

		status_t err = fFrontBuffer->InitCheck();
		if (err < B_OK) {
			delete fFrontBuffer;
			fFrontBuffer = NULL;
			ret = err;
		}

		if (err >= B_OK && doubleBuffered) {
			// backbuffer is always B_RGBA32
			// since we override IsDoubleBuffered(), the drawing buffer
			// is in effect also always B_RGBA32.
			BBitmap* backBitmap = new BBitmap(frame, 0, B_RGBA32);
			fBackBuffer = new BitmapBuffer(backBitmap);
	
			err = fBackBuffer->InitCheck();
			if (err < B_OK) {
				delete fBackBuffer;
				fBackBuffer = NULL;
				ret = err;
			}
		}

		if (ret >= B_OK) {
			// clear out buffers, alpha is 255 this way
			// TODO: maybe this should handle different color spaces in different ways
			if (fBackBuffer)
				memset(fBackBuffer->Bits(), 255, fBackBuffer->BitsLength());
			memset(fFrontBuffer->Bits(), 255, fFrontBuffer->BitsLength());
	
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

// GetMode
void
ViewHWInterface::GetMode(display_mode* mode)
{
	if (mode)
		*mode = fDisplayMode;
}

// GetDeviceInfo
status_t
ViewHWInterface::GetDeviceInfo(accelerant_device_info *info)
{
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
ViewHWInterface::GetModeList(display_mode **_modes, uint32 *_count)
{
	// DEPRECATED:
	// NOTE: Originally, I was going to figure out good timing values to be 
	// returned in each of the modes supported, but I won't bother, being this
	// won't be used much longer anyway. 

	const struct resolution { int32 width, height; } resolutions[] = {
		{640, 480}, {800, 600}, {1024, 768}, {1152, 864}, {1280, 960},
		{1280, 1024}, {1400, 1050}, {1600, 1200}
	};
	uint32 resolutionCount = sizeof(resolutions) / sizeof(resolutions[0]);
	const uint32 colors[] = {B_CMAP8, B_RGB15, B_RGB16, B_RGB32};
	uint32 count = resolutionCount * 4;

	display_mode *modes = new display_mode[count];
	if (modes == NULL)
		return B_NO_MEMORY;

	*_modes = modes;
	*_count = count;

	int32 index = 0;
	for (uint32 i = 0; i < resolutionCount; i++) {
		for (uint32 c = 0; c < 4; c++) {
			modes[index].virtual_width = resolutions[i].width;
			modes[index].virtual_height = resolutions[i].height;
			modes[index].space = colors[c];

			modes[index].h_display_start = 0;
			modes[index].v_display_start = 0;
			modes[index].timing.h_display = resolutions[i].width;
			modes[index].timing.v_display = resolutions[i].height;
			modes[index].timing.h_total = 22000;
			modes[index].timing.v_total = 22000;
			modes[index].timing.pixel_clock = ((uint32)modes[index].timing.h_total
				* modes[index].timing.v_total * 60) / 1000;
			modes[index].flags = B_PARALLEL_ACCESS;

			index++;
		}
	}

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

// IsDoubleBuffered
bool
ViewHWInterface::IsDoubleBuffered() const
{
	if (fFrontBuffer)
		return fBackBuffer != NULL;

	return HWInterface::IsDoubleBuffered();
}

// Invalidate
status_t
ViewHWInterface::Invalidate(const BRect& frame)
{
	status_t ret = HWInterface::Invalidate(frame);

	if (ret >= B_OK && fWindow)
		fWindow->Invalidate(frame);
	return ret;
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
