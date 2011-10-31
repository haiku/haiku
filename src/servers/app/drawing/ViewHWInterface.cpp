/*
 * Copyright 2001-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */


/*!	BView/BWindow combination HWInterface implementation */


#include "ViewHWInterface.h"

#include <new>
#include <stdio.h>

#include <Application.h>
#include <Bitmap.h>
#include <Cursor.h>
#include <Locker.h>
#include <Message.h>
#include <MessageFilter.h>
#include <MessageRunner.h>
#include <Region.h>
#include <Screen.h>
#include <String.h>
#include <View.h>
#include <Window.h>

#include <ServerProtocol.h>

#include "BBitmapBuffer.h"
#include "PortLink.h"
#include "ServerConfig.h"
#include "ServerCursor.h"
#include "UpdateQueue.h"


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

static const bool kDefaultDoubleBuffered = true;

enum {
	MSG_UPDATE = 'updt'
};


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


static int32
run_app_thread(void* cookie)
{
	if (BApplication* app = (BApplication*)cookie) {
		app->Lock();
		app->Run();
	}
	return 0;
}


//#define INPUTSERVER_TEST_MODE 1


class CardView : public BView {
public:
								CardView(BRect bounds);
	virtual						~CardView();

	virtual	void				AttachedToWindow();
	virtual	void				Draw(BRect updateRect);
	virtual	void				MessageReceived(BMessage* message);

								// CardView
			void				SetBitmap(const BBitmap* bitmap);

			void				ForwardMessage(BMessage* message = NULL);

private:
			port_id				fInputPort;
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

class CardMessageFilter : public BMessageFilter {
public:
								CardMessageFilter(CardView* view);

	virtual filter_result		Filter(BMessage* message, BHandler** _target);

private:
			CardView*			fView;
};


//	#pragma mark -


CardView::CardView(BRect bounds)
	:
	BView(bounds, "graphics card view", B_FOLLOW_ALL, B_WILL_DRAW),
	fBitmap(NULL)
{
	SetViewColor(B_TRANSPARENT_32_BIT);

#ifndef INPUTSERVER_TEST_MODE
	fInputPort = create_port(200, SERVER_INPUT_PORT);
#else
	fInputPort = create_port(100, "ViewInputDevice");
#endif

#ifdef ENABLE_INPUT_SERVER_EMULATION
	AddFilter(new CardMessageFilter(this));
#endif
}


CardView::~CardView()
{
}


void
CardView::AttachedToWindow()
{
}


void
CardView::Draw(BRect updateRect)
{
	if (fBitmap != NULL)
		DrawBitmapAsync(fBitmap, updateRect, updateRect);
}


/*!	These functions emulate the Input Server by sending the *exact* same kind of
	messages to the server's port. Being we're using a regular window, it would
	make little sense to do anything else.
*/
void
CardView::ForwardMessage(BMessage* message)
{
	if (message == NULL)
		message = Window()->CurrentMessage();
	if (message == NULL)
		return;

	// remove some fields that potentially mess up our own message processing
	BMessage copy = *message;
	copy.RemoveName("screen_where");
	copy.RemoveName("be:transit");
	copy.RemoveName("be:view_where");
	copy.RemoveName("be:cursor_needed");
	copy.RemoveName("_view_token");

	size_t length = copy.FlattenedSize();
	char stream[length];

	if (copy.Flatten(stream, length) == B_OK)
		write_port(fInputPort, 0, stream, length);
}


void
CardView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BView::MessageReceived(message);
			break;
	}
}


void
CardView::SetBitmap(const BBitmap* bitmap)
{
	if (bitmap != fBitmap) {
		fBitmap = bitmap;

		if (Parent())
			Invalidate();
	}
}


//	#pragma mark -


CardMessageFilter::CardMessageFilter(CardView* view)
	:
	BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE),
	fView(view)
{
}


filter_result
CardMessageFilter::Filter(BMessage* message, BHandler** target)
{
	switch (message->what) {
		case B_KEY_DOWN:
		case B_UNMAPPED_KEY_DOWN:
		case B_KEY_UP:
		case B_UNMAPPED_KEY_UP:
		case B_MOUSE_DOWN:
		case B_MOUSE_UP:
		case B_MOUSE_WHEEL_CHANGED:
			fView->ForwardMessage(message);
			return B_SKIP_MESSAGE;

		case B_MOUSE_MOVED:
		{
			int32 transit;
			if (message->FindInt32("be:transit", &transit) == B_OK
				&& transit == B_ENTERED_VIEW) {
				// A bug in R5 prevents this call from having an effect if
				// called elsewhere, and calling it here works, if we're lucky :-)
				BCursor cursor(kEmptyCursor);
				fView->SetViewCursor(&cursor, true);
			}
			fView->ForwardMessage(message);
			return B_SKIP_MESSAGE;
		}
	}

	return B_DISPATCH_MESSAGE;
}


//	#pragma mark -


CardWindow::CardWindow(BRect frame)
	:
	BWindow(frame, "Haiku App Server", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_NO_SERVER_SIDE_WINDOW_MODIFIERS),
	fUpdateRunner(NULL),
	fUpdateRegion(),
	fUpdateLock("update lock")
{
	fView = new CardView(Bounds());
	AddChild(fView);
	fView->MakeFocus();
		// make it receive key events
}


CardWindow::~CardWindow()
{
	delete fUpdateRunner;
}


void
CardWindow::MessageReceived(BMessage* msg)
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
					fView->Invalidate(frame);
//					fView->Invalidate();
				}
				fUpdateRegion.MakeEmpty();
				fUpdateLock.Unlock();
			} else {
				// see you next time
			}
			break;
		default:
			BWindow::MessageReceived(msg);
			break;
	}
	STRACE("CardWindow::MessageReceived() - exit\n");
}


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


void
CardWindow::SetBitmap(const BBitmap* bitmap)
{
	fView->SetBitmap(bitmap);
}


void
CardWindow::Invalidate(const BRect& frame)
{
	if (fUpdateLock.Lock()) {
		if (!fUpdateRunner) {
			BMessage message(MSG_UPDATE);
			fUpdateRunner = new BMessageRunner(BMessenger(this, this), &message,
				20000);
		}
		fUpdateRegion.Include(frame);
		fUpdateLock.Unlock();
	}
}


//	#pragma mark -


ViewHWInterface::ViewHWInterface()
	:
	HWInterface(kDefaultDoubleBuffered),
	fBackBuffer(NULL),
	fFrontBuffer(NULL),
	fWindow(NULL)
{
	fDisplayMode.virtual_width = 640;
	fDisplayMode.virtual_height = 480;
	fDisplayMode.space = B_RGBA32;
}


ViewHWInterface::~ViewHWInterface()
{
	if (fWindow) {
		fWindow->Lock();
		fWindow->Quit();
	}

	delete fBackBuffer;
	delete fFrontBuffer;

	be_app->Lock();
	be_app->Quit();
}


status_t
ViewHWInterface::Initialize()
{
	return B_OK;
}


status_t
ViewHWInterface::Shutdown()
{
	return B_OK;
}


status_t
ViewHWInterface::SetMode(const display_mode& mode)
{
	AutoWriteLocker _(this);

	status_t ret = B_OK;
	// prevent from doing the unnecessary
	if (fBackBuffer && fFrontBuffer
		&& fDisplayMode.virtual_width == mode.virtual_width
		&& fDisplayMode.virtual_height == mode.virtual_height
		&& fDisplayMode.space == mode.space)
		return ret;

	// check if we support the mode

	display_mode* modes;
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

	BRect frame(0.0, 0.0, fDisplayMode.virtual_width - 1,
		fDisplayMode.virtual_height - 1);

	// create the window if we don't have one already
	if (!fWindow) {
		// if the window has not been created yet, the BApplication
		// has not been created either, but we need one to display
		// a real BWindow in the test environment.
		// be_app->Run() needs to be called in another thread
		BApplication* app = new BApplication("application/x-vnd.haiku-app-server");
		app->Unlock();

		thread_id appThread = spawn_thread(run_app_thread, "app thread",
			B_NORMAL_PRIORITY, app);
		if (appThread >= B_OK)
			ret = resume_thread(appThread);
		else
			ret = appThread;

		if (ret < B_OK)
			return ret;

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
		if ((color_space)fDisplayMode.space != B_RGB32
			&& (color_space)fDisplayMode.space != B_RGBA32)
			doubleBuffered = true;

		BBitmap* frontBitmap
			= new BBitmap(frame, 0, (color_space)fDisplayMode.space);
		fFrontBuffer = new BBitmapBuffer(frontBitmap);

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
			fBackBuffer = new BBitmapBuffer(backBitmap);

			err = fBackBuffer->InitCheck();
			if (err < B_OK) {
				delete fBackBuffer;
				fBackBuffer = NULL;
				ret = err;
			}
		}

		_NotifyFrameBufferChanged();

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


void
ViewHWInterface::GetMode(display_mode* mode)
{
	if (mode && ReadLock()) {
		*mode = fDisplayMode;
		ReadUnlock();
	}
}


status_t
ViewHWInterface::GetDeviceInfo(accelerant_device_info* info)
{
	// We really don't have to provide anything here because this is strictly
	// a software-only driver, but we'll have some fun, anyway.
	if (ReadLock()) {
		info->version = 100;
		sprintf(info->name, "Haiku, Inc. ViewHWInterface");
		sprintf(info->chipset, "Haiku, Inc. Chipset");
		sprintf(info->serial_no, "3.14159265358979323846");
		info->memory = 134217728;	// 128 MB, not that we really have that much. :)
		info->dac_speed = 0xFFFFFFFF;	// *heh*

		ReadUnlock();
	}

	return B_OK;
}


status_t
ViewHWInterface::GetFrameBufferConfig(frame_buffer_config& config)
{
	if (fFrontBuffer == NULL)
		return B_ERROR;

	config.frame_buffer = fFrontBuffer->Bits();
	config.frame_buffer_dma = NULL;
	config.bytes_per_row = fFrontBuffer->BytesPerRow();

	return B_OK;
}


status_t
ViewHWInterface::GetModeList(display_mode** _modes, uint32* _count)
{
	AutoReadLocker _(this);

#if 1
	// setup a whole bunch of different modes
	const struct resolution { int32 width, height; } resolutions[] = {
		{640, 480}, {800, 600}, {1024, 768}, {1152, 864}, {1280, 960},
		{1280, 1024}, {1400, 1050}, {1600, 1200}
	};
	uint32 resolutionCount = sizeof(resolutions) / sizeof(resolutions[0]);
	const uint32 colors[] = {B_CMAP8, B_RGB15, B_RGB16, B_RGB32};
	uint32 count = resolutionCount * 4;

	display_mode* modes = new(std::nothrow) display_mode[count];
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
#else
	// support only a single mode, useful
	// for testing a specific mode
	display_mode *modes = new(nothrow) display_mode[1];
	modes[0].virtual_width = 640;
	modes[0].virtual_height = 480;
	modes[0].space = B_CMAP8;

	*_modes = modes;
	*_count = 1;
#endif

	return B_OK;
}


status_t
ViewHWInterface::GetPixelClockLimits(display_mode* mode, uint32* low,
	uint32* high)
{
	return B_ERROR;
}


status_t
ViewHWInterface::GetTimingConstraints(display_timing_constraints* constraints)
{
	return B_ERROR;
}


status_t
ViewHWInterface::ProposeMode(display_mode* candidate, const display_mode* low,
	const display_mode* high)
{
	// We should be able to get away with this because we're not dealing with
	// any specific hardware. This is a Good Thing(TM) because we can support
	// any hardware we wish within reasonable expectaions and programmer
	// laziness. :P
	return B_OK;
}


status_t
ViewHWInterface::SetDPMSMode(uint32 state)
{
	AutoWriteLocker _(this);

	return BScreen().SetDPMS(state);
}


uint32
ViewHWInterface::DPMSMode()
{
	AutoReadLocker _(this);

	return BScreen().DPMSState();
}


uint32
ViewHWInterface::DPMSCapabilities()
{
	AutoReadLocker _(this);

	return BScreen().DPMSCapabilites();
}


sem_id
ViewHWInterface::RetraceSemaphore()
{
	return -1;
}


status_t
ViewHWInterface::WaitForRetrace(bigtime_t timeout)
{
	// Locking shouldn't be necessary here - R5 should handle this for us. :)
	BScreen screen;
	return screen.WaitForRetrace(timeout);
}


RenderingBuffer*
ViewHWInterface::FrontBuffer() const
{
	return fFrontBuffer;
}


RenderingBuffer*
ViewHWInterface::BackBuffer() const
{
	return fBackBuffer;
}


bool
ViewHWInterface::IsDoubleBuffered() const
{
	if (fFrontBuffer)
		return fBackBuffer != NULL;

	return HWInterface::IsDoubleBuffered();
}


status_t
ViewHWInterface::Invalidate(const BRect& frame)
{
	status_t ret = HWInterface::Invalidate(frame);

	if (ret >= B_OK && fWindow && !IsDoubleBuffered())
		fWindow->Invalidate(frame);
	return ret;
}


status_t
ViewHWInterface::CopyBackToFront(const BRect& frame)
{
	status_t ret = HWInterface::CopyBackToFront(frame);

	if (ret >= B_OK && fWindow)
		fWindow->Invalidate(frame);
	return ret;
}
