/*
 * Copyright 2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Artur Wyszynski, harakash@gmail.com
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */


#include "GalliumContext.h"

#include "GLView.h"

#include "bitmap_wrapper.h"
#include "SoftwareWinsys.h"
extern "C" {
#include "glapi/glapi.h"
#include "main/context.h"
#include "main/framebuffer.h"
#include "main/renderbuffer.h"
#include "pipe/p_format.h"
#include "state_tracker/st_cb_fbo.h"
#include "state_tracker/st_cb_flush.h"
#include "state_tracker/st_context.h"
#include "state_tracker/st_gl_api.h"
#include "state_tracker/st_manager.h"
#include "state_tracker/sw_winsys.h"
#ifdef HAVE_LLVM
#include "llvmpipe/lp_public.h"
#else
#include "softpipe/sp_public.h"
#endif
}


#define TRACE_CONTEXT
#ifdef TRACE_CONTEXT
#	define TRACE(x...) printf("GalliumContext: " x)
#	define CALLED() TRACE("CALLED: %s\n", __PRETTY_FUNCTION__)
#else
#	define TRACE(x...)
#	define CALLED()
#endif
#define ERROR(x...) printf("GalliumContext: " x)


static void
hgl_viewport(struct gl_context* glContext, GLint x, GLint y,
	GLsizei width, GLsizei height)
{
	TRACE("%s(glContext: %p, x: %d, y: %d, width: %d, height: %d\n", __func__,
		glContext, x, y, width, height);
	struct hgl_context *context = (struct hgl_context*)glContext->DriverCtx;

	if (!context) {
		ERROR("%s: No context yet. bailing.\n", __func__);
		return;
	}

	int32 bitmapWidth;
	int32 bitmapHeight;

	get_bitmap_size(context->bitmap, &bitmapWidth, &bitmapHeight);

	if (width != bitmapWidth || height != bitmapHeight) {
		struct gl_framebuffer *draw = glContext->WinSysDrawBuffer;
		struct gl_framebuffer *read = glContext->WinSysReadBuffer;

		if (draw)
			_mesa_resize_framebuffer(glContext, draw, bitmapWidth, bitmapHeight);
		if (read)
			_mesa_resize_framebuffer(glContext, read, bitmapWidth, bitmapHeight);
	}
}


static st_visual*
hgl_fill_st_visual(gl_config* glVisual)
{
	struct st_visual* stVisual = CALLOC_STRUCT(st_visual);
	if (!stVisual) {
		ERROR("%s: Couldn't allocate st_visual\n", __func__);
		return NULL;
	}

	// Determine color format
	if (glVisual->redBits == 8) {
		if (glVisual->alphaBits == 8)
			stVisual->color_format = PIPE_FORMAT_B8G8R8A8_UNORM;
		else
			stVisual->color_format = PIPE_FORMAT_B8G8R8X8_UNORM;
	} else {
		stVisual->color_format = PIPE_FORMAT_B5G6R5_UNORM;
	}

	// Determine depth stencil format
	switch (glVisual->depthBits) {
		default:
		case 0:
			stVisual->depth_stencil_format = PIPE_FORMAT_NONE;
			break;
		case 16:
			stVisual->depth_stencil_format = PIPE_FORMAT_Z16_UNORM;
			break;
		case 24:
			if (glVisual->stencilBits == 0) {
				stVisual->depth_stencil_format = PIPE_FORMAT_Z24X8_UNORM;
				// or PIPE_FORMAT_X8Z24_UNORM?
			} else {
				stVisual->depth_stencil_format = PIPE_FORMAT_Z24_UNORM_S8_UINT;
				// or PIPE_FORMAT_S8_UINT_Z24_UNORM?
			}
			break;
		case 32:
			stVisual->depth_stencil_format = PIPE_FORMAT_Z32_UNORM;
			break;
	}

	stVisual->accum_format = (glVisual->haveAccumBuffer)
		? PIPE_FORMAT_R16G16B16A16_SNORM : PIPE_FORMAT_NONE;

	stVisual->buffer_mask |= ST_ATTACHMENT_FRONT_LEFT_MASK;
	stVisual->render_buffer = ST_ATTACHMENT_FRONT_LEFT;
	if (glVisual->doubleBufferMode) {
		stVisual->buffer_mask |= ST_ATTACHMENT_BACK_LEFT_MASK;
		stVisual->render_buffer = ST_ATTACHMENT_BACK_LEFT;
	}

	if (glVisual->stereoMode) {
		stVisual->buffer_mask |= ST_ATTACHMENT_FRONT_RIGHT_MASK;
		if (glVisual->doubleBufferMode)
			stVisual->buffer_mask |= ST_ATTACHMENT_BACK_RIGHT_MASK;
	}

	if (glVisual->haveDepthBuffer || glVisual->haveStencilBuffer)
		stVisual->buffer_mask |= ST_ATTACHMENT_DEPTH_STENCIL_MASK;

	return stVisual;
}


static INLINE unsigned
round_up(unsigned n, unsigned multiple)
{
	return (n + multiple - 1) & ~(multiple - 1);
}


static int
hook_stm_get_param(struct st_manager *smapi, enum st_manager_param param)
{
	CALLED();

	switch (param) {
		case ST_MANAGER_BROKEN_INVALIDATE:
			TRACE("%s: TODO: How should we handle BROKEN_INVALIDATE calls?\n",
				__func__);
			// For now we force validation of the framebuffer.
			return 1;
	}

	return 0;
}


GalliumContext::GalliumContext(ulong options)
	:
	fOptions(options),
	fCurrentContext(0),
	fScreen(NULL)
{
	CALLED();

	// Make all contexts a known value
	for (context_id i = 0; i < CONTEXT_MAX; i++)
		fContext[i] = NULL;

	CreateScreen();

	pipe_mutex_init(fMutex);
}


GalliumContext::~GalliumContext()
{
	CALLED();

	// Destroy our contexts
	pipe_mutex_lock(fMutex);
	for (context_id i = 0; i < CONTEXT_MAX; i++)
		DestroyContext(i);
	pipe_mutex_unlock(fMutex);

	pipe_mutex_destroy(fMutex);

	// TODO: Destroy fScreen
}


status_t
GalliumContext::CreateScreen()
{
	CALLED();

	// Allocate winsys and attach callback hooks
	struct sw_winsys* winsys = winsys_connect_hooks();

	if (!winsys) {
		ERROR("%s: Couldn't allocate sw_winsys!\n", __func__);
		return B_ERROR;
	}

	#ifdef HAVE_LLVM
	fScreen = llvmpipe_create_screen(winsys);
	#else
	fScreen = softpipe_create_screen(winsys);
	#endif

	if (fScreen == NULL) {
		ERROR("%s: Couldn't create screen!\n", __FUNCTION__);
		FREE(winsys);
		return B_ERROR;
	}

	const char* driverName = fScreen->get_name(fScreen);
	ERROR("%s: Using %s driver.\n", __func__, driverName);

	return B_OK;
}


context_id
GalliumContext::CreateContext(Bitmap *bitmap)
{
	CALLED();

	struct hgl_context* context = CALLOC_STRUCT(hgl_context);

	if (!context) {
		ERROR("%s: Couldn't create pipe context!\n", __FUNCTION__);
		return 0;
	}

	// Set up the initial things our context needs
	context->bitmap = bitmap;
	context->colorSpace = get_bitmap_color_space(bitmap);
	context->draw = NULL;
	context->read = NULL;
	context->st = NULL;

	context->api = st_gl_api_create();
	if (!context->api) {
		ERROR("%s: Couldn't obtain Mesa state tracker API!\n", __func__);
		return -1;
	}

	context->manager = CALLOC_STRUCT(st_manager);
	if (!context->manager) {
		ERROR("%s: Couldn't allocate Mesa state tracker manager!\n", __func__);
		return -1;
	}
	context->manager->get_param = hook_stm_get_param;

	// Calculate visual configuration
	const GLboolean rgbFlag		= ((fOptions & BGL_INDEX) == 0);
	const GLboolean alphaFlag	= ((fOptions & BGL_ALPHA) == BGL_ALPHA);
	const GLboolean dblFlag		= ((fOptions & BGL_DOUBLE) == BGL_DOUBLE);
	const GLboolean stereoFlag	= false;
	const GLint depth			= (fOptions & BGL_DEPTH) ? 24 : 0;
	const GLint stencil			= (fOptions & BGL_STENCIL) ? 8 : 0;
	const GLint accum			= 0;		// (options & BGL_ACCUM) ? 16 : 0;
	const GLint red				= rgbFlag ? 8 : 0;
	const GLint green			= rgbFlag ? 8 : 0;
	const GLint blue			= rgbFlag ? 8 : 0;
	const GLint alpha			= alphaFlag ? 8 : 0;

	TRACE("rgb      :\t%d\n", (bool)rgbFlag);
	TRACE("alpha    :\t%d\n", (bool)alphaFlag);
	TRACE("dbl      :\t%d\n", (bool)dblFlag);
	TRACE("stereo   :\t%d\n", (bool)stereoFlag);
	TRACE("depth    :\t%d\n", depth);
	TRACE("stencil  :\t%d\n", stencil);
	TRACE("accum    :\t%d\n", accum);
	TRACE("red      :\t%d\n", red);
	TRACE("green    :\t%d\n", green);
	TRACE("blue     :\t%d\n", blue);
	TRACE("alpha    :\t%d\n", alpha);

	gl_config* glVisual = _mesa_create_visual(dblFlag, stereoFlag, red, green,
		blue, alpha, depth, stencil, accum, accum, accum, alpha ? accum : 0, 1);

	if (!glVisual) {
		ERROR("%s: Couldn't create Mesa visual!\n", __func__);
		return -1;
	}

	TRACE("depthBits   :\t%d\n", glVisual->depthBits);
	TRACE("stencilBits :\t%d\n", glVisual->stencilBits);

	// Convert Mesa calculated visual into state tracker visual
	context->stVisual = hgl_fill_st_visual(glVisual);

	context->draw = new GalliumFramebuffer(context->stVisual);
	context->read = new GalliumFramebuffer(context->stVisual);

	if (!context->draw || !context->read) {
		ERROR("%s: Problem allocating framebuffer!\n", __func__);
		_mesa_destroy_visual(glVisual);
		return -1;
	}

	// We need to assign the screen *before* calling st_api create_context
	context->manager->screen = fScreen;

	// Build state tracker attributes
	struct st_context_attribs attribs;
	memset(&attribs, 0, sizeof(attribs));
	attribs.options.force_glsl_extensions_warn = false;
	attribs.profile = ST_PROFILE_DEFAULT;
	attribs.visual = *context->stVisual;
	attribs.major = 1;
	attribs.minor = 0;
	//attribs.flags |= ST_CONTEXT_FLAG_DEBUG;

	struct st_api* api = context->api;

	// Create context using state tracker api call
	enum st_context_error result;
	context->st = api->create_context(api, context->manager, &attribs,
		&result, context->st);

	if (!context->st) {
		ERROR("%s: Couldn't create mesa state tracker context!\n",
			__func__);
		switch (result) {
			case ST_CONTEXT_SUCCESS:
				ERROR("%s: State tracker error: SUCCESS?\n", __func__);
				break;
			case ST_CONTEXT_ERROR_NO_MEMORY:
				ERROR("%s: State tracker error: NO_MEMORY\n", __func__);
				break;
			case ST_CONTEXT_ERROR_BAD_API:
				ERROR("%s: State tracker error: BAD_API\n", __func__);
				break;
			case ST_CONTEXT_ERROR_BAD_VERSION:
				ERROR("%s: State tracker error: BAD_VERSION\n", __func__);
				break;
			case ST_CONTEXT_ERROR_BAD_FLAG:
				ERROR("%s: State tracker error: BAD_FLAG\n", __func__);
				break;
			case ST_CONTEXT_ERROR_UNKNOWN_ATTRIBUTE:
				ERROR("%s: State tracker error: BAD_ATTRIBUTE\n", __func__);
				break;
			case ST_CONTEXT_ERROR_UNKNOWN_FLAG:
				ERROR("%s: State tracker error: UNKNOWN_FLAG\n", __func__);
				break;
		}

		FREE(context);
		return -1;
	}

	// Init Gallium3D Post Processing
	context->postProcess = pp_init(fScreen, context->postProcessEnable);

	assert(!context->st->st_manager_private);
	context->st->st_manager_private = (void*)context;

	struct st_context *stContext = (struct st_context*)context->st;
	
	stContext->ctx->DriverCtx = context;
	stContext->ctx->Driver.Viewport = hgl_viewport;

	// TODO: Closely review this next context logic...
	context_id contextNext = -1;

	pipe_mutex_lock(fMutex);
	for (context_id i = 0; i < CONTEXT_MAX; i++) {
		if (fContext[i] == NULL) {
			fContext[i] = context;
			contextNext = i;
			break;
		}
	}
	pipe_mutex_unlock(fMutex);

	if (contextNext < 0) {
		ERROR("%s: The next context is invalid... something went wrong!\n",
			__func__);
		//st_destroy_context(context->st);
		FREE(context);
		_mesa_destroy_visual(glVisual);
		return -1;
	}

	TRACE("%s: context #%" B_PRIu64 " is the next available context\n",
		__func__, contextNext);

	return contextNext;
}


void
GalliumContext::DestroyContext(context_id contextID)
{
	// fMutex should be locked *before* calling DestoryContext

	// See if context is used
	if (!fContext[contextID])
		return;

	if (fContext[contextID]->st) {
		fContext[contextID]->st->flush(fContext[contextID]->st, 0, NULL);
		fContext[contextID]->st->destroy(fContext[contextID]->st);
	}

	if (fContext[contextID]->postProcess)
		pp_free(fContext[contextID]->postProcess);

	// Delete framebuffer objects
	if (fContext[contextID]->read)
		delete fContext[contextID]->read;
	if (fContext[contextID]->draw)
		delete fContext[contextID]->draw;

	if (fContext[contextID]->stVisual)
		FREE(fContext[contextID]->stVisual);

	if (fContext[contextID]->manager)
		FREE(fContext[contextID]->manager);

	FREE(fContext[contextID]);
}


status_t
GalliumContext::SetCurrentContext(Bitmap *bitmap, context_id contextID)
{
	CALLED();

	if (contextID < 0 || contextID > CONTEXT_MAX) {
		ERROR("%s: Invalid context ID range!\n", __func__);
		return B_ERROR;
	}

	pipe_mutex_lock(fMutex);
	context_id oldContextID = fCurrentContext;
	struct hgl_context* context = fContext[contextID];
	pipe_mutex_unlock(fMutex);

	if (!context) {
		ERROR("%s: Invalid context provided (#%" B_PRIu64 ")!\n",
			__func__, contextID);
		return B_ERROR;
	}

	struct st_api* api = context->api;

	if (!bitmap) {
		api->make_current(context->api, NULL, NULL, NULL);
		return B_ERROR;
	}

	// Everything seems valid, lets set the new context.
	fCurrentContext = contextID;

	if (oldContextID > 0 && oldContextID != contextID) {
		fContext[oldContextID]->st->flush(fContext[oldContextID]->st,
			ST_FLUSH_FRONT, NULL);
	}

	// We need to lock and unlock framebuffers before accessing them
	context->draw->Lock();
	context->read->Lock();
	api->make_current(context->api, context->st, context->draw->fBuffer,
		context->read->fBuffer);
	context->draw->Unlock();
	context->read->Unlock();

	// TODO: Init textures before post-processing them
	#if 0
	pp_init_fbos(context->postProcess,
		context->textures[ST_ATTACHMENT_BACK_LEFT]->width0,
		context->textures[ST_ATTACHMENT_BACK_LEFT]->height0);
	#endif

	context->bitmap = bitmap;
	//context->st->pipe->priv = context;

	return B_OK;
}


status_t
GalliumContext::SwapBuffers(context_id contextID)
{
	CALLED();

	pipe_mutex_lock(fMutex);
	struct hgl_context *context = fContext[contextID];
	pipe_mutex_unlock(fMutex);

	if (!context) {
		ERROR("%s: context not found\n", __func__);
		return B_ERROR;
	}

	// TODO: Where did st_notify_swapbuffers go?
	//st_notify_swapbuffers(context->draw->stfb);

	context->st->flush(context->st, ST_FLUSH_FRONT, NULL);

	struct st_context *stContext = (struct st_context*)context->st;

	unsigned nColorBuffers = stContext->state.framebuffer.nr_cbufs;
	for (unsigned i = 0; i < nColorBuffers; i++) {
		pipe_surface* surface = stContext->state.framebuffer.cbufs[i];
		if (!surface) {
			ERROR("%s: Color buffer %d invalid!\n", __func__, i);
			continue;
		}

		TRACE("%s: Flushing color buffer #%d\n", __func__, i);

		// We pass our destination bitmap to flush_fronbuffer which passes it
		// to the private winsys display call.
		fScreen->flush_frontbuffer(fScreen, surface->texture, 0, 0,
			context->bitmap);
	}

	#if 0
	// TODO... should we flush the z stencil buffer?
	pipe_surface* zSurface = stContext->state.framebuffer.zsbuf;
	fScreen->flush_frontbuffer(fScreen, surface->texture, 0, 0,
		context->bitmap);
	#endif

	return B_OK;
}
