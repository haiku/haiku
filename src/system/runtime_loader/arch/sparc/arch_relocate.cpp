/*
 * Copyright 2012-2018, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar R. Adema <ithamar@upgrade-android.com>
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "runtime_loader_private.h"

#include <runtime_loader.h>

//#define TRACE_RLD
#ifdef TRACE_RLD
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

status_t
arch_relocate_image(image_t *rootImage, image_t *image,
	SymbolLookupCache* cache)
{
	debugger("arch_relocate_image: Not Yet Implemented!");
	return B_OK;
}
