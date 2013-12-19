/*
 * Copyright 2013, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */ 


#include <File.h>

#include "BaseTranslator.h"
#include "PSDWriter.h"
#include "DataArray.h"


PSDWriter::PSDWriter(BPositionIO *stream)
{	
	fStream = stream;
	fReady = false;

	TranslatorBitmap header;
	stream->Seek(0, SEEK_SET);
	status_t err = stream->Read(&header, sizeof(TranslatorBitmap));
	if (err < B_OK)
		return;
	else if (err < (int)sizeof(TranslatorBitmap))
		return;
		
	fBitmapDataPos = stream->Position();

	BRect bounds;
	bounds.left = B_BENDIAN_TO_HOST_FLOAT(header.bounds.left);
	bounds.top = B_BENDIAN_TO_HOST_FLOAT(header.bounds.top);
	bounds.right = B_BENDIAN_TO_HOST_FLOAT(header.bounds.right);
	bounds.bottom = B_BENDIAN_TO_HOST_FLOAT(header.bounds.bottom);
	fInRowBytes = B_BENDIAN_TO_HOST_INT32(header.rowBytes);
	fColorSpace = (color_space)B_BENDIAN_TO_HOST_INT32(header.colors);
	fChannels = fColorSpace == B_RGB32 ? 3 : 4;

	fWidth = bounds.IntegerWidth() + 1;
	fHeight = bounds.IntegerHeight() + 1;
	
	fCompression = PSD_COMPRESSED_RAW;

	fReady = true;
}


PSDWriter::~PSDWriter()
{
}


bool
PSDWriter::IsReady(void)
{
	return fReady;
}


void
PSDWriter::SetCompression(int16 compression)
{
	fCompression = compression;
}


status_t
PSDWriter::Encode(BPositionIO *target)
{
	if (!fReady)
		return B_NO_TRANSLATOR;

	int32 channelSize = fWidth * fHeight;		

	fStream->Seek(fBitmapDataPos, SEEK_SET);

	BDataArray psdChannel[4];
	
	for (int i = 0; i < channelSize; i++) {
		uint8 rgba[4];
		fStream->Read(rgba, sizeof(uint32));
		psdChannel[0].Append((uint8)rgba[2]); // Red channel
		psdChannel[1].Append((uint8)rgba[1]); // Green channel
		psdChannel[2].Append((uint8)rgba[0]); // Blue channel
		if (fChannels == 4)
			psdChannel[3].Append((uint8)rgba[3]); // Alpha channel
	}

	// PSD header
	BDataArray psdHeader(64);
	psdHeader << "8BPS"; // Signature
	psdHeader << (uint16)1; // Version = 1
	psdHeader.Repeat(0, 6); // Reserved
	psdHeader << fChannels; // Channels
	psdHeader << fHeight << fWidth; // Image size
	psdHeader << (int16)8; // Depth
	psdHeader << (int16)PSD_COLOR_MODE_RGB; // ColorMode

	// Color mode section
	BDataArray psdColorModeSection(16);
	psdColorModeSection << (uint32)0;

	// Image resource section
	BDataArray psdImageResourceSection(64);
	psdImageResourceSection << "8BIM"; // Block signature
	psdImageResourceSection << (uint16)1005;
	psdImageResourceSection << (uint16)0;
	psdImageResourceSection << (uint32)16;
	uint8 resBlock[16] = {0x00, 0x48, 0x00, 0x00,
		0x00, 0x01, 0x00, 0x01,
		0x00, 0x48, 0x00, 0x00,
		0x00, 0x01, 0x00, 0x01};
	psdImageResourceSection.Append(resBlock, 16);	
	// Current layer info
	psdImageResourceSection << "8BIM"; // Block signature
	psdImageResourceSection << (uint16)1024;
	psdImageResourceSection << (uint16)0;
	psdImageResourceSection << (uint32)2;
	psdImageResourceSection << (uint16)0; // Set current layer to 0

	// Layer & mask section
	BDataArray psdLayersSection;
	psdLayersSection << (uint16)1; // Layers count
	psdLayersSection << (uint32)0; // Layer rect
	psdLayersSection << (uint32)0;
	psdLayersSection << (uint32)fHeight;
	psdLayersSection << (uint32)fWidth;	
	psdLayersSection << (uint16)fChannels;
	
	for (int dataChannel = 0; dataChannel < 3; dataChannel++) {
		psdLayersSection << (int16)dataChannel; // Channel num
		psdLayersSection << (uint32)(psdChannel[dataChannel].Length() + 2);
	}
	if (fChannels == 4) {
		psdLayersSection << (int16)-1; // Alpha channel id (-1)
		psdLayersSection << (uint32)(psdChannel[4].Length() + 2);
	}
	psdLayersSection << "8BIM";
	psdLayersSection << "norm"; // Blend mode = norm
	psdLayersSection << (uint8)255; // Opacity
	psdLayersSection << (uint8)0; // Clipping
	psdLayersSection << (uint8)1; // Flags
	psdLayersSection << (uint8)0; // Flags
	psdLayersSection << (uint32)24; // Extra data length
	psdLayersSection << (uint32)0; // Mask info
	psdLayersSection << (uint32)0;

	psdLayersSection << (uint8)15; // Layer name length
	uint8 layerName[16] = {"Layer #1       "};
	psdLayersSection.Append(layerName, 15); // Layer name

	for (int dataChannel = 0; dataChannel < fChannels; dataChannel++) {
		psdLayersSection << fCompression; // Compression mode
		psdLayersSection.Append(psdChannel[dataChannel].Buffer(),
			psdChannel[dataChannel].Length()); // Layer image data
	}
		
	psdHeader.WriteToStream(target);
	
	psdColorModeSection.WriteToStream(target);
	
	_WriteUInt32ToStream(target, psdImageResourceSection.Length());
	psdImageResourceSection.WriteToStream(target);

	_WriteUInt32ToStream(target, psdLayersSection.Length() + 4);
	_WriteUInt32ToStream(target, psdLayersSection.Length());
	psdLayersSection.WriteToStream(target);

	// Merged layer
	_WriteUInt16ToStream(target, fCompression); // Compression mode
	for (int dataChannel = 0; dataChannel < fChannels; dataChannel++)
		psdChannel[dataChannel].WriteToStream(target);

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
