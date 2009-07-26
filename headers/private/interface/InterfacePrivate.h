/*
 * Copyright 2007-2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini <stefano.ceccherini@gmail.com>
 */
#ifndef _INTERFACE_PRIVATE_H
#define _INTERFACE_PRIVATE_H


#include <GraphicsDefs.h>
#include <SupportDefs.h>


namespace BPrivate {

bool		get_mode_parameter(uint32 mode, int32& width, int32& height,
				uint32& colorSpace);
int32		get_bytes_per_row(color_space colorSpace, int32 width);

}	// namespace BPrivate


#endif	// _INTERFACE_PRIVATE_H
