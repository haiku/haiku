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

	fColorModeDataSize = _GetInt32FromStream(fStream);
	fColorModeDataPos = fStream->Position();
	_SkipStreamBlock(fStream, fColorModeDataSize);

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

	if (fDepth > 16)
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
		case PSD_COLOR_MODE_DUOTONE:
			return "Duotone";
	}
	return "";
}


psd_color_format
PSDLoader::_ColorFormat(void)
{
	psd_color_format format = PSD_COLOR_FORMAT_UNSUPPORTED;
	if (!fLoaded)
		return format;

	switch (fColorFormat) {
		case PSD_COLOR_MODE_BITS:
			format = PSD_COLOR_FORMAT_BITMAP;
			break;
		case PSD_COLOR_MODE_RGB:
			if (fChannels == 3)
				format = PSD_COLOR_FORMAT_RGB;
			else if (fChannels >= 4)
				format = PSD_COLOR_FORMAT_RGB_A;
			break;
		case PSD_COLOR_MODE_GRAYSCALE:
			if (fChannels == 1)
				format = PSD_COLOR_FORMAT_GRAY;
			else if (fChannels == 2)
				format = PSD_COLOR_FORMAT_GRAY_A;
			break;		
		case PSD_COLOR_MODE_MULTICHANNEL:
			if (fChannels == 3)
				format = PSD_COLOR_FORMAT_MULTICHANNEL;
			break;
		case PSD_COLOR_MODE_CMYK:
			if (fChannels == 3)
				format = PSD_COLOR_FORMAT_MULTICHANNEL;
			else if (fChannels == 4)
				format = PSD_COLOR_FORMAT_CMYK;
			else if (fChannels > 4)
				format = PSD_COLOR_FORMAT_CMYK_A;
			break;
		case PSD_COLOR_MODE_LAB:
			if (fChannels == 3)
				format = PSD_COLOR_FORMAT_LAB;
			else if (fChannels > 3)
				format = PSD_COLOR_FORMAT_LAB_A;
			break;
		case PSD_COLOR_MODE_DUOTONE:
			if (fChannels >= 1)
				format = PSD_COLOR_FORMAT_DUOTONE;
			break;
		case PSD_COLOR_MODE_INDEXED:
			if (fChannels >= 1 && fColorModeDataSize >= 3)
				format = PSD_COLOR_FORMAT_INDEXED;
			break;
		default:
			break;
	};

	return format;
}


status_t
PSDLoader::Decode(BPositionIO *target)
{
	if (!IsSupported())
		return B_NO_TRANSLATOR;

	fStreamBuffer = new uint8[fStreamSize];
	fStream->Seek(0, SEEK_SET);
	fStream->Read(fStreamBuffer, fStreamSize);

	int32 depthBytes = fDepth / 8;
	int32 rowBytes = (fWidth * fDepth) / 8;
	int32 channelBytes = rowBytes * fHeight;
	
	uint8 *imageData[5];
	for (int i = 0; i < fChannels; i++)
		imageData[i] = new uint8[channelBytes];

	
	switch (fCompression) {
		case PSD_COMPRESSED_RAW:
		{
			for (int channelIdx = 0; channelIdx < fChannels; channelIdx++) {
				uint8 *ptr = imageData[channelIdx];
				for (int i = 0; i < channelBytes; i++, ptr++)
					*ptr = (uint8)fStreamBuffer[fStreamPos++];
			}
			break;
		}
		case PSD_COMPRESSED_RLE:
		{
			fStreamPos += fHeight * fChannels * 2;
			for (int channelIdx = 0; channelIdx < fChannels; channelIdx++) {
				uint8 *ptr = imageData[channelIdx];
				// Read the RLE data.
				int count = 0;
				while (count < channelBytes) {
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
			break;
		}
		default:
			delete fStreamBuffer;
			for (int i = 0; i < fChannels; i++)
				delete imageData[i];
			return 	B_NO_TRANSLATOR;
	}

	delete fStreamBuffer;

	TranslatorBitmap bitsHeader;
	bitsHeader.magic = B_TRANSLATOR_BITMAP;
	bitsHeader.bounds.left = 0;
	bitsHeader.bounds.top = 0;
	bitsHeader.bounds.right = fWidth - 1;
	bitsHeader.bounds.bottom = fHeight - 1;

	psd_color_format colorFormat = _ColorFormat();

	if (colorFormat == PSD_COLOR_FORMAT_BITMAP) {
		bitsHeader.rowBytes = rowBytes;
		bitsHeader.dataSize = channelBytes;
		bitsHeader.colors = B_GRAY1;
	} else {
		bitsHeader.rowBytes = sizeof(uint32) * fWidth;
		bitsHeader.colors = B_RGBA32;
		bitsHeader.dataSize = bitsHeader.rowBytes * fHeight;
	}

	if (swap_data(B_UINT32_TYPE, &bitsHeader,
		sizeof(TranslatorBitmap), B_SWAP_HOST_TO_BENDIAN) != B_OK) {
		return B_NO_TRANSLATOR;
	}

	target->Write(&bitsHeader, sizeof(TranslatorBitmap));
	
	uint8 *lineData = new uint8[fWidth * sizeof(uint32)];
		
	switch (colorFormat) {
		case PSD_COLOR_FORMAT_BITMAP:
		{			
			int32 rowBytes = (fWidth / 8 ) * fHeight;
			for (int32 i = 0; i < rowBytes; i++)
				imageData[0][i]^=255;
			target->Write(imageData[0], rowBytes);
			break;
		}
		case PSD_COLOR_FORMAT_INDEXED:
		{
			int32 paletteSize = fColorModeDataSize / 3;		

			uint8 *colorData = new uint8[fColorModeDataSize];
			fStream->Seek(fColorModeDataPos, SEEK_SET);
			fStream->Read(colorData, fColorModeDataSize);

			uint8 *redPalette = colorData;
			uint8 *greenPalette = colorData + paletteSize;
			uint8 *bluePalette = colorData + paletteSize * 2;
			uint8 *cCh = imageData[0];
			for (int h = 0; h < fHeight; h++) {
				uint8 *ptr = lineData;
				for (int w = 0; w < fWidth; w++) {
					uint8 c = *cCh++;
					*ptr++ = bluePalette[c];
					*ptr++ = greenPalette[c];
					*ptr++ = redPalette[c];
					*ptr++ = 255;
				}
				target->Write(lineData, fWidth * sizeof(uint32));
			}
			delete colorData;
			break;
		}
		case PSD_COLOR_FORMAT_DUOTONE:
		case PSD_COLOR_FORMAT_GRAY:
		case PSD_COLOR_FORMAT_GRAY_A:
		{
			bool isAlpha = colorFormat == PSD_COLOR_FORMAT_GRAY_A;
			uint8 *yCh = imageData[0];
			uint8 *alphaCh = isAlpha ? imageData[1] : NULL;
			for (int h = 0; h < fHeight; h++) {
				uint8 *ptr = lineData;
				for (int w = 0; w < fWidth; w++) {
					uint8 y = *(yCh += depthBytes);
					*ptr++ = y;
					*ptr++ = y;
					*ptr++ = y;
					*ptr++ = isAlpha ? *(alphaCh += depthBytes) : 255;
				}
				target->Write(lineData, fWidth * sizeof(uint32));
			}
			break;
		}
		case PSD_COLOR_FORMAT_MULTICHANNEL:
		case PSD_COLOR_FORMAT_RGB:
		case PSD_COLOR_FORMAT_RGB_A:
		{
			bool isAlpha = colorFormat == PSD_COLOR_FORMAT_RGB_A;
			uint8 *rCh = imageData[0];
			uint8 *gCh = imageData[1];
			uint8 *bCh = imageData[2];
			uint8 *alphaCh =  isAlpha ?	imageData[3] : NULL;
			for (int h = 0; h < fHeight; h++) {
				uint8 *ptr = lineData;
				for (int w = 0; w < fWidth; w++) {
					*ptr++ = *(bCh += depthBytes);
					*ptr++ = *(gCh += depthBytes);
					*ptr++ = *(rCh += depthBytes);
					*ptr++ = isAlpha ? *(alphaCh += depthBytes) : 255;
				}
				target->Write(lineData, fWidth * sizeof(uint32));
			}
			break;
		}	
		case PSD_COLOR_FORMAT_CMYK:
		case PSD_COLOR_FORMAT_CMYK_A:
		{
			bool isAlpha = colorFormat == PSD_COLOR_FORMAT_CMYK_A;
			uint8 *cCh = imageData[0];
			uint8 *mCh = imageData[1];
			uint8 *yCh = imageData[2];
			uint8 *kCh = imageData[3];
			uint8 *alphaCh = isAlpha ? imageData[4] : NULL;
			for (int h = 0; h < fHeight; h++) {
				uint8 *ptr = lineData;
				for (int w = 0; w < fWidth; w++) {
					double c = 1.0 - *(cCh += depthBytes) / 255.0;
					double m = 1.0 - *(mCh += depthBytes) / 255.0;
					double y = 1.0 - *(yCh += depthBytes) / 255.0;
					double k = 1.0 - *(kCh += depthBytes) / 255.0;
					*ptr++ = (uint8)((1.0 - (y * (1.0 - k) + k)) * 255.0);
					*ptr++ = (uint8)((1.0 - (m * (1.0 - k) + k)) * 255.0);
					*ptr++ = (uint8)((1.0 - (c * (1.0 - k) + k)) * 255.0);
					*ptr++ = alphaCh ? *(alphaCh += depthBytes) : 255;
				}
				target->Write(lineData, fWidth * sizeof(uint32));
			}
			break;
		}
		case PSD_COLOR_FORMAT_LAB:
		case PSD_COLOR_FORMAT_LAB_A:
		{
			bool isAlpha = colorFormat == PSD_COLOR_FORMAT_LAB_A;
			uint8 *lCh = imageData[0];
			uint8 *aCh = imageData[1];
			uint8 *bCh = imageData[2];
			uint8 *alphaCh = isAlpha ? imageData[3] : NULL;
			for (int h = 0; h < fHeight; h++) {
				uint8 *ptr = lineData;
				for (int w = 0; w < fWidth; w++) {
					double L = *(lCh += depthBytes) / 255.0 * 100.0;
					double a = *(aCh += depthBytes) - 128.0;
					double b = *(bCh += depthBytes) - 128.0;

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
					*ptr++ = isAlpha ? *(alphaCh += depthBytes) : 255;
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
