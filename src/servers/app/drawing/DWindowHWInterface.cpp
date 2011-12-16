/*
 * Copyright 2001-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Stephan Aßmus <superstippi@gmx.de>
 */


/*!	BView/BDirectWindow/Accelerant combination HWInterface implementation
*/


#include "DWindowHWInterface.h"

#include <malloc.h>
#include <new>
#include <stdio.h>

#include <Application.h>
#include <Bitmap.h>
#include <Cursor.h>
#include <DirectWindow.h>
#include <Locker.h>
#include <Message.h>
#include <MessageFilter.h>
#include <MessageRunner.h>
#include <Region.h>
#include <Screen.h>
#include <String.h>
#include <View.h>

#include <Accelerant.h>
#include <graphic_driver.h>
#include <FindDirectory.h>
#include <image.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <ServerProtocol.h>

#include "DWindowBuffer.h"
#include "PortLink.h"
#include "RGBColor.h"
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
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};


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


class DView : public BView {
public:
								DView(BRect bounds);
	virtual						~DView();

								// DView
			void				ForwardMessage(BMessage* message = NULL);

private:
			port_id				fInputPort;
};

class DWindow : public BWindow {
public:
								DWindow(BRect frame,
									DWindowHWInterface* interface,
									DWindowBuffer* buffer);
	virtual						~DWindow();

	virtual	bool				QuitRequested();

//	virtual	void				DirectConnected(direct_buffer_info* info);

	virtual	void				FrameMoved(BPoint newOffset);

private:
	DWindowHWInterface*			fHWInterface;
	DWindowBuffer*				fBuffer;
};

class DirectMessageFilter : public BMessageFilter {
public:
								DirectMessageFilter(DView* view);

	virtual filter_result		Filter(BMessage *message, BHandler** _target);

private:
			DView*				fView;
};


//	#pragma mark -


DView::DView(BRect bounds)
	:
	BView(bounds, "graphics card view", B_FOLLOW_ALL, 0)
{
	SetViewColor(B_TRANSPARENT_COLOR);
#ifndef INPUTSERVER_TEST_MODE
	fInputPort = create_port(200, SERVER_INPUT_PORT);
#else
	fInputPort = create_port(100, "ViewInputDevice");
#endif

#ifdef ENABLE_INPUT_SERVER_EMULATION
	AddFilter(new DirectMessageFilter(this));
#endif
}


DView::~DView()
{
}


/*!	This function emulates the Input Server by sending the *exact* same kind of
	messages to the server's port. Being we're using a regular window, it would
	make little sense to do anything else.
*/
void
DView::ForwardMessage(BMessage* message)
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

	size_t length = copy.FlattenedSize();
	char stream[length];

	if (copy.Flatten(stream, length) == B_OK)
		write_port(fInputPort, 0, stream, length);
}


//	#pragma mark -


DirectMessageFilter::DirectMessageFilter(DView* view)
	:
	BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE),
	fView(view)
{
}


filter_result
DirectMessageFilter::Filter(BMessage* message, BHandler** target)
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


DWindow::DWindow(BRect frame, DWindowHWInterface* interface,
		DWindowBuffer* buffer)
	:
	BWindow(frame, "Haiku App Server", B_TITLED_WINDOW_LOOK,
		B_FLOATING_ALL_WINDOW_FEEL,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_NOT_MOVABLE),
	fHWInterface(interface),
	fBuffer(buffer)
{
	DView* view = new DView(Bounds());
	AddChild(view);
	view->MakeFocus();
		// make it receive key events
}


DWindow::~DWindow()
{
}


bool
DWindow::QuitRequested()
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


/*
void
DWindow::DirectConnected(direct_buffer_info* info)
{
//	fDesktop->LockClipping();
//
//	fEngine.Lock();
//
	switch(info->buffer_state & B_DIRECT_MODE_MASK) {
		case B_DIRECT_START:
		case B_DIRECT_MODIFY:
			fBuffer->SetTo(info);
//			fDesktop->SetOffset(info->window_bounds.left, info->window_bounds.top);
			break;
		case B_DIRECT_STOP:
			fBuffer->SetTo(NULL);
			break;
	}
//
//	fDesktop->SetMasterClipping(&fBuffer.WindowClipping());
//
//	fEngine.Unlock();
//
//	fDesktop->UnlockClipping();
}
*/


void
DWindow::FrameMoved(BPoint newOffset)
{
	fHWInterface->SetOffset((int32)newOffset.x, (int32)newOffset.y);
}


//	#pragma mark -


const int32 kDefaultParamsCount = 64;

DWindowHWInterface::DWindowHWInterface()
	:
	HWInterface(),
	fFrontBuffer(new DWindowBuffer()),
	fWindow(NULL),

	fXOffset(50),
	fYOffset(50),

	fCardFD(-1),
	fAccelerantImage(-1),
	fAccelerantHook(NULL),
	fEngineToken(NULL),
	fSyncToken(),

	// required hooks
	fAccAcquireEngine(NULL),
	fAccReleaseEngine(NULL),
	fAccSyncToToken(NULL),
	fAccGetModeCount(NULL),
	fAccGetModeList(NULL),
	fAccGetFrameBufferConfig(NULL),
	fAccSetDisplayMode(NULL),
	fAccGetDisplayMode(NULL),
	fAccGetPixelClockLimits(NULL),

	// optional accelerant hooks
	fAccGetTimingConstraints(NULL),
	fAccProposeDisplayMode(NULL),
	fAccFillRect(NULL),
	fAccInvertRect(NULL),
	fAccScreenBlit(NULL),
	fAccSetCursorShape(NULL),
	fAccMoveCursor(NULL),
	fAccShowCursor(NULL),

	fRectParams(new (std::nothrow) fill_rect_params[kDefaultParamsCount]),
	fRectParamsCount(kDefaultParamsCount),
	fBlitParams(new (std::nothrow) blit_params[kDefaultParamsCount]),
	fBlitParamsCount(kDefaultParamsCount)
{
	fDisplayMode.virtual_width = 800;
	fDisplayMode.virtual_height = 600;
	fDisplayMode.space = B_RGBA32;

	memset(&fSyncToken, 0, sizeof(sync_token));
}


DWindowHWInterface::~DWindowHWInterface()
{
	if (fWindow) {
		fWindow->Lock();
		fWindow->Quit();
	}

	delete[] fRectParams;
	delete[] fBlitParams;

	delete fFrontBuffer;

	be_app->Lock();
	be_app->Quit();
	delete be_app;
}


status_t
DWindowHWInterface::Initialize()
{
	status_t ret = HWInterface::Initialize();

	if (!fRectParams || !fBlitParams)
		return B_NO_MEMORY;

	if (ret >= B_OK) {
		for (int32 i = 1; fCardFD != B_ENTRY_NOT_FOUND; i++) {
			fCardFD = _OpenGraphicsDevice(i);
			if (fCardFD < 0) {
				STRACE(("Failed to open graphics device\n"));
				continue;
			}

			if (_OpenAccelerant(fCardFD) == B_OK)
				break;

			close(fCardFD);
			// _OpenAccelerant() failed, try to open next graphics card
		}

		return fCardFD >= 0 ? B_OK : fCardFD;
	}
	return ret;
}


/*!	\brief Opens a graphics device for read-write access
	\param deviceNumber Number identifying which graphics card to open (1 for
		first card)
	\return The file descriptor for the opened graphics device

	The deviceNumber is relative to the number of graphics devices that can be
	successfully opened.  One represents the first card that can be successfully
	opened (not necessarily the first one listed in the directory).
	Graphics drivers must be able to be opened more than once, so we really get
	the first working entry.
*/
int
DWindowHWInterface::_OpenGraphicsDevice(int deviceNumber)
{
	DIR *directory = opendir("/dev/graphics");
	if (!directory)
		return -1;

	// ToDo: the former R5 "stub" driver is called "vesa" under Haiku; however,
	//	we do not need to avoid this driver this way when is has been ported
	//	to the new driver architecture - the special case here can then be
	//	removed.
	int count = 0;
	struct dirent *entry = NULL;
	int current_card_fd = -1;
	char path[PATH_MAX];
	while (count < deviceNumber && (entry = readdir(directory)) != NULL) {
		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..") ||
			!strcmp(entry->d_name, "stub") || !strcmp(entry->d_name, "vesa"))
			continue;

		if (current_card_fd >= 0) {
			close(current_card_fd);
			current_card_fd = -1;
		}

		sprintf(path, "/dev/graphics/%s", entry->d_name);
		current_card_fd = open(path, B_READ_WRITE);
		if (current_card_fd >= 0)
			count++;
	}

	// Open VESA driver if we were not able to get a better one
	if (count < deviceNumber) {
		if (deviceNumber == 1) {
			sprintf(path, "/dev/graphics/vesa");
			current_card_fd = open(path, B_READ_WRITE);
		} else {
			close(current_card_fd);
			current_card_fd = B_ENTRY_NOT_FOUND;
		}
	}

	if (entry)
		fCardNameInDevFS = entry->d_name;

	return current_card_fd;
}


status_t
DWindowHWInterface::_OpenAccelerant(int device)
{
	char signature[1024];
	if (ioctl(device, B_GET_ACCELERANT_SIGNATURE,
			&signature, sizeof(signature)) != B_OK)
		return B_ERROR;

	STRACE(("accelerant signature is: %s\n", signature));

	struct stat accelerant_stat;
	const static directory_which dirs[] = {
		B_USER_ADDONS_DIRECTORY,
		B_COMMON_ADDONS_DIRECTORY,
		B_SYSTEM_ADDONS_DIRECTORY
	};

	fAccelerantImage = -1;

	for (uint32 i = 0; i < sizeof(dirs) / sizeof(directory_which); i++) {
		char path[PATH_MAX];
		if (find_directory(dirs[i], -1, false, path, PATH_MAX) != B_OK)
			continue;

		strcat(path, "/accelerants/");
		strcat(path, signature);
		if (stat(path, &accelerant_stat) != 0)
			continue;

		fAccelerantImage = load_add_on(path);
		if (fAccelerantImage >= 0) {
			if (get_image_symbol(fAccelerantImage, B_ACCELERANT_ENTRY_POINT,
				B_SYMBOL_TYPE_ANY, (void**)(&fAccelerantHook)) != B_OK ) {
				STRACE(("unable to get B_ACCELERANT_ENTRY_POINT\n"));
				unload_add_on(fAccelerantImage);
				fAccelerantImage = -1;
				return B_ERROR;
			}


			accelerant_clone_info_size cloneInfoSize;
			cloneInfoSize = (accelerant_clone_info_size)fAccelerantHook(
				B_ACCELERANT_CLONE_INFO_SIZE, NULL);
			if (!cloneInfoSize) {
				STRACE(("unable to get B_ACCELERANT_CLONE_INFO_SIZE (%s)\n", path));
				unload_add_on(fAccelerantImage);
				fAccelerantImage = -1;
				return B_ERROR;
			}

			ssize_t cloneSize = cloneInfoSize();
			void* cloneInfoData = malloc(cloneSize);

//			get_accelerant_clone_info getCloneInfo;
//			getCloneInfo = (get_accelerant_clone_info)fAccelerantHook(B_GET_ACCELERANT_CLONE_INFO, NULL);
//			if (!getCloneInfo) {
//				STRACE(("unable to get B_GET_ACCELERANT_CLONE_INFO (%s)\n", path));
//				unload_add_on(fAccelerantImage);
//				fAccelerantImage = -1;
//				return B_ERROR;
//			}
//			printf("getCloneInfo: %p\n", getCloneInfo);
//
//			getCloneInfo(cloneInfoData);
// TODO: this is what works for the ATI Radeon driver...
sprintf((char*)cloneInfoData, "graphics/%s", fCardNameInDevFS.String());

			clone_accelerant cloneAccelerant;
			cloneAccelerant = (clone_accelerant)fAccelerantHook(B_CLONE_ACCELERANT, NULL);
			if (!cloneAccelerant) {
				STRACE(("unable to get B_CLONE_ACCELERANT\n"));
				unload_add_on(fAccelerantImage);
				fAccelerantImage = -1;
				return B_ERROR;
			}
			status_t ret = cloneAccelerant(cloneInfoData);
			if (ret  != B_OK) {
				STRACE(("Cloning accelerant unsuccessful: %s\n", strerror(ret)));
				unload_add_on(fAccelerantImage);
				fAccelerantImage = -1;
				return B_ERROR;
			}

			break;
		}
	}

	if (fAccelerantImage < B_OK)
		return B_ERROR;

	if (_SetupDefaultHooks() != B_OK) {
		STRACE(("cannot setup default hooks\n"));

		uninit_accelerant uninitAccelerant = (uninit_accelerant)
			fAccelerantHook(B_UNINIT_ACCELERANT, NULL);
		if (uninitAccelerant != NULL)
			uninitAccelerant();

		unload_add_on(fAccelerantImage);
		return B_ERROR;
	} else {
		_UpdateFrameBufferConfig();
	}

	return B_OK;
}


status_t
DWindowHWInterface::_SetupDefaultHooks()
{
	// required
	fAccAcquireEngine = (acquire_engine)fAccelerantHook(B_ACQUIRE_ENGINE, NULL);
	fAccReleaseEngine = (release_engine)fAccelerantHook(B_RELEASE_ENGINE, NULL);
	fAccSyncToToken = (sync_to_token)fAccelerantHook(B_SYNC_TO_TOKEN, NULL);
	fAccGetModeCount = (accelerant_mode_count)fAccelerantHook(
		B_ACCELERANT_MODE_COUNT, NULL);
	fAccGetModeList = (get_mode_list)fAccelerantHook(B_GET_MODE_LIST, NULL);
	fAccGetFrameBufferConfig = (get_frame_buffer_config)fAccelerantHook(
		B_GET_FRAME_BUFFER_CONFIG, NULL);
	fAccSetDisplayMode = (set_display_mode)fAccelerantHook(
		B_SET_DISPLAY_MODE, NULL);
	fAccGetDisplayMode = (get_display_mode)fAccelerantHook(
		B_GET_DISPLAY_MODE, NULL);
	fAccGetPixelClockLimits = (get_pixel_clock_limits)fAccelerantHook(
		B_GET_PIXEL_CLOCK_LIMITS, NULL);

	if (!fAccAcquireEngine || !fAccReleaseEngine || !fAccGetFrameBufferConfig
		|| !fAccGetModeCount || !fAccGetModeList || !fAccSetDisplayMode
		|| !fAccGetDisplayMode || !fAccGetPixelClockLimits) {
		return B_ERROR;
	}

	// optional
	fAccGetTimingConstraints = (get_timing_constraints)fAccelerantHook(
		B_GET_TIMING_CONSTRAINTS, NULL);
	fAccProposeDisplayMode = (propose_display_mode)fAccelerantHook(
		B_PROPOSE_DISPLAY_MODE, NULL);

	// cursor
	fAccSetCursorShape = (set_cursor_shape)fAccelerantHook(
		B_SET_CURSOR_SHAPE, NULL);
	fAccMoveCursor = (move_cursor)fAccelerantHook(B_MOVE_CURSOR, NULL);
	fAccShowCursor = (show_cursor)fAccelerantHook(B_SHOW_CURSOR, NULL);

	// update acceleration hooks
	// TODO: would actually have to pass a valid display_mode!
	fAccFillRect = (fill_rectangle)fAccelerantHook(B_FILL_RECTANGLE, NULL);
	fAccInvertRect = (invert_rectangle)fAccelerantHook(B_INVERT_RECTANGLE, NULL);
	fAccScreenBlit = (screen_to_screen_blit)fAccelerantHook(
		B_SCREEN_TO_SCREEN_BLIT, NULL);

	return B_OK;
}


status_t
DWindowHWInterface::_UpdateFrameBufferConfig()
{
	frame_buffer_config config;
	if (fAccGetFrameBufferConfig(&config) != B_OK) {
		STRACE(("unable to get frame buffer config\n"));
		return B_ERROR;
	}

	fFrontBuffer->SetTo(&config, fXOffset, fYOffset, fDisplayMode.virtual_width,
		fDisplayMode.virtual_height, (color_space)fDisplayMode.space);

	return B_OK;
}


status_t
DWindowHWInterface::Shutdown()
{
	printf("DWindowHWInterface::Shutdown()\n");
	if (fAccelerantHook) {
		uninit_accelerant UninitAccelerant
			= (uninit_accelerant)fAccelerantHook(B_UNINIT_ACCELERANT, NULL);
		if (UninitAccelerant)
			UninitAccelerant();
	}

	if (fAccelerantImage >= 0)
		unload_add_on(fAccelerantImage);

	if (fCardFD >= 0)
		close(fCardFD);

	return B_OK;
}


status_t
DWindowHWInterface::SetMode(const display_mode& mode)
{
	AutoWriteLocker _(this);

	status_t ret = B_OK;
	// prevent from doing the unnecessary
	if (fFrontBuffer
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

		fWindow = new DWindow(frame.OffsetByCopy(fXOffset, fYOffset), this,
			fFrontBuffer);

		// fire up the window thread but don't show it on screen yet
		fWindow->Hide();
		fWindow->Show();
	}

	if (fWindow->Lock()) {
		// free and reallocate the bitmaps while the window is locked,
		// so that the view does not accidentally draw a freed bitmap

		if (ret >= B_OK) {
			// change the window size and update the bitmap used for drawing
			fWindow->ResizeTo(frame.Width(), frame.Height());
		}

		// window is hidden when this function is called the first time
		if (fWindow->IsHidden())
			fWindow->Show();

		fWindow->Unlock();
	} else {
		ret = B_ERROR;
	}

	_UpdateFrameBufferConfig();
	_NotifyFrameBufferChanged();

	return ret;
}


void
DWindowHWInterface::GetMode(display_mode* mode)
{
	if (mode && ReadLock()) {
		*mode = fDisplayMode;
		ReadUnlock();
	}
}


status_t
DWindowHWInterface::GetDeviceInfo(accelerant_device_info* info)
{
	// We really don't have to provide anything here because this is strictly
	// a software-only driver, but we'll have some fun, anyway.
	if (ReadLock()) {
		info->version = 100;
		sprintf(info->name, "Haiku, Inc. DWindowHWInterface");
		sprintf(info->chipset, "Haiku, Inc. Chipset");
		sprintf(info->serial_no, "3.14159265358979323846");
		info->memory = 134217728;	// 128 MB, not that we really have that much. :)
		info->dac_speed = 0xFFFFFFFF;	// *heh*

		ReadUnlock();
	}

	return B_OK;
}


status_t
DWindowHWInterface::GetFrameBufferConfig(frame_buffer_config& config)
{
	if (fFrontBuffer == NULL)
		return B_ERROR;

	config.frame_buffer = fFrontBuffer->Bits();
	config.frame_buffer_dma = NULL;
	config.bytes_per_row = fFrontBuffer->BytesPerRow();

	return B_OK;
}


status_t
DWindowHWInterface::GetModeList(display_mode** _modes, uint32* _count)
{
	AutoReadLocker _(this);

#if 1
	// setup a whole bunch of different modes
	const struct resolution { int32 width, height; } resolutions[] = {
		{640, 480}, {800, 600}, {1024, 768}, {1152, 864}, {1280, 960},
		{1280, 1024}, {1400, 1050}, {1600, 1200}
	};
	uint32 resolutionCount = sizeof(resolutions) / sizeof(resolutions[0]);
//	const uint32 colors[] = {B_CMAP8, B_RGB15, B_RGB16, B_RGB32};
	uint32 count = resolutionCount/* * 4*/;

	display_mode* modes = new(std::nothrow) display_mode[count];
	if (modes == NULL)
		return B_NO_MEMORY;

	*_modes = modes;
	*_count = count;

	int32 index = 0;
	for (uint32 i = 0; i < resolutionCount; i++) {
		modes[index].virtual_width = resolutions[i].width;
		modes[index].virtual_height = resolutions[i].height;
		modes[index].space = B_RGB32;

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
#else
	// support only a single mode, useful
	// for testing a specific mode
	display_mode *modes = new(std::nothrow) display_mode[1];
	modes[0].virtual_width = 800;
	modes[0].virtual_height = 600;
	modes[0].space = B_BRGB32;

	*_modes = modes;
	*_count = 1;
#endif

	return B_OK;
}


status_t
DWindowHWInterface::GetPixelClockLimits(display_mode* mode, uint32* low,
	uint32* high)
{
	return B_ERROR;
}


status_t
DWindowHWInterface::GetTimingConstraints(
	display_timing_constraints* constraints)
{
	return B_ERROR;
}


status_t
DWindowHWInterface::ProposeMode(display_mode* candidate,
	const display_mode* low, const display_mode* high)
{
	// We should be able to get away with this because we're not dealing with
	// any specific hardware. This is a Good Thing(TM) because we can support
	// any hardware we wish within reasonable expectaions and programmer
	// laziness. :P
	return B_OK;
}


sem_id
DWindowHWInterface::RetraceSemaphore()
{
	return -1;
}


status_t
DWindowHWInterface::WaitForRetrace(bigtime_t timeout)
{
	// Locking shouldn't be necessary here - R5 should handle this for us. :)
	BScreen screen;
	return screen.WaitForRetrace(timeout);
}


status_t
DWindowHWInterface::SetDPMSMode(uint32 state)
{
	AutoWriteLocker _(this);

	return BScreen().SetDPMS(state);
}


uint32
DWindowHWInterface::DPMSMode()
{
	AutoReadLocker _(this);

	return BScreen().DPMSState();
}


uint32
DWindowHWInterface::DPMSCapabilities()
{
	AutoReadLocker _(this);

	return BScreen().DPMSCapabilites();
}


uint32
DWindowHWInterface::AvailableHWAcceleration() const
{
	uint32 flags = 0;

	if (!IsDoubleBuffered()) {
		if (fAccScreenBlit)
			flags |= HW_ACC_COPY_REGION;
		if (fAccFillRect)
			flags |= HW_ACC_FILL_REGION;
		if (fAccInvertRect)
			flags |= HW_ACC_INVERT_REGION;
	}

	return flags;
}


void
DWindowHWInterface::CopyRegion(const clipping_rect* sortedRectList,
	uint32 count, int32 xOffset, int32 yOffset)
{
	if (fAccScreenBlit && fAccAcquireEngine) {
		if (fAccAcquireEngine(B_2D_ACCELERATION, 0xff, &fSyncToken,
				&fEngineToken) >= B_OK) {
			// make sure the blit_params cache is large enough
			if (fBlitParamsCount < count) {
				fBlitParamsCount = (count / kDefaultParamsCount + 1)
					* kDefaultParamsCount;
				// NOTE: realloc() could be used instead...
				blit_params* params
					= new(std::nothrow) blit_params[fBlitParamsCount];
				if (params) {
					delete[] fBlitParams;
					fBlitParams = params;
				} else {
					count = fBlitParamsCount;
				}
			}
			// convert the rects
			for (uint32 i = 0; i < count; i++) {
				fBlitParams[i].src_left
					= (uint16)sortedRectList[i].left + fXOffset;
				fBlitParams[i].src_top
					= (uint16)sortedRectList[i].top + fYOffset;

				fBlitParams[i].dest_left
					= (uint16)sortedRectList[i].left + xOffset + fXOffset;
				fBlitParams[i].dest_top
					= (uint16)sortedRectList[i].top + yOffset + fYOffset;

				// NOTE: width and height are expressed as distance, not count!
				fBlitParams[i].width = (uint16)(sortedRectList[i].right
					- sortedRectList[i].left);
				fBlitParams[i].height = (uint16)(sortedRectList[i].bottom
					- sortedRectList[i].top);
			}

			// go
			fAccScreenBlit(fEngineToken, fBlitParams, count);

			// done
			if (fAccReleaseEngine)
				fAccReleaseEngine(fEngineToken, &fSyncToken);

			// sync
			if (fAccSyncToToken)
				fAccSyncToToken(&fSyncToken);
		}
	}
}


void
DWindowHWInterface::FillRegion(/*const*/ BRegion& region,
	const rgb_color& color, bool autoSync)
{
	if (fAccFillRect && fAccAcquireEngine) {
		if (fAccAcquireEngine(B_2D_ACCELERATION, 0xff, &fSyncToken,
				&fEngineToken) >= B_OK) {
			// convert the region
			uint32 count;
			_RegionToRectParams(&region, &count);

			// go
			fAccFillRect(fEngineToken, _NativeColor(color), fRectParams, count);

			// done
			if (fAccReleaseEngine)
				fAccReleaseEngine(fEngineToken, &fSyncToken);

			// sync
			if (autoSync && fAccSyncToToken)
				fAccSyncToToken(&fSyncToken);
		}
	}
}


void
DWindowHWInterface::InvertRegion(/*const*/ BRegion& region)
{
	if (fAccInvertRect && fAccAcquireEngine) {
		if (fAccAcquireEngine(B_2D_ACCELERATION, 0xff, &fSyncToken,
				&fEngineToken) >= B_OK) {
			// convert the region
			uint32 count;
			_RegionToRectParams(&region, &count);

			// go
			fAccInvertRect(fEngineToken, fRectParams, count);

			// done
			if (fAccReleaseEngine)
				fAccReleaseEngine(fEngineToken, &fSyncToken);

			// sync
			if (fAccSyncToToken)
				fAccSyncToToken(&fSyncToken);

		} else {
			fprintf(stderr, "AcquireEngine failed!\n");
		}
	} else {
		fprintf(stderr, "AccelerantHWInterface::InvertRegion() called, but "
			"hook not available!\n");
	}
}


void
DWindowHWInterface::Sync()
{
	if (fAccSyncToToken)
		fAccSyncToToken(&fSyncToken);
}


RenderingBuffer*
DWindowHWInterface::FrontBuffer() const
{
	return fFrontBuffer;
}


RenderingBuffer*
DWindowHWInterface::BackBuffer() const
{
	return fFrontBuffer;
}


bool
DWindowHWInterface::IsDoubleBuffered() const
{
	return false;
}


status_t
DWindowHWInterface::Invalidate(const BRect& frame)
{
	return HWInterface::Invalidate(frame);
}


void
DWindowHWInterface::SetOffset(int32 left, int32 top)
{
	if (!WriteLock())
		return;

	fXOffset = left;
	fYOffset = top;

	_UpdateFrameBufferConfig();

	// TODO: someone would have to call DrawingEngine::Update() now!

	WriteUnlock();
}


void
DWindowHWInterface::_RegionToRectParams(/*const*/ BRegion* region,
	uint32* count) const
{
	*count = region->CountRects();
	if (fRectParamsCount < *count) {
		fRectParamsCount = (*count / kDefaultParamsCount + 1)
			* kDefaultParamsCount;
		// NOTE: realloc() could be used instead...
		fill_rect_params* params
			= new(std::nothrow) fill_rect_params[fRectParamsCount];
		if (params) {
			delete[] fRectParams;
			fRectParams = params;
		} else {
			*count = fRectParamsCount;
		}
	}

	for (uint32 i = 0; i < *count; i++) {
		clipping_rect r = region->RectAtInt(i);
		fRectParams[i].left = (uint16)r.left + fXOffset;
		fRectParams[i].top = (uint16)r.top + fYOffset;
		fRectParams[i].right = (uint16)r.right + fXOffset;
		fRectParams[i].bottom = (uint16)r.bottom + fYOffset;
	}
}


uint32
DWindowHWInterface::_NativeColor(const rgb_color& color) const
{
	// NOTE: This functions looks somehow suspicios to me.
	// It assumes that all graphics cards have the same native endianess, no?
	switch (fDisplayMode.space) {
		case B_CMAP8:
		case B_GRAY8:
			return RGBColor(color).GetColor8();

		case B_RGB15_BIG:
		case B_RGBA15_BIG:
		case B_RGB15_LITTLE:
		case B_RGBA15_LITTLE:
			return RGBColor(color).GetColor15();

		case B_RGB16_BIG:
		case B_RGB16_LITTLE:
			return RGBColor(color).GetColor16();

		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		case B_RGB32_LITTLE:
		case B_RGBA32_LITTLE: {
			uint32 native = (color.alpha << 24) | (color.red << 16)
				| (color.green << 8) | (color.blue);
			return native;
		}
	}
	return 0;
}
