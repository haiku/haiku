/*
 * supported_codecs.h - XviD plugin for the Haiku Operating System
 *
 * Copyright (C) 2007 Stephan Aßmus <superstippi@gmx.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#ifndef SUPPORTED_CODECS_H
#define SUPPORTED_CODECS_H


#include <MediaDefs.h>


struct codec_table_entry {
	media_format_family		family;
	uint32					fourcc;
	const char*				prettyName;
};

extern const struct codec_table_entry gCodecTable[];
extern const int gSupportedCodecsCount;
extern media_format gXvidFormats[];


#endif // SUPPORTED_CODECS_H
