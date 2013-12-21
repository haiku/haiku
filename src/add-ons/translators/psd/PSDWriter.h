/*
 * Copyright 2013, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#ifndef PSD_WRITER_H
#define PSD_WRITER_H

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

#include "PSDLoader.h"
#include "DataArray.h"

class PSDWriter {
public:
					PSDWriter(BPositionIO *stream);
					~PSDWriter();

	bool			IsReady(void);
	void			SetCompression(int16 compression);
	void			SetVersion(int16 version);
	status_t		Encode(BPositionIO *target);

private:
	void			_WriteInt64ToStream(BPositionIO *stream, int64);
	void			_WriteUInt64ToStream(BPositionIO *stream, uint64);
	void			_WriteInt32ToStream(BPositionIO *stream, int32);
	void			_WriteUInt32ToStream(BPositionIO *stream, uint32);
	void			_WriteInt16ToStream(BPositionIO *stream, int16);
	void			_WriteUInt16ToStream(BPositionIO *stream, uint16);
	void			_WriteInt8ToStream(BPositionIO *stream, int8);
	void			_WriteUInt8ToStream(BPositionIO *stream, uint8);
	void			_WriteFillBlockToStream(BPositionIO *stream,
						uint8 val, size_t count);
	void			_WriteBlockToStream(BPositionIO *stream,
						uint8 *block, size_t count);

	BDataArray*		_PackBits(uint8 *buff, int32  len);
	status_t		_LoadChannelsFromRGBA32(void);

	BPositionIO 	*fStream;	
	size_t			fBitmapDataPos;

	BDataArray 		psdChannel[4];
	BDataArray 		psdByteCounts[4];

	color_space		fColorSpace;
	int32			fInRowBytes;
	int16			fChannels;
	int16			fAlphaChannel;
	int32			fWidth;
	int32			fHeight;
	int16			fCompression;
	int16			fVersion;
	
	bool			fReady;
};


#endif	/* PSD_WRITER_H */
