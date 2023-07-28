/*
 * Copyright 2013, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "PSDLoader.h"

#include <Catalog.h>

#include "BaseTranslator.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PSDLoader"


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
	if (fSignature != 0x38425053) // 8BPS
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

	fImageResourceSectionSize = _GetInt32FromStream(fStream);
	fImageResourceSectionPos = fStream->Position();
	_SkipStreamBlock(fStream, fImageResourceSectionSize);

	// Skip [layer and mask] block
	if (fVersion == PSD_FILE)
		_SkipStreamBlock(fStream, _GetInt32FromStream(fStream));
	else if (fVersion == PSB_FILE)
		_SkipStreamBlock(fStream, _GetInt64FromStream(fStream));
	else
		return;

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

	if (fVersion != PSD_FILE && fVersion != PSB_FILE)
		return false;

	if (fChannels < 0 || fChannels > PSD_MAX_CHANNELS)
		return false;

	if (fDepth > 16)
		return false;

	if (_ColorFormat() == PSD_COLOR_FORMAT_UNSUPPORTED)
		return false;

	if (fCompression != PSD_COMPRESSED_RAW && fCompression != PSD_COMPRESSED_RLE)
		return false;

	return true;
}


BString
PSDLoader::ColorFormatName(void)
{
	switch (fColorFormat) {
		case PSD_COLOR_MODE_BITS:
			return B_TRANSLATE("Bitmap");
		case PSD_COLOR_MODE_GRAYSCALE:
			return B_TRANSLATE("Grayscale");
		case PSD_COLOR_MODE_INDEXED:
			return B_TRANSLATE("Indexed");
		case PSD_COLOR_MODE_RGB:
			return fChannels > 3 ? B_TRANSLATE("RGBA") : B_TRANSLATE("RGB");
		case PSD_COLOR_MODE_CMYK:
			return B_TRANSLATE("CMYK");
		case PSD_COLOR_MODE_MULTICHANNEL:
			return B_TRANSLATE("Multichannel");
		case PSD_COLOR_MODE_LAB:
			return B_TRANSLATE("Lab");
		case PSD_COLOR_MODE_DUOTONE:
			return B_TRANSLATE("Duotone");
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

	uint8 *imageData[PSD_MAX_CHANNELS];
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
			if (fVersion == PSD_FILE)
				fStreamPos += fHeight * fChannels * 2;
			else if (fVersion == PSB_FILE)
				fStreamPos += fHeight * fChannels * 4;

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
			delete[] fStreamBuffer;
			for (int i = 0; i < fChannels; i++)
				delete[] imageData[i];
			return 	B_NO_TRANSLATOR;
	}

	delete[] fStreamBuffer;

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

			if (_ParseImageResources() != B_OK)
				fTransparentIndex = 256;

			uint8 *redPalette = colorData;
			uint8 *greenPalette = colorData + paletteSize;
			uint8 *bluePalette = colorData + paletteSize * 2;
			int32 index = 0;
			for (int h = 0; h < fHeight; h++) {
				uint8 *ptr = lineData;
				for (int w = 0; w < fWidth; w++) {
					uint8 colorIndex = imageData[0][index];
					ptr[0] = bluePalette[colorIndex];
					ptr[1] = greenPalette[colorIndex];
					ptr[2] = redPalette[colorIndex];
					ptr[3] = colorIndex == fTransparentIndex ? 0 : 255;

					ptr += sizeof(uint32);
					index++;
				}
				target->Write(lineData, fWidth * sizeof(uint32));
			}
			delete[] colorData;
			break;
		}
		case PSD_COLOR_FORMAT_DUOTONE:
		case PSD_COLOR_FORMAT_GRAY:
		case PSD_COLOR_FORMAT_GRAY_A:
		{
			bool isAlpha = colorFormat == PSD_COLOR_FORMAT_GRAY_A;
			int32 index = 0;
			for (int h = 0; h < fHeight; h++) {
				uint8 *ptr = lineData;
				for (int w = 0; w < fWidth; w++) {
					ptr[0] = imageData[0][index];
					ptr[1] = imageData[0][index];
					ptr[2] = imageData[0][index];
					ptr[3] = isAlpha ? imageData[1][index] : 255;

					ptr += sizeof(uint32);
					index += depthBytes;
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
			int32 index = 0;
			for (int h = 0; h < fHeight; h++) {
				uint8 *ptr = lineData;
				for (int w = 0; w < fWidth; w++) {
					ptr[0] = imageData[2][index];
					ptr[1] = imageData[1][index];
					ptr[2] = imageData[0][index];
					ptr[3] = isAlpha ? imageData[3][index] : 255;

					ptr += sizeof(uint32);
					index += depthBytes;
				}
				target->Write(lineData, fWidth * sizeof(uint32));
			}
			break;
		}
		case PSD_COLOR_FORMAT_CMYK:
		case PSD_COLOR_FORMAT_CMYK_A:
		{
			bool isAlpha = colorFormat == PSD_COLOR_FORMAT_CMYK_A;
			int32 index = 0;
			for (int h = 0; h < fHeight; h++) {
				uint8 *ptr = lineData;
				for (int w = 0; w < fWidth; w++) {
					double c = 1.0 - imageData[0][index] / 255.0;
					double m = 1.0 - imageData[1][index] / 255.0;
					double y = 1.0 - imageData[2][index] / 255.0;
					double k = 1.0 - imageData[3][index] / 255.0;
					ptr[0] = (uint8)((1.0 - (y * (1.0 - k) + k)) * 255.0);
					ptr[1] = (uint8)((1.0 - (m * (1.0 - k) + k)) * 255.0);
					ptr[2] = (uint8)((1.0 - (c * (1.0 - k) + k)) * 255.0);
					ptr[3] = isAlpha ?  imageData[4][index] : 255;

					ptr += sizeof(uint32);
					index += depthBytes;
				}
				target->Write(lineData, fWidth * sizeof(uint32));
			}
			break;
		}
		case PSD_COLOR_FORMAT_LAB:
		case PSD_COLOR_FORMAT_LAB_A:
		{
			bool isAlpha = colorFormat == PSD_COLOR_FORMAT_LAB_A;
			int32 index = 0;
			for (int h = 0; h < fHeight; h++) {
				uint8 *ptr = lineData;
				for (int w = 0; w < fWidth; w++) {
					double L = imageData[0][index] / 255.0 * 100.0;
					double a = imageData[1][index] - 128.0;
					double b = imageData[2][index] - 128.0;

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

					ptr[0] = (uint8)(B * 255.0);
					ptr[1] = (uint8)(G * 255.0);
					ptr[2] = (uint8)(R * 255.0);
					ptr[3] = isAlpha ? imageData[3][index] : 255;

					ptr += sizeof(uint32);
					index += depthBytes;
				}
				target->Write(lineData, fWidth * sizeof(uint32));
			}
			break;
		}
		default:
			break;
	};

	delete[] lineData;
	for (int i = 0; i < fChannels; i++)
		delete[] imageData[i];

	return B_OK;
}


int64
PSDLoader::_GetInt64FromStream(BPositionIO *in)
{
	int64 ret;
	in->Read(&ret, sizeof(int64));
	return B_BENDIAN_TO_HOST_INT64(ret);
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


status_t
PSDLoader::_ParseImageResources(void)
{
	if (!fLoaded && fImageResourceSectionSize == 0)
		return B_ERROR;

	off_t currentPos = fStream->Position();
	fStream->Seek(fImageResourceSectionPos, SEEK_SET);

	while (fStream->Position() < currentPos + fImageResourceSectionSize) {
		int32 resBlockSignature = _GetInt32FromStream(fStream);
		if (resBlockSignature != 0x3842494D) // 8BIM
			return B_ERROR;

		uint16 resID = _GetInt16FromStream(fStream);

		BString resName, name;
		int nameLength = 0;
		while (true) {
			int charData = _GetUInt8FromStream(fStream);
			nameLength++;
			if (charData == 0) {
				if (nameLength % 2 == 1) {
					_GetUInt8FromStream(fStream);
					nameLength++;
				}
				break;
			} else
				name += charData;
			resName = name;
		}

		uint32 resSize = _GetInt32FromStream(fStream);

		if (resSize % 2 == 1)
			resSize++;

		switch (resID) {
			case 0x0417:
				fTransparentIndex = _GetInt16FromStream(fStream);
				break;
			default:
				_SkipStreamBlock(fStream, resSize);
		}
	}

	fStream->Seek(currentPos, SEEK_SET);

	return B_OK;
}
