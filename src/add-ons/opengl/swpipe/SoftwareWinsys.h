/*
 * Copyright 2006-2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */
#ifndef SOFTWAREWINSYS_H
#define SOFTWAREWINSYS_H


extern "C" {
#include "pipe/p_defines.h"
#include "state_tracker/st_api.h"
#include "state_tracker/sw_winsys.h"
}


struct haiku_displaytarget
{
	enum pipe_format format;
	unsigned width;
	unsigned height;
	unsigned stride;

	unsigned size;

	void* data;
};


struct sw_winsys* winsys_connect_hooks();


#endif