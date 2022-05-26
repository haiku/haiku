/*
 * Copyright 2012, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "ICNSLoader.h"
#include "BaseTranslator.h"


static int compareTypes(const void *a, const void *b)
{
	icns_type_t **typeItemA = (icns_type_t**)a;
	icns_type_t **typeItemB = (icns_type_t**)b;

	icns_icon_info_t imageInfoA = icns_get_image_info_for_type(**typeItemA);
	icns_icon_info_t imageInfoB = icns_get_image_info_for_type(**typeItemB);

	return imageInfoB.iconWidth - imageInfoA.iconWidth;
}


icns_type_t
ICNSFormat(float width, float height, color_space colors)
{
	int imageWidth = (int)ceil(width);
	int imageHeight = (int)ceil(height);

	if (imageWidth != imageHeight)
		return ICNS_NULL_TYPE;

	//other colors depth not supported now
	if (colors != B_RGB32 && colors != B_RGBA32)
		return ICNS_NULL_TYPE;

	switch (imageWidth) {
		case 16:
			return ICNS_16x16_32BIT_DATA;
		case 32:
			return ICNS_32x32_32BIT_DATA;
		case 48:
			return ICNS_48x48_32BIT_DATA;
		case 128:
			return ICNS_128X128_32BIT_DATA;
		case 256:
			return ICNS_256x256_32BIT_ARGB_DATA;
		case 512:
			return ICNS_512x512_32BIT_ARGB_DATA;
		case 1024:
			return ICNS_1024x1024_32BIT_ARGB_DATA;
	}
	return ICNS_NULL_TYPE;
}


ICNSLoader::ICNSLoader(BPositionIO *stream)
{
	fLoaded = false;
	fIconsCount = 0;

	stream->Seek(0, SEEK_END);
	fStreamSize = stream->Position();
	stream->Seek(0, SEEK_SET);

	if (fStreamSize <= 0)
		return;

	uint8* icnsDataBuffer = new uint8[fStreamSize];
	size_t readedBytes = stream->Read(icnsDataBuffer,fStreamSize);

	fIconFamily = NULL;
	int status = icns_import_family_data(readedBytes, icnsDataBuffer,
		&fIconFamily);

	if (status != 0) {
		delete[] icnsDataBuffer;
		return;
	}

	icns_byte_t *dataPtr = (icns_byte_t*)fIconFamily;
	off_t dataOffset = sizeof(icns_type_t) + sizeof(icns_size_t);

	while ((dataOffset+8) < fIconFamily->resourceSize) {
		icns_element_t	 iconElement;
		icns_size_t      iconDataSize;

		memcpy(&iconElement, (dataPtr + dataOffset), 8);
		iconDataSize = iconElement.elementSize - 8;

		if (IS_SPUPPORTED_TYPE(iconElement.elementType)) {
				icns_type_t* newTypeItem = new icns_type_t;
				*newTypeItem = iconElement.elementType;
				fFormatList.AddItem(newTypeItem);
				fIconsCount++;
		}
		dataOffset += iconElement.elementSize;
	}

	fFormatList.SortItems(compareTypes);

	delete[] icnsDataBuffer;

	fLoaded = true;
}


ICNSLoader::~ICNSLoader()
{
	if (fIconFamily != NULL)
		free(fIconFamily);

	icns_type_t* item;
	for (int32 i = 0; (item = (icns_type_t*)fFormatList.ItemAt(i)) != NULL; i++)
   		delete item;
   	fFormatList.MakeEmpty();
}


bool
ICNSLoader::IsLoaded(void)
{
	return fLoaded;
}


int
ICNSLoader::IconsCount(void)
{
	return fIconsCount;
}


int
ICNSLoader::GetIcon(BPositionIO *target, int index)
{
	if (index < 1 || index > fIconsCount || !fLoaded)
		return B_NO_TRANSLATOR;

	icns_image_t iconImage;
	memset(&iconImage, 0, sizeof(icns_image_t));

	icns_type_t typeItem = *((icns_type_t*)fFormatList.ItemAt(index - 1));
	int status = icns_get_image32_with_mask_from_family(fIconFamily,
		typeItem, &iconImage);

	if (status != 0)
		return B_NO_TRANSLATOR;

	TranslatorBitmap bitsHeader;
	bitsHeader.magic = B_TRANSLATOR_BITMAP;
	bitsHeader.bounds.left = 0;
	bitsHeader.bounds.top = 0;
	bitsHeader.bounds.right = iconImage.imageWidth - 1;
	bitsHeader.bounds.bottom = iconImage.imageHeight - 1;
	bitsHeader.rowBytes = sizeof(uint32) * iconImage.imageWidth;
	bitsHeader.colors = B_RGBA32;
	bitsHeader.dataSize = bitsHeader.rowBytes * iconImage.imageHeight;
	if (swap_data(B_UINT32_TYPE, &bitsHeader,
		sizeof(TranslatorBitmap), B_SWAP_HOST_TO_BENDIAN) != B_OK) {
		icns_free_image(&iconImage);
		return B_NO_TRANSLATOR;
	}
	target->Write(&bitsHeader, sizeof(TranslatorBitmap));

	uint8 *rowBuff = new uint8[iconImage.imageWidth * sizeof(uint32)];
	for (uint32 i = 0; i < iconImage.imageHeight; i++) {
		uint8 *rowData = iconImage.imageData
			+ (i * iconImage.imageWidth * sizeof(uint32));
		uint8 *rowBuffPtr = rowBuff;
		for (uint32 j=0; j < iconImage.imageWidth; j++) {
			rowBuffPtr[0] = rowData[2];
			rowBuffPtr[1] = rowData[1];
			rowBuffPtr[2] = rowData[0];
			rowBuffPtr[3] = rowData[3];
			rowBuffPtr += sizeof(uint32);
			rowData += sizeof(uint32);
		}
		target->Write(rowBuff, iconImage.imageWidth * sizeof(uint32));
	}
	delete[] rowBuff;
	icns_free_image(&iconImage);

	return B_OK;
}


ICNSSaver::ICNSSaver(BPositionIO *stream, uint32 rowBytes, icns_type_t type)
{
	fCreated = false;

	icns_icon_info_t imageTypeInfo = icns_get_image_info_for_type(type);
	int iconWidth = imageTypeInfo.iconWidth;
	int iconHeight = imageTypeInfo.iconHeight;
	int bpp = 32;

	uint8 *bits = new uint8[iconWidth * iconHeight * sizeof(uint32)];

	uint8 *rowPtr = bits;
	for (int i = 0; i < iconHeight; i++) {
		stream->Read(rowPtr, rowBytes);
		uint8 *bytePtr = rowPtr;
		for (int j=0; j < iconWidth; j++) {
			uint8 temp = bytePtr[0];
			bytePtr[0] = bytePtr[2];
			bytePtr[2] = temp;
			bytePtr += sizeof(uint32);
		}
		rowPtr += iconWidth * sizeof(uint32);
	}

	icns_create_family(&fIconFamily);

	icns_image_t icnsImage;
	icnsImage.imageWidth = iconWidth;
	icnsImage.imageHeight = iconHeight;
	icnsImage.imageChannels = 4;
	icnsImage.imagePixelDepth = 8;
	icnsImage.imageDataSize = iconWidth * iconHeight * 4;
	icnsImage.imageData = bits;

	icns_icon_info_t iconInfo;
	iconInfo.isImage = 1;
	iconInfo.iconWidth = icnsImage.imageWidth;
	iconInfo.iconHeight = icnsImage.imageHeight;
	iconInfo.iconBitDepth = bpp;
	iconInfo.iconChannels = (bpp == 32 ? 4 : 1);
	iconInfo.iconPixelDepth = bpp / iconInfo.iconChannels;

	icns_type_t iconType = icns_get_type_from_image_info(iconInfo);

	if (iconType == ICNS_NULL_TYPE) {
		delete[] bits;
		free(fIconFamily);
		fIconFamily = NULL;
		return;
	}

	icns_element_t *iconElement = NULL;
	int icnsErr = icns_new_element_from_image(&icnsImage, iconType,
		&iconElement);

	if (iconElement != NULL) {
		if (icnsErr == ICNS_STATUS_OK) {
			icns_set_element_in_family(&fIconFamily, iconElement);
			fCreated = true;
		}
		free(iconElement);
	}

	if (iconType != ICNS_1024x1024_32BIT_ARGB_DATA
		&& iconType != ICNS_512x512_32BIT_ARGB_DATA
		&& iconType != ICNS_256x256_32BIT_ARGB_DATA) {
		icns_type_t maskType =
			icns_get_mask_type_for_icon_type(iconType);

		icns_image_t icnsMask;
		icns_init_image_for_type(maskType, &icnsMask);

		uint32 iconDataOffset = 0;
		uint32 maskDataOffset = 0;

		while (iconDataOffset < icnsImage.imageDataSize
			&& maskDataOffset < icnsMask.imageDataSize) {
			icnsMask.imageData[maskDataOffset] =
				icnsImage.imageData[iconDataOffset + 3];
			iconDataOffset += 4;
			maskDataOffset += 1;
		}

		icns_element_t *maskElement = NULL;
		icnsErr = icns_new_element_from_mask(&icnsMask, maskType,
			&maskElement);

		if (maskElement != NULL) {
			if (icnsErr == ICNS_STATUS_OK) {
				icns_set_element_in_family(&fIconFamily,
					maskElement);
			} else
				fCreated = false;
			free(maskElement);
		}
		icns_free_image(&icnsMask);
	}

	if (!fCreated) {
		free(fIconFamily);
		fIconFamily = NULL;
	}

	delete[] bits;
}


ICNSSaver::~ICNSSaver()
{
	if (fIconFamily != NULL)
		free(fIconFamily);
}


int
ICNSSaver::SaveData(BPositionIO *target)
{
	icns_size_t dataSize;
	icns_byte_t *dataPtrOut;
	icns_export_family_data(fIconFamily, &dataSize, &dataPtrOut);
	if (dataSize != 0 && dataPtrOut != NULL) {
		if (target->Write(dataPtrOut, dataSize) == dataSize)
			return B_OK;
	}
	return B_ERROR;
}


bool
ICNSSaver::IsCreated(void)
{
	return fCreated;
}
