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
#include <DataIO.h>
#include <File.h>
#include <ByteOrder.h>
#include <List.h>

#define PSD_COMPRESSED_RAW	0
#define PSD_COMPRESSED_RLE	1

class PSDLoader {
public:
					PSDLoader(BPositionIO *stream);
					~PSDLoader();
		
	int 			Decode(BPositionIO *target);
	bool			IsLoaded(void);
	bool			IsSupported(void);

private:
	int32			GetInt32FromStream(BPositionIO *in);
	int16			GetInt16FromStream(BPositionIO *in);
	uint8			GetUInt8FromStream(BPositionIO *in);
	int8			GetInt8FromStream(BPositionIO *in);
	void			SkipStreamBlock(BPositionIO *in, size_t count);

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
