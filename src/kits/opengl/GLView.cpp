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


#include <assert.h>
#include <stdio.h>

#include <GLView.h>
// #include "GLRenderersManager.h"
#include "GLRenderer.h"

BGLView::BGLView(BRect rect, char *name, ulong resizingMode, ulong mode, ulong options)
   : BView(rect, name, B_FOLLOW_ALL_SIDES, mode | B_WILL_DRAW | B_FRAME_EVENTS), //  | B_FULL_UPDATE_ON_RESIZE)
		fRenderer(NULL)
{
	// TODO: get a renderer instance!
}


BGLView::~BGLView()
{
	if (fRenderer)
		fRenderer->Release();
}

void BGLView::LockGL()
{
	// TODO: acquire the OpenGL API lock it on this glview

	if (fRenderer)
		fRenderer->LockGL();
}

void BGLView::UnlockGL()
{
	if (fRenderer)
		fRenderer->UnlockGL();

	// TODO: release the GL API lock to others glviews
}

void BGLView::SwapBuffers()
{
	SwapBuffers(false);
}

void BGLView::SwapBuffers(bool vSync)
{
	if (fRenderer)
		fRenderer->SwapBuffers(vSync);
}

BView *	BGLView::EmbeddedView()
{
	return NULL;
}

status_t BGLView::CopyPixelsOut(BPoint source, BBitmap *dest)
{
	if (!fRenderer)
		return B_ERROR;
	
	if (! dest || ! dest->Bounds().IsValid())
		return B_BAD_VALUE;

	return fRenderer->CopyPixelsOut(source, dest);
}

status_t BGLView::CopyPixelsIn(BBitmap *source, BPoint dest)
{
	if (!fRenderer)
		return B_ERROR;
	
	if (! source || ! source->Bounds().IsValid())
		return B_BAD_VALUE;

	return fRenderer->CopyPixelsIn(source, dest);
}

/* Mesa's GLenum is not ulong but uint, so we can't use GLenum
   without breaking this method signature.
   Instead, we have to use the effective BeOS's SGI OpenGL GLenum type:
   unsigned long.
 */
void BGLView::ErrorCallback(unsigned long errorCode) 
{
	char msg[32];
	sprintf(msg, "GL: Error code $%04lx.", errorCode);
	// TODO: under BeOS R5, it call debugger(msg);
	fprintf(stderr, "%s\n", msg);
}

void BGLView::Draw(BRect updateRect)
{
	if (fRenderer)
		return fRenderer->Draw(updateRect);
	
	// TODO: auto-size and center the string
	MovePenTo(8, 32);
	DrawString("No OpenGL renderer available!");
}

void BGLView::AttachedToWindow()
{
	BView::AttachedToWindow();

	if (fRenderer) {
		// Don't paint white window background when resized
		SetViewColor(B_TRANSPARENT_32_BIT);
		fRenderer->AttachedToWindow();

		// Set default OpenGL viewport:
		glViewport(0, 0, Bounds().IntegerWidth(), Bounds().IntegerHeight());

		return;
	}
	
	// No Renderer, no rendering. Setup a minimal "No Renderer" string drawing context
	SetFont(be_bold_font);
	// SetFontSize(16);
}

void BGLView::AllAttached()
{
	BView::AllAttached();

	if (fRenderer)
		fRenderer->AllAttached();
}

void BGLView::DetachedFromWindow()
{
	if (fRenderer)
		fRenderer->DetachedFromWindow();

	BView::DetachedFromWindow();
}

void BGLView::AllDetached()
{
	if (fRenderer)
		fRenderer->AllDetached();

	BView::AllDetached();
}

void BGLView::FrameResized(float width, float height)
{
	if (fRenderer)
		fRenderer->FrameResized(width, height);

   	BView::FrameResized(width, height);
}

status_t BGLView::Perform(perform_code d, void *arg)
{
   	return BView::Perform(d, arg);
}


status_t BGLView::Archive(BMessage *data, bool deep) const
{
	return BView::Archive(data, deep);
}

void BGLView::MessageReceived(BMessage *msg)
{
	BView::MessageReceived(msg);
}

void BGLView::SetResizingMode(uint32 mode)
{
	BView::SetResizingMode(mode);
}

void BGLView::Show()
{
	BView::Show();
}

void BGLView::Hide()
{
	BView::Hide();
}

BHandler *BGLView::ResolveSpecifier(BMessage *msg, int32 index,
                                    BMessage *specifier, int32 form,
                                    const char *property)
{
	return BView::ResolveSpecifier(msg, index, specifier, form, property);
}

status_t BGLView::GetSupportedSuites(BMessage *data)
{
	return BView::GetSupportedSuites(data);
}

void BGLView::DirectConnected( direct_buffer_info *info )
{
	/* TODO:
         - update local copy of caller's BDirectWindow's direct_buffer_info
         - clip it against BGLView's visible region
         - pass the view's clipped direct_buffer_info to renderer's DirectConnect()  
	 */
	
#if 0
	if (! m_direct_connected && m_direct_connection_disabled) 
		return; 

	direct_info_locker->Lock(); 
	switch(info->buffer_state & B_DIRECT_MODE_MASK) { 
	case B_DIRECT_START: 
		m_direct_connected = true;
	case B_DIRECT_MODIFY: 
		// Get clipping information 
		if (m_clip_list)
			free(m_clip_list); 
		m_clip_list_count = info->clip_list_count; 
		m_clip_list = (clipping_rect *) malloc(m_clip_list_count*sizeof(clipping_rect)); 
		if (m_clip_list) { 
			memcpy(m_clip_list, info->clip_list, m_clip_list_count*sizeof(clipping_rect));
			fBits = (uint8 *) info->bits; 
			fRowBytes = info->bytes_per_row; 
			fFormat = info->pixel_format; 
			fBounds = info->window_bounds; 
			fDirty = true; 
		} 
		break; 
	case B_DIRECT_STOP: 
		fConnected = false; 
		break; 
	} 
	direct_info_locker->Unlock(); 
#endif

	if (fRenderer)
		fRenderer->DirectConnected(info);
}

void BGLView::EnableDirectMode( bool enabled )
{
	if (fRenderer)
		fRenderer->EnableDirectMode(enabled);
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


