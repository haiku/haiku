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


#ifdef HAIKU_TARGET_PLATFORM_BEOS
// WindowScreen commands
#define WS_MOVE_DISPLAY			0x00000108
#define WS_SET_FULLSCREEN 		0x00000881
#define WS_GET_FRAMEBUFFER 		0x00000eed
#define WS_GET_ACCELERANT_NAME 	0x00000ef4
#define WS_GET_DRIVER_NAME 		0x00000ef5
#define WS_DISPLAY_UTILS		0x00000ef9
#define WS_SET_LOCK_STATE		0x00000efb
#define WS_SET_DISPLAY_MODE 	0x00000efd
#define WS_SET_PALETTE			0x00000f27

#else

#include <AppServerLink.h>
#include <ServerProtocol.h>

using BPrivate::AppServerLink;

#endif

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
	: BWindow(BScreen().Frame(), title, B_TITLED_WINDOW,
		kWindowScreenFlag | B_NOT_MINIMIZABLE | B_NOT_CLOSABLE
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
	: BWindow(BScreen().Frame(), title, B_TITLED_WINDOW, 
		kWindowScreenFlag | B_NOT_MINIMIZABLE | B_NOT_CLOSABLE
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
	Disconnect();
	if (addon_state == 1)
		unload_add_on(addon_image);
	
	SetFullscreen(0);
	
	delete_sem(activate_sem);
	delete_sem(debug_sem);
	
	if (debug_state)
		activate_workspace(debug_workspace);
	
	free(new_space);
	free(old_space);
	free(mode_list);
}


void
BWindowScreen::Quit(void)
{
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
	if (lock_state == 1) {
		if (debug_state)
			debug_first = true;
		SetActiveState(0);
	}
}


void
BWindowScreen::WindowActivated(bool active)
{
	CALLED();
	window_state = active;
	if(active && lock_state == 0 && work_state)
		SetActiveState(1);
}


void
BWindowScreen::WorkspaceActivated(int32 ws, bool state)
{
	CALLED();
	work_state = state;
	if (state) {
		if (lock_state == 0 && window_state) {
			SetActiveState(1);
			if (!IsHidden()) {
				Activate(true);
				WindowActivated(true);
			}
		}
	} else {
		if (lock_state)
			SetActiveState(0);
	}
}


void
BWindowScreen::ScreenChanged(BRect screen_size, color_space depth)
{
	// Implemented in subclasses
}


void
BWindowScreen::Hide()
{
	if (lock_state == 1) {
		if (debug_state)
			debug_first = true;
		SetActiveState(0);
	}
	
	BWindow::Hide();
}


void
BWindowScreen::Show()
{
	BWindow::Show();
	if (!activate_state) {
		release_sem(activate_sem);
		activate_state = true;
	}
}


void
BWindowScreen::SetColorList(rgb_color *list, int32 first_index, int32 last_index)
{
	if (first_index < 0 || last_index > 255 || first_index > last_index)
		return;

	if (Lock()) {
		if (!activate_state) { 
			//If we aren't active, we just change our local palette
			for (int32 x = first_index; x <= last_index; x++)
				colorList[x] = list[x];
		} else {
			uint32 colorCount = last_index - first_index + 1;
			if (addon_state == 1) {
				set_indexed_colors sic = (set_indexed_colors)m_gah(B_SET_INDEXED_COLORS, NULL);
				if (sic != NULL)
					sic(colorCount, first_index, (uint8*)colorList, 0);
			}
	
			BScreen screen(this);
			screen.WaitForRetrace();	
			// Update our local palette too. We're doing this between two 
			// WaitForRetrace() calls so nobody will notice.
			for (int32 x = first_index; x <= last_index; x++)
				colorList[x] = list[x];
		
			// Tell the app_server about our changes
#ifdef HAIKU_TARGET_PLATFORM_BEOS
			_BAppServerLink_ link;
			link.fSession->swrite_l(WS_SET_PALETTE);
			link.fSession->swrite_l(screen_index);
			link.fSession->swrite_l(first_index);
			link.fSession->swrite_l(last_index);
			link.fSession->swrite(colorCount * sizeof(rgb_color), colorList);
			link.fSession->sync();
#endif		
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
	return (addon_state <= 1 && (card_info.flags & B_FRAME_BUFFER_CONTROL));
}


status_t
BWindowScreen::SetFrameBuffer(int32 width, int32 height)
{
	display_mode highMode = *new_space;

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
	status_t status = B_ERROR;

#ifdef HAIKU_TARGET_PLATFORM_BEOS	
	_BAppServerLink_ link;	
	link.fSession->swrite_l(WS_DISPLAY_UTILS);
	link.fSession->swrite_l(screen_index);
	link.fSession->swrite_l(WS_MOVE_DISPLAY);
	link.fSession->swrite(sizeof(int16), (int16*)&x);
	link.fSession->swrite(sizeof(int16), (int16*)&y);
	link.fSession->sync();	
	link.fSession->sread(sizeof(status), &status);
#endif

	if (status == B_OK) {		
		format_info.display_x = x;
		format_info.display_y = y;
		new_space->h_display_start = x;
		new_space->v_display_start = y;
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
	return colorList;
}


frame_buffer_info *
BWindowScreen::FrameBufferInfo()
{
	return &format_info;
}


graphics_card_hook
BWindowScreen::CardHookAt(int32 index)
{
	if (addon_state != 1)
		return 0;
	
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
	return &card_info;
}


void
BWindowScreen::RegisterThread(thread_id id)
{
	CALLED();
	while (acquire_sem(debug_sem) == B_INTERRUPTED)
		;
	++debug_list_count;
	debug_list = (thread_id *)realloc(debug_list, debug_list_count * sizeof(thread_id));		
	debug_list[debug_list_count - 1] = id;

	release_sem(debug_sem);
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
	if (debug_state) {		
		fprintf(stderr, "## Debugger(\"%s\").", label);
		fprintf(stderr, " Press Alt-F%ld or Cmd-F%ld to resume.\n", space0 + 1, space0 + 1);
		
		if (IsLocked())
			Unlock();
		
		activate_workspace(debug_workspace);

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


int32
BWindowScreen::SetFullscreen(int32 enable)
{
	int32 retval = -1;

#ifdef HAIKU_TARGET_PLATFORM_BEOS
	int32 result = -1;

	a_session->swrite_l(WS_SET_FULLSCREEN);
	a_session->swrite_l(server_token);
	a_session->swrite_l(enable);
	a_session->sync();
	a_session->sread(sizeof(result), &result);
	a_session->sread(sizeof(retval), &retval);
#endif
	
	return retval;
}


status_t
BWindowScreen::InitData(uint32 space, uint32 attributes)
{
	CALLED();
	
	BScreen screen(this);
	debug_state = attributes & B_ENABLE_DEBUGGER;
	debug_list_count = 0;
	debug_list = 0;
	debug_first = true;
	
	debug_sem = create_sem(1, "WindowScreen debug sem");
	old_space = (display_mode *)calloc(1, sizeof(display_mode));
	new_space = (display_mode *)calloc(1, sizeof(display_mode));
	
	screen.GetModeList(&mode_list, &mode_count);
	
	_attributes = attributes;
	
	display_mode *n_space = new_space;
	screen_index = 0;
	lock_state = 0;
	addon_state = 0;
	direct_enable = 0;
	window_state = 0;
	
	space_mode = 1;
	
	GetModeFromSpace(space, n_space);
	
	space0 = 0;
	
	SetWorkspaces(B_CURRENT_WORKSPACE);
	SetFullscreen(1);

	work_state = 1;
	debug_workspace = 0;
	memcpy(colorList, screen.ColorMap()->color_list, 256);
	
	GetCardInfo();

	activate_sem = create_sem(0, "WindowScreen start lock");
	activate_state = 0;
		
	return B_OK;
}


status_t
BWindowScreen::SetActiveState(int32 state)
{
	CALLED();
	status_t status = B_ERROR;
	if (state == 1) {
		be_app->HideCursor();
		if (be_app->IsCursorHidden() 
			&& (status = SetLockState(1)) == B_OK) {
			status = AssertDisplayMode(new_space);			
			if (status == B_OK) {
				if (!activate_state) {
					while (acquire_sem(activate_sem) == B_INTERRUPTED)
						;
				}
				SetColorList(colorList);
				if (debug_state && !debug_first) {
					SuspensionHook(true);
					Resume();					
				} else {
					debug_first = true;
					ScreenConnected(true);
				}
				if (status == B_OK)
					return status;
				
			} else
				SetLockState(0);	
				
			be_app->ShowCursor();
		} 				
	} else {
		if (debug_state && !debug_first) {
			Suspend();
			SuspensionHook(false);
		} else
			ScreenConnected(false);
			
		status = SetLockState(0);
		if (status == B_OK) {				
			be_app->ShowCursor();				
			if (activate_state) {
#ifdef HAIKU_TARGET_PLATFORM_BEOS				
				const color_map *colorMap = system_colors();
				_BAppServerLink_ link;

				link.fSession->swrite_l(WS_SET_PALETTE);
				link.fSession->swrite_l(screen_index);
				link.fSession->swrite_l(0);
				link.fSession->swrite_l(255);
				link.fSession->swrite(256 * sizeof(rgb_color), const_cast<rgb_color *>(colorMap->color_list));
				link.fSession->sync();
#endif
			}
		}
	}
	return status;
}


status_t
BWindowScreen::SetLockState(int32 state)
{
	CALLED();
	if (addon_state == 1 && state == 1) {
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

	status_t status = B_ERROR;

#ifdef HAIKU_TARGET_PLATFORM_BEOS	
	_BAppServerLink_ link;
	link.fSession->swrite_l(WS_SET_LOCK_STATE);
	link.fSession->swrite_l(screen_index);
	link.fSession->swrite_l(state);
	link.fSession->swrite_l(server_token);
	link.fSession->sync();
	link.fSession->sread(sizeof(status), &status);
#endif
	
	if (status == B_OK) {
		lock_state = state;
		if (state == 1) {
			if (addon_state == 0) {
				status = InitClone();
				
				if (status == B_OK) {
					addon_state = 1;
					m_wei = (wait_engine_idle)m_gah(B_WAIT_ENGINE_IDLE, NULL);
					m_re = (release_engine)m_gah(B_RELEASE_ENGINE, NULL);
					m_ae = (acquire_engine)m_gah(B_ACQUIRE_ENGINE, NULL);
				} else
					addon_state = -1;
			}
			
			if (addon_state == 1 && state == 1) {
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


void
BWindowScreen::GetCardInfo()
{
	CALLED();
	
	BScreen screen(this);
	display_mode mode;
	if (screen.GetMode(&mode) < B_OK)
		return;
	
	uint32 bits_per_pixel;
	switch(mode.space & 0x0fff) {
		case B_CMAP8:
			bits_per_pixel = 8;
			break;
		case B_RGB16:
			bits_per_pixel = 16;
			break;
		case B_RGB32:
			bits_per_pixel = 32;
			break;
		default:
			bits_per_pixel = 0;
			break;
	}
	
	card_info.version = 2;
	card_info.id = 0;
	card_info.bits_per_pixel = bits_per_pixel;
	card_info.width = mode.virtual_width;
	card_info.height = mode.virtual_height;
	
	if (mode.space & 0x10)
		strncpy(card_info.rgba_order, "rgba", 4);
	else
		strncpy(card_info.rgba_order, "bgra", 4);
	
	card_info.flags = 0;
	
	if (mode.flags & B_SCROLL)
		card_info.flags |= B_FRAME_BUFFER_CONTROL;
	if (mode.flags & B_PARALLEL_ACCESS)
		card_info.flags |= B_PARALLEL_BUFFER_ACCESS;
	
	screen_id id = screen.ID();
	
	status_t result = B_ERROR;
	
	frame_buffer_config config;
	
#ifdef HAIKU_TARGET_PLATFORM_BEOS
	_BAppServerLink_ link;
	link.fSession->swrite_l(WS_GET_FRAMEBUFFER);
	link.fSession->swrite_l(id.id);
	link.fSession->swrite_l(server_token);
	link.fSession->sync();
	link.fSession->sread(sizeof(result), &result);
	if (result == B_OK)
		link.fSession->sread(sizeof(frame_buffer_config), &config);
#else
	AppServerLink link;
	link.StartMessage(AS_GET_FRAME_BUFFER_CONFIG);
	link.Attach<screen_id>(id);
	if (link.FlushWithReply(result) == B_OK && result == B_OK)
		link.Read<frame_buffer_config>(&config);
	
#endif
	
	if (result == B_OK) {
		card_info.id = id.id;
		card_info.frame_buffer = config.frame_buffer;
		card_info.bytes_per_row = config.bytes_per_row;
	}
	
	memcpy(&card_info_global, &card_info, sizeof(graphics_card_info));
	
	CALLED();
}


void
BWindowScreen::Suspend()
{
	CALLED();
	
	while (acquire_sem(debug_sem) == B_INTERRUPTED)
		;

	// Suspend all the registered threads
	for (int i = 0; i < debug_list_count; i++) {
		snooze(10000);
		suspend_thread(debug_list[i]);
	}

	graphics_card_info *info = CardInfo();
	size_t fbSize = info->bytes_per_row * info->height; 

	// Save the content of the frame buffer into the local buffer
	debug_buffer = (char *)malloc(fbSize);
	memcpy(debug_buffer, info->frame_buffer, fbSize);
}


void
BWindowScreen::Resume()
{
	CALLED();
	graphics_card_info *info = CardInfo();
	
	// Copy the content of the debug_buffer back into the frame buffer.
	memcpy(info->frame_buffer, debug_buffer, info->bytes_per_row * info->height);
	free(debug_buffer);

	// Resume all the registered threads
	for (int i = 0; i < debug_list_count; i++)
		resume_thread(debug_list[i]);

	release_sem(debug_sem);
}


status_t
BWindowScreen::GetModeFromSpace(uint32 space, display_mode *dmode)
{
	CALLED();
	
	uint32 out_space;
	int32 width, height;
	status_t ret = B_ERROR;
	
	mode2parms(space, &out_space, &width, &height);
	// mode2parms converts the space to actual width, height and color_space, e.g B_8_BIT_640x480
	
	for (uint32 i = 0; i < mode_count; i++) {
		if (mode_list[i].space == out_space && mode_list[i].virtual_width == width
			&& mode_list[i].virtual_height == height) {
			
			memcpy(dmode, &mode_list[i], sizeof(display_mode));
			ret = B_OK;
			break;
		}
	}
	
	return ret;
}


status_t
BWindowScreen::InitClone()
{
	CALLED();
#ifndef HAIKU_TARGET_PLATFORM_BEOS
	AppServerLink link;
	link.StartMessage(AS_GET_ACCELERANT_NAME);
	link.Attach<int32>(screen_index);
	
	status_t status = B_ERROR;
	if (link.FlushWithReply(status) < B_OK || status < B_OK)
		return status;

	BString accelerantName;
	link.ReadString(accelerantName);
	addon_image = load_add_on(accelerantName.String()); 
	if (addon_image < B_OK)
		return addon_image;

	
	status = get_image_symbol(addon_image, "get_accelerant_hook", B_SYMBOL_TYPE_ANY, (void **)&m_gah);
	if (status < B_OK) {
		unload_add_on(addon_image);
		addon_image = -1;
		return status;
	}
	
	clone_accelerant clone = (clone_accelerant)m_gah(B_CLONE_ACCELERANT, 0);
	if (!clone) {
		unload_add_on(addon_image);
		addon_image = -1;
		return status;
	}

	status = B_ERROR;
	link.StartMessage(AS_GET_DRIVER_NAME);
	link.Attach<int32>(screen_index);
	if (link.FlushWithReply(status) == B_OK && status == B_OK) {
		BString driverName;
		link.ReadString(driverName);
		status = clone((void *)driverName.String());
	}
	
	if (status < B_OK) {
		unload_add_on(addon_image);
		addon_image = -1;
	}
	
	return status;	
	
#else
	_BAppServerLink_ link;
	link.fSession->swrite_l(WS_GET_ACCELERANT_NAME);
	link.fSession->swrite_l(screen_index);
	link.fSession->sync();
	
	status_t status;
	link.fSession->sread(sizeof(status), &status);
	if (status != B_OK)
		return status;
	
	int32 length;
	link.fSession->sread(sizeof(length), &length); // read length of accelerant's name
	
	char *addonName = new char[length + 1];
	link.fSession->sread(length, addonName); // read the accelerant's name
	addonName[length] = '\0';
	
	// load the accelerant
	addon_image = load_add_on(addonName); 
	delete[] addonName;
	
	if (addon_image < 0)
		return addon_image;
	
	// now get the symbol for GetAccelerantHook m_gah
	if (get_image_symbol(addon_image, "get_accelerant_hook",
						B_SYMBOL_TYPE_ANY, (void **)&m_gah) < 0)
		return B_ERROR;
	
	// now use m_gah to get a pointer to the accelerant's clone accelerant
	clone_accelerant clone = (clone_accelerant)m_gah(B_CLONE_ACCELERANT, 0);
	
	if (!clone)
		return B_ERROR;

	link.fSession->swrite_l(WS_GET_DRIVER_NAME); // get driver's name without the /dev/
	link.fSession->swrite_l(screen_index);
	link.fSession->sync();
	
	link.fSession->sread(sizeof(status), &status);
	if (status != B_OK)
		return status;
	
	link.fSession->sread(sizeof(length), &length);
	// result now contains the length of the buffer needed for the drivers path name
	
	char *path = new char[length + 1];
	link.fSession->sread(length, path);
	path[length] = '\0';
	// path now contains the driver's name
	
	// test if the driver supports cloning of the accelerant, using the path we got earlier
	result = clone((void *)path);
	delete[] path;
	
	if (result != 0) {
		unload_add_on(addon_image);
		addon_image = -1;
	}
	
	return result;
#endif
}


status_t
BWindowScreen::AssertDisplayMode(display_mode *dmode)
{
	status_t result = B_ERROR;

#ifdef HAIKU_TARGET_PLATFORM_BEOS
	_BAppServerLink_ link;
	link.fSession->swrite_l(WS_SET_DISPLAY_MODE); // check display_mode valid command
	link.fSession->swrite_l(screen_index);
	link.fSession->swrite(sizeof(display_mode), (void *)dmode);
	link.fSession->sync();
	link.fSession->sread(sizeof(result), &result);
#endif

	// if the result is B_OK, we copy the dmode to new_space
	if (result == B_OK) { 
		memcpy(new_space, dmode, sizeof(display_mode));
		space_mode = 1;
	}
	GetCardInfo();
	
	format_info.bits_per_pixel = card_info.bits_per_pixel;
	format_info.bytes_per_row = card_info.bytes_per_row;
	format_info.width = card_info.width;
	format_info.height = card_info.height;
	format_info.display_width = card_info.width;
	format_info.display_height = card_info.height;
	format_info.display_x = 0;
	format_info.display_y = 0;
	
	return result;
}
