//------------------------------------------------------------------------------
//
// Copyright 2002-2005, Haiku, Inc.
// Distributed under the terms of the MIT License.
//
//
//	File Name:		AccelerantHWInterface.cpp
//	Authors:		Michael Lotz <mmlr@mlotz.ch>
//					DarkWyrm <bpmagic@columbus.rr.com>
//					Stephan Aßmus <superstippi@gmx.de>
//	Description:	Accelerant based HWInterface implementation
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

#include <Accelerant.h>
#include <graphic_driver.h>
#include <FindDirectory.h>
#include <image.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "PortLink.h"
#include "ServerConfig.h"
#include "ServerCursor.h"
#include "ServerProtocol.h"
#include "UpdateQueue.h"

#include "AccelerantHWInterface.h"
#include "AccelerantBuffer.h"
#include "BitmapBuffer.h"


#ifdef DEBUG_DRIVER_MODULE
#	include <stdio.h>
#	define ATRACE(x) printf x
#else
#	define ATRACE(x) ;
#endif

const unsigned char kEmptyCursor[] = { 16, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

enum {
	MSG_UPDATE = 'updt',
};

// constructor
AccelerantHWInterface::AccelerantHWInterface()
	:	HWInterface(),
		fCardFD(-1),
		fAccelerantImage(-1),
		fAccelerantHook(NULL),
		fEngineToken(NULL),
		
		// required hooks
		fAccAcquireEngine(NULL),
		fAccReleaseEngine(NULL),
		fAccGetModeCount(NULL),
		fAccGetModeList(NULL),
		fAccGetFrameBufferConfig(NULL),
		fAccSetDisplayMode(NULL),
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
		
		// dpms hooks
		fAccDPMSCapabilities(NULL),
		fAccDPMSMode(NULL),
		fAccSetDPMSMode(NULL),		
		
		fModeCount(0),
		fModeList(NULL),
		
		fBackBuffer(NULL),
		fFrontBuffer(new AccelerantBuffer())
{
	fDisplayMode.virtual_width = 640;
	fDisplayMode.virtual_height = 480;
	fDisplayMode.space = B_RGB32;
}

// destructor
AccelerantHWInterface::~AccelerantHWInterface()
{
	delete fBackBuffer;
	delete fFrontBuffer;
	delete fModeList;
}

// Initialize
status_t
AccelerantHWInterface::Initialize()
{
	char path[PATH_MAX];
	
	fCardFD = OpenGraphicsDevice(1);
	if (fCardFD < 0) {
		ATRACE(("Failed to open graphics device\n"));
		return B_ERROR;
	}
	
	char signature[1024];
	if (ioctl(fCardFD, B_GET_ACCELERANT_SIGNATURE, &signature, sizeof(signature)) != B_OK) {
		close(fCardFD);
		return B_ERROR;
	}
	
	ATRACE(("accelerant signature is: %s\n", signature));
	
	struct stat accelerant_stat;
	const static directory_which dirs[] = {
		B_USER_ADDONS_DIRECTORY,
		B_COMMON_ADDONS_DIRECTORY,
		B_BEOS_ADDONS_DIRECTORY
	};
	
	fAccelerantImage = -1;
	for (int32 i = 0; i < 3; i++) {
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
				ATRACE(("unable to get B_ACCELERANT_ENTRY_POINT\n"));
				unload_add_on(fAccelerantImage);
				fAccelerantImage = -1;
				return B_ERROR;
			}
			
			init_accelerant InitAccelerant;
			InitAccelerant = (init_accelerant)fAccelerantHook(B_INIT_ACCELERANT, NULL);
			if (!InitAccelerant || InitAccelerant(fCardFD) != B_OK) {
				// TODO: continue / go on to the next graphics card?
				ATRACE(("InitAccelerant unsuccessful\n"));
				unload_add_on(fAccelerantImage);
				fAccelerantImage = -1;
				return B_ERROR;
			}
			
			break;
		}
	}
	
	if (fAccelerantImage < 0)
		return B_ERROR;
	
	if (SetupDefaultHooks() != B_OK) {
		ATRACE(("cannot setup default hooks\n"));
		return B_ERROR;
	}
	
	return B_OK;
}

/*!
	\brief Opens a graphics device for read-write access
	\param deviceNumber Number identifying which graphics card to open (1 for first card)
	\return The file descriptor for the opened graphics device
	
	The deviceNumber is relative to the number of graphics devices that can be successfully
	opened.  One represents the first card that can be successfully opened (not necessarily
	the first one listed in the directory).
*/
int
AccelerantHWInterface::OpenGraphicsDevice(int deviceNumber)
{
	DIR *directory = opendir("/dev/graphics");
	if (!directory)
		return -1;
	
	int count = 0;
	struct dirent *entry;
	int current_card_fd = -1;
	char path[PATH_MAX];
	while ((count < deviceNumber) && ((entry = readdir(directory)) != NULL)) {
		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..") ||
			!strcmp(entry->d_name, "stub"))
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
	
	// open stub if we were not able to get a "real" card
	if (count < deviceNumber) {
		if (deviceNumber == 1) {
			sprintf(path, "/dev/graphics/stub");
			current_card_fd = open(path, B_READ_WRITE);
		} else {
			close(current_card_fd);
			current_card_fd = -1;
		}
	}
	
	return current_card_fd;
}

status_t
AccelerantHWInterface::SetupDefaultHooks()
{
	// required
	fAccAcquireEngine = (acquire_engine)fAccelerantHook(B_ACQUIRE_ENGINE, NULL);
	fAccReleaseEngine = (release_engine)fAccelerantHook(B_RELEASE_ENGINE, NULL);
	fAccGetModeCount = (accelerant_mode_count)fAccelerantHook(B_ACCELERANT_MODE_COUNT, NULL);
	fAccGetModeList = (get_mode_list)fAccelerantHook(B_GET_MODE_LIST, NULL);
	fAccGetFrameBufferConfig = (get_frame_buffer_config)fAccelerantHook(B_GET_FRAME_BUFFER_CONFIG, NULL);
	fAccSetDisplayMode = (set_display_mode)fAccelerantHook(B_SET_DISPLAY_MODE, NULL);
	fAccGetPixelClockLimits = (get_pixel_clock_limits)fAccelerantHook(B_GET_PIXEL_CLOCK_LIMITS, NULL);
	
	if (!fAccAcquireEngine || !fAccReleaseEngine || !fAccGetFrameBufferConfig
		|| !fAccGetModeCount || !fAccGetModeList || !fAccSetDisplayMode
		|| !fAccGetPixelClockLimits)
		return B_ERROR;
	
	// optional
	fAccGetTimingConstraints = (get_timing_constraints)fAccelerantHook(B_GET_TIMING_CONSTRAINTS, NULL);
	fAccProposeDisplayMode = (propose_display_mode)fAccelerantHook(B_PROPOSE_DISPLAY_MODE, NULL);
	
	fAccFillRect = (fill_rectangle)fAccelerantHook(B_FILL_RECTANGLE, NULL);
	fAccInvertRect = (invert_rectangle)fAccelerantHook(B_INVERT_RECTANGLE, NULL);
	fAccScreenBlit = (screen_to_screen_blit)fAccelerantHook(B_SCREEN_TO_SCREEN_BLIT, NULL);
	
	fAccSetCursorShape = (set_cursor_shape)fAccelerantHook(B_SET_CURSOR_SHAPE, NULL);
	fAccMoveCursor = (move_cursor)fAccelerantHook(B_MOVE_CURSOR, NULL);
	fAccShowCursor = (show_cursor)fAccelerantHook(B_SHOW_CURSOR, NULL);
	
	// dpms
	fAccDPMSCapabilities = (dpms_capabilities)fAccelerantHook(B_DPMS_CAPABILITIES, NULL);
	fAccDPMSMode = (dpms_mode)fAccelerantHook(B_DPMS_MODE, NULL);
	fAccSetDPMSMode = (set_dpms_mode)fAccelerantHook(B_SET_DPMS_MODE, NULL);
	
	return B_OK;
}

// Shutdown
status_t
AccelerantHWInterface::Shutdown()
{
	if (fAccelerantHook) {
		uninit_accelerant UninitAccelerant = (uninit_accelerant)fAccelerantHook(B_UNINIT_ACCELERANT, NULL);
		if (UninitAccelerant)
			UninitAccelerant();
	}
	
	if (fAccelerantImage >= 0)
		unload_add_on(fAccelerantImage);
	
	if (fCardFD >= 0)
		close(fCardFD);
	
	return B_OK;
}

// SetMode
status_t
AccelerantHWInterface::SetMode(const display_mode &mode)
{
	// prevent from doing the unnecessary
	if (fModeCount > 0 && fBackBuffer && fFrontBuffer
		&& fDisplayMode.virtual_width == mode.virtual_width
		&& fDisplayMode.virtual_height == mode.virtual_height
		&& fDisplayMode.space == mode.space) {
		return B_OK;
	}
	
	// TODO: check if the mode is valid even (ie complies to the modes we said we would support)
	// or else ret = B_BAD_VALUE
	
	if (fModeCount <= 0 || !fModeList) {
		if (UpdateModeList() != B_OK || fModeCount <= 0) {
			ATRACE(("unable to update mode list\n"));
			return B_ERROR;
		}
	}
	
	for (int32 i = 0; i < fModeCount; i++) {
		if (fModeList[i].virtual_width == mode.virtual_width
			&& fModeList[i].virtual_height == mode.virtual_height
			&& fModeList[i].space == mode.space) {
			
			fDisplayMode = fModeList[i];
			break;
		}
	}
	
	if (fAccSetDisplayMode(&fDisplayMode) != B_OK) {
		ATRACE(("setting display mode failed\n"));
		return B_ERROR;
	}
	
	// update frontbuffer
	fFrontBuffer->SetDisplayMode(fDisplayMode);
	if (UpdateFrameBufferConfig() != B_OK)
		return B_ERROR;
	
	// update backbuffer if neccessary
	if (!fBackBuffer || fBackBuffer->Width() != fDisplayMode.virtual_width
		|| fBackBuffer->Height() != fDisplayMode.virtual_height) {
		// NOTE: backbuffer is always B_RGBA32, this simplifies the
		// drawing backend implementation tremendously for the time
		// being. The color space conversion is handled in CopyBackToFront()
		BRect bounds(0, 0, fDisplayMode.virtual_width, fDisplayMode.virtual_height);
		BBitmap *backBitmap = new BBitmap(bounds, 0, B_RGBA32);
		
		delete fBackBuffer;
		fBackBuffer = new BitmapBuffer(backBitmap);
		
		if (fBackBuffer->InitCheck() != B_OK) {
			delete fBackBuffer;
			fBackBuffer = NULL;
			return B_ERROR;
		}
		
		// clear out backbuffer, alpha is 255 this way
		// TODO: maybe this should handle different color spaces in different
		//		 ways
		memset(backBitmap->Bits(), 255, backBitmap->BitsLength());
	}
	
	return B_OK;
}

void
AccelerantHWInterface::GetMode(display_mode *mode)
{
	*mode = fDisplayMode;
}

status_t
AccelerantHWInterface::UpdateModeList()
{
	fModeCount = fAccGetModeCount();
	if (fModeCount <= 0)
		return B_ERROR;
	
	fModeList = new display_mode[fModeCount];;
	if (!fModeList)
		return B_NO_MEMORY;
	
	if (fAccGetModeList(fModeList) != B_OK) {
		ATRACE(("unable to get mode list\n"));
		return B_ERROR;
	}
	
	return B_OK;
}

status_t
AccelerantHWInterface::UpdateFrameBufferConfig()
{
	if (fAccGetFrameBufferConfig(&fFrameBufferConfig) != B_OK) {
		ATRACE(("unable to get frame buffer config\n"));
		return B_ERROR;
	}
	
	fFrontBuffer->SetFrameBufferConfig(fFrameBufferConfig);
	return B_OK;
}

// GetDeviceInfo
status_t
AccelerantHWInterface::GetDeviceInfo(accelerant_device_info *info)
{
	get_accelerant_device_info GetAccelerantDeviceInfo = (get_accelerant_device_info)fAccelerantHook(B_GET_ACCELERANT_DEVICE_INFO, NULL);
	if (!GetAccelerantDeviceInfo) {
		ATRACE(("No B_GET_ACCELERANT_DEVICE_INFO hook found\n"));
		return B_UNSUPPORTED;
	}
	
	return GetAccelerantDeviceInfo(info);
}

// GetModeList
status_t
AccelerantHWInterface::GetModeList(display_mode **modes, uint32 *count)
{
	if (!count || !modes)
		return B_BAD_VALUE;
	
	*modes = new display_mode[fModeCount];
	*count = fModeCount;
	
	memcpy(*modes, fModeList, sizeof(display_mode) * fModeCount);
	return B_OK;
}

status_t
AccelerantHWInterface::GetPixelClockLimits(display_mode *mode, uint32 *low, uint32 *high)
{
	if (!mode || !low || !high)
		return B_BAD_VALUE;
	
	return fAccGetPixelClockLimits(mode, low, high);
}

status_t
AccelerantHWInterface::GetTimingConstraints(display_timing_constraints *dtc)
{
	if (!dtc)
		return B_BAD_VALUE;
	
	if (fAccGetTimingConstraints)
		return fAccGetTimingConstraints(dtc);
	
	return B_UNSUPPORTED;
}

status_t
AccelerantHWInterface::ProposeMode(display_mode *candidate, const display_mode *low, const display_mode *high)
{
	if (!candidate || !low || !high)
		return B_BAD_VALUE;
	
	if (!fAccProposeDisplayMode)
		return B_UNSUPPORTED;
	
	// avoid const issues
	display_mode this_high, this_low;
	this_high = *high;
	this_low = *low;
	
	return fAccProposeDisplayMode(candidate, &this_low, &this_high);
}

// SetDPMSMode
status_t
AccelerantHWInterface::SetDPMSMode(const uint32 &state)
{
	if (!fAccSetDPMSMode)
		return B_UNSUPPORTED;
	
	return fAccSetDPMSMode(state);
}

// DPMSMode
uint32
AccelerantHWInterface::DPMSMode() const
{
	if (!fAccDPMSMode)
		return B_UNSUPPORTED;
	
	return fAccDPMSMode();
}

// DPMSCapabilities
uint32
AccelerantHWInterface::DPMSCapabilities() const
{
	if (!fAccDPMSCapabilities)
		return B_UNSUPPORTED;
	
	return fAccDPMSCapabilities();
}

// WaitForRetrace
status_t
AccelerantHWInterface::WaitForRetrace(bigtime_t timeout = B_INFINITE_TIMEOUT)
{
	accelerant_retrace_semaphore AccelerantRetraceSemaphore = (accelerant_retrace_semaphore)fAccelerantHook(B_ACCELERANT_RETRACE_SEMAPHORE, NULL);
	if (!AccelerantRetraceSemaphore)
		return B_UNSUPPORTED;
	
	sem_id sem = AccelerantRetraceSemaphore();
	if (sem < 0)
		return B_ERROR;
	
	return acquire_sem_etc(sem, 1, B_RELATIVE_TIMEOUT, timeout);
}

// FrontBuffer
RenderingBuffer *
AccelerantHWInterface::FrontBuffer() const
{
	return fFrontBuffer;
}

// BackBuffer
RenderingBuffer *
AccelerantHWInterface::BackBuffer() const
{
	return fBackBuffer;
}

// _DrawCursor
void
AccelerantHWInterface::_DrawCursor(BRect area) const
{
	return;
#if 0
	BRect cf = _CursorFrame();
	if (cf.IsValid() && area.Intersects(cf)) {
		// clip to common area
		area = area & cf;

		int32 left = (int32)floorf(area.left);
		int32 top = (int32)floorf(area.top);
		int32 right = (int32)ceilf(area.right);
		int32 bottom = (int32)ceilf(area.bottom);
		int32 width = right - left + 1;
		int32 height = bottom - top + 1;

		// make a bitmap from the backbuffer
		// that has the cursor blended on top of it

		// blending buffer
		uint8* buffer = new uint8[width * height * 4];

		// offset into back buffer
		uint8* src = (uint8*)fBackBuffer->Bits();
		uint32 srcBPR = fBackBuffer->BytesPerRow();
		src += top * srcBPR + left * 4;

		// offset into cursor bitmap
		uint8* crs = (uint8*)fCursor->Bits();
		uint32 crsBPR = fCursor->BytesPerRow();
		// since area is clipped to cf,
		// the diff between top and cf.top is always positive,
		// same for diff between left and cf.left
		crs += (top - (int32)floorf(cf.top)) * crsBPR
				+ (left - (int32)floorf(cf.left)) * 4;

		uint8* dst = buffer;

		// blending
		for (int32 y = top; y <= bottom; y++) {
			uint8* s = src;
			uint8* c = crs;
			uint8* d = dst;
			for (int32 x = left; x <= right; x++) {
				// assume backbuffer alpha = 255
				// TODO: it appears alpha in cursor us upside down
				uint8 a = 255 - c[3];
				d[0] = (((s[0] - c[0]) * a) + (c[0] << 8)) >> 8;
				d[1] = (((s[1] - c[1]) * a) + (c[1] << 8)) >> 8;
				d[2] = (((s[2] - c[2]) * a) + (c[2] << 8)) >> 8;
				d[3] = 255;
				s += 4;
				c += 4;
				d += 4;
			}
			crs += crsBPR;
			src += srcBPR;
			dst += width * 4;
		}

		// copy result to front buffer
		_CopyToFront(buffer, width * 4, left, top, right, bottom);

		delete[] buffer;
	}
#endif
}

/*void AccelerantHWInterface::CopyBitmap(ServerBitmap *bitmap, const BRect &source, const BRect &dest, const DrawData *d)
{
	if(!is_initialized || !bitmap || !d)
	{
		printf("CopyBitmap returned - not init or NULL bitmap\n");
		return;
	}
	
	// DON't set draw data here! your existing clipping region will be deleted
//	SetDrawData(d);
	
	// Oh, wow, is this going to be slow. Then again, AccelerantHWInterface was never meant to be very fast. It could
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

void AccelerantHWInterface::CopyToBitmap(ServerBitmap *destbmp, const BRect &sourcerect)
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

void AccelerantHWInterface::ConstrainClippingRegion(BRegion *reg)
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

bool AccelerantHWInterface::AcquireBuffer(FBBitmap *bmp)
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

void AccelerantHWInterface::ReleaseBuffer()
{
	if(!is_initialized)
		return;
	framebuffer->Unlock();
	screenwin->Unlock();
}

void AccelerantHWInterface::Invalidate(const BRect &r)
{
	if(!is_initialized)
		return;
	
	screenwin->Lock();
	screenwin->view->Draw(r);
	screenwin->Unlock();
}
*/
