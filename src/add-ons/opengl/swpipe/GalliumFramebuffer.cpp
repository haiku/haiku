/*
 * Copyright 2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Artur Wyszynski, harakash@gmail.com
 *      Alexander von Gluck IV, kallisti5@unixzen.com
 */


#include "GalliumFramebuffer.h"

extern "C" {
#include "main/context.h"
#include "main/framebuffer.h"
#include "main/renderbuffer.h"
#include "pipe/p_format.h"
}


#define TRACE_FRAMEBUFFER
#ifdef TRACE_FRAEMBUFFER
#   define TRACE(x...) printf("GalliumFramebuffer: " x)
#   define CALLED() TRACE("CALLED: %s\n", __PRETTY_FUNCTION__)
#else
#   define TRACE(x...)
#   define CALLED()
#endif
#define ERROR(x...) printf("GalliumFramebuffer: " x)


static boolean
hgl_framebuffer_flush_front(struct st_framebuffer_iface* stfb,
	enum st_attachment_type statt)
{
	CALLED();
	// TODO: I have *NO* idea how we are going to access this data...

	#if 0
	struct stw_st_framebuffer *stwfb = stw_st_framebuffer(stfb);
	pipe_mutex_lock(stwfb->fb->mutex);

	struct pipe_resource* resource = textures[statt];
	if (resource)
		stw_framebuffer_present_locked(...);
	#endif

	return TRUE;
}


static boolean
hgl_framebuffer_validate(struct st_framebuffer_iface* stfb,
	const enum st_attachment_type* statts, unsigned count,
	struct pipe_resource** out)
{
	CALLED();

	return TRUE;
}


GalliumFramebuffer::GalliumFramebuffer(struct st_visual* visual)
	:
	fBuffer(NULL)
{
	CALLED();
	fBuffer = CALLOC_STRUCT(st_framebuffer_iface);
	if (!fBuffer) {
		ERROR("%s: Couldn't calloc framebuffer!\n", __func__);
		return;
	}
	fBuffer->visual = visual;
	fBuffer->flush_front = hgl_framebuffer_flush_front;
	fBuffer->validate = hgl_framebuffer_validate;

	pipe_mutex_init(fMutex);
}


GalliumFramebuffer::~GalliumFramebuffer()
{
	CALLED();
	// We lock and unlock to try and make sure we wait for anything
	// using the framebuffer to finish
	Lock();
	if (!fBuffer) {
		ERROR("%s: Strange, no Gallium Framebuffer to free?\n", __func__);
		return;
	}
	FREE(fBuffer);
	Unlock();

	pipe_mutex_destroy(fMutex);
}


status_t
GalliumFramebuffer::Lock()
{
	CALLED();
	pipe_mutex_lock(fMutex);
	return B_OK;
}


status_t
GalliumFramebuffer::Unlock()
{
	CALLED();
	pipe_mutex_unlock(fMutex);
	return B_OK;
}
