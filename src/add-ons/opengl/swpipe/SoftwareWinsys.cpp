/*
 * Copyright 2006-2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Artur Wyszynski, harakash@gmail.com
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */


#include "SoftwareWinsys.h"

#include <stdio.h>

#include "bitmap_wrapper.h"
extern "C" {
#include "state_tracker/st_api.h"
#include "state_tracker/sw_winsys.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"
}


#define TRACE_WINSYS
#ifdef TRACE_WINSYS
#	define TRACE(x...) printf("GalliumWinsys: " x)
#	define CALLED() TRACE("CALLED: %s\n", __PRETTY_FUNCTION__)
#else
#	define TRACE(x...)
#	define CALLED()
#endif
#define ERROR(x...) printf("GalliumWinsys: " x)


// Cast
static INLINE struct haiku_displaytarget*
cast_haiku_displaytarget(struct sw_displaytarget* target)
{
	return (struct haiku_displaytarget *)target;
}


static void
hook_winsys_destroy(struct sw_winsys* winsys)
{
	CALLED();
	FREE(winsys);
}


static boolean
hook_winsys_is_displaytarget_format_supported(struct sw_winsys* winsys,
	unsigned textureUsage, enum pipe_format format)
{
	CALLED();
	// TODO STUB
	return true;
}


static struct sw_displaytarget*
hook_winsys_displaytarget_create(struct sw_winsys* winsys,
	unsigned textureUsage, enum pipe_format format, unsigned width,
	unsigned height, unsigned alignment, unsigned* stride)
{
	CALLED();

	struct haiku_displaytarget* haikuDisplayTarget
		= CALLOC_STRUCT(haiku_displaytarget);

	if (!haikuDisplayTarget) {
		ERROR("%s: Couldn't allocate Haiku display target!\n",
			__func__);
		return NULL;
	}

	haikuDisplayTarget->format = format;
	haikuDisplayTarget->width = width;
	haikuDisplayTarget->height = height;

	//unsigned bitsPerPixel = util_format_get_blocksizebits(format);
	unsigned colorsPerPalette = util_format_get_blocksize(format);

	haikuDisplayTarget->stride = align(width * colorsPerPalette, alignment);
	haikuDisplayTarget->size = haikuDisplayTarget->stride * height;

	haikuDisplayTarget->data
		= align_malloc(haikuDisplayTarget->size, alignment);

	if (!haikuDisplayTarget->data) {
		ERROR("%s: Couldn't allocate Haiku display target data!\n",
			__func__);
		FREE(haikuDisplayTarget);
		return NULL;
	}

	*stride = haikuDisplayTarget->stride;

	// Cast to ghost sw_displaytarget type
	return (struct sw_displaytarget*)haikuDisplayTarget;
}


static void
hook_winsys_displaytarget_destroy(struct sw_winsys* winsys,
	struct sw_displaytarget* displayTarget)
{
	CALLED();

	struct haiku_displaytarget* haikuDisplayTarget
		= cast_haiku_displaytarget(displayTarget);

	if (!haikuDisplayTarget) {
		ERROR("%s: Attempt to destroy non-existant display target!\n",
			__func__);
		return;
	}
	
	if (haikuDisplayTarget->data != NULL)
		align_free(haikuDisplayTarget->data);

	FREE(haikuDisplayTarget);
}


static struct sw_displaytarget*
hook_winsys_displaytarget_from_handle(struct sw_winsys* winsys,
	const struct pipe_resource* templat, struct winsys_handle* whandle,
	unsigned* stride)
{
	CALLED();
	return NULL;
}


static boolean
hook_winsys_displaytarget_get_handle(struct sw_winsys* winsys,
	struct sw_displaytarget* displayTarget, struct winsys_handle* whandle)
{
	CALLED();
	return FALSE;
}


static void*
hook_winsys_displaytarget_map(struct sw_winsys* winsys,
	struct sw_displaytarget* displayTarget, unsigned flags)
{
	CALLED();
	struct haiku_displaytarget* haikuDisplayTarget
		= cast_haiku_displaytarget(displayTarget);

	return haikuDisplayTarget->data;
}


static void
hook_winsys_displaytarget_unmap(struct sw_winsys* winsys,
	struct sw_displaytarget* disptarget)
{
	return;
}


static void
hook_winsys_displaytarget_display(struct sw_winsys* winsys,
	struct sw_displaytarget* displayTarget, void* contextPrivate)
{
	CALLED();

	if (!contextPrivate) {
		ERROR("%s: Passed invalid private pointer!\n", __func__);
		return;
	}

	Bitmap* bitmap = (Bitmap*)contextPrivate;

	struct haiku_displaytarget* haikuDisplayTarget
		= cast_haiku_displaytarget(displayTarget);

	copy_bitmap_bits(bitmap, haikuDisplayTarget->data,
		haikuDisplayTarget->size);

	return;
}


struct sw_winsys*
winsys_connect_hooks()
{
	CALLED();

	struct sw_winsys* winsys = CALLOC_STRUCT(sw_winsys);

	if (!winsys)
		return NULL;
	
	// Attach winsys hooks for Haiku
	winsys->destroy = hook_winsys_destroy;
	winsys->is_displaytarget_format_supported
		= hook_winsys_is_displaytarget_format_supported;
	winsys->displaytarget_create = hook_winsys_displaytarget_create;
	winsys->displaytarget_from_handle = hook_winsys_displaytarget_from_handle;
	winsys->displaytarget_get_handle = hook_winsys_displaytarget_get_handle;
	winsys->displaytarget_map = hook_winsys_displaytarget_map;
	winsys->displaytarget_unmap = hook_winsys_displaytarget_unmap;
	winsys->displaytarget_display = hook_winsys_displaytarget_display;
	winsys->displaytarget_destroy = hook_winsys_displaytarget_destroy;

	return winsys;
}
