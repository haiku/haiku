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


static graphics_card_info card_info_global;
fill_rectangle fill_rect_global;
screen_to_screen_blit blit_rect_global;
screen_to_screen_transparent_blit trans_blit_rect_global;
screen_to_screen_scaled_filtered_blit scaled_filtered_blit_rect_global;
wait_engine_idle wait_idle_global;
engine_token *et_global;
acquire_engine acquire_engine_global;
release_engine release_engine_global;


// Helper methods which translates the pre r5 graphics methods to r5 ones
static int32
card_sync()
{
	wait_idle_global();
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
	
	acquire_engine_global(B_2D_ACCELERATION, 0xff, NULL, &et_global);
	blit_rect_global(et_global, &param, 1);
	release_engine_global(et_global, NULL);
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
	
	acquire_engine_global(B_2D_ACCELERATION, 0xff, 0, &et_global);
	trans_blit_rect_global(et_global, transparent_color, &param, 1);
	release_engine_global(et_global, 0);
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
	
	acquire_engine_global(B_2D_ACCELERATION, 0xff, NULL, &et_global);
	scaled_filtered_blit_rect_global(et_global, &param, 1);
	release_engine_global(et_global, NULL);
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
	
	acquire_engine_global(B_2D_ACCELERATION, 0xff, NULL, &et_global);
	fill_rect_global(et_global, color_index, &param, 1);
	release_engine_global(et_global, NULL);
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
	
	acquire_engine_global(B_2D_ACCELERATION, 0xff, NULL, &et_global);
	fill_rect_global(et_global, color, &param, 1);
	release_engine_global(et_global, NULL);
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
	
	acquire_engine_global(B_2D_ACCELERATION, 0xff, NULL, &et_global);
	fill_rect_global(et_global, color, &param, 1);
	release_engine_global(et_global, NULL);
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
	: BWindow(BScreen().Frame().InsetByCopy(100, 100), title, B_TITLED_WINDOW,
		kWindowScreenFlag | B_NOT_MINIMIZABLE /*| B_NOT_CLOSABLE*/
			| B_NOT_ZOOMABLE | B_NOT_MOVABLE | B_NOT_RESIZABLE,
		B_CURRENT_WORKSPACE)
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
	: BWindow(BScreen().Frame().InsetByCopy(100, 100), title, B_TITLED_WINDOW, 
		kWindowScreenFlag | B_NOT_MINIMIZABLE /*| B_NOT_CLOSABLE*/
			| B_NOT_ZOOMABLE | B_NOT_MOVABLE | B_NOT_RESIZABLE,
		B_CURRENT_WORKSPACE)
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
	
	SetFullscreen(false);
	
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
	if (fLockState == 1) {
		if (fDebugState)
			fDebugFirst = true;
		SetActiveState(0);
	}
	
	BWindow::Hide();
}


void
BWindowScreen::Show()
{
	BWindow::Show();
	if (!fActivateState) {
		release_sem(fActivateSem);
		fActivateState = true;
	}
}


void
BWindowScreen::SetColorList(rgb_color *list, int32 first_index, int32 last_index)
{
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
				set_indexed_colors sic = (set_indexed_colors)m_gah(B_SET_INDEXED_COLORS, NULL);
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

	highMode.virtual_height = (int16)height;
	highMode.virtual_width = (int16)width;

	display_mode lowMode = highMode;
	display_mode mode = highMode;

	BScreen screen(this);
	status_t status = screen.ProposeMode(&mode, &lowMode, &highMode);
	
	// If the mode is supported, change the workspace
	// to that mode.
	if (status == B_OK)
		status = AssertDisplayMode(&mode);

	return status;
}


status_t
BWindowScreen::MoveDisplayArea(int32 x, int32 y)
{
	CALLED();
	status_t status = B_ERROR;

	// TODO: Ask app server to move the frame buffer area

	if (status == B_OK) {		
		fFrameBufferInfo.display_x = x;
		fFrameBufferInfo.display_y = y;
		fDisplayMode->h_display_start = x;
		fDisplayMode->v_display_start = y;
	}
	return status;
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
	return fColorList;
}


frame_buffer_info *
BWindowScreen::FrameBufferInfo()
{
	return &fFrameBufferInfo;
}


graphics_card_hook
BWindowScreen::CardHookAt(int32 index)
{
	if (fAddonImage < 0)
		return NULL;
	
	graphics_card_hook hook = NULL;
	
	switch (index) {
		case 5: // 8 bit fill rect
			hook = (graphics_card_hook)m_gah(B_FILL_RECTANGLE, 0);
			fill_rect = (fill_rectangle)hook;
			fill_rect_global = fill_rect;
			hook = (graphics_card_hook)draw_rect_8;
			break;
		case 6: // 32 bit fill rect
			hook = (graphics_card_hook)m_gah(B_FILL_RECTANGLE, 0);
			fill_rect = (fill_rectangle)hook;
			fill_rect_global = fill_rect;
			hook = (graphics_card_hook)draw_rect_32;
			break;
		case 7: // screen to screen blit
			hook = (graphics_card_hook)m_gah(B_SCREEN_TO_SCREEN_BLIT, 0);
			blit_rect = (screen_to_screen_blit)hook;
			blit_rect_global = blit_rect;
			hook = (graphics_card_hook)blit;
			break;
		case 8: // screen to screen scaled filtered blit
			hook = (graphics_card_hook)m_gah(B_SCREEN_TO_SCREEN_SCALED_FILTERED_BLIT, 0);
			scaled_filtered_blit_rect = (screen_to_screen_scaled_filtered_blit)hook;
			scaled_filtered_blit_rect_global = scaled_filtered_blit_rect;
			hook = (graphics_card_hook)scaled_filtered_blit;
			break;
		case 10: // sync aka wait for graphics card idle
			hook = (graphics_card_hook)card_sync;
			break;
		case 13: // 16 bit fill rect
			hook = (graphics_card_hook)m_gah(B_FILL_RECTANGLE, 0);
			fill_rect = (fill_rectangle)hook;
			fill_rect_global = fill_rect;
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
	BScreen screen;	
	if (dmode)
		screen.GetMode(dmode);

	return screen.Frame();
}


status_t
BWindowScreen::SetFullscreen(bool enable)
{
	status_t status = B_ERROR;

	// TODO: Set fullscreen
	
	return status;
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
	fScreenIndex = current_workspace();
	fLockState = 0;
	fAddonImage = -1;
	fWindowState = 0;
	fDebugWorkspace = 0;

	BScreen screen(this);	
	status_t status = screen.GetModeList(&fModeList, &fModeCount);
	if (status < B_OK) {
		printf("BScreen::GetModeList() returned %s\n", strerror(status));
		return status;
	}	
	
	display_mode newMode;
	status = GetModeFromSpace(space, &newMode);
	if (status < B_OK) {
		printf("GetModeFromSpace() returned %s\n", strerror(status));
		return status;
	}

	space_mode = 1;
	space0 = 0;
	
	SetWorkspaces(B_CURRENT_WORKSPACE);
	SetFullscreen(true);

	memcpy(fColorList, screen.ColorMap()->color_list, 256);
	
	status = GetCardInfo();
	if (status < B_OK)
		return status;
	
	fActivateSem = create_sem(0, "WindowScreen start lock");
	fActivateState = 0;
	
	fDebugSem = create_sem(1, "WindowScreen debug sem");
	fOldDisplayMode = (display_mode *)calloc(1, sizeof(display_mode));
	fDisplayMode = (display_mode *)calloc(1, sizeof(display_mode));
	
	memcpy(fDisplayMode, &newMode, sizeof(newMode));
	
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
		if (/*be_app->IsCursorHidden() && */(status = SetLockState(1)) == B_OK) {
			status = AssertDisplayMode(fDisplayMode);			
			printf("AssertDisplayMode() returned %s\n", strerror(status));
			if (status == B_OK) {
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
				SetLockState(0);	
				
			be_app->ShowCursor();
		} 				
	} else {
		if (fDebugState && !fDebugFirst) {
			Suspend();
			SuspensionHook(false);
		} else
			ScreenConnected(false);
			
		status = SetLockState(0);
		if (status == B_OK) {				
			be_app->ShowCursor();				
			if (fActivateState) {
				// TODO: Set palette
			}
		}
	}
	
	printf("SetActiveState() returning %s\n", strerror(status));
	return status;
}


status_t
BWindowScreen::SetLockState(int32 state)
{
	CALLED();
	if (fAddonImage >= 0 && state == 1) {
		m_wei();
		fill_rect_global = NULL;
		blit_rect_global = NULL;
		trans_blit_rect_global = NULL;
		scaled_filtered_blit_rect_global = NULL;
		wait_idle_global = NULL;
		et_global = NULL;
		acquire_engine_global = NULL;
		release_engine_global = NULL;
	}

	status_t status = B_OK;

	// TODO: Set lock state (whatever it means)
	
	if (status == B_OK) {
		fLockState = state;
		if (state == 1) {
			if (fAddonImage < 0) {
				status = InitClone();
				printf("InitClone() returned %s\n", strerror(status));
				if (status == B_OK) {
					m_wei = (wait_engine_idle)m_gah(B_WAIT_ENGINE_IDLE, NULL);
					m_re = (release_engine)m_gah(B_RELEASE_ENGINE, NULL);
					m_ae = (acquire_engine)m_gah(B_ACQUIRE_ENGINE, NULL);
				}
			}
			
			if (status == B_OK && state == 1) {
				fill_rect_global = fill_rect;
				blit_rect_global = blit_rect;
				trans_blit_rect_global = trans_blit_rect;
				scaled_filtered_blit_rect_global = scaled_filtered_blit_rect;
				wait_idle_global = m_wei;
				et_global = et;
				acquire_engine_global = m_ae;
				release_engine_global = m_re;
				m_wei();
			}
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
	fCardInfo.id = 0;
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
	
	frame_buffer_config config;
	AppServerLink link;
	link.StartMessage(AS_GET_FRAME_BUFFER_CONFIG);
	link.Attach<screen_id>(screen.ID());
	
	status_t result = B_ERROR;
	if (link.FlushWithReply(result) < B_OK || result < B_OK)
		return result;
	
	link.Read<frame_buffer_config>(&config);
	
	fCardInfo.id = screen.ID().id;
	fCardInfo.frame_buffer = config.frame_buffer;
	fCardInfo.bytes_per_row = config.bytes_per_row;
	
	memcpy(&card_info_global, &fCardInfo, sizeof(graphics_card_info));
	
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
	if (fAddonImage >= 0)
		return B_OK;

	CALLED();
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
					B_SYMBOL_TYPE_ANY, (void **)&m_gah);
	if (status < B_OK) {
		printf("InitClone: cannot get accelerant entry point\n");
		unload_add_on(fAddonImage);
		fAddonImage = -1;
		return status;
	}
	
	clone_accelerant clone = (clone_accelerant)m_gah(B_CLONE_ACCELERANT, 0);
	if (clone == NULL) {
		printf("InitClone: cannot get clone hook\n");
		unload_add_on(fAddonImage);
		fAddonImage = -1;
		return status;
	}

	status = B_ERROR;
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
	status_t status = B_OK;

	// TODO: Assert display mode: negotiation with app server

	if (status == B_OK) { 
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
