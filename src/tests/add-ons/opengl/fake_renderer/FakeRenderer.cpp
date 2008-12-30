/*
 * Copyright 2006-2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin, philippe.houdoin@free.fr
 */
/*
 * Mesa 3-D graphics library
 * Version:  6.1
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
 */

#include "FakeRenderer.h"

#include <stdio.h>

#include <Autolock.h>
#include <DirectWindowPrivate.h>
#include <GraphicsDefs.h>
#include <Screen.h>


extern const char * color_space_name(color_space space);


extern "C" _EXPORT BGLRenderer*
instantiate_gl_renderer(BGLView* view, ulong options,
	BGLDispatcher* dispatcher)
{
	return new FakeRenderer(view, options, dispatcher);
}


FakeRenderer::FakeRenderer(BGLView* view, ulong options,
		BGLDispatcher* dispatcher)
	: BGLRenderer(view, options, dispatcher),
	fOptions(options),
	fDrawLocker("direct draw locker"),
	fFrameBuffer(NULL),
	fColorSpace(B_NO_COLOR_SPACE),
	fRects(NULL),
	fConnected(false),
	fDisabled(false)
{
	fDrawSem = create_sem(0, "FakeRenderer draw");
	fDrawThread = spawn_thread(_DirectDrawThread, "FakeRenderer direct draw", B_DISPLAY_PRIORITY, this);
	resume_thread(fDrawThread);

}

FakeRenderer::~FakeRenderer()
{
	// Wakeup the draw thread by murdering its favorite semaphore
	delete_sem(fDrawSem);

	status_t exit_value;
	wait_for_thread(fDrawThread, &exit_value);

	free(fRects);
}


void
FakeRenderer::SwapBuffers(bool VSync)
{
	if (VSync && GLView()->Window()) {
		// TODO: find a way to check VSync support is actually working...
		BScreen screen(GLView()->Window());
		screen.WaitForRetrace();
	}

	// Simulate rendering a new buffer: randomized buffer color ;-)
	fDrawColor = make_color(rand() % 0xFF, rand() % 0xFF, rand() % 0xFF);

	if (!fConnected || fDisabled) {
		GLView()->LockLooper();
		// TODO : refresh area
		GLView()->UnlockLooper();
		return;
	}

	// Direct mode: wake up drawing thread
	release_sem(fDrawSem);
}


void
FakeRenderer::Draw(BRect updateRect)
{
/*
	if (fBitmap && (!fDirectModeEnabled || (fInfo == NULL)))
		GLView()->DrawBitmap(fBitmap, updateRect, updateRect);
*/	
}


void
FakeRenderer::EnableDirectMode(bool enabled)
{
	fDisabled = ! enabled;
}


void
FakeRenderer::DirectConnected(direct_buffer_info *info)
{
	if (!fConnected && fDisabled)
		return;
		
	BAutolock lock(fDrawLocker);
	
	switch (info->buffer_state & B_DIRECT_MODE_MASK) {
	case B_DIRECT_START:
		fConnected = true;
		/* fall through */
	case B_DIRECT_MODIFY:
		fFrameBuffer = (uint8 *) info->bits;
		fBytesPerRow = info->bytes_per_row;
		fColorSpace = info->pixel_format;
		
		free(fRects);
		fRectsCount = info->clip_list_count;
		fRects = (clipping_rect *) malloc(fRectsCount * sizeof(clipping_rect));
		memcpy(fRects, info->clip_list, fRectsCount * sizeof(clipping_rect));

		fprintf(stderr, "fFrameBuffer = %p\n"
						"fBytesPerRow = %d\n"
						"fColorSpace  = %s\n", fFrameBuffer, fBytesPerRow, color_space_name(fColorSpace));
		for (int i = 0; i < fRectsCount; i++) {
			fprintf(stderr, "fRects[%d] = %d, %d to %d, %d\n", 
				i, fRects[i].left, fRects[i].top, fRects[i].right, fRects[i].bottom);
		}
		break;
	
	case B_DIRECT_STOP:
		fConnected = false;
		break;
	}

	fprintf(stderr, "fConnected = %s\n", fConnected ? "true" : "false");
}


// ----


int32
FakeRenderer::_DirectDrawThread(void *data)
{	
	FakeRenderer *me = (FakeRenderer *) data;
	return me->_DirectDrawThread();
}


int32
FakeRenderer::_DirectDrawThread(void)
{
	// Let's wait forever/until semaphore death next redraw 
	while (acquire_sem(fDrawSem) == B_OK) {

		BAutolock lock(fDrawLocker);

		int i;
		int32 x, y;

		switch(fColorSpace) {
		case B_RGB32:
		case B_RGBA32:
			for (i = 0; i < fRectsCount; i++) {
				for (y = fRects[i].top; y <= fRects[i].bottom; y++) {
					uint8 * p = fFrameBuffer + ( y * fBytesPerRow ) + fRects[i].left * 4;
					for (x = fRects[i].left; x <= fRects[i].right; x++) {
						*p++ = fDrawColor.blue;
						*p++ = fDrawColor.green;
						*p++ = fDrawColor.red;
						*p++ = fDrawColor.alpha;
					}
				}
			}
			break;
	
		case B_RGB24:
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		case B_RGB24_BIG:
			/* fill this in with the color-space conversion
			 * code of your choosing
			 */
			break;

		case B_RGB16:
		case B_RGB15:
		case B_RGBA15:
		case B_RGB16_BIG:
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
			/* same here */
			break;

		case B_CMAP8:
			/* same here */
			break;

		default:
			/* unsupported mode */
			break;
		}
	
	} // while draw

	return 0;
}



