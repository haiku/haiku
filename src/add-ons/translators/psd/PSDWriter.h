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

class PSDWriter {
public:
					PSDWriter(BPositionIO *stream);
					~PSDWriter();

	status_t		EncodeFromRGBA(BPositionIO *target, uint8 *buff,
						int32 layers, int32 width, int32 height);

private:
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

	BPositionIO 	*fStream;
};


#endif	/* PSD_WRITER_H */
