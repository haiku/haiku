/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <DirectWindow.h>

BDirectWindow::BDirectWindow(BRect frame,
							 const char *title,
							 window_type type,
							 uint32 flags,
							 uint32 workspace)
 : 	BWindow(frame, title, type, flags, workspace)
{
}


BDirectWindow::BDirectWindow(BRect frame,
							 const char *title,
							 window_look look,
							 window_feel feel,
							 uint32 flags,
							 uint32 workspace)
 : 	BWindow(frame, title, look, feel, flags, workspace)
{
}


BDirectWindow::~BDirectWindow()
{
}


BArchivable *
BDirectWindow::Instantiate(BMessage *data)
{
	return NULL;
}


status_t
BDirectWindow::Archive(BMessage *data,
					   bool deep) const
{
	return B_ERROR;
}


void
BDirectWindow::Quit(void)
{
}


void
BDirectWindow::DispatchMessage(BMessage *message,
							   BHandler *handler)
{
}


void
BDirectWindow::MessageReceived(BMessage *message)
{
}


void
BDirectWindow::FrameMoved(BPoint new_position)
{
}


void
BDirectWindow::WorkspacesChanged(uint32 old_ws,
								 uint32 new_ws)
{
}


void
BDirectWindow::WorkspaceActivated(int32 ws,
								  bool state)
{
}


void
BDirectWindow::FrameResized(float new_width,
							float new_height)
{
}


void
BDirectWindow::Minimize(bool minimize)
{
}


void
BDirectWindow::Zoom(BPoint rec_position,
					float rec_width,
					float rec_height)
{
}


void
BDirectWindow::ScreenChanged(BRect screen_size,
							 color_space depth)
{
}


void
BDirectWindow::MenusBeginning()
{
}


void
BDirectWindow::MenusEnded()
{
}


void
BDirectWindow::WindowActivated(bool state)
{
}


void
BDirectWindow::Show()
{
}


void
BDirectWindow::Hide()
{
}


BHandler *
BDirectWindow::ResolveSpecifier(BMessage *msg,
								int32 index,
								BMessage *specifier,
								int32 form,
								const char *property)
{
	return NULL;
}


status_t
BDirectWindow::GetSupportedSuites(BMessage *data)
{
	return B_ERROR;
}


status_t
BDirectWindow::Perform(perform_code d,
					   void *arg)
{
	return B_ERROR;
}


void
BDirectWindow::task_looper()
{
}


BMessage *
BDirectWindow::ConvertToMessage(void *raw,
								int32 code)
{
	return NULL;
}


void
BDirectWindow::DirectConnected(direct_buffer_info *info)
{
}


status_t
BDirectWindow::GetClippingRegion(BRegion *region,
								 BPoint *origin) const
{
	return B_ERROR;
}


status_t
BDirectWindow::SetFullScreen(bool enable)
{
	return B_ERROR;
}


bool
BDirectWindow::IsFullScreen() const
{
	return false;
}


bool
BDirectWindow::SupportsWindowMode(screen_id)
{
	return false;
}


void
BDirectWindow::_ReservedDirectWindow1()
{
}


void
BDirectWindow::_ReservedDirectWindow2()
{
}


void
BDirectWindow::_ReservedDirectWindow3()
{
}


void
BDirectWindow::_ReservedDirectWindow4()
{
}


/* unimplemented for protection of the user:
 *
 * BDirectWindow::BDirectWindow()
 * BDirectWindow::BDirectWindow(BDirectWindow &)
 * BDirectWindow &BDirectWindow::operator=(BDirectWindow &)
 */

int32
BDirectWindow::DirectDeamonFunc(void *arg)
{
	return 0;
}


bool
BDirectWindow::LockDirect() const
{
	return false;
}


void
BDirectWindow::UnlockDirect() const
{
}


void
BDirectWindow::InitData()
{
}


void
BDirectWindow::DisposeData()
{
}


status_t
BDirectWindow::DriverSetup() const
{
	return B_ERROR;
}


