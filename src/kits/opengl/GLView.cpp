/*
 * Copyright 2006-2012, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, korli@users.berlios.de
 *		Philippe Houdoin, philippe.houdoin@free.fr
 *		Stefano Ceccherini, burton666@libero.it
 */


#include <GLView.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <DirectWindow.h>
#include <GLRenderer.h>

#include "DirectWindowPrivate.h"
#include "GLDispatcher.h"
#include "GLRendererRoster.h"


struct glview_direct_info {
	direct_buffer_info* direct_info;
	bool direct_connected;
	bool enable_direct_mode;

	glview_direct_info();
	~glview_direct_info();
};


BGLView::BGLView(BRect rect, const char* name, ulong resizingMode, ulong mode,
	ulong options)
	:
	BView(rect, name, B_FOLLOW_ALL_SIDES, mode | B_WILL_DRAW | B_FRAME_EVENTS),
		//  | B_FULL_UPDATE_ON_RESIZE)
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
	delete fClipInfo;
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


BView*
BGLView::EmbeddedView()
{
	return NULL;
}


void*
BGLView::GetGLProcAddress(const char* procName)
{
	BGLDispatcher* glDispatcher = NULL;

	if (fRenderer)
		glDispatcher = fRenderer->GLDispatcher();

	if (glDispatcher)
		return (void*)glDispatcher->AddressOf(procName);

	return NULL;
}


status_t
BGLView::CopyPixelsOut(BPoint source, BBitmap* dest)
{
	if (!fRenderer)
		return B_ERROR;

	if (!dest || !dest->Bounds().IsValid())
		return B_BAD_VALUE;

	return fRenderer->CopyPixelsOut(source, dest);
}


status_t
BGLView::CopyPixelsIn(BBitmap* source, BPoint dest)
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
	for (BView* view = this; view != NULL; view = view->Parent())
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
		LockGL();
		glViewport(0, 0, Bounds().IntegerWidth(), Bounds().IntegerHeight());
		UnlockGL();
		fRenderer->FrameResized(Bounds().IntegerWidth(),
			Bounds().IntegerHeight());

		if (fClipInfo) {
			fRenderer->DirectConnected(fClipInfo->direct_info);
			fRenderer->EnableDirectMode(fClipInfo->enable_direct_mode);
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
	for (BView* v = this; v; v = v->Parent())
		v->ConvertToParent(&fBounds);

	if (fRenderer) {
		LockGL();
		_LockDraw();
		_CallDirectConnected();
		fRenderer->FrameResized(width, height);
		_UnlockDraw();
		UnlockGL();
	}

	BView::FrameResized(width, height);
}


status_t
BGLView::Perform(perform_code d, void* arg)
{
	return BView::Perform(d, arg);
}


status_t
BGLView::Archive(BMessage* data, bool deep) const
{
	return BView::Archive(data, deep);
}


void
BGLView::MessageReceived(BMessage* msg)
{
	BView::MessageReceived(msg);
}


void
BGLView::SetResizingMode(uint32 mode)
{
	BView::SetResizingMode(mode);
}


void
BGLView::GetPreferredSize(float* _width, float* _height)
{
	if (_width)
		*_width = 0;
	if (_height)
		*_height = 0;
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


BHandler*
BGLView::ResolveSpecifier(BMessage* msg, int32 index, BMessage* specifier,
	int32 form, const char* property)
{
	return BView::ResolveSpecifier(msg, index, specifier, form, property);
}


status_t
BGLView::GetSupportedSuites(BMessage* data)
{
	return BView::GetSupportedSuites(data);
}


void
BGLView::DirectConnected(direct_buffer_info* info)
{
	if (fClipInfo == NULL) {
		fClipInfo = new (std::nothrow) glview_direct_info();
		if (fClipInfo == NULL)
			return;
	}

	direct_buffer_info* localInfo = fClipInfo->direct_info;

	switch (info->buffer_state & B_DIRECT_MODE_MASK) {
		case B_DIRECT_START:
			fClipInfo->direct_connected = true;
			memcpy(localInfo, info, DIRECT_BUFFER_INFO_AREA_SIZE);
			_UnlockDraw();
			break;

		case B_DIRECT_MODIFY:
			_LockDraw();
			memcpy(localInfo, info, DIRECT_BUFFER_INFO_AREA_SIZE);
			_UnlockDraw();
			break;

		case B_DIRECT_STOP:
			fClipInfo->direct_connected = false;
			_LockDraw();
			break;
	}

	if (fRenderer)
		_CallDirectConnected();
}


void
BGLView::EnableDirectMode(bool enabled)
{
	if (fRenderer)
		fRenderer->EnableDirectMode(enabled);
	if (fClipInfo == NULL) {
		fClipInfo = new (std::nothrow) glview_direct_info();
		if (fClipInfo == NULL)
			return;
	}

	fClipInfo->enable_direct_mode = enabled;
}


void
BGLView::_LockDraw()
{
	if (!fClipInfo || !fClipInfo->enable_direct_mode)
		return;

	fDrawLock.Lock();
}


void
BGLView::_UnlockDraw()
{
	if (!fClipInfo || !fClipInfo->enable_direct_mode)
		return;

	fDrawLock.Unlock();
}


void
BGLView::_CallDirectConnected()
{
	if (!fClipInfo)
		return;

	direct_buffer_info* localInfo = fClipInfo->direct_info;
	direct_buffer_info* info = (direct_buffer_info*)malloc(
		DIRECT_BUFFER_INFO_AREA_SIZE);
	if (info == NULL)
		return;

	memcpy(info, localInfo, DIRECT_BUFFER_INFO_AREA_SIZE);

	// Collect the rects into a BRegion, then clip to the view's bounds
	BRegion region;
	for (uint32 c = 0; c < localInfo->clip_list_count; c++)
		region.Include(localInfo->clip_list[c]);
	BRegion boundsRegion = fBounds.OffsetByCopy(localInfo->window_bounds.left,
		localInfo->window_bounds.top);
	info->window_bounds = boundsRegion.RectAtInt(0);
		// window_bounds are now view bounds
	region.IntersectWith(&boundsRegion);

	info->clip_list_count = region.CountRects();
	info->clip_bounds = region.FrameInt();

	for (uint32 c = 0; c < info->clip_list_count; c++)
		info->clip_list[c] = region.RectAtInt(c);
	fRenderer->DirectConnected(info);
	free(info);
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


// #pragma mark -


// BeOS compatibility: contrary to others BView's contructors,
// BGLView one wants a non-const name argument.
BGLView::BGLView(BRect rect, char* name, ulong resizingMode, ulong mode,
	ulong options)
	:
	BView(rect, name, B_FOLLOW_ALL_SIDES, mode | B_WILL_DRAW | B_FRAME_EVENTS),
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


#if 0
// TODO: implement BGLScreen class...


BGLScreen::BGLScreen(char* name, ulong screenMode, ulong options,
		status_t* error, bool debug)
	:
	BWindowScreen(name, screenMode, error, debug)
{
}


BGLScreen::~BGLScreen()
{
}


void
BGLScreen::LockGL()
{
}


void
BGLScreen::UnlockGL()
{
}


void
BGLScreen::SwapBuffers()
{
}


void
BGLScreen::ErrorCallback(unsigned long errorCode)
{
	// Mesa's GLenum is not ulong but uint!
	char msg[32];
	sprintf(msg, "GL: Error code $%04lx.", errorCode);
	// debugger(msg);
	fprintf(stderr, "%s\n", msg);
	return;
}


void
BGLScreen::ScreenConnected(bool enabled)
{
}


void
BGLScreen::FrameResized(float width, float height)
{
	return BWindowScreen::FrameResized(width, height);
}


status_t
BGLScreen::Perform(perform_code d, void* arg)
{
	return BWindowScreen::Perform(d, arg);
}


status_t
BGLScreen::Archive(BMessage* data, bool deep) const
{
	return BWindowScreen::Archive(data, deep);
}


void
BGLScreen::MessageReceived(BMessage* msg)
{
	BWindowScreen::MessageReceived(msg);
}


void
BGLScreen::Show()
{
	BWindowScreen::Show();
}


void
BGLScreen::Hide()
{
	BWindowScreen::Hide();
}


BHandler*
BGLScreen::ResolveSpecifier(BMessage* msg, int32 index, BMessage* specifier,
	int32 form, const char* property)
{
	return BWindowScreen::ResolveSpecifier(msg, index, specifier,
		form, property);
}


status_t
BGLScreen::GetSupportedSuites(BMessage* data)
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


const char* color_space_name(color_space space)
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
	direct_info = (direct_buffer_info*)calloc(1, DIRECT_BUFFER_INFO_AREA_SIZE);
	direct_connected = false;
	enable_direct_mode = false;
}


glview_direct_info::~glview_direct_info()
{
	free(direct_info);
}

