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
#ifndef MESARENDERER_H
#define MESARENDERER_H

#include "GLRenderer.h"

extern "C" {

#include "context.h"

}

class MesaRenderer : public BGLRenderer
{
public:
				MesaRenderer(BGLView *view, ulong bgl_options, BGLDispatcher *dispatcher);
	virtual				~MesaRenderer();

	virtual	void 		SwapBuffers(bool VSync = false);
	virtual	void		Draw(BRect updateRect);
	virtual status_t    CopyPixelsOut(BPoint source, BBitmap *dest);
	virtual status_t    CopyPixelsIn(BBitmap *source, BPoint dest);

private:
	BBitmap		*fBitmap;

	GLcontext 	*fContext;
	GLvisual	*fVisual;
	GLframebuffer 	*fFrameBuffer;

};

#endif	// MESARENDERER_H

