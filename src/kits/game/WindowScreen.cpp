/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <WindowScreen.h>

BWindowScreen::BWindowScreen(const char *title,
							 uint32 space,
							 status_t *error,
							 bool debug_enable)
 :	BWindow(BRect(0,0,0,0), title, B_UNTYPED_WINDOW, B_ASYNCHRONOUS_CONTROLS, B_CURRENT_WORKSPACE) // XXX this needs to be fixed
{
}


BWindowScreen::BWindowScreen(const char *title,
							 uint32 space,
							 uint32 attributes,
							 status_t *error)
 :	BWindow(BRect(0,0,0,0), title, B_UNTYPED_WINDOW, B_ASYNCHRONOUS_CONTROLS, B_CURRENT_WORKSPACE) // XXX this needs to be fixed
{
}


BWindowScreen::~BWindowScreen()
{
}


void
BWindowScreen::Quit(void)
{
}


void
BWindowScreen::ScreenConnected(bool active)
{
}


void
BWindowScreen::Disconnect()
{
}


void
BWindowScreen::WindowActivated(bool active)
{
}


void
BWindowScreen::WorkspaceActivated(int32 ws,
								  bool state)
{
}


void
BWindowScreen::ScreenChanged(BRect screen_size,
							 color_space depth)
{
}


void
BWindowScreen::Hide()
{
}


void
BWindowScreen::Show()
{
}


void
BWindowScreen::SetColorList(rgb_color *list,
							int32 first_index,
							int32 last_index)
{
}


status_t
BWindowScreen::SetSpace(uint32 space)
{
	return B_ERROR;
}


bool
BWindowScreen::CanControlFrameBuffer()
{
	return false;
}


status_t
BWindowScreen::SetFrameBuffer(int32 width,
							  int32 height)
{
	return B_ERROR;
}


status_t
BWindowScreen::MoveDisplayArea(int32 x,
							   int32 y)
{
	return B_ERROR;
}


void *
BWindowScreen::IOBase()
{
	return NULL;
}


rgb_color *
BWindowScreen::ColorList()
{
	return NULL;
}


frame_buffer_info *
BWindowScreen::FrameBufferInfo()
{
	return NULL;
}


graphics_card_hook
BWindowScreen::CardHookAt(int32 index)
{
	return 0;
}


graphics_card_info *
BWindowScreen::CardInfo()
{
	return NULL;
}


void
BWindowScreen::RegisterThread(thread_id id)
{
}


void
BWindowScreen::SuspensionHook(bool active)
{
}


void
BWindowScreen::Suspend(char *label)
{
}


status_t
BWindowScreen::Perform(perform_code d,
					   void *arg)
{
	return B_ERROR;
}


void
BWindowScreen::_ReservedWindowScreen1()
{
}


void
BWindowScreen::_ReservedWindowScreen2()
{
}


void
BWindowScreen::_ReservedWindowScreen3()
{
}


void
BWindowScreen::_ReservedWindowScreen4()
{
}


/* unimplemented for protection of the user:
 *
 * BWindowScreen::BWindowScreen()
 * BWindowScreen::BWindowScreen(BWindowScreen &)
 * BWindowScreen &BWindowScreen::operator=(BWindowScreen &)
 */


BRect
BWindowScreen::CalcFrame(int32 index,
						 int32 space,
						 display_mode *dmode)
{
	BRect dummy;
	return dummy;
}


int32
BWindowScreen::SetFullscreen(int32 enable)
{
	return 0;
}


status_t
BWindowScreen::InitData(uint32 space,
						uint32 attributes)
{
	return B_ERROR;
}


status_t
BWindowScreen::SetActiveState(int32 state)
{
	return B_ERROR;
}


status_t
BWindowScreen::SetLockState(int32 state)
{
	return B_ERROR;
}


void
BWindowScreen::GetCardInfo()
{
}


void
BWindowScreen::Suspend()
{
}


void
BWindowScreen::Resume()
{
}


status_t
BWindowScreen::GetModeFromSpace(uint32 space,
								display_mode *dmode)
{
	return B_ERROR;
}


status_t
BWindowScreen::InitClone()
{
	return B_ERROR;
}


status_t
BWindowScreen::AssertDisplayMode(display_mode *dmode)
{
	return B_ERROR;
}


