/*
 * Copyright 2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck IV, kallisti5@unixzen.com
 */
#ifndef GALLIUMFRAMEBUFFER_H
#define GALLIUMFRAMEBUFFER_H


extern "C" {
#include "os/os_thread.h"
#include "pipe/p_screen.h"
#include "state_tracker/st_api.h"
}


class GalliumFramebuffer {
public:
							GalliumFramebuffer(struct st_visual* visual,
								void* privateContext);
							~GalliumFramebuffer();
		status_t			Lock();
		status_t			Unlock();

		struct st_framebuffer_iface* fBuffer;

private:
		pipe_mutex			fMutex;
};


#endif /* GALLIUMFRAMEBUFFER_H */
