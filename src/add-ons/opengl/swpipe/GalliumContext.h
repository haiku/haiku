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
#include "os/os_thread.h"
}

#include "bitmap_wrapper.h"


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

	Bitmap* bitmap;
	color_space colorSpace;

	struct st_framebuffer_iface* draw;
	struct st_framebuffer_iface* read;
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
