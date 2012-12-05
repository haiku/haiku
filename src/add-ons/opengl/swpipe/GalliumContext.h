/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */
#ifndef GALLIUMCONTEXT_H
#define GALLIUMCONTEXT_H


extern "C" {
#include "state_tracker/st_api.h"
#include "pipe/p_compiler.h"
#include "pipe/p_screen.h"
#include "postprocess/filters.h"
#include "os/os_thread.h"
}
#include "bitmap_wrapper.h"
#include "GalliumFramebuffer.h"


#define CONTEXT_MAX 32

// HACK: offsetof must be redefined before loading sp_context.h
// in GalliumContext.cpp
#undef offsetof
#define offsetof( type, member ) ((size_t) &((type *)0)->member)


typedef int64 context_id;

struct hgl_context
{
	struct st_api* api;
		// State Tracker API
	struct st_manager* manager;
		// State Tracker Manager
	struct st_context_iface* st;
		// State Tracker Interface Object
	struct st_visual* stVisual;
		// State Tracker Visual

	struct pipe_resource* textures[ST_ATTACHMENT_COUNT];

	// Post processing
	struct pp_queue_t* postProcess;
	unsigned int postProcessEnable[PP_FILTERS];

	Bitmap* bitmap;
	color_space colorSpace;

	GalliumFramebuffer* draw;
	GalliumFramebuffer* read;
};


class GalliumContext {
public:
							GalliumContext(ulong options);
							~GalliumContext();

		context_id			CreateContext(Bitmap* bitmap);
		void				DestroyContext(context_id contextID);
		context_id			GetCurrentContext() { return fCurrentContext; };
		status_t			SetCurrentContext(Bitmap *bitmap,
								context_id contextID);

		status_t			SwapBuffers(context_id contextID);

private:
		status_t			CreateScreen();
		void				Flush();

		ulong				fOptions;

		struct hgl_context*	fContext[CONTEXT_MAX];
		context_id			fCurrentContext;
		
		struct pipe_screen*	fScreen;
		pipe_mutex			fMutex;
};
	

#endif /* GALLIUMCONTEXT_H */
