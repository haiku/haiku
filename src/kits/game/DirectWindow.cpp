//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		DirectWindow.cpp
//	Author:			Christopher ML Zumwalt May (zummy@users.sf.net)
//	Description:	BFileGameSound allow the developer to directly draw to the
//					app_server's frame buffer.
//------------------------------------------------------------------------------

#include <memory.h>

#include "DirectWindow.h"

BDirectWindow::BDirectWindow(BRect frame,
							 const char *title,
							 window_type type,
							 uint32 flags,
							 uint32 workspace)
 		: 	BWindow(frame, title, type, flags, workspace),
 			fIsFullScreen(false),
 			fIsConnected(false)
{
	InitData(frame);
}


BDirectWindow::BDirectWindow(BRect frame,
							 const char *title,
							 window_look look,
							 window_feel feel,
							 uint32 flags,
							 uint32 workspace)
 		: 	BWindow(frame, title, look, feel, flags, workspace),
 			fIsFullScreen(false),
 			fIsConnected(false)
{
	InitData(frame);
}


BDirectWindow::~BDirectWindow()
{
	if (fIsConnected)
	{
		fBufferInfo->buffer_state = B_DIRECT_STOP;
		DoDirectConnected();
	}
	
	delete fBufferInfo;
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
	if (fIsConnected) Disconnect();

	BWindow::Quit();
}


void
BDirectWindow::DispatchMessage(BMessage *message,
							   BHandler *handler)
{
	BWindow::DispatchMessage(message, handler);
}


void
BDirectWindow::MessageReceived(BMessage *message)
{
	BWindow::MessageReceived(message);
}


void
BDirectWindow::FrameMoved(BPoint new_position)
{
	clipping_rect * bounds = &fBufferInfo->window_bounds;
	
	int32 dx = int32(new_position.x - bounds->left);
	int32 dy = int32(new_position.y - bounds->top);
	
	bounds->top += dy; 
	bounds->left += dx;
	bounds->right += dx;
	bounds->bottom += dy;
	
	fBufferInfo->buffer_state = direct_buffer_state(B_DIRECT_MODIFY | B_BUFFER_MOVED);
	DoDirectConnected();
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
	if (state)
		Connect();
	else
		Disconnect();
}


void
BDirectWindow::FrameResized(float new_width,
							float new_height)
{
	clipping_rect * bounds = &fBufferInfo->window_bounds;
	
	bounds->right = int32(bounds->left + new_width);
	bounds->bottom = int32(bounds->top + new_height);
	
	fBufferInfo->buffer_state = direct_buffer_state(B_DIRECT_MODIFY | B_BUFFER_RESIZED);
	DoDirectConnected();
}


void
BDirectWindow::Minimize(bool minimize)
{
	if (minimize)
	{
		Disconnect();
		BWindow::Minimize(false);
	}
	else
	{
		BWindow::Minimize(true);
		Connect();
	}
}


void
BDirectWindow::Zoom(BPoint rec_position,
					float rec_width,
					float rec_height)
{
	BWindow::Zoom(rec_position, rec_width, rec_height);
}


void
BDirectWindow::ScreenChanged(BRect screen_size,
							 color_space depth)
{
	direct_buffer_state state = B_DIRECT_MODIFY;

	if (fIsConnected && fIsFullScreen)
	{
		ResizeTo(screen_size.right, screen_size.bottom);
		state = direct_buffer_state(state | B_BUFFER_RESIZED);				
	}
	
	GetFrameBuffer();
	
	fBufferInfo->buffer_state = direct_buffer_state(state | B_BUFFER_RESET);
	DoDirectConnected();
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
	CreateClippingList();
}


void
BDirectWindow::Show()
{
	BWindow::Show();
	
	Connect();
}


void
BDirectWindow::Hide()
{
	Disconnect();

	BWindow::Hide();
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


//BMessage *
//BDirectWindow::ConvertToMessage(void *raw,
//								int32 code)
//{
//	return NULL;
//}


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
	return fIsFullScreen;
}


bool
BDirectWindow::SupportsWindowMode(screen_id)
{
	return true;
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


void
BDirectWindow::_ReservedDirectWindow5()
{
}


void
BDirectWindow::_ReservedDirectWindow6()
{
}


/* unimplemented for protection of the user:
 *
 * BDirectWindow::BDirectWindow()
 * BDirectWindow::BDirectWindow(BDirectWindow &)
 * BDirectWindow &BDirectWindow::operator=(BDirectWindow &)
 */

 
void
BDirectWindow::InitData(BRect frame)
{
	fBufferInfo = new direct_buffer_info;
	memset(fBufferInfo, 0, sizeof(direct_buffer_info));
	
	fBufferInfo->window_bounds.top = int32(frame.top);
	fBufferInfo->window_bounds.left = int32(frame.left);
	fBufferInfo->window_bounds.right = int32(frame.right);
	fBufferInfo->window_bounds.bottom = int32(frame.bottom);
	
	GetFrameBuffer();
}


void
BDirectWindow::DoDirectConnected()
{
	if (fIsConnected)
	{
		direct_buffer_info * info = new direct_buffer_info;
		memcpy(info, fBufferInfo, sizeof(direct_buffer_info));
		
		DirectConnected(info);
		
		delete info;
	}
}

void
BDirectWindow::Connect()
{
	fIsConnected = true;
	fBufferInfo->buffer_state = B_DIRECT_START;
	DoDirectConnected();
}

void
BDirectWindow::Disconnect()
{
	fBufferInfo->buffer_state = B_DIRECT_STOP;
	DoDirectConnected();
	fIsConnected = false;
}

void
BDirectWindow::CreateClippingList()
{
}

void
BDirectWindow::GetFrameBuffer()
{
}

