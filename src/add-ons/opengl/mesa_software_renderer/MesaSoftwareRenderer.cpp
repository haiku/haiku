/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
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

	fClearColor[BE_RCOMP] = 0;
	fClearColor[BE_GCOMP] = 0;
	fClearColor[BE_BCOMP] = 0;
	fClearColor[BE_ACOMP] = 0;

	fClearIndex = 0;

	// We don't support single buffering (yet): double buffering forced.
	options |= BGL_DOUBLE;

	const GLboolean rgbFlag = ((options & BGL_INDEX) == 0);
	const GLboolean alphaFlag = ((options & BGL_ALPHA) == BGL_ALPHA);
	const GLboolean dblFlag = ((options & BGL_DOUBLE) == BGL_DOUBLE);
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
	functions.GetBufferSize = GetBufferSize;
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

	_mesa_enable_sw_extensions(fContext);
	_mesa_enable_1_3_extensions(fContext);
	_mesa_enable_1_4_extensions(fContext);
	_mesa_enable_1_5_extensions(fContext);

	// create core framebuffer
	fFrameBuffer = _mesa_create_framebuffer(fVisual, depth > 0 ? GL_TRUE : GL_FALSE,
		stencil > 0 ? GL_TRUE: GL_FALSE, accum > 0 ? GL_TRUE : GL_FALSE, alphaFlag);

	/* Initialize the software rasterizer and helper modules. */
	_swrast_CreateContext(fContext);
	_ac_CreateContext(fContext);
	_tnl_CreateContext(fContext);
	_swsetup_CreateContext(fContext);
	_swsetup_Wakeup(fContext);

	MesaSoftwareRenderer * mr = (MesaSoftwareRenderer *) fContext->DriverCtx;
	struct swrast_device_driver * swdd = _swrast_GetDeviceDriverReference( fContext );
	TNLcontext * tnl = TNL_CONTEXT(fContext);

	assert(mr->fContext == fContext );
	assert(tnl);
	assert(swdd);

	// Use default TCL pipeline
	tnl->Driver.RunPipeline = _tnl_run_pipeline;
 
	swdd->SetBuffer = this->SetBuffer;
	
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
	_mesa_destroy_visual(fVisual);
	_mesa_destroy_framebuffer(fFrameBuffer);
	_mesa_destroy_context(fContext);
 
	delete fBitmap;
}


void
MesaSoftwareRenderer::LockGL()
{	
	BGLRenderer::LockGL();
	
	UpdateState(fContext, 0);
	_mesa_make_current(fContext, fFrameBuffer);
}


void
MesaSoftwareRenderer::UnlockGL()
{

	BGLRenderer::UnlockGL();
}


void
MesaSoftwareRenderer::SwapBuffers(bool VSync)
{
	_mesa_notifySwapBuffers(fContext);

	if (fBitmap) {
		CALLED();
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
				strncat(buffer, "Haiku's Mesa " MESA_VERSION_STRING " Software Renderer", sizeof(buffer));

			   // Append any CPU-specific information.
#ifdef USE_X86_ASM
				if (_mesa_x86_cpu_features)
					strncat(buffer, " x86", sizeof(buffer));
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

				strncat(buffer, " SPARC", sizeof(buffer));

#elif defined(USE_PPC_ASM)

				if (_mesa_ppc_cpu_features)
					strncat(buffer, (cpu_has_64) ? " PowerPC 64" : " PowerPC", sizeof(buffer));

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
MesaSoftwareRenderer::Clear(GLcontext *ctx, GLbitfield mask,
                               GLboolean all, GLint x, GLint y,
                               GLint width, GLint height)
{
	CALLED();
	if (mask & DD_FRONT_LEFT_BIT)
		ClearFront(ctx, all, x, y, width, height);
	if (mask & DD_BACK_LEFT_BIT)
		ClearBack(ctx, all, x, y, width, height);

	mask &= ~(DD_FRONT_LEFT_BIT | DD_BACK_LEFT_BIT);
	if (mask)
		_swrast_Clear( ctx, mask, all, x, y, width, height );
}


void 
MesaSoftwareRenderer::ClearFront(GLcontext *ctx,
                         GLboolean all, GLint x, GLint y,
                         GLint width, GLint height)
{
	CALLED();
	
	MesaSoftwareRenderer *mr = (MesaSoftwareRenderer *) ctx->DriverCtx;
	BGLView *bglview = mr->GLView();
	assert(bglview);

	bglview->SetHighColor(mr->fClearColor[BE_RCOMP],
			mr->fClearColor[BE_GCOMP],
			mr->fClearColor[BE_BCOMP],
			mr->fClearColor[BE_ACOMP]);
	bglview->SetLowColor(mr->fClearColor[BE_RCOMP],
			mr->fClearColor[BE_GCOMP],
			mr->fClearColor[BE_BCOMP],
			mr->fClearColor[BE_ACOMP]);
	if (all) {
		BRect b = bglview->Bounds();
		bglview->FillRect(b);
	} else {
		// XXX untested
		BRect b;
		b.left = x;
		b.right = x + width;
		b.bottom = mr->fHeight - y - 1;
		b.top = b.bottom - height;
		bglview->FillRect(b);
	}

	// restore drawing color
#if 0
	bglview->SetHighColor(md->mColor[BE_RCOMP],
		md->mColor[BE_GCOMP],
		md->mColor[BE_BCOMP],
		md->mColor[BE_ACOMP]);
	bglview->SetLowColor(md->mColor[BE_RCOMP],
		md->mColor[BE_GCOMP],
		md->mColor[BE_BCOMP],
		md->mColor[BE_ACOMP]);
#endif
}


void 
MesaSoftwareRenderer::ClearBack(GLcontext *ctx,
                        GLboolean all, GLint x, GLint y,
                        GLint width, GLint height)
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
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {
				start[j] = clearPixel;
			}
			start += mr->fWidth;
		}
	}
}


void 
MesaSoftwareRenderer::UpdateState( GLcontext *ctx, GLuint new_state )
{
	struct swrast_device_driver *	swdd = _swrast_GetDeviceDriverReference( ctx );

	_swrast_InvalidateState( ctx, new_state );
	_swsetup_InvalidateState( ctx, new_state );
	_ac_InvalidateState( ctx, new_state );
	_tnl_InvalidateState( ctx, new_state );

	if (ctx->Color.DrawBuffer[0] == GL_FRONT) {
		/* read/write front buffer */
		swdd->WriteRGBASpan = WriteRGBASpanFront;
		swdd->WriteRGBSpan = WriteRGBSpanFront;
		swdd->WriteRGBAPixels = WriteRGBAPixelsFront;
		swdd->WriteMonoRGBASpan = WriteMonoRGBASpanFront;
		swdd->WriteMonoRGBAPixels = WriteMonoRGBAPixelsFront;
		swdd->WriteCI32Span = WriteCI32SpanFront;
		swdd->WriteCI8Span = WriteCI8SpanFront;
		swdd->WriteMonoCISpan = WriteMonoCISpanFront;
		swdd->WriteCI32Pixels = WriteCI32PixelsFront;
		swdd->WriteMonoCIPixels = WriteMonoCIPixelsFront;
		swdd->ReadRGBASpan = ReadRGBASpanFront;
		swdd->ReadRGBAPixels = ReadRGBAPixelsFront;
		swdd->ReadCI32Span = ReadCI32SpanFront;
		swdd->ReadCI32Pixels = ReadCI32PixelsFront;
	} else {
		/* read/write back buffer */
		swdd->WriteRGBASpan = WriteRGBASpanBack;
		swdd->WriteRGBSpan = WriteRGBSpanBack;
		swdd->WriteRGBAPixels = WriteRGBAPixelsBack;
		swdd->WriteMonoRGBASpan = WriteMonoRGBASpanBack;
		swdd->WriteMonoRGBAPixels = WriteMonoRGBAPixelsBack;
		swdd->WriteCI32Span = WriteCI32SpanBack;
		swdd->WriteCI8Span = WriteCI8SpanBack;
		swdd->WriteMonoCISpan = WriteMonoCISpanBack;
		swdd->WriteCI32Pixels = WriteCI32PixelsBack;
		swdd->WriteMonoCIPixels = WriteMonoCIPixelsBack;
		swdd->ReadRGBASpan = ReadRGBASpanBack;
		swdd->ReadRGBAPixels = ReadRGBAPixelsBack;
		swdd->ReadCI32Span = ReadCI32SpanBack;
		swdd->ReadCI32Pixels = ReadCI32PixelsBack;
	}
}


void 
MesaSoftwareRenderer::GetBufferSize(GLframebuffer * framebuffer, GLuint *width,
                            GLuint *height)
{
	GET_CURRENT_CONTEXT(ctx);
	if (!ctx)
		return;

	MesaSoftwareRenderer * mr = (MesaSoftwareRenderer *) ctx->DriverCtx;
	BGLView *bglview = mr->GLView();
	assert(bglview);

	BRect b = bglview->Bounds();
	*width = (GLuint) b.IntegerWidth() + 1; // (b.right - b.left + 1);
	*height = (GLuint) b.IntegerHeight() + 1; // (b.bottom - b.top + 1);
	mr->fBottom = (GLint) b.bottom;

	if (ctx->Visual.doubleBufferMode) {
		if (*width != mr->fWidth || *height != mr->fHeight) {
			// allocate new size of back buffer bitmap
			if (mr->fBitmap)
				delete mr->fBitmap;
			BRect rect(0.0, 0.0, *width - 1, *height - 1);
			mr->fBitmap = new BBitmap(rect, B_RGBA32);
		}
	} else {
		mr->fBitmap = NULL;
	}

	mr->fWidth = *width;
	mr->fHeight = *height;
}


void 
MesaSoftwareRenderer::SetBuffer(GLcontext *ctx, GLframebuffer *buffer,
                            GLenum mode)
{
   /* TODO */
	(void) ctx;
	(void) buffer;
	(void) mode;
}


// Plot a pixel.  (0,0) is upper-left corner
// This is only used when drawing to the front buffer.
inline void Plot(BGLView *bglview, int x, int y)
{
	// XXX There's got to be a better way!
	BPoint p(x, y), q(x+1, y);
	bglview->StrokeLine(p, q);
}


void
MesaSoftwareRenderer::WriteRGBASpanFront(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 CONST GLubyte rgba[][4],
                                 const GLubyte mask[])
{
	CALLED();
	
	MesaSoftwareRenderer *mr = (MesaSoftwareRenderer *) ctx->DriverCtx;
	BGLView *bglview = mr->GLView();
	assert(bglview);
	int flippedY = mr->fBottom - y;
	if (mask) {
		for (GLuint i = 0; i < n; i++) {
			if (mask[i]) {
				bglview->SetHighColor(rgba[i][0], rgba[i][1], rgba[i][2], rgba[i][3]);
				Plot(bglview, x++, flippedY);
			}
		}
	} else {
		for (GLuint i = 0; i < n; i++) {
			bglview->SetHighColor(rgba[i][0], rgba[i][1], rgba[i][2], rgba[i][3]);
			Plot(bglview, x++, flippedY);
		}
	}
}


void
MesaSoftwareRenderer::WriteRGBSpanFront(const GLcontext *ctx, GLuint n,
                                GLint x, GLint y,
                                CONST GLubyte rgba[][3],
                                const GLubyte mask[])
{
	CALLED();
	
	MesaSoftwareRenderer *mr = (MesaSoftwareRenderer *) ctx->DriverCtx;
	BGLView *bglview = mr->GLView();
	assert(bglview);
	int flippedY = mr->fBottom - y;
	if (mask) {
		for (GLuint i = 0; i < n; i++) {
			if (mask[i]) {
				bglview->SetHighColor(rgba[i][0], rgba[i][1], rgba[i][2]);
				Plot(bglview, x++, flippedY);
			}
		}
	} else {
		for (GLuint i = 0; i < n; i++) {
			bglview->SetHighColor(rgba[i][0], rgba[i][1], rgba[i][2]);
			Plot(bglview, x++, flippedY);
		}
	}
}


void
MesaSoftwareRenderer::WriteMonoRGBASpanFront(const GLcontext *ctx, GLuint n,
                                     GLint x, GLint y,
                                     const GLchan color[4],
                                     const GLubyte mask[])
{
	CALLED();
	
	MesaSoftwareRenderer *mr = (MesaSoftwareRenderer *) ctx->DriverCtx;
	BGLView *bglview = mr->GLView();
	assert(bglview);
	int flippedY = mr->fBottom - y;
	bglview->SetHighColor(color[RCOMP], color[GCOMP], color[BCOMP]);
	if (mask) {
		for (GLuint i = 0; i < n; i++) {
			if (mask[i]) {
				Plot(bglview, x++, flippedY);
			}
		}
	} else {
		for (GLuint i = 0; i < n; i++) {
			Plot(bglview, x++, flippedY);
		}
	}
}

void
MesaSoftwareRenderer::WriteRGBAPixelsFront(const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   CONST GLubyte rgba[][4],
                                   const GLubyte mask[] )
{
	CALLED();
	MesaSoftwareRenderer *mr = (MesaSoftwareRenderer *) ctx->DriverCtx;
	BGLView *bglview = mr->GLView();
	assert(bglview);
	if (mask) {
		for (GLuint i = 0; i < n; i++) {
			if (mask[i]) {
				bglview->SetHighColor(rgba[i][0], rgba[i][1], rgba[i][2]);
				Plot(bglview, x[i], mr->fBottom - y[i]);
			}
		}
	} else {
		for (GLuint i = 0; i < n; i++) {
			bglview->SetHighColor(rgba[i][0], rgba[i][1], rgba[i][2]);
			Plot(bglview, x[i], mr->fBottom - y[i]);
		}
	}
}


void
MesaSoftwareRenderer::WriteMonoRGBAPixelsFront(const GLcontext *ctx, GLuint n,
                                       const GLint x[], const GLint y[],
                                       const GLchan color[4],
                                       const GLubyte mask[])
{
	CALLED();
	MesaSoftwareRenderer *mr = (MesaSoftwareRenderer *) ctx->DriverCtx;
	BGLView *bglview = mr->GLView();
	assert(bglview);
	// plot points using current color
	bglview->SetHighColor(color[RCOMP], color[GCOMP], color[BCOMP]);
	if (mask) {
		for (GLuint i = 0; i < n; i++) {
			if (mask[i]) {
				Plot(bglview, x[i], mr->fBottom - y[i]);
			}
		}
	} else {
		for (GLuint i = 0; i < n; i++) {
			Plot(bglview, x[i], mr->fBottom - y[i]);
		}
	}
}


void
MesaSoftwareRenderer::WriteCI32SpanFront( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                             const GLuint index[], const GLubyte mask[] )
{
 	printf("WriteCI32SpanFront() not implemented yet!\n");
   // TODO
}

void
MesaSoftwareRenderer::WriteCI8SpanFront( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                            const GLubyte index[], const GLubyte mask[] )
{
 	printf("WriteCI8SpanFront() not implemented yet!\n");
   // TODO
}

void
MesaSoftwareRenderer::WriteMonoCISpanFront( const GLcontext *ctx, GLuint n,
                                    GLint x, GLint y,
                                    GLuint colorIndex, const GLubyte mask[] )
{
 	printf("WriteMonoCISpanFront() not implemented yet!\n");
   // TODO
}


void
MesaSoftwareRenderer::WriteCI32PixelsFront( const GLcontext *ctx, GLuint n,
                                    const GLint x[], const GLint y[],
                                    const GLuint index[], const GLubyte mask[] )
{
 	printf("WriteCI32PixelsFront() not implemented yet!\n");
   // TODO
}

void
MesaSoftwareRenderer::WriteMonoCIPixelsFront( const GLcontext *ctx, GLuint n,
                                      const GLint x[], const GLint y[],
                                      GLuint colorIndex, const GLubyte mask[] )
{
 	printf("WriteMonoCIPixelsFront() not implemented yet!\n");
   // TODO
}


void
MesaSoftwareRenderer::ReadCI32SpanFront( const GLcontext *ctx,
                                 GLuint n, GLint x, GLint y, GLuint index[] )
{
 	printf("ReadCI32SpanFront() not implemented yet!\n");
  // TODO
}


void
MesaSoftwareRenderer::ReadRGBASpanFront( const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y, GLubyte rgba[][4] )
{
 	printf("ReadRGBASpanFront() not implemented yet!\n");
   // TODO
}


void
MesaSoftwareRenderer::ReadCI32PixelsFront( const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   GLuint indx[], const GLubyte mask[] )
{
 	printf("ReadCI32PixelsFront() not implemented yet!\n");
   // TODO
}


void
MesaSoftwareRenderer::ReadRGBAPixelsFront( const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   GLubyte rgba[][4], const GLubyte mask[] )
{
 	printf("ReadRGBAPixelsFront() not implemented yet!\n");
   // TODO
}




void
MesaSoftwareRenderer::WriteRGBASpanBack(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 CONST GLubyte rgba[][4],
                                 const GLubyte mask[])
{
	//CALLED();
	MesaSoftwareRenderer *mr = (MesaSoftwareRenderer *) ctx->DriverCtx;
	BBitmap *bitmap = mr->fBitmap;

	assert(bitmap);

	int row = mr->fBottom - y;
	uint8 * ptr = (uint8 *) bitmap->Bits() + (row * bitmap->BytesPerRow()) + x * 4;
 	uint32 * pixel = (uint32 *) ptr;
	
	if (mask) {
		while(n--) {
			if (*mask++)
				*pixel = PACK_B_RGBA32(rgba[0]);
			pixel++;
			rgba++;
		};
	} else {
		while(n--) {
			*pixel++ = PACK_B_RGBA32(rgba[0]);
			rgba++;
		};
	};
 }


void
MesaSoftwareRenderer::WriteRGBSpanBack(const GLcontext *ctx, GLuint n,
                                GLint x, GLint y,
                                CONST GLubyte rgb[][3],
                                const GLubyte mask[])
{
	CALLED();
	MesaSoftwareRenderer *mr = (MesaSoftwareRenderer *) ctx->DriverCtx;
	BBitmap *bitmap = mr->fBitmap;

	assert(bitmap);

	int row = mr->fBottom - y;
	uint8 * ptr = (uint8 *) bitmap->Bits() + (row * bitmap->BytesPerRow()) + x * 4;
 	uint32 * pixel = (uint32 *) ptr;
	
	if (mask) {
		while(n--) {
			if (*mask++)
				*pixel = PACK_B_RGB32(rgb[0]);
			pixel++;
			rgb++;
		};
	} else {
		while(n--) {
			*pixel++ = PACK_B_RGB32(rgb[0]);
			rgb++;
		};
	};
}




void
MesaSoftwareRenderer::WriteMonoRGBASpanBack(const GLcontext *ctx, GLuint n,
                                    GLint x, GLint y,
                                    const GLchan color[4], const GLubyte mask[])
{
	CALLED();
	MesaSoftwareRenderer *mr = (MesaSoftwareRenderer *) ctx->DriverCtx;
	BBitmap *bitmap = mr->fBitmap;

	assert(bitmap);

	int row = mr->fBottom - y;
	uint8 * ptr = (uint8 *) bitmap->Bits() + (row * bitmap->BytesPerRow()) + x * 4;
 	uint32 * pixel = (uint32 *) ptr;
	uint32 pixel_color = PACK_B_RGBA32(color);
	
	if (mask) {
		while(n--) {
			if (*mask++)
				*pixel = pixel_color;
			pixel++;
		};
	} else {
		while(n--) {
			*pixel++ = pixel_color;
		};
	};
}


void
MesaSoftwareRenderer::WriteRGBAPixelsBack(const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   CONST GLubyte rgba[][4],
                                   const GLubyte mask[] )
{
	//CALLED();
	MesaSoftwareRenderer *mr = (MesaSoftwareRenderer *) ctx->DriverCtx;
	BBitmap *bitmap = mr->fBitmap;

	assert(bitmap);
#if 0
	while(n--) {
		if (*mask++) {
			int row = md->m_bottom - *y;
			uint8 * pixel = (uint8 *) bitmap->Bits() + (row * bitmap->BytesPerRow()) + *x * 4;
			*((uint32 *) pixel) = PACK_B_RGBA32(rgba[0]);
		};
		x++;
		y++;
		rgba++;
	};
#else
	if (mask) {
		for (GLuint i = 0; i < n; i++) {
			if (mask[i]) {
				GLubyte *pixel = (GLubyte *) bitmap->Bits()
					+ ((mr->fBottom - y[i]) * bitmap->BytesPerRow()) + x[i] * 4;
				pixel[BE_RCOMP] = rgba[i][RCOMP];
				pixel[BE_GCOMP] = rgba[i][GCOMP];
				pixel[BE_BCOMP] = rgba[i][BCOMP];
				pixel[BE_ACOMP] = rgba[i][ACOMP];
			}
		}
	} else {
		for (GLuint i = 0; i < n; i++) {
			GLubyte *pixel = (GLubyte *) bitmap->Bits()
				+ ((mr->fBottom - y[i]) * bitmap->BytesPerRow()) + x[i] * 4;
			pixel[BE_RCOMP] = rgba[i][RCOMP];
			pixel[BE_GCOMP] = rgba[i][GCOMP];
			pixel[BE_BCOMP] = rgba[i][BCOMP];
			pixel[BE_ACOMP] = rgba[i][ACOMP];
		}
	}
#endif
}


void
MesaSoftwareRenderer::WriteMonoRGBAPixelsBack(const GLcontext *ctx, GLuint n,
                                      const GLint x[], const GLint y[],
                                      const GLchan color[4],
                                      const GLubyte mask[])
{
	CALLED();
	MesaSoftwareRenderer *mr = (MesaSoftwareRenderer *) ctx->DriverCtx;
	BBitmap *bitmap = mr->fBitmap;

	assert(bitmap);

	uint32 pixel_color = PACK_B_RGBA32(color);
#if 0	
	while(n--) {
		if (*mask++) {
			int row = md->m_bottom - *y;
			uint8 * pixel = (uint8 *) bitmap->Bits() + (row * bitmap->BytesPerRow()) + *x * 4;

			*((uint32 *) pixel) = pixel_color;
		};
		x++;
		y++;
	};
#else
	if (mask) {
		for (GLuint i = 0; i < n; i++) {
			if (mask[i]) {
				GLubyte * ptr = (GLubyte *) bitmap->Bits()
					+ ((mr->fBottom - y[i]) * bitmap->BytesPerRow()) + x[i] * 4;
				*((uint32 *) ptr) = pixel_color;
			}
		}
	} else {
		for (GLuint i = 0; i < n; i++) {
			GLubyte * ptr = (GLubyte *) bitmap->Bits()
				+ ((mr->fBottom - y[i]) * bitmap->BytesPerRow()) + x[i] * 4;
			*((uint32 *) ptr) = pixel_color;
		}
	}
#endif
}


void
MesaSoftwareRenderer::WriteCI32SpanBack( const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 const GLuint index[], const GLubyte mask[] )
{
 	printf("WriteCI32SpanBack() not implemented yet!\n");
   // TODO
}

void
MesaSoftwareRenderer::WriteCI8SpanBack( const GLcontext *ctx, GLuint n,
                                GLint x, GLint y,
                                const GLubyte index[], const GLubyte mask[] )
{
  	printf("WriteCI8SpanBack() not implemented yet!\n");
  // TODO
}

void
MesaSoftwareRenderer::WriteMonoCISpanBack( const GLcontext *ctx, GLuint n,
                                   GLint x, GLint y,
                                   GLuint colorIndex, const GLubyte mask[] )
{
 	printf("WriteMonoCISpanBack() not implemented yet!\n");
   // TODO
}


void
MesaSoftwareRenderer::WriteCI32PixelsBack( const GLcontext *ctx, GLuint n,
                                   const GLint x[], const GLint y[],
                                   const GLuint index[], const GLubyte mask[] )
{
 	printf("WriteCI32PixelsBack() not implemented yet!\n");
   // TODO
}

void
MesaSoftwareRenderer::WriteMonoCIPixelsBack( const GLcontext *ctx, GLuint n,
                                     const GLint x[], const GLint y[],
                                     GLuint colorIndex, const GLubyte mask[] )
{
 	printf("WriteMonoCIPixelsBack() not implemented yet!\n");
   // TODO
}


void
MesaSoftwareRenderer::ReadCI32SpanBack( const GLcontext *ctx,
                                GLuint n, GLint x, GLint y, GLuint index[] )
{
 	printf("ReadCI32SpanBack() not implemented yet!\n");
   // TODO
}


void
MesaSoftwareRenderer::ReadRGBASpanBack( const GLcontext *ctx, GLuint n,
                                GLint x, GLint y, GLubyte rgba[][4] )
{
	CALLED();
	MesaSoftwareRenderer *mr = (MesaSoftwareRenderer *) ctx->DriverCtx;
	const BBitmap *bitmap = mr->fBitmap;
	assert(bitmap);
	int row = mr->fBottom - y;
	const GLubyte *pixel = (GLubyte *) bitmap->Bits()
		+ (row * bitmap->BytesPerRow()) + x * 4;

	for (GLuint i = 0; i < n; i++) {
		rgba[i][RCOMP] = pixel[BE_RCOMP];
		rgba[i][GCOMP] = pixel[BE_GCOMP];
		rgba[i][BCOMP] = pixel[BE_BCOMP];
		rgba[i][ACOMP] = pixel[BE_ACOMP];
		pixel += 4;
	}
}


void
MesaSoftwareRenderer::ReadCI32PixelsBack( const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   GLuint indx[], const GLubyte mask[] )
{
 	printf("ReadCI32PixelsBack() not implemented yet!\n");
   // TODO
}


void
MesaSoftwareRenderer::ReadRGBAPixelsBack( const GLcontext *ctx,
                                  GLuint n, const GLint x[], const GLint y[],
                                  GLubyte rgba[][4], const GLubyte mask[] )
{
	CALLED();
	MesaSoftwareRenderer *mr = (MesaSoftwareRenderer *) ctx->DriverCtx;
	const BBitmap *bitmap = mr->fBitmap;
	assert(bitmap);

	if (mask) {
		for (GLuint i = 0; i < n; i++) {
			if (mask[i]) {
				GLubyte *pixel = (GLubyte *) bitmap->Bits()
					+ ((mr->fBottom - y[i]) * bitmap->BytesPerRow()) + x[i] * 4;
				rgba[i][RCOMP] = pixel[BE_RCOMP];
				rgba[i][GCOMP] = pixel[BE_GCOMP];
				rgba[i][BCOMP] = pixel[BE_BCOMP];
				rgba[i][ACOMP] = pixel[BE_ACOMP];
			};
		};
	} else {
		for (GLuint i = 0; i < n; i++) {
			GLubyte *pixel = (GLubyte *) bitmap->Bits()
				+ ((mr->fBottom - y[i]) * bitmap->BytesPerRow()) + x[i] * 4;
			rgba[i][RCOMP] = pixel[BE_RCOMP];
			rgba[i][GCOMP] = pixel[BE_GCOMP];
			rgba[i][BCOMP] = pixel[BE_BCOMP];
			rgba[i][ACOMP] = pixel[BE_ACOMP];
		};
	};
}

