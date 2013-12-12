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
	PSD_COLOR_MODE_LAB = 9
};


enum psd_color_format {
	PSD_COLOR_FORMAT_UNSUPPORTED,
	PSD_COLOR_FORMAT_RGB_8,
	PSD_COLOR_FORMAT_RGB_A_8,
	PSD_COLOR_FORMAT_GRAY_8,
	PSD_COLOR_FORMAT_GRAY_A_8,
	PSD_COLOR_FORMAT_MULTICHANNEL_8,
	PSD_COLOR_FORMAT_CMYK_8,
	PSD_COLOR_FORMAT_CMYK_A_8,
	PSD_COLOR_FORMAT_LAB_8,
	PSD_COLOR_FORMAT_LAB_A_8
};


class PSDLoader {
public:
					PSDLoader(BPositionIO *stream);
					~PSDLoader();
		
	int 			Decode(BPositionIO *target);
	bool			IsLoaded(void);
	bool			IsSupported(void);
	
	BString			ColorFormatName(void);

private:
	int32			_GetInt32FromStream(BPositionIO *in);
	int16			_GetInt16FromStream(BPositionIO *in);
	uint8			_GetUInt8FromStream(BPositionIO *in);
	int8			_GetInt8FromStream(BPositionIO *in);
	void			_SkipStreamBlock(BPositionIO *in, size_t count);
	
	int32			_ColorFormat(void);

	BPositionIO 	*fStream;
	uint8			*fStreamBuffer;
	size_t			fStreamSize;
	size_t			fStreamPos;
	
	int32 			fSignature;
	int16 			fVersion;
	int16 			fChannels;
	int32 			fHeight;
	int32			fWidth;
	int16			fDepth;
	int16			fColorFormat;
	int16			fCompression;
	
	bool			fLoaded;
};


#endif	/* PSD_LOADER_H */
