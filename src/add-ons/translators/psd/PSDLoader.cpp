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

	fSignature = _GetInt32FromStream(fStream);
	if (fSignature != 0x38425053)
		return;

	fVersion = _GetInt16FromStream(fStream);
	
	// Skip reserved data
	_SkipStreamBlock(fStream, 6);

	fChannels = _GetInt16FromStream(fStream);
	fHeight = _GetInt32FromStream(fStream);
	fWidth = _GetInt32FromStream(fStream);
	fDepth = _GetInt16FromStream(fStream);	
	fColorFormat = _GetInt16FromStream(fStream);

	// Skip mode data
	_SkipStreamBlock(fStream, _GetInt32FromStream(fStream));
	// Skip image resources
	_SkipStreamBlock(fStream, _GetInt32FromStream(fStream));
	// Skip reserved data
	_SkipStreamBlock(fStream, _GetInt32FromStream(fStream));

	fCompression = _GetInt16FromStream(fStream);

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

	if (_ColorFormat() == PSD_COLOR_FORMAT_UNSUPPORTED)
		return false;

	if (fCompression != PSD_COMPRESSED_RAW
		&& fCompression != PSD_COMPRESSED_RLE)
		return false;

	return true;
}


BString
PSDLoader::ColorFormatName(void)
{
	switch (fColorFormat) {
		case PSD_COLOR_MODE_BITS:
			return "Bitmap";
		case PSD_COLOR_MODE_GRAYSCALE:
			return "Grayscale";
		case PSD_COLOR_MODE_INDEXED:
			return "Indexed";
		case PSD_COLOR_MODE_RGB:
			return fChannels > 3 ? "RGBA" : "RGB";
		case PSD_COLOR_MODE_CMYK:
			return "CMYK";
		case PSD_COLOR_MODE_MULTICHANNEL:
			return "Multichannel";
		case PSD_COLOR_MODE_LAB:
			return "Lab";
	}
	return "";
}


int32
PSDLoader::_ColorFormat(void)
{
	int32 format = PSD_COLOR_FORMAT_UNSUPPORTED;
	if (!fLoaded)
		return format;

	switch (fColorFormat) {
		case PSD_COLOR_MODE_RGB:
			if (fChannels == 3)
				format = PSD_COLOR_FORMAT_RGB_8;
			if (fChannels >= 4)
				format = PSD_COLOR_FORMAT_RGB_A_8;
			break;
		case PSD_COLOR_MODE_GRAYSCALE:
			if (fChannels == 1)
				format = PSD_COLOR_FORMAT_GRAY_8;
			if (fChannels == 2)
				format = PSD_COLOR_FORMAT_GRAY_A_8;
			break;		
		case PSD_COLOR_MODE_MULTICHANNEL:
			if (fChannels == 3)
				format = PSD_COLOR_FORMAT_MULTICHANNEL_8;
			break;
		case PSD_COLOR_MODE_CMYK:
			if (fChannels == 3)
				format = PSD_COLOR_FORMAT_MULTICHANNEL_8;
			if (fChannels == 4)
				format = PSD_COLOR_FORMAT_CMYK_8;
			if (fChannels > 4)
				format = PSD_COLOR_FORMAT_CMYK_A_8;
			break;
		case PSD_COLOR_MODE_LAB:
			if (fChannels == 3)
				format = PSD_COLOR_FORMAT_LAB_8;
			if (fChannels > 3)
				format = PSD_COLOR_FORMAT_LAB_A_8;
			break;
		default:
			break;
	};

	return format;
}


int
PSDLoader::Decode(BPositionIO *target)
{
	if (!IsSupported())
		return B_NO_TRANSLATOR;

	fStreamBuffer = new uint8[fStreamSize];
	fStream->Seek(0, SEEK_SET);
	fStream->Read(fStreamBuffer, fStreamSize);

	uint8 *imageData[5];
	for (int i = 0; i < fChannels; i++)
		imageData[i] = new uint8[fWidth * fHeight];

	int pixelCount = fWidth * fHeight;

	if (fCompression == PSD_COMPRESSED_RAW) {
		for (int channelIdx = 0; channelIdx < fChannels; channelIdx++) {
			uint8 *ptr = imageData[channelIdx];
			for (int i = 0; i < pixelCount; i++, ptr++)
				*ptr = (uint8)fStreamBuffer[fStreamPos++];
		}
	}

	if (fCompression == PSD_COMPRESSED_RLE) {
		fStreamPos += fHeight * fChannels * 2;
		for (int channelIdx = 0; channelIdx < fChannels; channelIdx++) {
			uint8 *ptr = imageData[channelIdx];
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
						*ptr++ = (int8)fStreamBuffer[fStreamPos++];
						len--;
					}
				} else if (len > 128) {
					int8 val = (int8)fStreamBuffer[fStreamPos++];
					len ^= 255;
					len += 2;
					count += len;
					while (len) {
						*ptr++ = val;
						len--;
					}
				}
			}
		}
	}
	
	delete fStreamBuffer;

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
	
	uint8 *lineData = new uint8[fWidth * sizeof(uint32)];
	
	int32 colorFormat = _ColorFormat();
	
	switch (colorFormat) {
		case PSD_COLOR_FORMAT_GRAY_8:
		case PSD_COLOR_FORMAT_GRAY_A_8:
		{
			bool isAlpha = colorFormat == PSD_COLOR_FORMAT_GRAY_A_8;
			uint8 *yCh = imageData[0];
			uint8 *alphaCh = isAlpha ? imageData[1] : NULL;
			for (int h = 0; h < fHeight; h++) {
				uint8 *ptr = lineData;
				for (int w = 0; w < fWidth; w++) {
					uint8 y = *yCh++;
					*ptr++ = y;
					*ptr++ = y;
					*ptr++ = y;
					*ptr++ = isAlpha ? *alphaCh++ : 255;
				}
				target->Write(lineData, fWidth * sizeof(uint32));
			}
			break;
		}
		case PSD_COLOR_FORMAT_MULTICHANNEL_8:
		case PSD_COLOR_FORMAT_RGB_8:
		case PSD_COLOR_FORMAT_RGB_A_8:
		{
			bool isAlpha = colorFormat == PSD_COLOR_FORMAT_RGB_A_8;
			uint8 *rCh = imageData[0];
			uint8 *gCh = imageData[1];
			uint8 *bCh = imageData[2];
			uint8 *alphaCh =  isAlpha ?	imageData[3] : NULL;
			for (int h = 0; h < fHeight; h++) {
				uint8 *ptr = lineData;
				for (int w = 0; w < fWidth; w++) {
					*ptr++ = *bCh++;
					*ptr++ = *gCh++;
					*ptr++ = *rCh++;
					*ptr++ = isAlpha ? *alphaCh++ : 255;
				}
				target->Write(lineData, fWidth * sizeof(uint32));
			}
			break;
		}	
		case PSD_COLOR_FORMAT_CMYK_8:
		case PSD_COLOR_FORMAT_CMYK_A_8:
		{
			bool isAlpha = colorFormat == PSD_COLOR_FORMAT_CMYK_A_8;
			uint8 *cCh = imageData[0];
			uint8 *mCh = imageData[1];
			uint8 *yCh = imageData[2];
			uint8 *kCh = imageData[3];
			uint8 *alphaCh = isAlpha ? imageData[4] : NULL;
			for (int h = 0; h < fHeight; h++) {
				uint8 *ptr = lineData;
				for (int w = 0; w < fWidth; w++) {
					double c = 1.0 - (*cCh++ / 255.0);
					double m = 1.0 - (*mCh++ / 255.0);
					double y = 1.0 - (*yCh++ / 255.0);
					double k = 1.0 - (*kCh++ / 255.0);
					*ptr++ = (uint8)((1.0 - (y * (1.0 - k) + k)) * 255.0);
					*ptr++ = (uint8)((1.0 - (m * (1.0 - k) + k)) * 255.0);
					*ptr++ = (uint8)((1.0 - (c * (1.0 - k) + k)) * 255.0);
					*ptr++ = alphaCh ? *alphaCh++ : 255;
				}
				target->Write(lineData, fWidth * sizeof(uint32));
			}
			break;
		}
		case PSD_COLOR_FORMAT_LAB_8:
		case PSD_COLOR_FORMAT_LAB_A_8:
		{
			bool isAlpha = colorFormat == PSD_COLOR_FORMAT_LAB_A_8;
			uint8 *lCh = imageData[0];
			uint8 *aCh = imageData[1];
			uint8 *bCh = imageData[2];
			uint8 *alphaCh = isAlpha ? imageData[3] : NULL;
			for (int h = 0; h < fHeight; h++) {
				uint8 *ptr = lineData;
				for (int w = 0; w < fWidth; w++) {
					double L = ((*lCh++) / 255.0) * 100.0;
					double a = (*aCh++) - 128.0;
					double b = (*bCh++) - 128.0;

					double Y = L * (1.0 / 116.0) + 16.0 / 116.0;
					double X = a * (1.0 / 500.0) + Y;
					double Z = b * (-1.0 / 200.0) + Y;

					X = X > 6.0 / 29.0 ? X * X * X : X * (108.0 / 841.0)
						- (432.0 / 24389.0);
					Y = L > 8.0 ? Y * Y * Y : L * (27.0 / 24389.0);
					Z = Z > 6.0 / 29.0 ? Z * Z * Z : Z * (108.0 / 841.0)
						- (432.0 / 24389.0);

					double R = X * (1219569.0 / 395920.0)
						+ Y * (-608687.0 / 395920.0)
						+ Z * (-107481.0 / 197960.0);
					double G = X * (-80960619.0 / 87888100.0)
						+ Y * (82435961.0 / 43944050.0)
						+ Z * (3976797.0 / 87888100.0);
					double B = X * (93813.0 / 1774030.0)
						+ Y * (-180961.0 / 887015.0)
						+ Z * (107481.0 / 93370.0);

					R = R > 0.0031308 ?	pow(R, 1.0 / 2.4) * 1.055 - 0.055
						: R * 12.92;
					G = G > 0.0031308 ?	pow(G, 1.0 / 2.4) * 1.055 - 0.055
						: G * 12.92;
					B = B > 0.0031308 ?	pow(B, 1.0 / 2.4) * 1.055 - 0.055
						: B * 12.92;

					R = (R < 0) ? 0 : ((R > 1) ? 1 : R);
					G = (G < 0) ? 0 : ((G > 1) ? 1 : G);
					B = (B < 0) ? 0 : ((B > 1) ? 1 : B);
					
					*ptr++ = (uint8)(B * 255.0);
					*ptr++ = (uint8)(G * 255.0);
					*ptr++ = (uint8)(R * 255.0);
					*ptr++ = isAlpha ? *alphaCh++ : 255;
				}
				target->Write(lineData, fWidth * sizeof(uint32));
			}
			break;
		}
		default:
			break;
	};

	delete lineData;
	for (int i = 0; i < fChannels; i++)
		delete imageData[i];

	return B_OK;
}


int32
PSDLoader::_GetInt32FromStream(BPositionIO *in)
{
	int32 ret;
	in->Read(&ret, sizeof(int32));
	return B_BENDIAN_TO_HOST_INT32(ret);
}


int16
PSDLoader::_GetInt16FromStream(BPositionIO *in)
{
	int16 ret;
	in->Read(&ret, sizeof(int16));
	return B_BENDIAN_TO_HOST_INT16(ret);
}


int8
PSDLoader::_GetInt8FromStream(BPositionIO *in)
{
	int8 ret;
	in->Read(&ret, sizeof(int8));
	return ret;
}


uint8
PSDLoader::_GetUInt8FromStream(BPositionIO *in)
{
	uint8 ret;
	in->Read(&ret, sizeof(uint8));
	return ret;
}


void
PSDLoader::_SkipStreamBlock(BPositionIO *in, size_t count)
{	
	in->Seek(count, SEEK_CUR);
}
