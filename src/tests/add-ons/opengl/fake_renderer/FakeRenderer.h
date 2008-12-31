/*
 * Copyright 2006-2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Philippe Houdoin, philippe.houdoin@free.fr
 */

#ifndef FAKE_RENDERER_H
#define FAKE_RENDERER_H

#include "GLRenderer.h"

class FakeRenderer : public BGLRenderer {
public:
							FakeRenderer(BGLView* view,
								ulong bgl_options, 
								BGLDispatcher* dispatcher);
		virtual				~FakeRenderer();

		virtual	void 		SwapBuffers(bool VSync = false);
		virtual	void		Draw(BRect updateRect);

		virtual	void		EnableDirectMode(bool enabled);
		virtual	void		DirectConnected(direct_buffer_info* info);

private:
		static int32 		_DirectDrawThread(void *data);
		int32 				_DirectDrawThread();

		ulong				fOptions;

		sem_id				fDrawSem;
		thread_id			fDrawThread;
		BLocker				fDrawLocker;

		rgb_color			fDrawColor;			

		uint8 *				fFrameBuffer;
		int32				fBytesPerRow;
		color_space			fColorSpace;
		clipping_rect		fBounds;
		clipping_rect *		fRects;
		int					fRectsCount;

		bool				fConnected;
		bool				fDisabled;
};

#endif	// FAKE_RENDERER_H

