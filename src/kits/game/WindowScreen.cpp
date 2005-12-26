/* 
 * Copyright 2002-2005,
 * Marcus Overhagen, 
 * Stefano Ceccherini (burton666@libero.it),
 * Carwyn Jones (turok2@currantbun.com)
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Application.h>
#include <Screen.h>
#include <String.h>
#include <WindowScreen.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <input_globals.h>
#include <InputServerTypes.h> // For IS_SET_MOUSE_POSITION
#include <WindowPrivate.h>

#include <AppServerLink.h>
#include <ServerProtocol.h>

using BPrivate::AppServerLink;


#define TRACE_WINDOWSCREEN 1
#if TRACE_WINDOWSCREEN
#define CALLED() printf("%s\n", __PRETTY_FUNCTION__);
#else
#define CALLED() ;
#endif


// Acceleration hooks pointers
static fill_rectangle sFillRectHook;
static screen_to_screen_blit sBlitRectHook;
static screen_to_screen_transparent_blit sTransparentBlitHook;
static screen_to_screen_scaled_filtered_blit sScaledFilteredBlitHook;
static wait_engine_idle sWaitIdleHook;
static acquire_engine sAcquireEngineHook;
static release_engine sReleaseEngineHook;

static engine_token *sEngineToken;


// Helper methods which translates the pre r5 graphics methods to r5 ones
static int32
card_sync()
{
	sWaitIdleHook();
	return 0;
}


static int32
blit(int32 sx, int32 sy, int32 dx, int32 dy, int32 width, int32 height)
{
	blit_params param;
	param.src_left = sx;
	param.src_top = sy;
	param.dest_left = dx;
	param.dest_top = dy;
	param.width = width;
	param.height = height;
	
	sAcquireEngineHook(B_2D_ACCELERATION, 0xff, NULL, &sEngineToken);
	sBlitRectHook(sEngineToken, &param, 1);
	sReleaseEngineHook(sEngineToken, NULL);
	return 0;
}

// TODO: This function seems not to be exported through CardHookAt().
// At least, nothing I've tried uses it. 
/*
static int32
transparent_blit(int32 sx, int32 sy, int32 dx, int32 dy,
			int32 width, int32 height, uint32 transparent_color)
{
	blit_params param;
	param.src_left = sx;
	param.src_top = sy;
	param.dest_left = dx;
	param.dest_top = dy;
	param.width = width;
	param.height = height;
	
	sAcquireEngineHook(B_2D_ACCELERATION, 0xff, 0, &sEngineToken);
	sTransparentBlitHook(sEngineToken, transparent_color, &param, 1);
	sReleaseEngineHook(sEngineToken, 0);
	return 0;
}
*/

static int32
scaled_filtered_blit(int32 sx, int32 sy, int32 sw, int32 sh, int32 dx, int32 dy, int32 dw, int32 dh)
{
	scaled_blit_params param;
	param.src_left = sx;
	param.src_top = sy;
	param.src_width = sw;
	param.src_height = sh;
	param.dest_left = dx;
	param.dest_top = dy;
	param.dest_width = dw;
	param.dest_height = dh;
	
	sAcquireEngineHook(B_2D_ACCELERATION, 0xff, NULL, &sEngineToken);
	sScaledFilteredBlitHook(sEngineToken, &param, 1);
	sReleaseEngineHook(sEngineToken, NULL);
	return 0;
}


static int32
draw_rect_8(int32 sx, int32 sy, int32 sw, int32 sh, uint8 color_index)
{
	fill_rect_params param;
	param.left = sx;
	param.top = sy;
	param.right = sw;
	param.bottom = sh;
	
	sAcquireEngineHook(B_2D_ACCELERATION, 0xff, NULL, &sEngineToken);
	sFillRectHook(sEngineToken, color_index, &param, 1);
	sReleaseEngineHook(sEngineToken, NULL);
	return 0;
}


static int32
draw_rect_16(int32 sx, int32 sy, int32 sw, int32 sh, uint16 color)
{
	fill_rect_params param;
	param.left = sx;
	param.top = sy;
	param.right = sw;
	param.bottom = sh;
	
	sAcquireEngineHook(B_2D_ACCELERATION, 0xff, NULL, &sEngineToken);
	sFillRectHook(sEngineToken, color, &param, 1);
	sReleaseEngineHook(sEngineToken, NULL);
	return 0;
}


static int32
draw_rect_32(int32 sx, int32 sy, int32 sw, int32 sh, uint32 color)
{
	fill_rect_params param;
	param.left = sx;
	param.top = sy;
	param.right = sw;
	param.bottom = sh;
	
	sAcquireEngineHook(B_2D_ACCELERATION, 0xff, NULL, &sEngineToken);
	sFillRectHook(sEngineToken, color, &param, 1);
	sReleaseEngineHook(sEngineToken, NULL);
	return 0;
}


static void
mode2parms(uint32 space, uint32 *out_space, int32 *width, int32 *height)
{
	switch (space) {
		case B_8_BIT_640x480:
			*out_space = B_CMAP8;
			*width = 640; *height = 480;
			break;
		case B_8_BIT_800x600:
			*out_space = B_CMAP8;
			*width = 800; *height = 600;
			break;
		case B_8_BIT_1024x768:
			*out_space = B_CMAP8;
			*width = 1024; *height = 768;
			break;
		case B_8_BIT_1280x1024:
			*out_space = B_CMAP8;
			*width = 1280; *height = 1024;
			break;
		case B_8_BIT_1600x1200:
			*out_space = B_CMAP8;
			*width = 1600; *height = 1200;
			break;
		case B_16_BIT_640x480:
			*out_space = B_RGB16;
			*width = 640; *height = 480;
			break;
		case B_16_BIT_800x600:
			*out_space = B_RGB16;
			*width = 800; *height = 600;
			break;
		case B_16_BIT_1024x768:
			*out_space = B_RGB16;
			*width = 1024; *height = 768;
			break;
		case B_16_BIT_1280x1024:
			*out_space = B_RGB16;
			*width = 1280; *height = 1024;
			break;
		case B_16_BIT_1600x1200:
			*out_space = B_RGB16;
			*width = 1600; *height = 1200;
			break;
		case B_32_BIT_640x480:
			*out_space = B_RGB32;
			*width = 640; *height = 480;
			break;
		case B_32_BIT_800x600:
			*out_space = B_RGB32;
			*width = 800; *height = 600;
			break;
		case B_32_BIT_1024x768:
			*out_space = B_RGB32;
			*width = 1024; *height = 768;
			break;
		case B_32_BIT_1280x1024:
			*out_space = B_RGB32;
			*width = 1280; *height = 1024;
			break;
		case B_32_BIT_1600x1200:
			*out_space = B_RGB32;
			*width = 1600; *height = 1200;
			break;
    		case B_8_BIT_1152x900:
    			*out_space = B_CMAP8;
    			*width = 1152; *height = 900;
    			break;
    		case B_16_BIT_1152x900:
    			*out_space = B_RGB16;
    			*width = 1152; *height = 900;
    			break;
    		case B_32_BIT_1152x900:
    			*out_space = B_RGB32;
    			*width = 1152; *height = 900;
    			break;
		case B_15_BIT_640x480:
			*out_space = B_RGB15;
			*width = 640; *height = 480;
			break;
		case B_15_BIT_800x600:
			*out_space = B_RGB15;
			*width = 800; *height = 600;
			break;
		case B_15_BIT_1024x768:
			*out_space = B_RGB15;
			*width = 1024; *height = 768;
			break;
		case B_15_BIT_1280x1024:
			*out_space = B_RGB15;
			*width = 1280; *height = 1024;
			break;
		case B_15_BIT_1600x1200:
			*out_space = B_RGB15;
			*width = 1600; *height = 1200;
			break;
    		case B_15_BIT_1152x900:
    			*out_space = B_RGB15;
    			*width = 1152; *height = 900;
    			break;
	}
}


//	#pragma mark - public API calls


void
set_mouse_position(int32 x, int32 y)
{
	BMessage command(IS_SET_MOUSE_POSITION);
	BMessage reply;
	
	command.AddPoint("where", BPoint(x, y));
	_control_input_server_(&command, &reply);
}


//	#pragma mark -


BWindowScreen::BWindowScreen(const char *title, uint32 space,
				status_t *error, bool debug_enable)
	: BWindow(BScreen().Frame().InsetByCopy(200, 200), title, B_TITLED_WINDOW,
		kWindowScreenFlag | B_NOT_MINIMIZABLE | B_NOT_CLOSABLE
			| B_NOT_ZOOMABLE | B_NOT_MOVABLE | B_NOT_RESIZABLE, B_CURRENT_WORKSPACE)
{
	CALLED();
	uint32 attributes = 0;
	if (debug_enable)
		attributes |= B_ENABLE_DEBUGGER;

	status_t status = InitData(space, attributes);	
	if (error)
		*error = status;
}


BWindowScreen::BWindowScreen(const char *title, uint32 space,
				uint32 attributes, status_t *error)
	: BWindow(BScreen().Frame().InsetByCopy(200, 200), title, B_TITLED_WINDOW, 
		kWindowScreenFlag | B_NOT_MINIMIZABLE | B_NOT_CLOSABLE
			| B_NOT_ZOOMABLE | B_NOT_MOVABLE | B_NOT_RESIZABLE, B_CURRENT_WORKSPACE)
{
	CALLED();
	status_t status = InitData(space, attributes);	
	if (error)
		*error = status;
}


BWindowScreen::~BWindowScreen()
{
	CALLED();
	Disconnect();
	if (fAddonImage >= 0)
		unload_add_on(fAddonImage);
	
	delete_sem(fActivateSem);
	delete_sem(fDebugSem);
	
	if (fDebugState)
		activate_workspace(fDebugWorkspace);
	
	free(fDisplayMode);
	free(fOldDisplayMode);
	free(fModeList);
}


void
BWindowScreen::Quit(void)
{
	CALLED();
	Disconnect();
	BWindow::Quit();
}


void
BWindowScreen::ScreenConnected(bool active)
{
	// Implemented in subclasses
}


void
BWindowScreen::Disconnect()
{
	CALLED();
	if (fLockState == 1) {
		if (fDebugState)
			fDebugFirst = true;
		SetActiveState(0);
	}
}


void
BWindowScreen::WindowActivated(bool active)
{
	CALLED();
	fWindowState = active;
	if(active && fLockState == 0 && fWorkState)
		SetActiveState(1);
}


void
BWindowScreen::WorkspaceActivated(int32 ws, bool state)
{
	CALLED();
	fWorkState = state;
	if (state) {
		if (fLockState == 0 && fWindowState) {
			SetActiveState(1);
			if (!IsHidden()) {
				Activate(true);
				WindowActivated(true);
			}
		}
	} else if (fLockState)
		SetActiveState(0);
}


void
BWindowScreen::ScreenChanged(BRect screen_size, color_space depth)
{
	// Implemented in subclasses
}


void
BWindowScreen::Hide()
{
	CALLED();
	Disconnect();	
	BWindow::Hide();
}


void
BWindowScreen::Show()
{
	CALLED();
	BWindow::Show();
	if (!fActivateState) {
		release_sem(fActivateSem);
		fActivateState = true;
	}
}


void
BWindowScreen::SetColorList(rgb_color *list, int32 first_index, int32 last_index)
{
	CALLED();
	if (first_index < 0 || last_index > 255 || first_index > last_index)
		return;

	if (Lock()) {
		if (!fActivateState) { 
			//If we aren't active, we just change our local palette
			for (int32 x = first_index; x <= last_index; x++)
				fColorList[x] = list[x];
		} else {
			uint32 colorCount = last_index - first_index + 1;
			if (fAddonImage >= 0) {
				set_indexed_colors sic = (set_indexed_colors)fGetAccelerantHook(B_SET_INDEXED_COLORS, NULL);
				if (sic != NULL)
					sic(colorCount, first_index, (uint8 *)fColorList, 0);
			}
	
			BScreen screen(this);
			screen.WaitForRetrace();	
			// Update our local palette too. We're doing this between two 
			// WaitForRetrace() calls so nobody will notice.
			for (int32 x = first_index; x <= last_index; x++)
				fColorList[x] = list[x];
		
			// TODO: Tell the app_server about our changes

			screen.WaitForRetrace();
		}
	
		Unlock();
	}
}


status_t
BWindowScreen::SetSpace(uint32 space)
{
	CALLED();
	display_mode mode;
	status_t status = GetModeFromSpace(space, &mode);
	if (status == B_OK)
		status = AssertDisplayMode(&mode);
	return status;
}


bool
BWindowScreen::CanControlFrameBuffer()
{
	return (fAddonImage >= 0 && (fCardInfo.flags & B_FRAME_BUFFER_CONTROL));
}


status_t
BWindowScreen::SetFrameBuffer(int32 width, int32 height)
{
	CALLED();
	display_mode highMode = *fDisplayMode;
	highMode.flags |= B_SCROLL;
	
	highMode.virtual_height = (int16)height;
	highMode.virtual_width = (int16)width;

	display_mode lowMode = highMode;
	display_mode mode = highMode;

	BScreen screen(this);
	status_t status = screen.ProposeMode(&mode, &lowMode, &highMode);
	if (status == B_OK)
		status = AssertDisplayMode(&mode);

	return status;
}


status_t
BWindowScreen::MoveDisplayArea(int32 x, int32 y)
{
	CALLED();
	move_display_area moveDisplayArea = (move_display_area)fGetAccelerantHook(B_MOVE_DISPLAY, NULL);
	if (moveDisplayArea && moveDisplayArea((int16)x, (int16)y) == B_OK) {	
		fFrameBufferInfo.display_x = x;
		fFrameBufferInfo.display_y = y;
		fDisplayMode->h_display_start = x;
		fDisplayMode->v_display_start = y;
		return B_OK;
	}
	return B_ERROR;
}


void *
BWindowScreen::IOBase()
{
	// Not supported
	return NULL;
}


rgb_color *
BWindowScreen::ColorList()
{
	CALLED();
	return fColorList;
}


frame_buffer_info *
BWindowScreen::FrameBufferInfo()
{
	CALLED();	
	return &fFrameBufferInfo;
}


graphics_card_hook
BWindowScreen::CardHookAt(int32 index)
{
	CALLED();
	if (fAddonImage < 0)
		return NULL;
	
	graphics_card_hook hook = NULL;
	
	switch (index) {
		case 5: // 8 bit fill rect
			hook = (graphics_card_hook)draw_rect_8;
			break;
		case 6: // 32 bit fill rect
			hook = (graphics_card_hook)draw_rect_32;
			break;
		case 7: // screen to screen blit
			hook = (graphics_card_hook)blit;
			break;
		case 8: // screen to screen scaled filtered blit
			hook = (graphics_card_hook)scaled_filtered_blit;
			break;
		case 10: // sync aka wait for graphics card idle
			hook = (graphics_card_hook)card_sync;
			break;
		case 13: // 16 bit fill rect
			hook = (graphics_card_hook)draw_rect_16;
			break;
		default:
			break;
	}
	
	return hook;
}


graphics_card_info *
BWindowScreen::CardInfo()
{
	CALLED();
	return &fCardInfo;
}


void
BWindowScreen::RegisterThread(thread_id id)
{
	CALLED();
	while (acquire_sem(fDebugSem) == B_INTERRUPTED)
		;
	void *newDebugList = realloc(fDebugList, (fDebugListCount + 1) * sizeof(thread_id));
	if (newDebugList != NULL) {
		fDebugList = (thread_id *)newDebugList;	
		fDebugList[fDebugListCount] = id;
		fDebugListCount++;
	}
	release_sem(fDebugSem);
}


void
BWindowScreen::SuspensionHook(bool active)
{
	// Implemented in subclasses
}


void
BWindowScreen::Suspend(char *label)
{
	CALLED();
	if (fDebugState) {		
		fprintf(stderr, "## Debugger(\"%s\").", label);
		fprintf(stderr, " Press Alt-F%ld or Cmd-F%ld to resume.\n", space0 + 1, space0 + 1);
		
		if (IsLocked())
			Unlock();
		
		activate_workspace(fDebugWorkspace);

		// Suspend ourself
		suspend_thread(find_thread(NULL));

		Lock();
	}
}


status_t
BWindowScreen::Perform(perform_code d, void *arg)
{
	return inherited::Perform(d, arg);
}


// Reserved for future binary compatibility
void BWindowScreen::_ReservedWindowScreen1() {}
void BWindowScreen::_ReservedWindowScreen2() {}
void BWindowScreen::_ReservedWindowScreen3() {}
void BWindowScreen::_ReservedWindowScreen4() {}


/* unimplemented for protection of the user:
 *
 * BWindowScreen::BWindowScreen()
 * BWindowScreen::BWindowScreen(BWindowScreen &)
 * BWindowScreen &BWindowScreen::operator=(BWindowScreen &)
 */


BRect
BWindowScreen::CalcFrame(int32 index, int32 space, display_mode *dmode)
{
	CALLED();
	BScreen screen;	
	if (dmode)
		screen.GetMode(dmode);

	return screen.Frame();
}


status_t
BWindowScreen::InitData(uint32 space, uint32 attributes)
{
	CALLED();
	
	fDebugState = attributes & B_ENABLE_DEBUGGER;
	fDebugListCount = 0;
	fDebugList = NULL;
	fDebugFirst = true;
	
	fAttributes = attributes;	
	fScreenIndex = fDebugWorkspace = current_workspace();
	fLockState = 0;
	fAddonImage = -1;
	fWindowState = 0;

	BScreen screen(this);	
	status_t status = screen.GetModeList(&fModeList, &fModeCount);
	if (status < B_OK)
		return status;
	
	display_mode newMode;
	status = GetModeFromSpace(space, &newMode);
	if (status < B_OK)
		return status;

	space_mode = 1;
	space0 = 0;
	
	memcpy(fColorList, screen.ColorMap()->color_list, 256);
	
	status = GetCardInfo();
	if (status < B_OK)
		return status;
	
	fActivateSem = create_sem(0, "WindowScreen start lock");
	fActivateState = 0;
	
	fDebugSem = create_sem(1, "WindowScreen debug sem");
	fOldDisplayMode = (display_mode *)calloc(1, sizeof(display_mode));
	fDisplayMode = (display_mode *)calloc(1, sizeof(display_mode));
	memcpy(fDisplayMode, &newMode, sizeof(display_mode));

	fWorkState = 1;
		
	return B_OK;
}


status_t
BWindowScreen::SetActiveState(int32 state)
{
	CALLED();
	status_t status = B_ERROR;
	if (state == 1) {
		//be_app->HideCursor();
		status = AssertDisplayMode(fDisplayMode);
		if (status == B_OK && (status = SetupAccelerantHooks(true)) == B_OK) {			
			if (!fActivateState) {
				while (acquire_sem(fActivateSem) == B_INTERRUPTED)
					;
			}
			
			SetColorList(fColorList);
			if (fDebugState && !fDebugFirst) {
				SuspensionHook(true);
				Resume();					
			} else {
				fDebugFirst = true;
				ScreenConnected(true);
			}
			
			if (status == B_OK)
				return status;
			
		} else
			SetupAccelerantHooks(false);	
				
		be_app->ShowCursor();				
	} else {
		AssertDisplayMode(fOldDisplayMode);
		if (fDebugState && !fDebugFirst) {
			Suspend();
			SuspensionHook(false);
		} else
			ScreenConnected(false);
			
		status = SetupAccelerantHooks(false);
		if (status == B_OK) {				
			be_app->ShowCursor();				
			if (fActivateState) {
				// TODO: Set palette
			}
		}
	}
	
	return status;
}


status_t
BWindowScreen::SetupAccelerantHooks(bool enable)
{
	CALLED();
	if (fAddonImage >= 0) {
		m_wei();
		sFillRectHook = NULL;
		sBlitRectHook = NULL;
		sTransparentBlitHook = NULL;
		sScaledFilteredBlitHook = NULL;
		sWaitIdleHook = NULL;
		sEngineToken = NULL;
		sAcquireEngineHook = NULL;
		sReleaseEngineHook = NULL;
	}

	if (enable)
		fLockState = 1;
	else
		fLockState = 0;
	
	status_t status = B_OK;
	if (enable) {
		if (fAddonImage < 0) {
			status = InitClone();
			if (status == B_OK) {
				m_wei = (wait_engine_idle)fGetAccelerantHook(B_WAIT_ENGINE_IDLE, NULL);
				m_re = (release_engine)fGetAccelerantHook(B_RELEASE_ENGINE, NULL);
				m_ae = (acquire_engine)fGetAccelerantHook(B_ACQUIRE_ENGINE, NULL);
				fill_rect = (fill_rectangle)fGetAccelerantHook(B_FILL_RECTANGLE, NULL);
				blit_rect = (screen_to_screen_blit)fGetAccelerantHook(B_SCREEN_TO_SCREEN_BLIT, NULL);
				trans_blit_rect = (screen_to_screen_transparent_blit)fGetAccelerantHook(B_SCREEN_TO_SCREEN_TRANSPARENT_BLIT, NULL);
				scaled_filtered_blit_rect = (screen_to_screen_scaled_filtered_blit)fGetAccelerantHook(B_SCREEN_TO_SCREEN_SCALED_FILTERED_BLIT, NULL);
			}
		}
		
		if (status == B_OK) {
			sFillRectHook = fill_rect;
			sBlitRectHook = blit_rect;
			sTransparentBlitHook = trans_blit_rect;
			sScaledFilteredBlitHook = scaled_filtered_blit_rect;
			sWaitIdleHook = m_wei;
			sAcquireEngineHook = m_ae;
			sReleaseEngineHook = m_re;
			m_wei();
		}
	}
	
	return status;
}


status_t
BWindowScreen::GetCardInfo()
{
	CALLED();
	
	BScreen screen(this);
	display_mode mode;
	status_t status = screen.GetMode(&mode);
	if (status < B_OK)
		return status;
	
	uint32 bitsPerPixel;
	switch(mode.space & 0x0fff) {
		case B_CMAP8:
			bitsPerPixel = 8;
			break;
		case B_RGB16:
			bitsPerPixel = 16;
			break;
		case B_RGB32:
			bitsPerPixel = 32;
			break;
		default:
			bitsPerPixel = 0;
			break;
	}
	
	fCardInfo.version = 2;
	fCardInfo.id = screen.ID().id;
	fCardInfo.bits_per_pixel = bitsPerPixel;
	fCardInfo.width = mode.virtual_width;
	fCardInfo.height = mode.virtual_height;
	
	if (mode.space & 0x10)
		strncpy(fCardInfo.rgba_order, "rgba", 4);
	else
		strncpy(fCardInfo.rgba_order, "bgra", 4);
	
	fCardInfo.flags = 0;
	if (mode.flags & B_SCROLL)
		fCardInfo.flags |= B_FRAME_BUFFER_CONTROL;
	if (mode.flags & B_PARALLEL_ACCESS)
		fCardInfo.flags |= B_PARALLEL_BUFFER_ACCESS;
	
	AppServerLink link;
	link.StartMessage(AS_GET_FRAME_BUFFER_CONFIG);
	link.Attach<screen_id>(screen.ID());
	
	status_t result = B_ERROR;
	if (link.FlushWithReply(result) < B_OK || result < B_OK)
		return result;
	
	frame_buffer_config config;	
	link.Read<frame_buffer_config>(&config);
	
	fCardInfo.frame_buffer = config.frame_buffer;
	fCardInfo.bytes_per_row = config.bytes_per_row;
	
	return B_OK;
}


void
BWindowScreen::Suspend()
{
	CALLED();
	
	while (acquire_sem(fDebugSem) == B_INTERRUPTED)
		;

	// Suspend all the registered threads
	for (int32 i = 0; i < fDebugListCount; i++) {
		snooze(10000);
		suspend_thread(fDebugList[i]);
	}

	graphics_card_info *info = CardInfo();
	size_t fbSize = info->bytes_per_row * info->height; 

	// Save the content of the frame buffer into the local buffer
	fDebugBuffer = (char *)malloc(fbSize);
	memcpy(fDebugBuffer, info->frame_buffer, fbSize);
}


void
BWindowScreen::Resume()
{
	CALLED();
	graphics_card_info *info = CardInfo();
	
	// Copy the content of the debug_buffer back into the frame buffer.
	memcpy(info->frame_buffer, fDebugBuffer, info->bytes_per_row * info->height);
	free(fDebugBuffer);

	// Resume all the registered threads
	for (int32 i = 0; i < fDebugListCount; i++)
		resume_thread(fDebugList[i]);

	release_sem(fDebugSem);
}


status_t
BWindowScreen::GetModeFromSpace(uint32 space, display_mode *dmode)
{
	CALLED();
	
	uint32 destSpace;
	int32 width, height;
	status_t status = B_ERROR;
	
	mode2parms(space, &destSpace, &width, &height);
	// mode2parms converts the space to actual width, height and color_space, e.g B_8_BIT_640x480
	
	for (uint32 i = 0; i < fModeCount; i++) {
		if (fModeList[i].space == destSpace && fModeList[i].virtual_width == width
			&& fModeList[i].virtual_height == height) {
			
			memcpy(dmode, &fModeList[i], sizeof(display_mode));
			status = B_OK;
			break;
		}
	}
	
	return status;
}


status_t
BWindowScreen::InitClone()
{
	CALLED();
	
	if (fAddonImage >= 0)
		return B_OK;

	AppServerLink link;
	link.StartMessage(AS_GET_ACCELERANT_PATH);
	link.Attach<int32>(fScreenIndex);
	
	status_t status = B_ERROR;
	if (link.FlushWithReply(status) < B_OK || status < B_OK)
		return status;

	BString accelerantPath;
	link.ReadString(accelerantPath);
	fAddonImage = load_add_on(accelerantPath.String()); 
	if (fAddonImage < B_OK) {
		printf("InitClone: cannot load accelerant image\n");
		return fAddonImage;
	}

	status = get_image_symbol(fAddonImage, B_ACCELERANT_ENTRY_POINT,
					B_SYMBOL_TYPE_TEXT, (void **)&fGetAccelerantHook);
	if (status < B_OK) {
		printf("InitClone: cannot get accelerant entry point\n");
		unload_add_on(fAddonImage);
		fAddonImage = -1;
		return status;
	}
	
	status = B_ERROR;
	clone_accelerant clone = (clone_accelerant)fGetAccelerantHook(B_CLONE_ACCELERANT, 0);
	if (clone == NULL) {
		printf("InitClone: cannot get clone hook\n");
		unload_add_on(fAddonImage);
		fAddonImage = -1;
		return status;
	}

	link.StartMessage(AS_GET_DRIVER_PATH);
	link.Attach<int32>(fScreenIndex);
	if (link.FlushWithReply(status) == B_OK && status == B_OK) {
		BString driverPath;
		link.ReadString(driverPath);
		status = clone((void *)driverPath.String());
	}
	
	if (status < B_OK) {
		printf("InitClone: cannot clone accelerant\n");
		unload_add_on(fAddonImage);
		fAddonImage = -1;
	}
	
	return status;
}


status_t
BWindowScreen::AssertDisplayMode(display_mode *dmode)
{
	CALLED();

	status_t status = B_ERROR;
	BScreen screen(this);
	display_mode mode;
	status = screen.GetMode(&mode);
	if (status < B_OK)
		return status;
	if (mode.virtual_height != dmode->virtual_height || mode.virtual_width != dmode->virtual_width
			|| mode.space != dmode->space || mode.flags != dmode->flags) {
		status = screen.SetMode(dmode);
		if (status < B_OK) {
			printf("AssertDisplayMode: Setting mode failed: %s\n", strerror(status));
			return status;
		}
		memcpy(fDisplayMode, dmode, sizeof(display_mode));
		space_mode = 1;
	}

	status = GetCardInfo();
	if (status < B_OK)
		return status;

	fFrameBufferInfo.bits_per_pixel = fCardInfo.bits_per_pixel;
	fFrameBufferInfo.bytes_per_row = fCardInfo.bytes_per_row;
	fFrameBufferInfo.width = fCardInfo.width;
	fFrameBufferInfo.height = fCardInfo.height;
	fFrameBufferInfo.display_width = fCardInfo.width;
	fFrameBufferInfo.display_height = fCardInfo.height;
	fFrameBufferInfo.display_x = 0;
	fFrameBufferInfo.display_y = 0;
	
	return status;
}
