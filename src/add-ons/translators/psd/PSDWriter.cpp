/*
 * Copyright 2013, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */ 


#include <File.h>

#include "BaseTranslator.h"
#include "PSDWriter.h"


PSDWriter::PSDWriter(BPositionIO *stream)
{	
	fStream = stream;
}


PSDWriter::~PSDWriter()
{
}


status_t
PSDWriter::EncodeFromRGBA(BPositionIO *target, uint8 *buff,
	int32 layers, int32 width, int32 height)
{
	int32 channelSize = width * height;

	_WriteUInt32ToStream(target, 0x38425053); // 8BPS
	_WriteUInt16ToStream(target, 1); // Version = 1
	_WriteFillBlockToStream(target, 0, 6); // reserved
	_WriteInt16ToStream(target, layers); // Channels
	_WriteInt32ToStream(target, height); // Height
	_WriteInt32ToStream(target, width); // Width
	_WriteInt16ToStream(target, 8); // Depth = 8
	_WriteInt16ToStream(target, PSD_COLOR_MODE_RGB); // ColorMode
	_WriteUInt32ToStream(target, 0); // ColorModeBlockSize = 0

	size_t sizePos = target->Position();
	_WriteUInt32ToStream(target, 0); // ImageResourceBlockSize = 0 for now
	_WriteUInt32ToStream(target, 0x3842494D); // 8BIM
	_WriteUInt16ToStream(target, 1005);
	_WriteUInt16ToStream(target, 0);
	_WriteUInt32ToStream(target, 16);
	uint8 resBlock[16] = {0x00, 0x48, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01,
		0x00, 0x48, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01};
	_WriteBlockToStream(target, resBlock, 16);

	// current layer info
	_WriteUInt32ToStream(target, 0x3842494D); // 8BIM
	_WriteUInt16ToStream(target, 1024);
	_WriteUInt16ToStream(target, 0);
	_WriteUInt32ToStream(target, 2);
	_WriteUInt16ToStream(target, 0); // Set current layer to 0

	int32 resBlockSize = target->Position() - sizePos;
	size_t lastPos = target->Position();
	target->Seek(sizePos, SEEK_SET);
	_WriteUInt32ToStream(target, resBlockSize - sizeof(uint32));
	target->Seek(lastPos, SEEK_SET);

	sizePos = target->Position();
	_WriteUInt32ToStream(target, 0); // Layer & mask block size = 0
	_WriteUInt32ToStream(target, 0); // Layer info block size = 0

	_WriteUInt16ToStream(target, 1); // Layers count

	_WriteUInt32ToStream(target, 0); // Layer rect
	_WriteUInt32ToStream(target, 0);
	_WriteUInt32ToStream(target, height);
	_WriteUInt32ToStream(target, width);

	_WriteInt16ToStream(target, layers); // Layer Channels

	for (int channels = 0; channels < 3; channels++) {
		_WriteInt16ToStream(target, channels); // Channel num
		_WriteUInt32ToStream(target, channelSize + 2); // Channel size
	}

	if (layers == 4) {
		_WriteInt16ToStream(target, -1);
		_WriteUInt32ToStream(target, channelSize + 2); // Alpha channel size
	}

	_WriteUInt32ToStream(target, 0x3842494D); // 8BIM

	uint8 blendModeKey[4] = {'n','o','r','m'};
	_WriteBlockToStream(target, blendModeKey, 4);  // Blend mode norm

	_WriteUInt8ToStream(target, 255); // Alpha

	_WriteUInt8ToStream(target, 0); // Clipping
	_WriteUInt8ToStream(target, 1); // Flags
	_WriteUInt8ToStream(target, 0); // Flags

	_WriteUInt32ToStream(target, 24); // Extra data length
	_WriteUInt32ToStream(target, 0); // Mask info
	_WriteUInt32ToStream(target, 0);

	_WriteUInt8ToStream(target, 15); // Layer name length
	uint8 layerName[16] = {"Layer #1       "};
	_WriteBlockToStream(target, layerName, 15); // Layer name

	for (int dataChannel = 0; dataChannel < layers; dataChannel++) {
		_WriteInt16ToStream(target, PSD_COMPRESSED_RAW); // Compression mode
		_WriteBlockToStream(target, buff + dataChannel * channelSize,
			channelSize); // Layer image data
	}

	uint32 layerBlockSize = target->Position() - sizePos;

/*	if (layerBlockSize % 2 != 0) {
		_WriteUInt8ToStream(target, 0);
		layerBlockSize++;
	}*/

	lastPos = target->Position();
	target->Seek(sizePos, SEEK_SET);
	_WriteUInt32ToStream(target, layerBlockSize - 4);
	_WriteUInt32ToStream(target, layerBlockSize - 8);
	target->Seek(lastPos, SEEK_SET);

	_WriteUInt16ToStream(target, PSD_COMPRESSED_RAW); // Compression mode
	_WriteBlockToStream(target, buff,  channelSize * layers);

	return B_OK;
}


void
PSDWriter::_WriteInt32ToStream(BPositionIO *stream, int32 val)
{
	val = B_HOST_TO_BENDIAN_INT32(val);
	stream->Write(&val, sizeof(int32));
}


void
PSDWriter::_WriteUInt32ToStream(BPositionIO *stream, uint32 val)
{
	val = B_HOST_TO_BENDIAN_INT32(val);
	stream->Write(&val, sizeof(uint32));
}


void
PSDWriter::_WriteInt16ToStream(BPositionIO *stream, int16 val)
{
	val = B_HOST_TO_BENDIAN_INT16(val);
	stream->Write(&val, sizeof(int16));
}


void
PSDWriter::_WriteUInt16ToStream(BPositionIO *stream, uint16 val)
{
	val = B_HOST_TO_BENDIAN_INT16(val);
	stream->Write(&val, sizeof(int16));
}


void
PSDWriter::_WriteInt8ToStream(BPositionIO *stream, int8 val)
{
	stream->Write(&val, sizeof(int8));
}


void
PSDWriter::_WriteUInt8ToStream(BPositionIO *stream, uint8 val)
{
	stream->Write(&val, sizeof(uint8));
}


void
PSDWriter::_WriteFillBlockToStream(BPositionIO *stream,
	uint8 val, size_t count)
{
	for (size_t i = 0; i < count; i++)
		stream->Write(&val, sizeof(uint8));
}


void
PSDWriter::_WriteBlockToStream(BPositionIO *stream,
	uint8 *val, size_t count)
{
	stream->Write(val, count);
}
