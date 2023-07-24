/*
 * Copyright 2013, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#ifndef PSD_LOADER_H
#define PSD_LOADER_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <Translator.h>
#include <TranslatorFormats.h>
#include <TranslationDefs.h>
#include <GraphicsDefs.h>
#include <InterfaceDefs.h>
#include <String.h>
#include <DataIO.h>
#include <File.h>
#include <ByteOrder.h>
#include <List.h>

#define PSD_MAX_CHANNELS 16


enum psd_version {
	PSD_FILE = 1,
	PSB_FILE = 2
};


enum psd_compressed_type {
	PSD_COMPRESSED_RAW	= 0,
	PSD_COMPRESSED_RLE = 1
};


enum psd_color_mode {
	PSD_COLOR_MODE_BITS = 0,
	PSD_COLOR_MODE_GRAYSCALE = 1,
	PSD_COLOR_MODE_INDEXED = 2,
	PSD_COLOR_MODE_RGB = 3,
	PSD_COLOR_MODE_CMYK = 4,
	PSD_COLOR_MODE_MULTICHANNEL = 7,
	PSD_COLOR_MODE_DUOTONE = 8,
	PSD_COLOR_MODE_LAB = 9
};


enum psd_color_format {
	PSD_COLOR_FORMAT_UNSUPPORTED,
	PSD_COLOR_FORMAT_BITMAP,
	PSD_COLOR_FORMAT_RGB,
	PSD_COLOR_FORMAT_RGB_A,
	PSD_COLOR_FORMAT_GRAY,
	PSD_COLOR_FORMAT_GRAY_A,
	PSD_COLOR_FORMAT_MULTICHANNEL,
	PSD_COLOR_FORMAT_CMYK,
	PSD_COLOR_FORMAT_CMYK_A,
	PSD_COLOR_FORMAT_LAB,
	PSD_COLOR_FORMAT_LAB_A,
	PSD_COLOR_FORMAT_DUOTONE,
	PSD_COLOR_FORMAT_INDEXED
};


class PSDLoader {
public:
					PSDLoader(BPositionIO *stream);
					~PSDLoader();

	status_t		Decode(BPositionIO *target);
	bool			IsLoaded(void);
	bool			IsSupported(void);

	BString			ColorFormatName(void);

private:
	int64			_GetInt64FromStream(BPositionIO *in);
	int32			_GetInt32FromStream(BPositionIO *in);
	int16			_GetInt16FromStream(BPositionIO *in);
	uint8			_GetUInt8FromStream(BPositionIO *in);
	int8			_GetInt8FromStream(BPositionIO *in);
	void			_SkipStreamBlock(BPositionIO *in, size_t count);
	
	status_t		_ParseImageResources(void);

	psd_color_format _ColorFormat(void);

	BPositionIO 	*fStream;
	uint8			*fStreamBuffer;
	size_t			fStreamSize;
	size_t			fStreamPos;

	size_t			fColorModeDataSize;
	size_t			fColorModeDataPos;
	
	off_t			fImageResourceSectionSize;
	off_t			fImageResourceSectionPos;
	
	int32 			fSignature;
	int16 			fVersion;
	int16 			fChannels;
	int32 			fHeight;
	int32			fWidth;
	int16			fDepth;
	int16			fColorFormat;
	int16			fCompression;

	uint16			fTransparentIndex;

	bool			fLoaded;
};


#endif	/* PSD_LOADER_H */
