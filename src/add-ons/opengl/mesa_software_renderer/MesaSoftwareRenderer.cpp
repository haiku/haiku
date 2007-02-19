/*
 * Copyright 2006-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, korli@users.berlios.de
 * 		Philippe Houdoin, philippe.houdoin@free.fr
 */
/*
 * Mesa 3-D graphics library
 * Version:  6.1
 * 
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
 */
 
#define CALLED() 			//printf("CALLED %s\n",__PRETTY_FUNCTION__)

#include "MesaSoftwareRenderer.h"
extern "C" {
	#include "array_cache/acache.h"
	#include "extensions.h"
	#include "drivers/common/driverfuncs.h"
	#include "main/colormac.h"
	#include "main/buffers.h"
	#include "main/framebuffer.h"
	#include "main/renderbuffer.h"
	#include "main/state.h"
	#include "main/version.h"
	#include "swrast/swrast.h"
	#include "swrast_setup/swrast_setup.h"
	#include "tnl/tnl.h"
	#include "tnl/t_context.h"
	#include "tnl/t_pipeline.h"
	
	
	#if defined(USE_X86_ASM)
	#include "x86/common_x86_asm.h"
	#endif

	#if defined(USE_PPC_ASM)
	#include "ppc/common_ppc_features.h"
	#endif
}

extern const char * color_space_name(color_space space);

// BeOS component ordering for B_RGBA32 bitmap format
#if B_HOST_IS_LENDIAN
	#define BE_RCOMP 2
	#define BE_GCOMP 1
	#define BE_BCOMP 0
	#define BE_ACOMP 3

	#define PACK_B_RGBA32(color) (color[BCOMP] | (color[GCOMP] << 8) | \
							(color[RCOMP] << 16) | (color[ACOMP] << 24))

	#define PACK_B_RGB32(color) (color[BCOMP] | (color[GCOMP] << 8) | \
  							(color[RCOMP] << 16) | 0xFF000000)
#else
	// Big Endian B_RGBA32 bitmap format
	#define BE_RCOMP 1
	#define BE_GCOMP 2
	#define BE_BCOMP 3
	#define BE_ACOMP 0

	#define PACK_B_RGBA32(color) (color[ACOMP] | (color[RCOMP] << 8) | \
							(color[GCOMP] << 16) | (color[BCOMP] << 24))

	#define PACK_B_RGB32(color) ((color[RCOMP] << 8) | (color[GCOMP] << 16) | \
  							(color[BCOMP] << 24) | 0xFF000000)
#endif

/**********************************************************************/
/*****        Read/write spans/arrays of pixels                   *****/
/**********************************************************************/

/* 8-bit RGBA */
#define NAME(PREFIX) PREFIX##_RGBA8
#define RB_TYPE GLubyte
#define SPAN_VARS \
	MesaSoftwareRenderer *mr = (MesaSoftwareRenderer *) ctx->DriverCtx;
#define INIT_PIXEL_PTR(P, X, Y) \
	GLubyte *P = ((GLubyte **) mr->GetRows())[Y] + (X) * 4
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

extern "C" _EXPORT BGLRenderer * 
instanciate_gl_renderer(BGLView *view, ulong options, BGLDispatcher *dispatcher)
{
	return new MesaSoftwareRenderer(view, options, dispatcher);
}


MesaSoftwareRenderer::MesaSoftwareRenderer(BGLView *view, ulong options, BGLDispatcher *dispatcher)
	: BGLRenderer(view, options, dispatcher),
	fBitmap(NULL),
	fContext(NULL),
	fVisual(NULL),
	fFrameBuffer(NULL)
{
	CALLED();

	fClearColor[BE_RCOMP] = 0;
	fClearColor[BE_GCOMP] = 0;
	fClearColor[BE_BCOMP] = 0;
	fClearColor[BE_ACOMP] = 0;

	fClearIndex = 0;

	// We force single buffering for the time being
	//options &= !BGL_DOUBLE;

	const GLboolean rgbFlag = ((options & BGL_INDEX) == 0);
	const GLboolean alphaFlag = ((options & BGL_ALPHA) == BGL_ALPHA);
	const GLboolean dblFlag = false; //((options & BGL_DOUBLE) == BGL_DOUBLE);
	const GLboolean stereoFlag = false;
	const GLint depth = (options & BGL_DEPTH) ? 16 : 0;
	const GLint stencil = (options & BGL_STENCIL) ? 8 : 0;
	const GLint accum = (options & BGL_ACCUM) ? 16 : 0;
	const GLint index = (options & BGL_INDEX) ? 32 : 0;
	const GLint red = rgbFlag ? 8 : 0;
	const GLint green = rgbFlag ? 8 : 0;
	const GLint blue = rgbFlag ? 8 : 0;
	const GLint alpha = alphaFlag ? 8 : 0;

	//fOptions = options | BGL_INDIRECT;
	struct dd_function_table functions;

	fVisual = _mesa_create_visual( rgbFlag, dblFlag, stereoFlag, red, green, blue, alpha,
				index, depth, stencil, accum, accum, accum, accum, 1);

	// Initialize device driver function table
	_mesa_init_driver_functions(&functions);

	functions.GetString 	= GetString;
	functions.UpdateState 	= UpdateState;
	functions.Clear 	= Clear;
	functions.ClearIndex 	= ClearIndex;
	functions.ClearColor 	= ClearColor;
	functions.Error		= Error;
        functions.Viewport      = Viewport;

	// create core context
	fContext = _mesa_create_context(fVisual, NULL, &functions, this);
	if (!fContext) {
		_mesa_destroy_visual(fVisual);
		return;
	}

	// create core framebuffer
	fFrameBuffer = _mesa_create_framebuffer(fVisual);

	/* Initialize the software rasterizer and helper modules. */
	_swrast_CreateContext(fContext);
	_ac_CreateContext(fContext);
	_tnl_CreateContext(fContext);
	_swsetup_CreateContext(fContext);
	_swsetup_Wakeup(fContext);

	fRenderBuffer = _mesa_new_renderbuffer(fContext, BUFFER_FRONT_LEFT);

	fRenderBuffer->InternalFormat = GL_RGBA;
	fRenderBuffer->_BaseFormat = GL_RGBA;
	fRenderBuffer->DataType = GL_UNSIGNED_BYTE;
	fRenderBuffer->Data = NULL;
	fRenderBuffer->AllocStorage = RenderbufferStorage;

	fRenderBuffer->GetRow = get_row_RGBA8;
	fRenderBuffer->GetValues = get_values_RGBA8;
	fRenderBuffer->PutRow = put_row_RGBA8;
	fRenderBuffer->PutRowRGB = put_row_rgb_RGBA8;
	fRenderBuffer->PutMonoRow = put_mono_row_RGBA8;
	fRenderBuffer->PutValues = put_values_RGBA8;
	fRenderBuffer->PutMonoValues = put_mono_values_RGBA8;

	_mesa_add_renderbuffer(fFrameBuffer, BUFFER_FRONT_LEFT, fRenderBuffer);

	_mesa_add_soft_renderbuffers(fFrameBuffer,
                                   GL_FALSE,
                                   fVisual->haveDepthBuffer,
                                   fVisual->haveStencilBuffer,
                                   fVisual->haveAccumBuffer,
                                   GL_FALSE,
                                   GL_FALSE );

	// Use default TCL pipeline
	TNL_CONTEXT( fContext )->Driver.RunPipeline = _tnl_run_pipeline;

	_mesa_enable_sw_extensions(fContext);
	_mesa_enable_1_3_extensions(fContext);
	_mesa_enable_1_4_extensions(fContext);
	_mesa_enable_1_5_extensions(fContext);
 
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
	_mesa_destroy_visual(fVisual);
	_mesa_destroy_framebuffer(fFrameBuffer);
	_mesa_destroy_context(fContext);
 
	delete fBitmap;
}


void
MesaSoftwareRenderer::LockGL()
{
	CALLED();
	BGLRenderer::LockGL();

	_mesa_make_current(fContext, fFrameBuffer, fFrameBuffer);

	BRect b = GLView()->Bounds();
	uint32 width = b.IntegerWidth() + 1;
	uint32 height = b.IntegerHeight() + 1;
	if (width != fWidth || height != fHeight || !fBitmap) {
		// allocate new size of back buffer bitmap
		if (fBitmap)
			delete fBitmap;
		BRect rect(0.0, 0.0, width - 1, height - 1);
		fBitmap = new BBitmap(rect, B_RGBA32);
		for (uint i = 0; i < height; i++)
			fRowAddr[height - i - 1] = (GLvoid *) ((GLubyte *) fBitmap->Bits() + i * fBitmap->BytesPerRow());
		
		_mesa_resize_framebuffer(fContext, fFrameBuffer, width, height);
		fWidth = width;
		fHeight = height;
		fRenderBuffer->Data = fBitmap->Bits();
	}	
}


void
MesaSoftwareRenderer::UnlockGL()
{
	CALLED();
	_mesa_make_current(fContext, NULL, NULL);
	BGLRenderer::UnlockGL();
}


void
MesaSoftwareRenderer::SwapBuffers(bool VSync)
{
	CALLED();
	_mesa_notifySwapBuffers(fContext);

	if (fBitmap) {
		GLView()->LockLooper();
		GLView()->DrawBitmap(fBitmap);
		GLView()->UnlockLooper();
	}
}


void
MesaSoftwareRenderer::Draw(BRect updateRect)
{
	CALLED();
	if (fBitmap)
		GLView()->DrawBitmap(fBitmap, updateRect, updateRect);
}


status_t
MesaSoftwareRenderer::CopyPixelsOut(BPoint location, BBitmap *bitmap)
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
	
	uint8 *ps = (uint8 *) fBitmap->Bits();
	uint8 *pd = (uint8 *) bitmap->Bits();
	uint32 *s, *d;
	uint32 y;
	for (y = (uint32) sr.top; y <= (uint32) sr.bottom; y++) {
		s = (uint32 *) (ps + y * fBitmap->BytesPerRow());
		s += (uint32) sr.left;
		
		d = (uint32 *) (pd + (y + (uint32) (dr.top - sr.top)) * bitmap->BytesPerRow());
		d += (uint32) dr.left;
		
		memcpy(d, s, dr.IntegerWidth() * 4);
	}
	return B_OK;
}


status_t
MesaSoftwareRenderer::CopyPixelsIn(BBitmap *bitmap, BPoint location)
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
	
	uint8 *ps = (uint8 *) bitmap->Bits();
	uint8 *pd = (uint8 *) fBitmap->Bits();
	uint32 *s, *d;
	uint32 y;
	for (y = (uint32) sr.top; y <= (uint32) sr.bottom; y++) {
		s = (uint32 *) (ps + y * bitmap->BytesPerRow());
		s += (uint32) sr.left;
		
		d = (uint32 *) (pd + (y + (uint32) (dr.top - sr.top)) * fBitmap->BytesPerRow());
		d += (uint32) dr.left;
		
		memcpy(d, s, dr.IntegerWidth() * 4);
	}
	return B_OK;
}


void 
MesaSoftwareRenderer::Viewport(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
	CALLED();
	/* poll for window size change and realloc software Z/stencil/etc if needed */
	_mesa_ResizeBuffersMESA();
}


const GLubyte *
MesaSoftwareRenderer::GetString(GLcontext *ctx, GLenum name)
{
	switch (name) {
		case GL_RENDERER: {
			static char buffer[256] = { '\0' };

			if (!buffer[0] ) {
				// Let's build an renderer string
				// TODO: add SVN revision
				strncat(buffer, "Haiku's Mesa " MESA_VERSION_STRING " Software Renderer", sizeof(buffer));

			   // Append any CPU-specific information.
#ifdef USE_X86_ASM
				if (_mesa_x86_cpu_features)
					strncat(buffer, ", optimized for x86", sizeof(buffer));
	#ifdef USE_MMX_ASM
   				if (cpu_has_mmx)
   					strncat(buffer, (cpu_has_mmxext) ? "/MMX+" : "/MMX", sizeof(buffer));
	#endif
	#ifdef USE_3DNOW_ASM
				if (cpu_has_3dnow)
   					strncat(buffer, (cpu_has_3dnowext) ? "/3DNow!+" : "/3DNow!", sizeof(buffer));
	#endif
	#ifdef USE_SSE_ASM
				if (cpu_has_xmm)
   					strncat(buffer, (cpu_has_xmm2) ? "/SSE2" : "/SSE", sizeof(buffer));
	#endif

#elif defined(USE_SPARC_ASM)

				strncat(buffer, ", optimized for SPARC", sizeof(buffer));

#elif defined(USE_PPC_ASM)

				if (_mesa_ppc_cpu_features)
					strncat(buffer, (cpu_has_64) ? ", optimized for PowerPC 64" : ", optimized for PowerPC", sizeof(buffer));

	#ifdef USE_VMX_ASM
				if (cpu_has_vmx)
					strncat(buffer, "/Altivec", sizeof(buffer));
	#endif

				if (! cpu_has_fpu)
					strncat(buffer, "/No FPU", sizeof(buffer));
#endif				
			
			}
			return (const GLubyte *) buffer;
		}
		default:
			// Let core library handle all other cases
			return NULL;
	}
}


void 
MesaSoftwareRenderer::Error(GLcontext *ctx)
{
	MesaSoftwareRenderer *mr = (MesaSoftwareRenderer *) ctx->DriverCtx;
	if (mr && mr->GLView())
		mr->GLView()->ErrorCallback((unsigned long) ctx->ErrorValue);
}


void 
MesaSoftwareRenderer::ClearIndex(GLcontext *ctx, GLuint index)
{
	CALLED();
	
	MesaSoftwareRenderer *mr = (MesaSoftwareRenderer *) ctx->DriverCtx;
	mr->fClearIndex = index;
}


void 
MesaSoftwareRenderer::ClearColor(GLcontext *ctx, const GLfloat color[4])
{
	CALLED();
	
	MesaSoftwareRenderer *mr = (MesaSoftwareRenderer *) ctx->DriverCtx;
	CLAMPED_FLOAT_TO_CHAN(mr->fClearColor[BE_RCOMP], color[0]);
	CLAMPED_FLOAT_TO_CHAN(mr->fClearColor[BE_GCOMP], color[1]);
	CLAMPED_FLOAT_TO_CHAN(mr->fClearColor[BE_BCOMP], color[2]);
	CLAMPED_FLOAT_TO_CHAN(mr->fClearColor[BE_ACOMP], color[3]); 
	assert(mr->GLView());
}


void 
MesaSoftwareRenderer::Clear(GLcontext *ctx, GLbitfield mask)
{
	CALLED();
	if (mask & BUFFER_BIT_FRONT_LEFT)
		ClearFront(ctx);

	mask &= ~(BUFFER_BIT_FRONT_LEFT | BUFFER_BIT_BACK_LEFT);
	if (mask)
		_swrast_Clear( ctx, mask);
}


void 
MesaSoftwareRenderer::ClearFront(GLcontext *ctx)
{
	CALLED();
	
	MesaSoftwareRenderer *mr = (MesaSoftwareRenderer *) ctx->DriverCtx;
	BGLView *bglview = mr->GLView();
	assert(bglview);
	BBitmap *bitmap = mr->fBitmap;
	assert(bitmap);
	GLuint *start = (GLuint *) bitmap->Bits();
	const GLuint *clearPixelPtr = (const GLuint *) mr->fClearColor;
	const GLuint clearPixel = B_LENDIAN_TO_HOST_INT32(*clearPixelPtr);

	int x = ctx->DrawBuffer->_Xmin;
	int y = ctx->DrawBuffer->_Ymin;
	uint32 width = ctx->DrawBuffer->_Xmax - x;
	uint32 height = ctx->DrawBuffer->_Ymax - y;
	GLboolean all = (width == ctx->DrawBuffer->Width && height == ctx->DrawBuffer->Height);

	if (all) {
		const int numPixels = mr->fWidth * mr->fHeight;
		if (clearPixel == 0) {
			memset(start, 0, numPixels * 4);
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


void 
MesaSoftwareRenderer::UpdateState( GLcontext *ctx, GLuint new_state )
{
	if (!ctx)
		return;

	CALLED();
	_swrast_InvalidateState( ctx, new_state );
	_swsetup_InvalidateState( ctx, new_state );
	_ac_InvalidateState( ctx, new_state );
	_tnl_InvalidateState( ctx, new_state );
}


GLboolean
MesaSoftwareRenderer::RenderbufferStorage(GLcontext *ctx, struct gl_renderbuffer *render,
                        GLenum internalFormat, GLuint width, GLuint height )
{
     return GL_TRUE;
}
