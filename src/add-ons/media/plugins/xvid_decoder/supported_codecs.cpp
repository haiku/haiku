/*
 * supported_codecs.cpp - XviD plugin for the Haiku Operating System
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
#include <MediaDefs.h>

#include "supported_codecs.h"

const struct codec_table_entry gCodecTable[] = {
//	{B_QUICKTIME_FORMAT_FAMILY,	'3IV1', "3ivx Delta v1"},
//	{B_QUICKTIME_FORMAT_FAMILY,	'3IV2', "3ivx v2"},				// [seems to works but corrupts memory]
//	{B_QUICKTIME_FORMAT_FAMILY,	'3IVX', "3ivx"},

	{B_QUICKTIME_FORMAT_FAMILY,	'DIVX', "MPEG4 Video"},			// OpenDivX
	{B_QUICKTIME_FORMAT_FAMILY,	'divx', "MPEG4 Video"},			// OpenDivX [confirmed]
	{B_QUICKTIME_FORMAT_FAMILY,	'mp4v', "MPEG4 Video"},			// OpenDivX
	{B_QUICKTIME_FORMAT_FAMILY,	'XVID', "XVID"},				// OpenDivX [confirmed]
	
//	{B_AVI_FORMAT_FAMILY,		'3IV1', "3ivx Delta v1"},
//	{B_AVI_FORMAT_FAMILY,		'3IV2', "3ivx v2"},				// [seems to works but corrupts memory]
//	{B_AVI_FORMAT_FAMILY,		'3IVX', "3ivx"},

	{B_AVI_FORMAT_FAMILY,		'DIVX', "MPEG4 Video"},			// OpenDivX
	{B_AVI_FORMAT_FAMILY,		'divx', "MPEG4 Video"},			// OpenDivX
	{B_AVI_FORMAT_FAMILY,		'XVID', "XVID"},				// XVID
	{B_AVI_FORMAT_FAMILY,		'xvid', "XVID"},				// XVID
	{B_AVI_FORMAT_FAMILY,		'DX50', "DivX 5"},				// DivX 5.0 [confirmed]
	{B_AVI_FORMAT_FAMILY,		'dx50', "DivX 5"},				// DivX 5.0

	{B_AVI_FORMAT_FAMILY,		'MP4S', "MPEG4 Video"},			// from ffmpeg
	{B_AVI_FORMAT_FAMILY,		'M2S2', "MPEG4 Video"},			// from ffmpeg
	{B_AVI_FORMAT_FAMILY,		'M4S2', "MPEG4 Video"},			// from ffmpeg

	{B_AVI_FORMAT_FAMILY,		'DIV1', "MPEG4 Video"},			// from mplayer
	{B_AVI_FORMAT_FAMILY,		'BLZ0', "MPEG4 Video"},			// from mplayer
	{B_AVI_FORMAT_FAMILY,		'mp4v', "MPEG4 Video"},			// from mplayer -- OpenDivx
	{B_AVI_FORMAT_FAMILY,		'UMP4', "MPEG4 Video"},			// from mplayer

	{B_AVI_FORMAT_FAMILY,		'FMP4', "FFMpeg MPEG4 Video"},	// ffmpeg's own version


	{B_AVI_FORMAT_FAMILY,		'\004\0\0\0', "MPEG4 Video"},	// some broken avi use this
//	{B_AVI_FORMAT_FAMILY,		'DIV3', "DivX ;-)"},			// default signature when
																// using MSMPEG4
//	{B_AVI_FORMAT_FAMILY,		'div3', "DivX ;-)"},
//	{B_AVI_FORMAT_FAMILY,		'DIV4', "DivX ;-)"},
//	{B_AVI_FORMAT_FAMILY,		'div4', "DivX ;-)"},
//	{B_AVI_FORMAT_FAMILY,		'DIV2', "DivX ;-)"},
//	{B_AVI_FORMAT_FAMILY,		'DIV5', "DivX ;-)"},
//	{B_AVI_FORMAT_FAMILY,		'DIV6', "DivX ;-)"},
//	{B_AVI_FORMAT_FAMILY,		'COL0', "DivX ;-)"},
//	{B_AVI_FORMAT_FAMILY,		'COL1', "DivX ;-)"},
	
//	{B_AVI_FORMAT_FAMILY,		'MP41', "Microsoft MPEG4 v1"},	// microsoft mpeg4 v1
//	{B_AVI_FORMAT_FAMILY,		'MP42', "Microsoft MPEG4 v2"},	// seems to be broken
//	{B_AVI_FORMAT_FAMILY,		'MP43', "Microsoft MPEG4 v3"},	// microsoft mpeg4 v3 [confirmed unsupported]
//	{B_AVI_FORMAT_FAMILY,		'MPG3', "Microsoft MPEG4"},
//	{B_AVI_FORMAT_FAMILY,		'MPG4', "Microsoft MPEG4"},
//	{B_AVI_FORMAT_FAMILY,		'AP41', "Angel Potion MPEG4"},	// AngelPotion 1
};

const int gSupportedCodecsCount = sizeof(gCodecTable) / sizeof(codec_table_entry);

media_format gXvidFormats[gSupportedCodecsCount];

