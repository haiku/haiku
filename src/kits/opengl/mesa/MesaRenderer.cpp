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

#include "MesaRenderer.h"

extern const char * color_space_name(color_space space);

MesaRenderer::MesaRenderer(BGLView *view, ulong bgl_options, BGLDispatcher *dispatcher)
	: BGLRenderer(view, bgl_options, dispatcher),
	fBitmap(NULL),
	fContext(NULL),
	fVisual(NULL),
	fFrameBuffer(NULL)
{

}

MesaRenderer::~MesaRenderer()
{
	_mesa_destroy_visual(fVisual);
	_mesa_destroy_framebuffer(fFrameBuffer);
	_mesa_destroy_context(fContext);
 
	delete fBitmap;
}


void
MesaRenderer::SwapBuffers(bool VSync = false)
{
	_mesa_notifySwapBuffers(fContext);

	if (fBitmap) {
		GLView()->LockLooper();
		GLView()->DrawBitmap(fBitmap);
		GLView()->UnlockLooper();
	}
}


void
MesaRenderer::Draw(BRect updateRect)
{
	if (fBitmap)
		GLView()->DrawBitmap(fBitmap, updateRect, updateRect);
}


status_t
MesaRenderer::CopyPixelsOut(BPoint location, BBitmap *bitmap)
{
	color_space scs = fBitmap->ColorSpace();
	color_space dcs = bitmap->ColorSpace();

	if (scs != dcs && (scs != B_RGBA32 || dcs != B_RGB32)) {
		fprintf(stderr, "CopyPixelsOut(): incompatible color space: %s != %s\n",
			color_space_name(scs),
			color_space_name(dcs));
		return B_BAD_TYPE;
	}
	
	BRect sr = fBitmap->Bounds();
	BRect dr = bitmap->Bounds();

	sr = sr & dr.OffsetBySelf(location);
	dr = sr.OffsetByCopy(-location.x, -location.y); 
	
	uint8 *ps = (uint8 *) fBitmap->Bits();
	uint8 *pd = (uint8 *) bitmap->Bits();
	uint32 *s, *d;
	uint32 y;
	for (y = (uint32) sr.top; y <= (uint32) sr.bottom; y++) {
		s = (uint32 *) (ps + y * fBitmap->BytesPerRow());
		s += (uint32) sr.left;
		
		d = (uint32 *) (pd + (y + (uint32) (dr.top - sr.top)) * bitmap->BytesPerRow());
		d += (uint32) dr.left;
		
		memcpy(d, s, dr.IntegerWidth() * 4);
	}
	return B_OK;
}


status_t
MesaRenderer::CopyPixelsIn(BBitmap *bitmap, BPoint location)
{
	color_space scs = bitmap->ColorSpace();
	color_space dcs = fBitmap->ColorSpace();

	if (scs != dcs && (dcs != B_RGBA32 || scs != B_RGB32)) {
		fprintf(stderr, "CopyPixelsIn(): incompatible color space: %s != %s\n",
			color_space_name(scs),
			color_space_name(dcs));
		return B_BAD_TYPE;
	}
	
	BRect sr = bitmap->Bounds();
	BRect dr = fBitmap->Bounds();

	sr = sr & dr.OffsetBySelf(location);
	dr = sr.OffsetByCopy(-location.x, -location.y); 
	
	uint8 *ps = (uint8 *) bitmap->Bits();
	uint8 *pd = (uint8 *) fBitmap->Bits();
	uint32 *s, *d;
	uint32 y;
	for (y = (uint32) sr.top; y <= (uint32) sr.bottom; y++) {
		s = (uint32 *) (ps + y * bitmap->BytesPerRow());
		s += (uint32) sr.left;
		
		d = (uint32 *) (pd + (y + (uint32) (dr.top - sr.top)) * fBitmap->BytesPerRow());
		d += (uint32) dr.left;
		
		memcpy(d, s, dr.IntegerWidth() * 4);
	}
	return B_OK;
}

