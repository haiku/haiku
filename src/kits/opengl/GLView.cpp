/*
 * Copyright 2006-2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, korli@users.berlios.de
 *		Philippe Houdoin, philippe.houdoin@free.fr
 *		Stefano Ceccherini, burton666@libero.it
 */

/*
 * Mesa 3-D graphics library
 * Version:  6.1
 * 
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include <GLView.h>

#include <assert.h>
#include <stdio.h>

#include <DirectWindow.h>
#include <GLRenderer.h>

#include "DirectWindowPrivate.h"
#include "GLRendererRoster.h"

struct glview_direct_info {
	direct_buffer_info *direct_info;
	bool direct_connected;
	bool enable_direct_mode;
	
	glview_direct_info();
	~glview_direct_info();
};


BGLView::BGLView(BRect rect, char *name, ulong resizingMode, ulong mode,
		ulong options)
   : BView(rect, name, B_FOLLOW_ALL_SIDES, mode | B_WILL_DRAW | B_FRAME_EVENTS), //  | B_FULL_UPDATE_ON_RESIZE)
	fGc(NULL),
	fOptions(options),
	fDitherCount(0),
	fDrawLock("BGLView draw lock"),
	fDisplayLock("BGLView display lock"),
	fClipInfo(NULL),	
	fRenderer(NULL),
	fRoster(NULL),
	fDitherMap(NULL)	
{
	fRoster = new GLRendererRoster(this, options);
}


BGLView::~BGLView()
{
	delete (glview_direct_info *)fClipInfo;
	if (fRenderer)
		fRenderer->Release();
}


void
BGLView::LockGL()
{
	// TODO: acquire the OpenGL API lock it on this glview

	fDisplayLock.Lock();
	if (fRenderer)
		fRenderer->LockGL();
}


void
BGLView::UnlockGL()
{
	if (fRenderer)
		fRenderer->UnlockGL();
	fDisplayLock.Unlock();
	
	// TODO: release the GL API lock to others glviews
}


void
BGLView::SwapBuffers()
{
	SwapBuffers(false);
}


void
BGLView::SwapBuffers(bool vSync)
{
	if (fRenderer) {
		_LockDraw();
		fRenderer->SwapBuffers(vSync);
		_UnlockDraw();
	}
}


BView *
BGLView::EmbeddedView()
{
	return NULL;
}


status_t
BGLView::CopyPixelsOut(BPoint source, BBitmap *dest)
{
	if (!fRenderer)
		return B_ERROR;
	
	if (!dest || !dest->Bounds().IsValid())
		return B_BAD_VALUE;

	return fRenderer->CopyPixelsOut(source, dest);
}


status_t
BGLView::CopyPixelsIn(BBitmap *source, BPoint dest)
{
	if (!fRenderer)
		return B_ERROR;
	
	if (!source || !source->Bounds().IsValid())
		return B_BAD_VALUE;

	return fRenderer->CopyPixelsIn(source, dest);
}


/*!	Mesa's GLenum is not ulong but uint, so we can't use GLenum
	without breaking this method signature.
	Instead, we have to use the effective BeOS's SGI OpenGL GLenum type:
	unsigned long.
 */
void
BGLView::ErrorCallback(unsigned long errorCode) 
{
	char msg[32];
	sprintf(msg, "GL: Error code $%04lx.", errorCode);
	// TODO: under BeOS R5, it call debugger(msg);
	fprintf(stderr, "%s\n", msg);
}


void
BGLView::Draw(BRect updateRect)
{
	if (fRenderer) {
		_LockDraw();	
		fRenderer->Draw(updateRect);
		_UnlockDraw();
		return;	
	}
	// TODO: auto-size and center the string
	MovePenTo(8, 32);
	DrawString("No OpenGL renderer available!");
}


void
BGLView::AttachedToWindow()
{
	BView::AttachedToWindow();

	fBounds = Bounds();
	for (BView *view = this; view != NULL; view = view->Parent())
		view->ConvertToParent(&fBounds);

	fRenderer = fRoster->GetRenderer();
	if (fRenderer != NULL) {
		// Jackburton: The following code was commented because it doesn't look
		// good in "direct" mode:
		// when the window is moved, the app_server doesn't paint the view's
		// background, and the stuff behind the window itself shows up. 
		// Setting the view color to black, instead, looks a bit more elegant.
#if 0
		// Don't paint white window background when resized
		SetViewColor(B_TRANSPARENT_32_BIT);
#else
		SetViewColor(0, 0, 0);
#endif
		// Set default OpenGL viewport:
		glViewport(0, 0, Bounds().IntegerWidth(), Bounds().IntegerHeight());
		fRenderer->FrameResized(Bounds().IntegerWidth(), Bounds().IntegerHeight());

		if (fClipInfo) {
			fRenderer->DirectConnected(
				((glview_direct_info *)fClipInfo)->direct_info);
			fRenderer->EnableDirectMode(
				((glview_direct_info *)fClipInfo)->enable_direct_mode);
		}

		return;
	}
	
	fprintf(stderr, "no renderer found! \n");

	// No Renderer, no rendering. Setup a minimal "No Renderer" string drawing
	// context
	SetFont(be_bold_font);
	// SetFontSize(16);
}


void
BGLView::AllAttached()
{
	BView::AllAttached();
}

	
void
BGLView::DetachedFromWindow()
{
	if (fRenderer)
		fRenderer->Release();
	fRenderer = NULL;

	BView::DetachedFromWindow();
}


void
BGLView::AllDetached()
{
	BView::AllDetached();
}


void
BGLView::FrameResized(float width, float height)
{
	fBounds = Bounds();
	for (BView *v = this; v; v = v->Parent())
		v->ConvertToParent(&fBounds);

	if (fRenderer) {
		LockGL();
		_LockDraw();
		fRenderer->FrameResized(width, height);
		_UnlockDraw();
		UnlockGL();
	}

   	BView::FrameResized(width, height);
}


status_t
BGLView::Perform(perform_code d, void *arg)
{
   	return BView::Perform(d, arg);
}


status_t
BGLView::Archive(BMessage *data, bool deep) const
{
	return BView::Archive(data, deep);
}


void
BGLView::MessageReceived(BMessage *msg)
{
	BView::MessageReceived(msg);
}


void
BGLView::SetResizingMode(uint32 mode)
{
	BView::SetResizingMode(mode);
}


void
BGLView::Show()
{
	BView::Show();
}


void
BGLView::Hide()
{
	BView::Hide();
}


BHandler *
BGLView::ResolveSpecifier(BMessage *msg, int32 index,
                                    BMessage *specifier, int32 form,
                                    const char *property)
{
	return BView::ResolveSpecifier(msg, index, specifier, form, property);
}


status_t
BGLView::GetSupportedSuites(BMessage *data)
{
	return BView::GetSupportedSuites(data);
}


void
BGLView::DirectConnected(direct_buffer_info *info)
{
	if (!fClipInfo/* && m_direct_connection_disabled*/) {
		fClipInfo = new glview_direct_info();
	}

	glview_direct_info *glviewDirectInfo = (glview_direct_info *)fClipInfo;
	direct_buffer_info *localInfo = glviewDirectInfo->direct_info;
	
	switch(info->buffer_state & B_DIRECT_MODE_MASK) { 
		case B_DIRECT_START:
			glviewDirectInfo->direct_connected = true;
			_UnlockDraw();
		case B_DIRECT_MODIFY:
		{
			_LockDraw();
			localInfo->buffer_state = info->buffer_state;
			localInfo->driver_state = info->driver_state;
			localInfo->bits = info->bits;
			localInfo->pci_bits = info->pci_bits;
			localInfo->bytes_per_row = info->bytes_per_row;
			localInfo->bits_per_pixel = info->bits_per_pixel;
			localInfo->pixel_format = info->pixel_format;
			localInfo->layout = info->layout;
			localInfo->orientation = info->orientation;
			//memcpy(&localInfo->_reserved, info->_reserved, sizeof(info->_reserved));
			//localInfo->_dd_type_ = info->_dd_type_;
			//localInfo->_dd_token_ = info->_dd_token_;

			// Collect the rects into a BRegion, then clip to the view's bounds
			BRegion region;
			for (uint32 c = 0; c < info->clip_list_count; c++)
				region.Include(info->clip_list[c]);
			BRegion boundsRegion = fBounds.OffsetByCopy(info->window_bounds.left, info->window_bounds.top);
			localInfo->window_bounds = boundsRegion.RectAtInt(0); // window_bounds are now view bounds
			region.IntersectWith(&boundsRegion);
				
			localInfo->clip_list_count = region.CountRects();
			localInfo->clip_bounds = region.FrameInt();
			
			for (uint32 c = 0; c < localInfo->clip_list_count; c++)
				localInfo->clip_list[c] = region.RectAtInt(c);
			
			_UnlockDraw();
			break; 
		}		
		case B_DIRECT_STOP: 
			glviewDirectInfo->direct_connected = false;
			_LockDraw();
			break; 
	} 
	
	if (fRenderer)
		fRenderer->DirectConnected(localInfo);
}


void
BGLView::EnableDirectMode(bool enabled)
{
	if (fRenderer)
		fRenderer->EnableDirectMode(enabled);
	if (!fClipInfo) {
                fClipInfo = new glview_direct_info();
	}
	((glview_direct_info *)fClipInfo)->enable_direct_mode = enabled;
}


void
BGLView::_LockDraw()
{
	glview_direct_info *info = (glview_direct_info *)fClipInfo;

	if (!info || !info->enable_direct_mode)
		return;

	fDrawLock.Lock();
}


void
BGLView::_UnlockDraw()
{
	glview_direct_info *info = (glview_direct_info *)fClipInfo;

	if (!info || !info->enable_direct_mode)
		return;
	
	fDrawLock.Unlock();
}


//---- virtual reserved methods ----------

void BGLView::_ReservedGLView1() {}
void BGLView::_ReservedGLView2() {}
void BGLView::_ReservedGLView3() {}
void BGLView::_ReservedGLView4() {}
void BGLView::_ReservedGLView5() {}
void BGLView::_ReservedGLView6() {}
void BGLView::_ReservedGLView7() {}
void BGLView::_ReservedGLView8() {}

#if 0
// Not implemented!!!

BGLView::BGLView(const BGLView &v)
	: BView(v)
{
   // XXX not sure how this should work
   printf("Warning BGLView::copy constructor not implemented\n");
}

BGLView &BGLView::operator=(const BGLView &v)
{
   printf("Warning BGLView::operator= not implemented\n");
	return *this;
}
#endif

// #pragma mark -

#if 0

// TODO: implement BGLScreen class...

BGLScreen::BGLScreen(char *name, ulong screenMode, ulong options,
			status_t *error, bool debug)
  : BWindowScreen(name, screenMode, error, debug)
{
}

BGLScreen::~BGLScreen()
{
}

void BGLScreen::LockGL()
{
}

void BGLScreen::UnlockGL()
{
}

void BGLScreen::SwapBuffers()
{
}

void BGLScreen::ErrorCallback(unsigned long errorCode) // Mesa's GLenum is not ulong but uint!
{
	char msg[32];
	sprintf(msg, "GL: Error code $%04lx.", errorCode);
	// debugger(msg);
	fprintf(stderr, "%s\n", msg);
	return;
}

void BGLScreen::ScreenConnected(bool enabled)
{
}


void BGLScreen::FrameResized(float width, float height)
{
   return BWindowScreen::FrameResized(width, height);
}

status_t BGLScreen::Perform(perform_code d, void *arg)
{
   return BWindowScreen::Perform(d, arg);
}


status_t BGLScreen::Archive(BMessage *data, bool deep) const
{
   return BWindowScreen::Archive(data, deep);
}

void BGLScreen::MessageReceived(BMessage *msg)
{
   BWindowScreen::MessageReceived(msg);
}

void BGLScreen::Show()
{
   BWindowScreen::Show();
}

void BGLScreen::Hide()
{
   BWindowScreen::Hide();
}

BHandler *BGLScreen::ResolveSpecifier(BMessage *msg, int32 index,
                                    BMessage *specifier, int32 form,
                                    const char *property)
{
   return BWindowScreen::ResolveSpecifier(msg, index, specifier, form, property);
}

status_t BGLScreen::GetSupportedSuites(BMessage *data)
{
   return BWindowScreen::GetSupportedSuites(data);
}


//---- virtual reserved methods ----------

void BGLScreen::_ReservedGLScreen1() {}
void BGLScreen::_ReservedGLScreen2() {}
void BGLScreen::_ReservedGLScreen3() {}
void BGLScreen::_ReservedGLScreen4() {}
void BGLScreen::_ReservedGLScreen5() {}
void BGLScreen::_ReservedGLScreen6() {}
void BGLScreen::_ReservedGLScreen7() {}
void BGLScreen::_ReservedGLScreen8() {}

#endif

const char * color_space_name(color_space space)
{
#define C2N(a)	case a:	return #a

	switch (space) {
	C2N(B_RGB24);
	C2N(B_RGB32);
	C2N(B_RGBA32);
	C2N(B_RGB32_BIG);
	C2N(B_RGBA32_BIG);
	C2N(B_GRAY8);
	C2N(B_GRAY1);
	C2N(B_RGB16);
	C2N(B_RGB15);
	C2N(B_RGBA15);
	C2N(B_CMAP8);
	default:
		return "Unknown!";
	};

#undef C2N
};


glview_direct_info::glview_direct_info()
{
	// TODO: See direct_window_data() in app_server's ServerWindow.cpp
	direct_info = (direct_buffer_info *)calloc(1, DIRECT_BUFFER_INFO_AREA_SIZE);
	direct_connected = false;
	enable_direct_mode = false;
}


glview_direct_info::~glview_direct_info()
{
	free(direct_info);
}

