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


void _init_global_fonts_();
extern "C" status_t _fini_interface_kit_();


namespace BPrivate {

bool		get_mode_parameter(uint32 mode, int32& width, int32& height,
				uint32& colorSpace);
int32		get_bytes_per_row(color_space colorSpace, int32 width);

void		get_workspaces_layout(uint32* _columns, uint32* _rows);
void		set_workspaces_layout(uint32 columns, uint32 rows);

bool		get_control_look(BString& path);
status_t	set_control_look(const BString& path);

}	// namespace BPrivate


#endif	// _INTERFACE_PRIVATE_H
