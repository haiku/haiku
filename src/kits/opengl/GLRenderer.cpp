/*
 * Copyright 2006-2008, Philippe Houdoin. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "GLRenderer.h"

#include "GLDispatcher.h"


BGLRenderer::BGLRenderer(BGLView* view, ulong glOptions,
	BGLDispatcher* dispatcher)
	:
	fRefCount(1),
	fView(view),
	fOptions(glOptions),
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
BGLRenderer::SwapBuffers(bool VSync)
{
}


void
BGLRenderer::Draw(BRect updateRect)
{
}


status_t
BGLRenderer::CopyPixelsOut(BPoint source, BBitmap* dest)
{
	return B_ERROR;
}


status_t
BGLRenderer::CopyPixelsIn(BBitmap* source, BPoint dest)
{
	return B_ERROR;
}


void
BGLRenderer::FrameResized(float width, float height)
{
}


void
BGLRenderer::DirectConnected(direct_buffer_info* info)
{
}


void
BGLRenderer::EnableDirectMode(bool enabled)
{
}


status_t BGLRenderer::_Reserved_Renderer_0(int32 n, void* p) { return B_ERROR; }
status_t BGLRenderer::_Reserved_Renderer_1(int32 n, void* p) { return B_ERROR; }
status_t BGLRenderer::_Reserved_Renderer_2(int32 n, void* p) { return B_ERROR; }
status_t BGLRenderer::_Reserved_Renderer_3(int32 n, void* p) { return B_ERROR; }
status_t BGLRenderer::_Reserved_Renderer_4(int32 n, void* p) { return B_ERROR; }
