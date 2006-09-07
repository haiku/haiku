/*
 * Copyright 2006, Philippe Houdoin. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "GLDispatcher.h"
#include "GLRenderer.h"

BGLRenderer::BGLRenderer(BGLView *view, ulong bgl_options, BGLDispatcher *dispatcher)
	: fRefCount(1),
	fOptions(bgl_options),
	fDispatcher(dispatcher)
{

}


BGLRenderer::~BGLRenderer()
{
	delete fDispatcher;
}


void 
BGLRenderer::Acquire()
{
	atomic_add(&fRefCount, 1);
}


void
BGLRenderer::Release()
{
	if (atomic_add(&fRefCount, -1) < 1)
		delete this;
}


void 
BGLRenderer::LockGL()
{
}


void 
BGLRenderer::UnlockGL()
{
}


void
BGLRenderer::SwapBuffers(bool VSync = false)
{
}


void
BGLRenderer::Draw(BRect updateRect)
{
}


status_t
BGLRenderer::CopyPixelsOut(BPoint source, BBitmap *dest)
{
	return B_OK;
}


status_t
BGLRenderer::CopyPixelsIn(BBitmap *source, BPoint dest)
{
	return B_OK;
}


void
BGLRenderer::AttachedToWindow()
{
}


void
BGLRenderer::AllAttached()
{
}


void
BGLRenderer::DetachedFromWindow()
{
}


void
BGLRenderer::AllDetached()
{
}


void
BGLRenderer::FrameResized(float width, float height)
{
}


void 
BGLRenderer::DirectConnected(direct_buffer_info *info)
{
}


void 
BGLRenderer::EnableDirectMode(bool enabled)
{
}


status_t BGLRenderer::_Reserved_Renderer_0(int32 n, void *p) { return B_ERROR; }
status_t BGLRenderer::_Reserved_Renderer_1(int32 n, void *p) { return B_ERROR; }
status_t BGLRenderer::_Reserved_Renderer_2(int32 n, void *p) { return B_ERROR; }
status_t BGLRenderer::_Reserved_Renderer_3(int32 n, void *p) { return B_ERROR; }
status_t BGLRenderer::_Reserved_Renderer_4(int32 n, void *p) { return B_ERROR; }

