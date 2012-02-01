/*
 * Copyright 2006-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, korli@users.berlios.de
 *		Philippe Houdoin, philippe.houdoin@free.fr
 *		Artur Wyszynski, harakash@gmail.com
 *		Alexander von Gluck, kallisti5@unixzen.com
 */
/*
 * Mesa 3-D graphics library
 * Version:  6.1
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
 */


#include "MesaSoftwareRenderer.h"

#include <Autolock.h>
#include <DirectWindowPrivate.h>
#include <GraphicsDefs.h>
#include <Screen.h>
#include <stdio.h>
#include <string.h>

extern "C" {
#include "extensions.h"
#include "drivers/common/driverfuncs.h"
#include "drivers/common/meta.h"
#include "main/colormac.h"
#include "main/cpuinfo.h"
#include "main/buffers.h"
#include "main/formats.h"
#include "main/framebuffer.h"
#include "main/renderbuffer.h"
#include "swrast/swrast.h"
#if __GNUC__ > 2
#include "swrast/s_renderbuffer.h"
#endif
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"
#include "vbo/vbo.h"


//#define TRACE_SOFTGL
#ifdef TRACE_SOFTGL
#	define TRACE(x...) printf("MesaSoftwareRenderer: " x)
#	define CALLED() printf("MesaSoftwareRenderer: %s\n", __PRETTY_FUNCTION__)
#else
#	define TRACE(x...)
#	define CALLED()
#endif

#define ERROR(x...) printf("MesaSoftwareRenderer: " x)
}


extern const char* color_space_name(color_space space);


// BeOS component ordering for B_RGBA32 bitmap format
#if B_HOST_IS_LENDIAN
#define BE_RCOMP 2
#define BE_GCOMP 1
#define BE_BCOMP 0
#define BE_ACOMP 3
#else
// Big Endian B_RGBA32 bitmap format
#define BE_RCOMP 1
#define BE_GCOMP 2
#define BE_BCOMP 3
#define BE_ACOMP 0
#endif


/**********************************************************************/
/*****        Read/write spans/arrays of pixels                   *****/
/**********************************************************************/
extern "C" {

/* 32-bit RGBA */
#define NAME(PREFIX) PREFIX##_RGBA32
#define RB_TYPE GLubyte
#define SPAN_VARS \
	MesaSoftwareRenderer* mr = (MesaSoftwareRenderer*)ctx->DriverCtx;
#define INIT_PIXEL_PTR(P, X, Y) \
	GLubyte* P = ((GLubyte**)mr->GetRows())[Y] + (X) * 4
#define INC_PIXEL_PTR(P) P += 4
#define STORE_PIXEL(DST, X, Y, VALUE) \
	DST[BE_RCOMP] = VALUE[RCOMP];  \
	DST[BE_GCOMP] = VALUE[GCOMP];  \
	DST[BE_BCOMP] = VALUE[BCOMP];  \
	DST[BE_ACOMP] = VALUE[ACOMP]
#define STORE_PIXEL_RGB(DST, X, Y, VALUE) \
	DST[BE_RCOMP] = VALUE[RCOMP];  \
	DST[BE_GCOMP] = VALUE[GCOMP];  \
	DST[BE_BCOMP] = VALUE[BCOMP];  \
	DST[BE_ACOMP] = 255
#define FETCH_PIXEL(DST, SRC) \
	DST[RCOMP] = SRC[BE_RCOMP];  \
	DST[GCOMP] = SRC[BE_GCOMP];  \
	DST[BCOMP] = SRC[BE_BCOMP];  \
	DST[ACOMP] = SRC[BE_ACOMP]
#include "swrast/s_spantemp.h"

/* 32-bit RGB */
#define NAME(PREFIX) PREFIX##_RGB32
#define RB_TYPE GLubyte
#define SPAN_VARS \
	MesaSoftwareRenderer* mr = (MesaSoftwareRenderer*)ctx->DriverCtx;
#define INIT_PIXEL_PTR(P, X, Y) \
	GLuint* P = (GLuint*)(((GLubyte**)mr->GetRows())[Y] + (X) * 4)
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL(DST, X, Y, VALUE) \
	*DST = ( ((VALUE[RCOMP]) << 16) | \
		((VALUE[GCOMP]) << 8) | \
		((VALUE[BCOMP]) ) )
#define FETCH_PIXEL(DST, SRC) \
	DST[RCOMP] = ((*SRC & 0x00ff0000) >> 16);  \
	DST[GCOMP] = ((*SRC & 0x0000ff00) >> 8);  \
	DST[BCOMP] = ((*SRC & 0x000000ff)); \
	DST[ACOMP] = 0xff;
#include "swrast/s_spantemp.h"

/* 24-bit RGB */
#define NAME(PREFIX) PREFIX##_RGB24
#define RB_TYPE GLubyte
#define SPAN_VARS \
	MesaSoftwareRenderer* mr = (MesaSoftwareRenderer*)ctx->DriverCtx;
#define INIT_PIXEL_PTR(P, X, Y) \
	GLubyte* P = ((GLubyte**)mr->GetRows())[Y] + (X) * 3
#define INC_PIXEL_PTR(P) P += 3
#define STORE_PIXEL(DST, X, Y, VALUE) \
	DST[BE_RCOMP] = VALUE[RCOMP]; \
	DST[BE_GCOMP] = VALUE[GCOMP]; \
	DST[BE_BCOMP] = VALUE[BCOMP];
#define FETCH_PIXEL(DST, SRC) \
	DST[RCOMP] = SRC[BE_RCOMP];  \
	DST[GCOMP] = SRC[BE_GCOMP];  \
	DST[BCOMP] = SRC[BE_BCOMP]; \
	DST[ACOMP] = 0xff;
#include "swrast/s_spantemp.h"

/* 16-bit RGB */
#define NAME(PREFIX) PREFIX##_RGB16
#define RB_TYPE GLubyte
#define SPAN_VARS \
	MesaSoftwareRenderer* mr = (MesaSoftwareRenderer*)ctx->DriverCtx;
#define INIT_PIXEL_PTR(P, X, Y) \
	GLushort* P = (GLushort*)(((GLubyte**)mr->GetRows())[Y] + (X) * 2)
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL(DST, X, Y, VALUE) \
	*DST = ( (((VALUE[RCOMP]) & 0xf8) << 8) | \
		(((VALUE[GCOMP]) & 0xfc) << 3) | \
		(((VALUE[BCOMP])       ) >> 3) )
#define FETCH_PIXEL(DST, SRC) \
	DST[RCOMP] = ((*SRC & 0xf800) >> 8); \
	DST[GCOMP] = ((*SRC & 0x07e0) >> 3); \
	DST[BCOMP] = ((*SRC & 0x001f) << 3); \
	DST[ACOMP] = 0xff
#include "swrast/s_spantemp.h"

/* 15-bit RGB */
#define NAME(PREFIX) PREFIX##_RGB15
#define RB_TYPE GLubyte
#define SPAN_VARS \
	MesaSoftwareRenderer* mr = (MesaSoftwareRenderer*)ctx->DriverCtx;
#define INIT_PIXEL_PTR(P, X, Y) \
	GLushort* P = (GLushort*)(((GLubyte**)mr->GetRows())[Y] + (X) * 2)
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL(DST, X, Y, VALUE) \
	*DST = ( (((VALUE[RCOMP]) & 0xf8) << 7) | \
		(((VALUE[GCOMP]) & 0xf8) << 2) | \
		(((VALUE[BCOMP])       ) >> 3) )
#define FETCH_PIXEL(DST, SRC) \
	DST[RCOMP] = ((*SRC & 0x7c00) >> 7); \
	DST[GCOMP] = ((*SRC & 0x03e0) >> 2); \
	DST[BCOMP] = ((*SRC & 0x001f) << 3); \
	DST[ACOMP] = 0xff
#include "swrast/s_spantemp.h"
}


extern "C" _EXPORT BGLRenderer*
instantiate_gl_renderer(BGLView* view, ulong options,
	BGLDispatcher* dispatcher)
{
	return new MesaSoftwareRenderer(view, options, dispatcher);
}


MesaSoftwareRenderer::MesaSoftwareRenderer(BGLView* view, ulong options,
	BGLDispatcher* dispatcher)
	: BGLRenderer(view, options, dispatcher),
	fBitmap(NULL),
	fDirectModeEnabled(false),
	fInfo(NULL),
	fInfoLocker("info locker"),
	fContext(NULL),
	fVisual(NULL),
	fFrameBuffer(NULL),
	fFrontRenderBuffer(NULL),
	fBackRenderBuffer(NULL),
	fColorSpace(B_NO_COLOR_SPACE)
{
	CALLED();

	fClearColor[BE_RCOMP] = 0;
	fClearColor[BE_GCOMP] = 0;
	fClearColor[BE_BCOMP] = 0;
	fClearColor[BE_ACOMP] = 0;

	fClearIndex = 0;

	fColorSpace = BScreen(GLView()->Window()).ColorSpace();

	// We force single buffering for the time being
	options &= ~BGL_DOUBLE;

	const GLboolean rgbFlag = ((options & BGL_INDEX) == 0);
	const GLboolean alphaFlag = ((options & BGL_ALPHA) == BGL_ALPHA);
	const GLboolean dblFlag = ((options & BGL_DOUBLE) == BGL_DOUBLE);
	const GLboolean stereoFlag = false;
	const GLint depth = (options & BGL_DEPTH) ? 16 : 0;
	const GLint stencil = (options & BGL_STENCIL) ? 8 : 0;
	const GLint accum = (options & BGL_ACCUM) ? 16 : 0;
	const GLint red = rgbFlag ? 8 : 0;
	const GLint green = rgbFlag ? 8 : 0;
	const GLint blue = rgbFlag ? 8 : 0;
	const GLint alpha = alphaFlag ? 8 : 0;

	fOptions = options; // | BGL_INDIRECT;
	struct dd_function_table functions;

	fVisual = _mesa_create_visual(dblFlag, stereoFlag, red, green,
		blue, alpha, depth, stencil, accum, accum, accum,
		alpha ? accum : 0, 1);

	// Initialize device driver function table
	_mesa_init_driver_functions(&functions);

	functions.GetString 	= _GetString;
	functions.UpdateState 	= _UpdateState;
	functions.GetBufferSize = NULL;
	functions.Error			= _Error;
	functions.Viewport		= _Viewport;
	functions.Flush			= _Flush;

	// create core context
#if HAIKU_MESA_VER <= 708
	fContext = _mesa_create_context(fVisual, NULL, &functions, this);
#else
	fContext = _mesa_create_context(API_OPENGL, fVisual, NULL,
		&functions, this);
#endif

	if (!fContext) {
		ERROR("%s: Failed to create Mesa context!\n", __func__);
		_mesa_destroy_visual(fVisual);
		return;
	}

	/* Initialize the software rasterizer and helper modules. */
	_swrast_CreateContext(fContext);
	_vbo_CreateContext(fContext);
	_tnl_CreateContext(fContext);
	_swsetup_CreateContext(fContext);
	_swsetup_Wakeup(fContext);

	// Use default TCL pipeline
	TNL_CONTEXT(fContext)->Driver.RunPipeline = _tnl_run_pipeline;

	_mesa_meta_init(fContext);
	_mesa_enable_sw_extensions(fContext);
	_mesa_enable_1_3_extensions(fContext);
	_mesa_enable_1_4_extensions(fContext);
	_mesa_enable_1_5_extensions(fContext);
	_mesa_enable_2_0_extensions(fContext);
	_mesa_enable_2_1_extensions(fContext);

	// create core framebuffer
	fFrameBuffer = (struct msr_framebuffer*)calloc(1,
		sizeof(*fFrameBuffer));
	_mesa_initialize_window_framebuffer(&fFrameBuffer->base, fVisual);

	// Setup front render buffer
	fFrontRenderBuffer = _NewRenderBuffer(true);
	if (fFrontRenderBuffer == NULL) {
		_mesa_destroy_visual(fVisual);
		return;
	}
	_mesa_add_renderbuffer(&fFrameBuffer->base, BUFFER_FRONT_LEFT,
		&fFrontRenderBuffer->base);

	// Setup back render buffer (if needed)
	if (fVisual->doubleBufferMode) {
		fBackRenderBuffer = _NewRenderBuffer(false);
		if (fBackRenderBuffer == NULL) {
			_mesa_destroy_visual(fVisual);
			return;
		}
		_mesa_add_renderbuffer(&fFrameBuffer->base, BUFFER_BACK_LEFT,
			&fBackRenderBuffer->base);
	}

#if HAIKU_MESA_VER >= 709
	// New Mesa
	_swrast_add_soft_renderbuffers(&fFrameBuffer->base, GL_FALSE,
		fVisual->haveDepthBuffer, fVisual->haveStencilBuffer,
		fVisual->haveAccumBuffer, alphaFlag, GL_FALSE);
#else
	// Old Mesa
	_mesa_add_soft_renderbuffers(&fFrameBuffer->base, GL_FALSE,
		fVisual->haveDepthBuffer, fVisual->haveStencilBuffer,
		fVisual->haveAccumBuffer, alphaFlag, GL_FALSE);
#endif

	BRect bounds = view->Bounds();
	fWidth = fNewWidth = (GLint)bounds.Width();
	fHeight = fNewHeight = (GLint)bounds.Height();

	// some stupid applications (Quake2) don't even think about calling LockGL()
	// before using glGetString and its glGet*() friends...
	// so make sure there is at least a valid context.

	if (!_mesa_get_current_context()) {
		LockGL();
		// not needed, we don't have a looper yet: UnlockLooper();
	}
}


MesaSoftwareRenderer::~MesaSoftwareRenderer()
{
	CALLED();
	_swsetup_DestroyContext(fContext);
	_swrast_DestroyContext(fContext);
	_tnl_DestroyContext(fContext);
	_vbo_DestroyContext(fContext);
	_mesa_destroy_visual(fVisual);
	_mesa_destroy_framebuffer(&fFrameBuffer->base);
	_mesa_destroy_context(fContext);

	free(fInfo);

	delete fBitmap;
}


void
MesaSoftwareRenderer::LockGL()
{
	CALLED();
	BGLRenderer::LockGL();

	_mesa_make_current(fContext, &fFrameBuffer->base, &fFrameBuffer->base);

	color_space colorSpace = BScreen(GLView()->Window()).ColorSpace();

	BAutolock lock(fInfoLocker);
	if (fDirectModeEnabled && fInfo != NULL) {
		fNewWidth = fInfo->window_bounds.right
			- fInfo->window_bounds.left + 1;
		fNewHeight = fInfo->window_bounds.bottom
			- fInfo->window_bounds.top + 1;
	}

	if (fColorSpace != colorSpace) {
		fColorSpace = colorSpace;
		_SetupRenderBuffer(fFrontRenderBuffer, fColorSpace);
		if (fVisual->doubleBufferMode)
			_SetupRenderBuffer(fBackRenderBuffer, fColorSpace);
	}

	if (fBitmap && fNewWidth == fWidth
		&& fNewHeight == fHeight)
		return;

	fWidth = fNewWidth;
	fHeight = fNewHeight;

	_AllocateBitmap();
}


void
MesaSoftwareRenderer::UnlockGL()
{
	CALLED();
	_mesa_make_current(fContext, NULL, NULL);
	if ((fOptions & BGL_DOUBLE) == 0) {
		SwapBuffers();
	}
	BGLRenderer::UnlockGL();
}


void
MesaSoftwareRenderer::SwapBuffers(bool VSync)
{
	CALLED();

	if (!fBitmap)
		return;

	if (fVisual->doubleBufferMode)
		_mesa_notifySwapBuffers(fContext);

	if (!fDirectModeEnabled || fInfo == NULL) {
		if (GLView()->LockLooperWithTimeout(1000) == B_OK) {
			GLView()->DrawBitmap(fBitmap, B_ORIGIN);
			GLView()->UnlockLooper();
		}
	} else {
		// TODO: Here the BGLView needs to be drawlocked.
		_CopyToDirect();
	}

	if (VSync) {
		BScreen screen(GLView()->Window());
		screen.WaitForRetrace();
	}
}


void
MesaSoftwareRenderer::Draw(BRect updateRect)
{
	CALLED();
	if (fBitmap && (!fDirectModeEnabled || (fInfo == NULL)))
		GLView()->DrawBitmap(fBitmap, updateRect, updateRect);
}


status_t
MesaSoftwareRenderer::CopyPixelsOut(BPoint location, BBitmap* bitmap)
{
	CALLED();
	color_space scs = fBitmap->ColorSpace();
	color_space dcs = bitmap->ColorSpace();

	if (scs != dcs && (scs != B_RGBA32 || dcs != B_RGB32)) {
		fprintf(stderr, "CopyPixelsOut(): incompatible color space: %s != %s\n",
			color_space_name(scs),
			color_space_name(dcs));
		return B_BAD_TYPE;
	}

	BRect sr = fBitmap->Bounds();
	BRect dr = bitmap->Bounds();

	sr = sr & dr.OffsetBySelf(location);
	dr = sr.OffsetByCopy(-location.x, -location.y);

	uint8* ps = (uint8*) fBitmap->Bits();
	uint8* pd = (uint8*) bitmap->Bits();
	uint32* s;
	uint32* d;
	uint32 y;
	for (y = (uint32) sr.top; y <= (uint32)sr.bottom; y++) {
		s = (uint32*)(ps + y * fBitmap->BytesPerRow());
		s += (uint32) sr.left;

		d = (uint32*)(pd + (y + (uint32)(dr.top - sr.top))
			* bitmap->BytesPerRow());
		d += (uint32) dr.left;

		memcpy(d, s, dr.IntegerWidth() * 4);
	}
	return B_OK;
}


status_t
MesaSoftwareRenderer::CopyPixelsIn(BBitmap* bitmap, BPoint location)
{
	CALLED();
	color_space scs = bitmap->ColorSpace();
	color_space dcs = fBitmap->ColorSpace();

	if (scs != dcs && (dcs != B_RGBA32 || scs != B_RGB32)) {
		fprintf(stderr, "CopyPixelsIn(): incompatible color space: %s != %s\n",
			color_space_name(scs),
			color_space_name(dcs));
		return B_BAD_TYPE;
	}

	BRect sr = bitmap->Bounds();
	BRect dr = fBitmap->Bounds();

	sr = sr & dr.OffsetBySelf(location);
	dr = sr.OffsetByCopy(-location.x, -location.y);

	uint8* ps = (uint8*) bitmap->Bits();
	uint8* pd = (uint8*) fBitmap->Bits();
	uint32* s;
	uint32* d;
	uint32 y;
	for (y = (uint32) sr.top; y <= (uint32)sr.bottom; y++) {
		s = (uint32*)(ps + y * bitmap->BytesPerRow());
		s += (uint32) sr.left;

		d = (uint32*)(pd + (y + (uint32)(dr.top - sr.top))
			* fBitmap->BytesPerRow());
		d += (uint32) dr.left;

		memcpy(d, s, dr.IntegerWidth() * 4);
	}
	return B_OK;
}


void
MesaSoftwareRenderer::EnableDirectMode(bool enabled)
{
	fDirectModeEnabled = enabled;
}


void
MesaSoftwareRenderer::DirectConnected(direct_buffer_info* info)
{
	// TODO: I'm not sure we need to do this: BGLView already
	// keeps a local copy of the direct_buffer_info passed by
	// BDirectWindow::DirectConnected().
	BAutolock lock(fInfoLocker);
	if (info) {
		if (!fInfo) {
			fInfo = (direct_buffer_info*)malloc(DIRECT_BUFFER_INFO_AREA_SIZE);
			if (!fInfo)
				return;
		}
		memcpy(fInfo, info, DIRECT_BUFFER_INFO_AREA_SIZE);
	} else if (fInfo) {
		free(fInfo);
		fInfo = NULL;
	}
}


void
MesaSoftwareRenderer::FrameResized(float width, float height)
{
	BAutolock lock(fInfoLocker);
	fNewWidth = (GLuint)width;
	fNewHeight = (GLuint)height;
}


void
MesaSoftwareRenderer::_AllocateBitmap()
{
	CALLED();
	// allocate new size of back buffer bitmap
	delete fBitmap;
	fBitmap = NULL;

	if (fWidth < 1 || fHeight < 1) {
		TRACE("%s: Cannot allocate bitmap < 1x1!\n", __func__);
		return;
	}
	BRect rect(0.0, 0.0, fWidth - 1, fHeight - 1);
	fBitmap = new BBitmap(rect, fColorSpace);
	for (uint i = 0; i < fHeight; i++) {
		fRowAddr[fHeight - i - 1] = (GLvoid *)((GLubyte *)fBitmap->Bits()
			+ i * fBitmap->BytesPerRow());
	}

	_mesa_resize_framebuffer(fContext, &fFrameBuffer->base, fWidth, fHeight);
	fFrontRenderBuffer->base.Data = fBitmap->Bits();
	fFrontRenderBuffer->size = fBitmap->BitsLength();
	if (fVisual->doubleBufferMode)
		fBackRenderBuffer->size = fBitmap->BitsLength();
	fFrameBuffer->width = fWidth;
	fFrameBuffer->height = fHeight;
}


// #pragma mark - static


void
MesaSoftwareRenderer::_Error(gl_context* ctx)
{
	MesaSoftwareRenderer* mr = (MesaSoftwareRenderer*)ctx->DriverCtx;
	if (mr && mr->GLView())
		mr->GLView()->ErrorCallback((unsigned long)ctx->ErrorValue);
}


const GLubyte*
MesaSoftwareRenderer::_GetString(gl_context* ctx, GLenum name)
{

	switch (name) {
		case GL_VENDOR:
			return (const GLubyte*) "Mesa Project";
		case GL_RENDERER: {
			_mesa_get_cpu_features();
			static char buffer[256] = { '\0' };

			if (!buffer[0]) {
				char* cpuInfo = _mesa_get_cpu_string();
				// Let's build an renderer string
				sprintf(buffer, "Software Rasterizer for %s", cpuInfo);
				free(cpuInfo);
			}
			return (const GLubyte*) buffer;
		}
		default:
			// Let core library handle all other cases
			return NULL;
	}
}


void
MesaSoftwareRenderer::_Viewport(gl_context* ctx, GLint x, GLint y, GLsizei w,
	GLsizei h)
{
	CALLED();

	gl_framebuffer* draw = ctx->WinSysDrawBuffer;
	gl_framebuffer* read = ctx->WinSysReadBuffer;
	struct msr_framebuffer* msr = msr_framebuffer(draw);

	_mesa_resize_framebuffer(ctx, draw, msr->width, msr->height);
	_mesa_resize_framebuffer(ctx, read, msr->width, msr->height);
}


void
MesaSoftwareRenderer::_UpdateState(gl_context* ctx, GLuint new_state)
{
	if (!ctx)
		return;

	CALLED();
	_swrast_InvalidateState(ctx, new_state);
	_swsetup_InvalidateState(ctx, new_state);
	_vbo_InvalidateState(ctx, new_state);
	_tnl_InvalidateState(ctx, new_state);
}


void
MesaSoftwareRenderer::_ClearFront(gl_context* ctx)
{
	CALLED();

	MesaSoftwareRenderer* mr = (MesaSoftwareRenderer*)ctx->DriverCtx;
	BGLView* bglview = mr->GLView();
	assert(bglview);
	BBitmap* bitmap = mr->fBitmap;
	assert(bitmap);
	GLuint* start = (GLuint*)bitmap->Bits();
	size_t pixelSize = 0;
	get_pixel_size_for(bitmap->ColorSpace(), &pixelSize, NULL, NULL);
	const GLuint* clearPixelPtr = (const GLuint*)mr->fClearColor;
	const GLuint clearPixel = B_LENDIAN_TO_HOST_INT32(*clearPixelPtr);

	int x = ctx->DrawBuffer->_Xmin;
	int y = ctx->DrawBuffer->_Ymin;
	uint32 width = ctx->DrawBuffer->_Xmax - x;
	uint32 height = ctx->DrawBuffer->_Ymax - y;
	GLboolean all = (width == ctx->DrawBuffer->Width
		&& height == ctx->DrawBuffer->Height);

	if (all) {
		const int numPixels = mr->fWidth * mr->fHeight;
		if (clearPixel == 0) {
			memset(start, 0, numPixels * pixelSize);
		} else {
			for (int i = 0; i < numPixels; i++) {
				start[i] = clearPixel;
			}
		}
	} else {
		// XXX untested
		start += y * mr->fWidth + x;
		for (uint32 i = 0; i < height; i++) {
			for (uint32 j = 0; j < width; j++) {
				start[j] = clearPixel;
			}
			start += mr->fWidth;
		}
	}
}


GLboolean
MesaSoftwareRenderer::_FrontRenderbufferStorage(gl_context* ctx,
	struct gl_renderbuffer* render, GLenum internalFormat,
	GLuint width, GLuint height)
{
	CALLED();

	render->Data = NULL;
	render->Width = width;
	render->Height = height;
#if HAIKU_MESA_VER >= 712
	render->RowStride = width;
#endif

	return GL_TRUE;
}


GLboolean
MesaSoftwareRenderer::_BackRenderbufferStorage(gl_context* ctx,
	struct gl_renderbuffer* render, GLenum internalFormat,
	GLuint width, GLuint height)
{
	struct msr_renderbuffer* mrb = msr_renderbuffer(render);
	free(render->Data);
	_FrontRenderbufferStorage(ctx, render, internalFormat, width, height);
	render->Data = malloc(mrb->size);
	return GL_TRUE;
}


void
MesaSoftwareRenderer::_Flush(gl_context* ctx)
{
	CALLED();
	MesaSoftwareRenderer* mr = (MesaSoftwareRenderer*)ctx->DriverCtx;
	if ((mr->fOptions & BGL_DOUBLE) == 0) {
		// TODO: SwapBuffers() can call _CopyToDirect(), which should
		// be always called with with the BGLView drawlocked.
		// This is not always the case if called from here.
		mr->SwapBuffers();
	}
}


struct msr_renderbuffer*
MesaSoftwareRenderer::_NewRenderBuffer(bool front)
{
	CALLED();
	struct msr_renderbuffer *msr
		= (struct msr_renderbuffer*)calloc(1, sizeof *msr);

	if (!msr) {
		ERROR("%s: Failed calloc RenderBuffer\n", __func__);
		return NULL;
	}

	_mesa_init_renderbuffer(&msr->base, 0);

	if (_SetupRenderBuffer(msr, fColorSpace) != B_OK) {
		free(msr);
		return NULL;
	}

	if (front)
		msr->base.AllocStorage = _FrontRenderbufferStorage;
	else {
		msr->base.AllocStorage = _BackRenderbufferStorage;
		msr->base.Delete = _DeleteBackBuffer;
	}

	return msr;
}


status_t
MesaSoftwareRenderer::_SetupRenderBuffer(
	struct msr_renderbuffer* buffer, color_space colorSpace)
{
	CALLED();

	buffer->base.DataType = GL_UNSIGNED_BYTE;
	buffer->base.Data = NULL;

	switch (colorSpace) {
		case B_RGBA32:
			buffer->base._BaseFormat = GL_RGBA;
			buffer->base.Format = MESA_FORMAT_ARGB8888;

			buffer->base.GetRow = get_row_RGBA32;
			buffer->base.GetValues = get_values_RGBA32;
			buffer->base.PutRow = put_row_RGBA32;
			buffer->base.PutValues = put_values_RGBA32;
#if HAIKU_MESA_VER <= 711
			buffer->base.PutRowRGB = put_row_rgb_RGBA32;
			buffer->base.PutMonoRow = put_mono_row_RGBA32;
			buffer->base.PutMonoValues = put_values_RGBA32;
#endif
			break;
		case B_RGB32:
			buffer->base._BaseFormat = GL_RGB;
#if HAIKU_MESA_VER >= 709
			// XRGB8888 only in newer Mesa
			buffer->base.Format = MESA_FORMAT_XRGB8888;
#else
			buffer->base.Format = MESA_FORMAT_ARGB8888;
#endif

			buffer->base.GetRow = get_row_RGB32;
			buffer->base.GetValues = get_values_RGB32;
			buffer->base.PutRow = put_row_RGB32;
			buffer->base.PutValues = put_values_RGB32;
#if HAIKU_MESA_VER <= 711
			buffer->base.PutRowRGB = put_row_rgb_RGB32;
			buffer->base.PutMonoRow = put_mono_row_RGB32;
			buffer->base.PutMonoValues = put_values_RGB32;
#endif
			break;
		case B_RGB24:
			buffer->base._BaseFormat = GL_RGB;
			buffer->base.Format = MESA_FORMAT_RGB888;

			buffer->base.GetRow = get_row_RGB24;
			buffer->base.GetValues = get_values_RGB24;
			buffer->base.PutRow = put_row_RGB24;
			buffer->base.PutValues = put_values_RGB24;
#if HAIKU_MESA_VER <= 711
			buffer->base.PutRowRGB = put_row_rgb_RGB24;
			buffer->base.PutMonoRow = put_mono_row_RGB24;
			buffer->base.PutMonoValues = put_values_RGB24;
#endif
			break;
		case B_RGB16:
			buffer->base._BaseFormat = GL_RGB;
			buffer->base.Format = MESA_FORMAT_RGB565;

			buffer->base.GetRow = get_row_RGB16;
			buffer->base.GetValues = get_values_RGB16;
			buffer->base.PutRow = put_row_RGB16;
			buffer->base.PutValues = put_values_RGB16;
#if HAIKU_MESA_VER <= 711
			buffer->base.PutRowRGB = put_row_rgb_RGB16;
			buffer->base.PutMonoRow = put_mono_row_RGB16;
			buffer->base.PutMonoValues = put_values_RGB16;
#endif
			break;
		case B_RGB15:
			buffer->base._BaseFormat = GL_RGB;
			buffer->base.Format = MESA_FORMAT_ARGB1555;

			buffer->base.GetRow = get_row_RGB15;
			buffer->base.GetValues = get_values_RGB15;
			buffer->base.PutRow = put_row_RGB15;
			buffer->base.PutValues = put_values_RGB15;
#if HAIKU_MESA_VER <= 711
			buffer->base.PutRowRGB = put_row_rgb_RGB15;
			buffer->base.PutMonoRow = put_mono_row_RGB15;
			buffer->base.PutMonoValues = put_values_RGB15;
#endif
			break;
		default:
			fprintf(stderr, "Unsupported screen color space %s\n",
				color_space_name(fColorSpace));
			debugger("Unsupported OpenGL color space");
			return B_ERROR;
	}
	return B_OK;
}


void
MesaSoftwareRenderer::_DeleteBackBuffer(struct gl_renderbuffer* rb)
{
	CALLED();
	free(rb->Data);
	free(rb);
}


void
MesaSoftwareRenderer::_CopyToDirect()
{
	BAutolock lock(fInfoLocker);

	// check the bitmap size still matches the size
	if (fInfo->window_bounds.bottom - fInfo->window_bounds.top
		!= fBitmap->Bounds().IntegerHeight()
		|| fInfo->window_bounds.right - fInfo->window_bounds.left
			!= fBitmap->Bounds().IntegerWidth())
		return;

	uint8 bytesPerPixel = fInfo->bits_per_pixel / 8;
	uint32 bytesPerRow = fBitmap->BytesPerRow();
	for (uint32 i = 0; i < fInfo->clip_list_count; i++) {
		clipping_rect *clip = &fInfo->clip_list[i];
		int32 height = clip->bottom - clip->top + 1;
		int32 bytesWidth
			= (clip->right - clip->left + 1) * bytesPerPixel;
		uint8* p = (uint8*)fInfo->bits + clip->top
			* fInfo->bytes_per_row + clip->left * bytesPerPixel;
		uint8* b = (uint8*)fBitmap->Bits()
			+ (clip->top - fInfo->window_bounds.top) * bytesPerRow
			+ (clip->left - fInfo->window_bounds.left)
				* bytesPerPixel;

		for (int y = 0; y < height; y++) {
			memcpy(p, b, bytesWidth);
			p += fInfo->bytes_per_row;
			b += bytesPerRow;
		}
	}
}
