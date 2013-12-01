/*
 * Copyright 2013, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */ 


#include "PSDLoader.h"
#include "BaseTranslator.h"


PSDLoader::PSDLoader(BPositionIO *src)
{
	fLoaded = false;
	fStream = src;

	fStream->Seek(0, SEEK_END);
	fStreamSize = fStream->Position();
	fStream->Seek(0, SEEK_SET);

	if (fStreamSize <= 0)
		return;

	fStream->Seek(0, SEEK_SET);

	fSignature = GetInt32FromStream(fStream);
	if (fSignature != 0x38425053)
		return;

	fVersion = GetInt16FromStream(fStream);
	
	//Skip reserved data
	SkipStreamBlock(fStream, 6);

	fChannels = GetInt16FromStream(fStream);
	fHeight = GetInt32FromStream(fStream);
	fWidth = GetInt32FromStream(fStream);
	fDepth = GetInt16FromStream(fStream);	
	fColorFormat = GetInt16FromStream(fStream);

	//Skip mode data
	SkipStreamBlock(fStream, GetInt32FromStream(fStream));
	//Skip image resources
	SkipStreamBlock(fStream, GetInt32FromStream(fStream));
	//Skip reserved data
	SkipStreamBlock(fStream, GetInt32FromStream(fStream));

	fCompression = GetInt16FromStream(fStream);

	fStreamPos = fStream->Position();

	fLoaded = true;
}


PSDLoader::~PSDLoader()
{
}


bool
PSDLoader::IsLoaded(void)
{
	return fLoaded;
}


bool
PSDLoader::IsSupported(void)
{
	if (!fLoaded)
		return false;

	if (fVersion != 1)
		return false;

    if (fChannels < 0 || fChannels > 16)
    	return false;

	if (fDepth != 8)
		return false;

	if (fColorFormat != 3)
		return false;

	if (fCompression != PSD_COMPRESSED_RAW &&
		fCompression != PSD_COMPRESSED_RLE)
		return false;

	return true;
}


int
PSDLoader::Decode(BPositionIO *target)
{
	if(!IsSupported())
		return B_NO_TRANSLATOR;

	fStreamBuffer = new uint8[fStreamSize];
	fStream->Seek(0, SEEK_SET);
	fStream->Read(fStreamBuffer, fStreamSize);

	uint8 *imageData = new uint8[fWidth * fHeight * sizeof(uint32)];

	int pixelCount = fWidth * fHeight;
	
	//RGBA to BRGA order mapping table
	int	channelMap[4] = {2, 1, 0, 3};

	if (fCompression == PSD_COMPRESSED_RAW) {
      for (int channelIdx = 0; channelIdx < 4; channelIdx++) {
         uint8 *ptr = imageData + channelMap[channelIdx];
         if (channelIdx > fChannels) {
            for (int i = 0; i < pixelCount; i++, ptr += 4)
            	*ptr = (channelIdx == 3) ? 255 : 0;
         } else {
            for (int i = 0; i < pixelCount; i++, ptr += 4)
               *ptr = (uint8)fStreamBuffer[fStreamPos++];
         }
      }
    }

    if (fCompression == PSD_COMPRESSED_RLE) {
    	fStreamPos += fHeight * fChannels * 2;

    	for (int channelIdx = 0; channelIdx < 4; channelIdx++) {
    		uint8 *ptr = imageData + channelMap[channelIdx];

    		if (channelIdx >= fChannels) {
            	for (int i = 0; i < pixelCount; i++, ptr += 4)
            		*ptr = (channelIdx == 3 ? 255 : 0);
         	} else {
            // Read the RLE data.
            int count = 0;
            while (count < pixelCount) {
               uint8 len = (uint8)fStreamBuffer[fStreamPos++]; 
               if (len == 128) {
                  continue;                  
               } else if (len < 128) {                      
                  len++;
                  count += len;
                  while (len) {
                     *ptr = (int8)fStreamBuffer[fStreamPos++];
                     ptr += 4;
                     len--;
                  }
               } else if (len > 128) {
                  int8 val = (int8)fStreamBuffer[fStreamPos++];
                  len ^= 255;
                  len += 2;
                  count += len;
                  while (len) {
                     *ptr = val;
                     ptr += 4;
                     len--;
                  }
               }
            }
         }
    	}
    }

	TranslatorBitmap bitsHeader;
	bitsHeader.magic = B_TRANSLATOR_BITMAP;
	bitsHeader.bounds.left = 0;
	bitsHeader.bounds.top = 0;
	bitsHeader.bounds.right = fWidth - 1;
	bitsHeader.bounds.bottom = fHeight - 1;
	bitsHeader.rowBytes = sizeof(uint32) * fWidth;
	bitsHeader.colors = B_RGBA32;
	bitsHeader.dataSize = bitsHeader.rowBytes * fHeight;

	if (swap_data(B_UINT32_TYPE, &bitsHeader,
		sizeof(TranslatorBitmap), B_SWAP_HOST_TO_BENDIAN) != B_OK) {
		return B_NO_TRANSLATOR;
	}

	target->Write(&bitsHeader, sizeof(TranslatorBitmap));
	target->Write(imageData, fHeight * fWidth * sizeof(uint32));

	delete imageData;
	delete fStreamBuffer;

	return B_OK;
}


int32
PSDLoader::GetInt32FromStream(BPositionIO *in)
{
	int32 ret;
	in->Read(&ret, sizeof(int32));
	return B_BENDIAN_TO_HOST_INT32(ret);
}


int16
PSDLoader::GetInt16FromStream(BPositionIO *in)
{
	int16 ret;
	in->Read(&ret, sizeof(int16));
	return B_BENDIAN_TO_HOST_INT16(ret);
}


int8
PSDLoader::GetInt8FromStream(BPositionIO *in)
{
	int8 ret;
	in->Read(&ret, sizeof(int8));
	return ret;
}


uint8
PSDLoader::GetUInt8FromStream(BPositionIO *in)
{
	uint8 ret;
	in->Read(&ret, sizeof(uint8));
	return ret;
}


void
PSDLoader::SkipStreamBlock(BPositionIO *in, size_t count)
{	
	in->Seek(count, SEEK_CUR);
}
