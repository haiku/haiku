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
	fAlphaChannel = -1;
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
	
	switch (fColorSpace) {
		case B_GRAY8:
		case B_CMAP8:
			fChannels = 1;
			break;
		case B_RGBA32:
			fChannels = 4;
			fAlphaChannel = 3;
			break;
		case B_RGB32:
			fChannels = 3;
			break;
		default:
			return;
	};

	fWidth = bounds.IntegerWidth() + 1;
	fHeight = bounds.IntegerHeight() + 1;
	
	fVersion = PSD_FILE;
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


void
PSDWriter::SetVersion(int16 ver)
{
	fVersion = ver;
}


status_t
PSDWriter::Encode(BPositionIO *target)
{
	if (!fReady)
		return B_NO_TRANSLATOR;

	status_t status = _LoadChannelsFromRGBA32();
	if (status != B_OK)
		return status;

	// PSD header
	BDataArray psdHeader(64);
	psdHeader << "8BPS"; // Signature
	psdHeader << (uint16)fVersion; // Version
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
	
	for (int channelIdx = 0; channelIdx < fChannels; channelIdx++) {
		if (channelIdx == 3)
			psdLayersSection << (int16)-1; // Alpha channel id (-1)
		else
			psdLayersSection << (int16)channelIdx; // Channel num
			
		if (fCompression == PSD_COMPRESSED_RAW) {
			if (fVersion == PSD_FILE) {
					psdLayersSection << (uint32)(psdChannel[channelIdx].Length()
						+ sizeof(int16));
			} else {
					psdLayersSection << (uint64)(psdChannel[channelIdx].Length()
						+ sizeof(int16));
			}
		} else {
			if (fVersion == PSD_FILE) {
				psdLayersSection << (uint32)(psdChannel[channelIdx].Length()
					+ psdByteCounts[channelIdx].Length() + sizeof(int16));
			} else {
				psdLayersSection << (uint64)(psdChannel[channelIdx].Length()
					+ psdByteCounts[channelIdx].Length() + sizeof(int16));
			}
		}
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

	if (fCompression == PSD_COMPRESSED_RAW) {
		for (int channelIdx = 0; channelIdx < fChannels; channelIdx++) {
			psdLayersSection << fCompression; // Compression mode
			psdLayersSection.Append(psdChannel[channelIdx]); // Channel data
		}
	} else {	
		for (int channelIdx = 0; channelIdx < fChannels; channelIdx++) {
			psdLayersSection << fCompression; // Compression mode
			psdLayersSection.Append(psdByteCounts[channelIdx]); // Bytes count
			psdLayersSection.Append(psdChannel[channelIdx]); // Channel data
		}
	}

	if (fCompression == PSD_COMPRESSED_RLE
		&& psdLayersSection.Length() % 2 != 0) {
		psdLayersSection << (uint8)0;
	}

	psdHeader.WriteToStream(target);
	
	psdColorModeSection.WriteToStream(target);
	
	_WriteUInt32ToStream(target, psdImageResourceSection.Length());
	psdImageResourceSection.WriteToStream(target);

	if (fVersion == PSD_FILE) {
		_WriteUInt32ToStream(target, psdLayersSection.Length() + sizeof(int32));
		_WriteUInt32ToStream(target, psdLayersSection.Length());
	} else {
		_WriteUInt64ToStream(target, psdLayersSection.Length() + sizeof(int64));
		_WriteUInt64ToStream(target, psdLayersSection.Length());		
	}
	psdLayersSection.WriteToStream(target);

	// Merged layer
	_WriteUInt16ToStream(target, fCompression); // Compression mode
	
	if (fCompression == PSD_COMPRESSED_RLE) {
		for (int channelIdx = 0; channelIdx < fChannels; channelIdx++)
			psdByteCounts[channelIdx].WriteToStream(target);
	}

	for (int channelIdx = 0; channelIdx < fChannels; channelIdx++)
		psdChannel[channelIdx].WriteToStream(target);

	return B_OK;
}


BDataArray*
PSDWriter::_PackBits(uint8 *buff, int32  len)
{
	BDataArray *packedBits = new BDataArray();

	int32  count = len;  
	len = 0;

	while (count > 0) {
		int i;
		for (i = 0; (i < 128) && (buff[0] == buff[i]) && (count - i > 0); i++);
		if (i < 2) {
			for (i = 0; i < 128; i++) {
				bool b1 = buff[i] != buff[i + 1];
				bool b3 = buff[i] != buff[i + 2];
				bool b2 = count - (i + 2) < 1;
				if (count - (i + 1) <= 0)
					break;
				if (!(b1 || b2 || b3))
					break;
			}

			if (count == 1)
				i = 1;

			if (i > 0) {
				packedBits->Append((uint8)(i - 1));
				for (int j = 0; j < i; j++)
					packedBits->Append((uint8)buff[j]);
				buff += i;
				count -= i;
				len += (i + 1);
			}
		} else {
			packedBits->Append((uint8)(-(i - 1)));
			packedBits->Append((uint8)(*buff));
			buff += i;
			count -= i;
			len += 2;
		}
	}
	return packedBits;
}


status_t
PSDWriter::_LoadChannelsFromRGBA32(void)
{
	if (fColorSpace != B_RGB32 && fColorSpace != B_RGBA32)
		return B_NO_TRANSLATOR;

	int32 channelSize = fWidth * fHeight;

	fStream->Seek(fBitmapDataPos, SEEK_SET);

	if (fCompression == PSD_COMPRESSED_RAW) {
		for (int i = 0; i < channelSize; i++) {
			uint8 rgba[4];
			fStream->Read(rgba, sizeof(uint32));
			psdChannel[0].Append((uint8)rgba[2]); // Red channel
			psdChannel[1].Append((uint8)rgba[1]); // Green channel
			psdChannel[2].Append((uint8)rgba[0]); // Blue channel
			if (fChannels == 4)
				psdChannel[3].Append((uint8)rgba[3]); // Alpha channel
		}
		return B_OK;
	} else if (fCompression == PSD_COMPRESSED_RLE) {
		for (int32 h = 0; h < fHeight; h++) {
			BDataArray lineData[4];
			
			for (int32 w = 0; w < fWidth; w++) {
				uint8 rgba[4];
				fStream->Read(rgba, sizeof(uint32));
				lineData[0].Append((uint8)rgba[2]); // Red channel
				lineData[1].Append((uint8)rgba[1]); // Green channel
				lineData[2].Append((uint8)rgba[0]); // Blue channel
				if (fChannels == 4)
					lineData[3].Append((uint8)rgba[3]); // Alpha channel
			}
			
			for (int channelIdx = 0; channelIdx < fChannels; channelIdx++) {
				BDataArray *packedLine = _PackBits(lineData[channelIdx].Buffer(),
					lineData[channelIdx].Length());

				if (fVersion == PSD_FILE)
					psdByteCounts[channelIdx].Append((uint16)packedLine->Length());
				else
					psdByteCounts[channelIdx].Append((uint32)packedLine->Length());

				psdChannel[channelIdx].Append(*packedLine);
				delete packedLine;
			}
		}
		return B_OK;
	}
	return B_NO_TRANSLATOR;
}


void
PSDWriter::_WriteInt64ToStream(BPositionIO *stream, int64 val)
{
	val = B_HOST_TO_BENDIAN_INT64(val);
	stream->Write(&val, sizeof(int32));
}


void
PSDWriter::_WriteUInt64ToStream(BPositionIO *stream, uint64 val)
{
	val = B_HOST_TO_BENDIAN_INT64(val);
	stream->Write(&val, sizeof(uint64));
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
