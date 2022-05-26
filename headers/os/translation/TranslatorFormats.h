/*
 * Copyright 2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _TRANSLATOR_FORMATS_H
#define _TRANSLATOR_FORMATS_H


#include <GraphicsDefs.h>
#include <Rect.h>


// Standard fields for the extension message
extern char B_TRANSLATOR_EXT_HEADER_ONLY[];
extern char B_TRANSLATOR_EXT_DATA_ONLY[];
extern char B_TRANSLATOR_EXT_COMMENT[];
extern char B_TRANSLATOR_EXT_TIME[];
extern char B_TRANSLATOR_EXT_FRAME[];
extern char B_TRANSLATOR_EXT_BITMAP_RECT[];
extern char B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE[];
extern char B_TRANSLATOR_EXT_BITMAP_PALETTE[];
extern char B_TRANSLATOR_EXT_SOUND_CHANNEL[];
extern char B_TRANSLATOR_EXT_SOUND_MONO[];
extern char B_TRANSLATOR_EXT_SOUND_MARKER[];
extern char B_TRANSLATOR_EXT_SOUND_LOOP[];


// Standard formats

enum TranslatorGroups {
	B_TRANSLATOR_BITMAP		= 'bits',
	B_TRANSLATOR_PICTURE	= 'pict',
	B_TRANSLATOR_SOUND		= 'nois',
	B_TRANSLATOR_TEXT		= 'TEXT',
	B_TRANSLATOR_MIDI		= 'midi',
	B_TRANSLATOR_MEDIA		= 'mhi!',
	B_TRANSLATOR_NONE		= 'none',
	B_TRANSLATOR_ANY_TYPE	= 0
};


enum {
	// Bitmap formats
	B_GIF_FORMAT			= 'GIF ',
	B_JPEG_FORMAT			= 'JPEG',
	B_PNG_FORMAT			= 'PNG ',
	B_PPM_FORMAT			= 'PPM ',
	B_TGA_FORMAT			= 'TGA ',
	B_BMP_FORMAT			= 'BMP ',
	B_TIFF_FORMAT			= 'TIFF',
	B_WEBP_FORMAT			= 'WebP',

	// Picture formats
	B_DXF_FORMAT			= 'DXF ',
	B_EPS_FORMAT			= 'EPS ',
	B_PICT_FORMAT			= 'PICT',

	// Sound formats
	B_WAV_FORMAT			= 'WAV ',
	B_AIFF_FORMAT			= 'AIFF',
	B_CD_FORMAT				= 'CD  ',
	B_AU_FORMAT				= 'AU  ',

	// Text formats
	B_STYLED_TEXT_FORMAT	= 'STXT'
};


// Bitmap format (always in big endian)
struct TranslatorBitmap {
	uint32		magic;		// B_TRANSLATOR_BITMAP
	BRect		bounds;
	uint32		rowBytes;
	color_space	colors;
	uint32		dataSize;

	// actual data follows
};


// Sound format (always in big endian)
struct TranslatorSound {
	uint32		magic;		// B_TRANSLATOR_SOUND
	uint32		channels;
	float		sampleFreq;
	uint32		numFrames;

	// actual data follows
};


// Text format (always in big endian)
struct TranslatorStyledTextRecordHeader {
	uint32		magic;
	uint32		header_size;
	uint32		data_size;
};

struct TranslatorStyledTextStreamHeader {
	enum {
		STREAM_HEADER_MAGIC = 'STXT'
	};

	TranslatorStyledTextRecordHeader header;
	int32		version;
};

struct TranslatorStyledTextTextHeader {
	enum {
		TEXT_HEADER_MAGIC = 'TEXT'
	};

	TranslatorStyledTextRecordHeader header;
	int32		charset;

	// actual data follows
};

struct TranslatorStyledTextStyleHeader {
	enum {
		STYLE_HEADER_MAGIC = 'STYL'
	};

	TranslatorStyledTextRecordHeader header;
	uint32		apply_offset;
	uint32		apply_length;

	// flattened style follows
};


#endif	// _TRANSLATOR_FORMATS_H
