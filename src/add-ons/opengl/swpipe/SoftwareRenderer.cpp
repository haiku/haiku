/*
 * Copyright 2006-2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, korli@users.berlios.de
 *		Philippe Houdoin, philippe.houdoin@free.fr
 *		Artur Wyszynski, harakash@gmail.com
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */


#include "SoftwareRenderer.h"

#include <Autolock.h>
#include <DirectWindowPrivate.h>
#include <GraphicsDefs.h>
#include <Screen.h>
#include <stdio.h>
#include <sys/time.h>
#include <new>


#define TRACE_SOFTWARE
#ifdef TRACE_SOFTWARE
#	define TRACE(x...) printf("SoftwareRenderer: " x)
#	define CALLED() TRACE("CALLED: %s\n", __PRETTY_FUNCTION__)
#else
#	define TRACE(x...)
#	define CALLED()
#endif
#define ERROR(x...)	printf("SoftwareRenderer: " x)


extern const char* color_space_name(color_space space);


extern "C" _EXPORT BGLRenderer*
instantiate_gl_renderer(BGLView *view, ulong opts, BGLDispatcher *dispatcher)
{
	return new SoftwareRenderer(view, opts, dispatcher);
}

SoftwareRenderer::SoftwareRenderer(BGLView *view, ulong options,
	BGLDispatcher* dispatcher)
	:
	BGLRenderer(view, options, dispatcher),
	fBitmap(NULL),
	fDirectModeEnabled(false),
	fInfo(NULL),
	fInfoLocker("info locker"),
	fOptions(options),
	fColorSpace(B_NO_COLOR_SPACE)
{
	CALLED();

	// Disable double buffer for the moment.
	options &= ~BGL_DOUBLE;

	// Initialize the "Haiku Software GL Pipe"
	time_t beg;
	time_t end;
	beg = time(NULL);
	fContextObj = new GalliumContext(options);
	end = time(NULL);
	TRACE("Haiku Software GL Pipe initialization time: %f.\n",
		difftime(end, beg));

	// Allocate a bitmap
	BRect b = view->Bounds();
	fColorSpace = BScreen(view->Window()).ColorSpace();
	TRACE("%s: Colorspace:\t%s\n", __func__, color_space_name(fColorSpace));

	fWidth = (GLint)b.IntegerWidth();
	fHeight = (GLint)b.IntegerHeight();
	fNewWidth = fWidth;
	fNewHeight = fHeight;

	_AllocateBitmap();

	// Initialize the first "Haiku Software GL Pipe" context
	beg = time(NULL);
	fContextID = fContextObj->CreateContext(fBitmap);
	end = time(NULL);

	if (fContextID < 0)
		ERROR("%s: There was an error creating the context!\n", __func__);
	else {
		TRACE("%s: Haiku Software GL Pipe context creation time: %f.\n",
			__func__, difftime(end, beg));
	}

	if (!fContextObj->GetCurrentContext())
		LockGL();
}


SoftwareRenderer::~SoftwareRenderer()
{
	CALLED();

	if (fContextObj)
		delete fContextObj;
	if (fBitmap)
		delete fBitmap;
}


void
SoftwareRenderer::LockGL()
{
//	CALLED();
	BGLRenderer::LockGL();

	color_space cs = BScreen(GLView()->Window()).ColorSpace();

	BAutolock lock(fInfoLocker);
	if (fDirectModeEnabled && fInfo != NULL) {
		fNewWidth = fInfo->window_bounds.right - fInfo->window_bounds.left;
			// + 1;
		fNewHeight = fInfo->window_bounds.bottom - fInfo->window_bounds.top;
			// + 1;
	}

	if (fBitmap && cs == fColorSpace && fNewWidth == fWidth
		&& fNewHeight == fHeight) {
		fContextObj->SetCurrentContext(fBitmap, fContextID);
		return;
	}

	fColorSpace = cs;
	fWidth = fNewWidth;
	fHeight = fNewHeight;

	_AllocateBitmap();
	fContextObj->SetCurrentContext(fBitmap, fContextID);
}


void
SoftwareRenderer::UnlockGL()
{
//	CALLED();
	if ((fOptions & BGL_DOUBLE) == 0) {
		SwapBuffers();
	}
	fContextObj->SetCurrentContext(NULL, fContextID);
	BGLRenderer::UnlockGL();
}


void
SoftwareRenderer::SwapBuffers(bool vsync)
{
//	CALLED();
	if (!fBitmap)
		return;

	BScreen screen(GLView()->Window());

	fContextObj->SwapBuffers(fContextID);

	BAutolock lock(fInfoLocker);

	if (!fDirectModeEnabled || fInfo == NULL) {
		if (GLView()->LockLooperWithTimeout(1000) == B_OK) {
			GLView()->DrawBitmap(fBitmap, B_ORIGIN);
			GLView()->UnlockLooper();
			if (vsync)
				screen.WaitForRetrace();
		}
		return;
	}

	// check the bitmap size still matches the size
	if (fInfo->window_bounds.bottom - fInfo->window_bounds.top
			!= fBitmap->Bounds().IntegerHeight()
			|| fInfo->window_bounds.right - fInfo->window_bounds.left
			!= fBitmap->Bounds().IntegerWidth()) {
		ERROR("%s: Bitmap size doesn't match size!\n", __func__);
		return;
	}
	uint8 bytesPerPixel = fInfo->bits_per_pixel / 8;
	uint32 bytesPerRow = fBitmap->BytesPerRow();
	for (uint32 i = 0; i < fInfo->clip_list_count; i++) {
		clipping_rect *clip = &fInfo->clip_list[i];
		int32 height = clip->bottom - clip->top + 1;
		int32 bytesWidth
			= (clip->right - clip->left + 1) * bytesPerPixel;
		bytesWidth -= bytesPerPixel;
		uint8 *p = (uint8 *)fInfo->bits + clip->top
			* fInfo->bytes_per_row + clip->left * bytesPerPixel;
		uint8 *b = (uint8 *)fBitmap->Bits()
			+ (clip->top - fInfo->window_bounds.top) * bytesPerRow
			+ (clip->left - fInfo->window_bounds.left) * bytesPerPixel;

		for (int y = 0; y < height - 1; y++) {
			memcpy(p, b, bytesWidth);
			p += fInfo->bytes_per_row;
			b += bytesPerRow;
		}
	}

	if (vsync)
		screen.WaitForRetrace();
}


void
SoftwareRenderer::Draw(BRect updateRect)
{
//	CALLED();
	if ((!fDirectModeEnabled || fInfo == NULL) && fBitmap)
		GLView()->DrawBitmap(fBitmap, updateRect, updateRect);
}


status_t
SoftwareRenderer::CopyPixelsOut(BPoint location, BBitmap *bitmap)
{
	CALLED();
	color_space scs = fBitmap->ColorSpace();
	color_space dcs = bitmap->ColorSpace();

	if (scs != dcs && (scs != B_RGBA32 || dcs != B_RGB32)) {
		ERROR("%s::CopyPixelsOut(): incompatible color space: %s != %s\n",
			__PRETTY_FUNCTION__, color_space_name(scs), color_space_name(dcs));
		return B_BAD_TYPE;
	}

	BRect sr = fBitmap->Bounds();
	BRect dr = bitmap->Bounds();

//	int32 w1 = sr.IntegerWidth();
//	int32 h1 = sr.IntegerHeight();
//	int32 w2 = dr.IntegerWidth();
//	int32 h2 = dr.IntegerHeight();

	sr = sr & dr.OffsetBySelf(location);
	dr = sr.OffsetByCopy(-location.x, -location.y);

	uint8 *ps = (uint8 *) fBitmap->Bits();
	uint8 *pd = (uint8 *) bitmap->Bits();
	uint32 *s, *d;
	uint32 y;
	for (y = (uint32) sr.top; y <= (uint32) sr.bottom; y++) {
		s = (uint32 *)(ps + y * fBitmap->BytesPerRow());
		s += (uint32) sr.left;

		d = (uint32 *)(pd + (y + (uint32)(dr.top - sr.top))
			* bitmap->BytesPerRow());
		d += (uint32) dr.left;
		memcpy(d, s, dr.IntegerWidth() * 4);
	}

	return B_OK;
}


status_t
SoftwareRenderer::CopyPixelsIn(BBitmap *bitmap, BPoint location)
{
	CALLED();

	color_space sourceCS = bitmap->ColorSpace();
	color_space destinationCS = fBitmap->ColorSpace();

	if (sourceCS != destinationCS
		&& (sourceCS != B_RGB32 || destinationCS != B_RGBA32)) {
		ERROR("%s::CopyPixelsIn(): incompatible color space: %s != %s\n",
			__PRETTY_FUNCTION__, color_space_name(sourceCS),
			color_space_name(destinationCS));
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
		s = (uint32 *)(ps + y * bitmap->BytesPerRow());
		s += (uint32) sr.left;

		d = (uint32 *)(pd + (y + (uint32)(dr.top - sr.top))
			* fBitmap->BytesPerRow());
		d += (uint32) dr.left;

		memcpy(d, s, dr.IntegerWidth() * 4);
	}

	return B_OK;
}


void
SoftwareRenderer::EnableDirectMode(bool enabled)
{
	fDirectModeEnabled = enabled;
}


void
SoftwareRenderer::DirectConnected(direct_buffer_info *info)
{
//	CALLED();
	BAutolock lock(fInfoLocker);
	if (info) {
		if (!fInfo) {
			fInfo = (direct_buffer_info *)calloc(1,
				DIRECT_BUFFER_INFO_AREA_SIZE);
		}
		memcpy(fInfo, info, DIRECT_BUFFER_INFO_AREA_SIZE);
	} else if (fInfo) {
		free(fInfo);
		fInfo = NULL;
	}
}


void
SoftwareRenderer::FrameResized(float width, float height)
{
//	CALLED();
	BAutolock lock(fInfoLocker);
	fNewWidth = (GLuint)width;
	fNewHeight = (GLuint)height;
}


void
SoftwareRenderer::_AllocateBitmap()
{
//	CALLED();

	// allocate new size of back buffer bitmap
	BAutolock lock(fInfoLocker);
	delete fBitmap;
	fBitmap = NULL;
	if (fWidth < 1 || fHeight < 1) {
		TRACE("%s: Can't allocate bitmap of %dx%d\n", __func__,
			fWidth, fHeight);
		return;
	}
	BRect rect(0.0, 0.0, fWidth, fHeight);
	fBitmap = new (std::nothrow) BBitmap(rect, fColorSpace);
	if (fBitmap == NULL) {
		TRACE("%s: Can't create bitmap!\n", __func__);
		return;
	}
#if 0
	// debug..
	void *data = fBitmap->Bits();
	memset(data, 0xcc, fBitmap->BitsLength());
#endif
}
