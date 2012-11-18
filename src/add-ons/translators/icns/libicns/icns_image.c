/*
File:       icns_image.c
Copyright (C) 2001-2012 Mathew Eis <mathew@eisbox.net>
              2007 Thomas Lübking <thomas.luebking@web.de>
              2002 Chenxiao Zhao <chenxiao.zhao@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the
Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, 
Boston, MA 02110-1301, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "icns.h"
#include "icns_internals.h"
#include "icns_colormaps.h"


int icns_get_image32_with_mask_from_family(icns_family_t *iconFamily,icns_type_t iconType,icns_image_t *imageOut)
{
	int		error = ICNS_STATUS_OK;
	icns_type_t	maskType = ICNS_NULL_TYPE;
	icns_element_t	*iconElement = NULL;
	icns_element_t	*maskElement = NULL;
	icns_image_t	iconImage;
	icns_image_t	maskImage;
	unsigned long	dataCount = 0;
	icns_byte_t	dataValue = 0;
	unsigned long	pixelCount = 0;
	unsigned long	pixelID = 0;
	icns_byte_t	colorIndex = 0;	
	
	memset ( &iconImage, 0, sizeof(icns_image_t) );
	memset ( &maskImage, 0, sizeof(icns_image_t) );
	
	if(iconFamily == NULL)
	{
		icns_print_err("icns_get_image32_with_mask_from_family: Icon family is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	if(imageOut == NULL)
	{
		icns_print_err("icns_get_image32_with_mask_from_family: Icon image is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	else
	{
		icns_free_image(imageOut);
	}
	
	#ifdef ICNS_DEBUG
	{
		char typeStr[5];
		printf("Making 32-bit image...\n");
		printf("  using icon type '%s'\n",icns_type_str(iconType,typeStr));
	}
	#endif
	
	
	if((iconType == ICNS_128X128_8BIT_MASK) || \
	(iconType == ICNS_48x48_8BIT_MASK) || \
	(iconType == ICNS_32x32_8BIT_MASK) || \
	(iconType == ICNS_16x16_8BIT_MASK) )
	{
		icns_print_err("icns_get_image32_with_mask_from_family: Can't make an image with mask from a mask\n");
		return ICNS_STATUS_INVALID_DATA;
	}
	
	// Preliminaries checked - carry on with the icon/mask merge
	
	// Load icon element then image
	error = icns_get_element_from_family(iconFamily,iconType,&iconElement);
	
	if(error) {
		icns_print_err("icns_get_image32_with_mask_from_family: Unable to load icon element from icon family!\n");
		goto cleanup;
	}
	
	error = icns_get_image_from_element(iconElement,&iconImage);
	
	if(error) {
		icns_print_err("icns_get_image32_with_mask_from_family: Unable to load icon image data from icon element!\n");
		goto cleanup;
	}
	
	// We used the jp2 processor for these two, so we're done!
	if( (iconType == ICNS_256x256_32BIT_ARGB_DATA) || (iconType == ICNS_512x512_32BIT_ARGB_DATA) || (iconType == ICNS_1024x1024_32BIT_ARGB_DATA) ) {
		memcpy(imageOut,&iconImage,sizeof(icns_image_t));
		if(iconElement != NULL) {
		free(iconElement);
		iconElement = NULL;
		}
		return error;
	}
	
	maskType = icns_get_mask_type_for_icon_type(iconType);
	
	#ifdef ICNS_DEBUG
	{
		char typeStr[5];
		printf("  using mask type '%s'\n",icns_type_str(maskType,typeStr));
	}
	#endif
	
	if( (maskType == ICNS_NULL_DATA) )
	{
		char typeStr[5];
		icns_print_err("icns_get_image32_with_mask_from_family: Can't find mask for type '%s'\n",icns_type_str(iconType,typeStr));
		return ICNS_STATUS_DATA_NOT_FOUND;
	}
	
	// Load mask element then image...
	error = icns_get_element_from_family(iconFamily,maskType,&maskElement);
	
	// Note that we could arguably recover from not having a mask
	// by creating a dummy blank mask. However, the icns data type
	// should always have the corresponding mask present. This
	// function was designed to retreive a VALID image... There are
	// other API functions better used if the goal is editing, data
	// recovery, etc.
	if(error) {
		icns_print_err("icns_get_image32_with_mask_from_family: Unable to load mask element from icon family!\n");
		goto cleanup;
	}
	
	error = icns_get_mask_from_element(maskElement,&maskImage);
	
	if(error) {
		icns_print_err("icns_get_image32_with_mask_from_family: Unable to load mask image data from icon element!\n");
		goto cleanup;
	}
	
	if(iconImage.imageWidth != maskImage.imageWidth) {
		icns_print_err("icns_get_image32_with_mask_from_family: icon and mask widths do not match! (%d != %d)\n",iconImage.imageWidth,maskImage.imageHeight);
		goto cleanup;
	}
	
	if(iconImage.imageHeight != maskImage.imageHeight) {
		icns_print_err("icns_get_image32_with_mask_from_family: icon and mask heights do not match! (%d != %d)\n",iconImage.imageHeight,maskImage.imageHeight);
		goto cleanup;
	}
	
	// Unpack image pixels if depth is < 32
	if((iconImage.imagePixelDepth * iconImage.imageChannels) < 32)
	{
		icns_byte_t	*oldData = NULL;
		icns_byte_t	*newData = NULL;
		icns_uint32_t	oldBitDepth = 0;
		unsigned long	newBlockSize = 0;
		unsigned long	newDataSize = 0;
		icns_colormap_rgb_t	colorRGB;
		
		oldBitDepth = (iconImage.imagePixelDepth * iconImage.imageChannels);
		
		pixelCount = iconImage.imageWidth * iconImage.imageHeight;
		
		newBlockSize = iconImage.imageWidth * 32;
		newDataSize = newBlockSize * iconImage.imageHeight;
		
		oldData = iconImage.imageData;
		newData = (icns_byte_t *)malloc(newDataSize);
		
		if(newData == NULL)
		{
			icns_print_err("icns_get_image32_with_mask_from_family: Unable to allocate memory block of size: %d!\n",(int)newDataSize);
			return ICNS_STATUS_NO_MEMORY;
		}
		
		dataCount = 0;
		
		// 8-Bit Icon Image Data Types
		if((iconType == ICNS_48x48_8BIT_DATA) || \
		(iconType == ICNS_32x32_8BIT_DATA) || \
		(iconType == ICNS_16x16_8BIT_DATA) || \
		(iconType == ICNS_16x12_8BIT_DATA) )
		{
			if(oldBitDepth != 8)
			{
				icns_print_err("icns_get_image32_with_mask_from_family: Invalid bit depth - type mismatch!\n");
				free(newData);
				error = ICNS_STATUS_INVALID_DATA;
				goto cleanup;
			}
			for(pixelID = 0; pixelID < pixelCount; pixelID++)
			{
				colorIndex = oldData[dataCount++];
				colorRGB = icns_colormap_8[colorIndex];
				newData[pixelID * 4 + 0] = colorRGB.r;
				newData[pixelID * 4 + 1] = colorRGB.g;
				newData[pixelID * 4 + 2] = colorRGB.b;
				newData[pixelID * 4 + 3] = 0xff;
			}
		}
		// 4-Bit Icon Image Data Types
		else if((iconType == ICNS_48x48_4BIT_DATA) || \
		(iconType == ICNS_32x32_4BIT_DATA) || \
		(iconType == ICNS_16x16_4BIT_DATA) || \
		(iconType == ICNS_16x12_4BIT_DATA) )
		{
			if(oldBitDepth != 4)
			{
				icns_print_err("icns_get_image32_with_mask_from_family: Invalid bit depth - type mismatch!\n");
				free(newData);
				error = ICNS_STATUS_INVALID_DATA;
				goto cleanup;
			}
			for(pixelID = 0; pixelID < pixelCount; pixelID++)
			{
				if(pixelID % 2 == 0)
					dataValue = oldData[dataCount++];
				colorIndex = (dataValue & 0xF0) >> 4;
				dataValue = dataValue << 4;
				colorRGB = icns_colormap_4[colorIndex];
				newData[pixelID * 4 + 0] = colorRGB.r;
				newData[pixelID * 4 + 1] = colorRGB.g;
				newData[pixelID * 4 + 2] = colorRGB.b;
				newData[pixelID * 4 + 3] = 0xFF;
			}
		}
		// 1-Bit Icon Image Data Types
		else if((iconType == ICNS_48x48_1BIT_DATA) || \
		(iconType == ICNS_32x32_1BIT_DATA) || \
		(iconType == ICNS_16x16_1BIT_DATA) || \
		(iconType == ICNS_16x12_1BIT_DATA) )
		{
			if(oldBitDepth != 1)
			{
				icns_print_err("icns_get_image32_with_mask_from_family: Invalid bit depth - type mismatch!\n");
				free(newData);
				error = ICNS_STATUS_INVALID_DATA;
				goto cleanup;
			}
			for(pixelID = 0; pixelID < pixelCount; pixelID++)
			{
				if(pixelID % 8 == 0)
					dataValue = oldData[dataCount++];
				colorIndex = (dataValue & 0x80) ? 0x00 : 0xFF;
				dataValue = dataValue << 1;
				newData[pixelID * 4 + 0] = colorIndex;
				newData[pixelID * 4 + 1] = colorIndex;
				newData[pixelID * 4 + 2] = colorIndex;
				newData[pixelID * 4 + 3] = 0xFF;
			}
		}
		else
		{
			char typeStr[5];
			icns_print_err("icns_get_image32_with_mask_from_family: Unpack error - unknown icon type! ('%s')\n",icns_type_str(iconType,typeStr));
			free(newData);
			error = ICNS_STATUS_INVALID_DATA;
			goto cleanup;
		}
		
		iconImage.imagePixelDepth = 8;
		iconImage.imageChannels = 4;
		iconImage.imageDataSize = newDataSize;
		iconImage.imageData = newData;
		
		free(oldData);
	}
	
	// 8-Bit Icon Mask Data Types
	if((maskType == ICNS_128X128_8BIT_MASK) || \
	(maskType == ICNS_48x48_8BIT_MASK) || \
	(maskType == ICNS_32x32_8BIT_MASK) || \
	(maskType == ICNS_16x16_8BIT_MASK) )
	{
		pixelCount = maskImage.imageWidth * maskImage.imageHeight;
		dataCount = 0;
		if((maskImage.imagePixelDepth * maskImage.imageChannels) != 8)
		{
			icns_print_err("icns_get_image32_with_mask_from_family: Invalid bit depth - mismatch!\n");
			error = ICNS_STATUS_INVALID_DATA;
			goto cleanup;
		}
		for(pixelID = 0; pixelID < pixelCount; pixelID++)
		{
			iconImage.imageData[pixelID * 4 + 3] = maskImage.imageData[dataCount++];
		}
	}
	// 1-Bit Icon Mask Data Types
	else if((maskType == ICNS_48x48_1BIT_MASK) || \
	(maskType == ICNS_32x32_1BIT_MASK) || \
	(maskType == ICNS_16x16_1BIT_MASK) || \
	(maskType == ICNS_16x12_1BIT_MASK) )
	{
		pixelCount = maskImage.imageWidth * maskImage.imageHeight;
		dataCount = 0;
		if((maskImage.imagePixelDepth * maskImage.imageChannels) != 1)
		{
			icns_print_err("icns_get_image32_with_mask_from_family: Invalid bit depth - mismatch!\n");
			error = ICNS_STATUS_INVALID_DATA;
			goto cleanup;
		}
		for(pixelID = 0; pixelID < pixelCount; pixelID++)
		{
			if(pixelID % 8 == 0)
				dataValue = maskImage.imageData[dataCount++];
			colorIndex = (dataValue & 0x80) ? 0xFF : 0x00;
			dataValue = dataValue << 1;
			iconImage.imageData[pixelID * 4 + 3] = colorIndex;
		}
	}
	else
	{		
		char typeStr[5];
		icns_print_err("icns_get_image32_with_mask_from_family: Unpack error - unknown mask type! ('%s')\n",icns_type_str(maskType,typeStr));
		error = ICNS_STATUS_INVALID_DATA;
		goto cleanup;
	}
	
cleanup:
	
	if(maskElement != NULL) {
		free(maskElement);
		maskElement = NULL;
	}
	icns_free_image(&maskImage);
	
	if(iconElement != NULL) {
		free(iconElement);
		iconElement = NULL;
	}
	
	// We only free the icon image if there was an error. Otherwise, we
	// pass the data onto the outgoing image
	if(error) {
		icns_free_image(&iconImage);
	} else {
		memcpy(imageOut,&iconImage,sizeof(icns_image_t));
		#ifdef ICNS_DEBUG
		printf("Finished 32-bit image...\n");
		printf("  height: %d\n",imageOut->imageHeight);
		printf("  width: %d\n",imageOut->imageHeight);
		printf("  channels: %d\n",imageOut->imageChannels);
		printf("  pixel depth: %d\n",imageOut->imagePixelDepth);
		printf("  data size: %d\n",(int)(imageOut->imageDataSize));
		#endif
	}
	
	return error;
}


//***************************** icns_get_image_from_element **************************//
// Actual conversion of the icon data into uncompressed raw pixels

int icns_get_image_from_element(icns_element_t *iconElement,icns_image_t *imageOut)
{
	int		error = ICNS_STATUS_OK;
	unsigned long	dataCount = 0;
	icns_type_t	elementType = ICNS_NULL_TYPE;
	icns_size_t	elementSize = 0;
	icns_type_t	iconType = ICNS_NULL_TYPE;
	unsigned long	rawDataSize = 0;
	icns_byte_t	*rawDataPtr = NULL;
	icns_uint32_t	iconBitDepth = 0;
	unsigned long	iconDataRowSize = 0;
	
	if(iconElement == NULL)
	{
		icns_print_err("icns_get_image_from_element: Icon element is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	if(imageOut == NULL)
	{
		icns_print_err("icns_get_image_from_element: Icon image structure is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	elementType = iconElement->elementType;
	elementSize = iconElement->elementSize;
	
	#if ICNS_DEBUG
	{
		char typeStr[5];
		printf("Retreiving image from icon element...\n");
		printf("  type is: '%s'\n",icns_type_str(elementType,typeStr));
		printf("  size is: %d\n",(int)elementSize);
	}	
	#endif
	
	if(elementSize <= 8)
	{
		icns_print_err("icns_get_image_from_element: Invalid element size! (%d)\n",elementSize);
		return ICNS_STATUS_INVALID_DATA;
	}
	
	iconType = elementType;
	rawDataSize = elementSize - sizeof(icns_type_t) - sizeof(icns_size_t);
	rawDataPtr = (icns_byte_t*)&(iconElement->elementData[0]);
	
	#if ICNS_DEBUG
	printf("  data size is: %d\n",(int)rawDataSize);
	#endif
	
	switch(iconType)
	{
		// 32-Bit Icon Image Data Types ( > 256px )
		case ICNS_256x256_32BIT_ARGB_DATA:
		case ICNS_512x512_32BIT_ARGB_DATA:
		case ICNS_1024x1024_32BIT_ARGB_DATA:
			{
				uint8_t magicPNG[] = {0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A};
				uint8_t magicByt[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; 
			
				ICNS_READ_UNALIGNED(magicByt[0], rawDataPtr, 8);
				
				// 256x256+ sizes may or may not be PNG dta as of 10.7 Lion, so check
				if(memcmp(&magicByt[0], &magicPNG[0], 8) == 0) {
					// We know to use the PNG processor
					error = icns_png_to_image((int)rawDataSize, (icns_byte_t *)rawDataPtr, imageOut);
				} else {
					// We assume use of the jp2 processor
					error = icns_jp2_to_image((int)rawDataSize, (icns_byte_t *)rawDataPtr, imageOut);
				}
				return error;
			}
		case ICNS_128X128_32BIT_DATA:
		case ICNS_48x48_32BIT_DATA:
		case ICNS_32x32_32BIT_DATA:
		case ICNS_16x16_32BIT_DATA:

			error = icns_init_image_for_type(iconType,imageOut);
			if(error)
			{
				icns_print_err("icns_get_image_from_element: Error allocating new icns image!\n");
				icns_free_image(imageOut);
				return error;
			}
			
			iconBitDepth = imageOut->imagePixelDepth * imageOut->imageChannels;
			iconDataRowSize = imageOut->imageWidth * iconBitDepth / ICNS_BYTE_BITS;
			
			if(rawDataSize < imageOut->imageDataSize)
			{
				icns_uint32_t	pixelCount = 0;
				icns_size_t	decodedDataSize = imageOut->imageDataSize;
				icns_byte_t	*decodedImageData = imageOut->imageData;
	
				pixelCount = imageOut->imageWidth * imageOut->imageHeight;
				error = icns_decode_rle24_data(rawDataSize,rawDataPtr,pixelCount,&decodedDataSize,&decodedImageData);
				if(error)
				{
					icns_print_err("icns_get_image_from_element: Error decoding RLE data!\n");
					icns_free_image(imageOut);
					return error;
				}
				else
				{
					imageOut->imageDataSize = decodedDataSize;
					imageOut->imageData = decodedImageData;
				}
			}
			else
			{
				unsigned long	pixelCount = 0;
				icns_byte_t	*swapPtr = NULL;
				icns_argb_t	*pixelPtr = NULL;
	
				for(dataCount = 0; dataCount < imageOut->imageHeight; dataCount++)
					memcpy(&(((char*)(imageOut->imageData))[dataCount*iconDataRowSize]),&(((char*)(rawDataPtr))[dataCount*iconDataRowSize]),iconDataRowSize);
				
				pixelCount = imageOut->imageWidth * imageOut->imageHeight;
				#ifdef ICNS_DEBUG
					printf("Converting %d pixels from argb to rgba\n",(int)pixelCount);
				#endif
				swapPtr = imageOut->imageData;
				for(dataCount = 0; dataCount < pixelCount; dataCount++)
				{
					pixelPtr = (icns_argb_t *)(swapPtr + (dataCount * 4));
					*((icns_rgba_t *)pixelPtr) = ICNS_ARGB_TO_RGBA( *((icns_argb_t *)pixelPtr) );
				}
			}
			break;
		case ICNS_48x48_8BIT_DATA:
		case ICNS_32x32_8BIT_DATA:
		case ICNS_16x16_8BIT_DATA:
		case ICNS_16x12_8BIT_DATA:
		case ICNS_48x48_4BIT_DATA:
		case ICNS_32x32_4BIT_DATA:
		case ICNS_16x16_4BIT_DATA:
		case ICNS_16x12_4BIT_DATA:
		case ICNS_48x48_1BIT_DATA:
		case ICNS_32x32_1BIT_DATA:
		case ICNS_16x16_1BIT_DATA:
		case ICNS_16x12_1BIT_DATA:

			error = icns_init_image_for_type(iconType,imageOut);
			if(error)
			{
				icns_print_err("icns_get_image_from_element: Error allocating new icns image!\n");
				icns_free_image(imageOut);
				return error;
			}
			
			iconBitDepth = imageOut->imagePixelDepth * imageOut->imageChannels;
			iconDataRowSize = imageOut->imageWidth * iconBitDepth / ICNS_BYTE_BITS;
			
			for(dataCount = 0; dataCount < imageOut->imageHeight; dataCount++)
				memcpy(&(((char*)(imageOut->imageData))[dataCount*iconDataRowSize]),&(((char*)(rawDataPtr))[dataCount*iconDataRowSize]),iconDataRowSize);
			break;
		default:
			{
				char typeStr[5];
				icns_print_err("icns_get_image_from_element: Unknown icon type! ('%s')\n",icns_type_str(iconType,typeStr));
				icns_free_image(imageOut);
			}
			return ICNS_STATUS_INVALID_DATA;
	}
	
	return error;
}

//***************************** icns_get_mask_from_element **************************//
// Actual conversion of the mask data into uncompressed raw pixels

int icns_get_mask_from_element(icns_element_t *maskElement,icns_image_t *imageOut)
{
	int		error = ICNS_STATUS_OK;
	unsigned long	dataCount = 0;
	icns_type_t	elementType = ICNS_NULL_TYPE;
	icns_size_t	elementSize = 0;
	icns_type_t	maskType = ICNS_NULL_TYPE;
	unsigned long	rawDataSize = 0;
	icns_byte_t	*rawDataPtr = NULL;
	icns_uint32_t	maskBitDepth = 0;
	unsigned long	maskDataSize = 0;
	unsigned long	maskDataRowSize = 0;
			
	if(maskElement == NULL)
	{
		icns_print_err("icns_get_mask_from_element: Mask element is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	if(imageOut == NULL)
	{
		icns_print_err("icns_get_mask_from_element: Mask image structure is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	elementType = maskElement->elementType;
	elementSize = maskElement->elementSize;
	
	#if ICNS_DEBUG
	{
		char typeStr[5];
		printf("Retreiving image from mask element...\n");
		printf("  type is: '%s'\n",icns_type_str(elementType,typeStr));
		printf("  size is: %d\n",(int)elementSize);
	}
	#endif
	
	if(elementSize <= 8)
	{
		icns_print_err("icns_get_image_from_element: Invalid element size! (%d)\n",elementSize);
		return ICNS_STATUS_INVALID_DATA;
	}
	
	maskType = elementType;
	rawDataSize = elementSize - sizeof(icns_type_t) - sizeof(icns_size_t);
	rawDataPtr = (icns_byte_t*)&(maskElement->elementData[0]);
	
	#if ICNS_DEBUG
	printf("  data size is: %d\n",(int)rawDataSize);	
	#endif

	switch(maskType)
	{
		case ICNS_128X128_8BIT_MASK:
		case ICNS_48x48_8BIT_MASK:
		case ICNS_32x32_8BIT_MASK:
		case ICNS_16x16_8BIT_MASK:

			error = icns_init_image_for_type(maskType,imageOut);
			if(error)
			{
				icns_print_err("icns_get_mask_from_element: Error allocating new icns image!\n");
				icns_free_image(imageOut);
				return error;
			}
			
			maskBitDepth = imageOut->imagePixelDepth * imageOut->imageChannels;
			
			if(maskBitDepth != 8) {
				icns_print_err("icns_get_mask_from_element: Unknown bit depth!\n");
				icns_free_image(imageOut);
				return ICNS_STATUS_INVALID_DATA;
			}
			
			maskDataSize = imageOut->imageDataSize;
			maskDataRowSize = imageOut->imageWidth * maskBitDepth / ICNS_BYTE_BITS;
			
			for(dataCount = 0; dataCount < imageOut->imageHeight; dataCount++)
				memcpy(&(((char*)(imageOut->imageData))[dataCount*maskDataRowSize]),&(((char*)(rawDataPtr))[dataCount*maskDataRowSize]),maskDataRowSize);

			break;
		case ICNS_48x48_1BIT_MASK:
		case ICNS_32x32_1BIT_MASK:
		case ICNS_16x16_1BIT_MASK:
		case ICNS_16x12_1BIT_MASK:

			error = icns_init_image_for_type(maskType,imageOut);
			if(error)
			{
				icns_print_err("icns_get_mask_from_element: Error allocating new icns image!\n");
				icns_free_image(imageOut);
				return error;
			}
	
			maskBitDepth = imageOut->imagePixelDepth * imageOut->imageChannels;
			
			if(maskBitDepth != 1) {
				icns_print_err("icns_get_mask_from_element: Unknown bit depth!\n");
				icns_free_image(imageOut);
				return ICNS_STATUS_INVALID_DATA;
			}
			
			maskDataSize = imageOut->imageDataSize;
			maskDataRowSize = imageOut->imageWidth * maskBitDepth / ICNS_BYTE_BITS;
			
			#if ICNS_DEBUG
			printf("  raw mask data size is: %d\n",(int)rawDataSize);	
			printf("  image mask data size is: %d\n",(int)maskDataSize);	
			#endif
	
			if(rawDataSize == (maskDataSize * 2) )
			{
				#if ICNS_DEBUG
				printf("  mask data in second memory block\n");	
				#endif					
				// Mask data found - Copy the second block of memory
				for(dataCount = 0; dataCount < imageOut->imageHeight; dataCount++)
					memcpy(&(((char*)(imageOut->imageData))[dataCount*maskDataRowSize]),&(((char*)(rawDataPtr))[dataCount*maskDataRowSize+maskDataSize]),maskDataRowSize);
			}
			else
			{
				#if ICNS_DEBUG
				printf("  using icon data from first memory block\n");	
				#endif					
				// Hmm, no mask - copy the first block of memory
				for(dataCount = 0; dataCount < imageOut->imageHeight; dataCount++)
					memcpy(&(((char*)(imageOut->imageData))[dataCount*maskDataRowSize]),&(((char*)(rawDataPtr))[dataCount*maskDataRowSize]),maskDataRowSize);
			}
			
			break;
		default:
			{
				char typeStr[5];
				icns_print_err("icns_get_mask_from_element: Unknown mask type! ('%s')\n",icns_type_str(maskType,typeStr));
				icns_free_image(imageOut);
			}
			return ICNS_STATUS_INVALID_DATA;
	}
	
	return error;
}



//***************************** icns_init_image_for_type **************************//
// Initialize a new image structure for holding the data
// using the information for the specified type

int icns_init_image_for_type(icns_type_t iconType,icns_image_t *imageOut)
{
	icns_icon_info_t iconInfo;
	
	if(imageOut == NULL)
	{
		icns_print_err("icns_init_image_for_type: Icon image structure is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	// Determine what the height and width ought to be, to check the incoming image
	
	iconInfo = icns_get_image_info_for_type(iconType);
	
	if(iconType != iconInfo.iconType)
	{
		char typeStr[5];
		icns_print_err("icns_init_image_for_type: Couldn't determine information for type! ('%s')\n",icns_type_str(iconType,typeStr));
		return ICNS_STATUS_INVALID_DATA;
	}
	
	return icns_init_image(iconInfo.iconWidth,iconInfo.iconHeight,iconInfo.iconChannels,iconInfo.iconPixelDepth,imageOut);

}

//***************************** icns_init_image **************************//
// Initialize a new image structure for holding the data

int icns_init_image(icns_uint32_t iconWidth,icns_uint32_t iconHeight,icns_uint32_t iconChannels,icns_uint32_t iconPixelDepth,icns_image_t *imageOut)
{
	icns_uint32_t	iconBitDepth = 0;
	unsigned long	iconDataSize = 0;
	unsigned long	iconDataRowSize = 0;

	if(imageOut == NULL)
	{
		icns_print_err("icns_init_image: Icon image structure is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}

	if(iconWidth == 0)
	{
		icns_print_err("icns_init_image: icon width is 0!\n");
		return ICNS_STATUS_INVALID_DATA;
	}

	if(iconHeight == 0)
	{
		icns_print_err("icns_init_image: icon height is 0!\n");
		return ICNS_STATUS_INVALID_DATA;
	}

	if(iconChannels == 0)
	{
		icns_print_err("icns_init_image: icon channels is 0!\n");
		return ICNS_STATUS_INVALID_DATA;
	}

	if(iconPixelDepth == 0)
	{
		icns_print_err("icns_init_image: icon pixel depth is 0!\n");
		return ICNS_STATUS_INVALID_DATA;
	}

	iconBitDepth = iconPixelDepth * iconChannels;
	iconDataRowSize = iconWidth * iconBitDepth / ICNS_BYTE_BITS;
	iconDataSize = iconHeight * iconDataRowSize;
	
	#ifdef ICNS_DEBUG
	printf("Initializing new image...\n");
	printf("  width is: %d\n",iconWidth);
	printf("  height is: %d\n",iconHeight);
	printf("  channels are: %d\n",iconChannels);
	printf("  bit depth is: %d\n",iconBitDepth);
	printf("  data size is: %d\n",(int)iconDataSize);
	#endif
	
	imageOut->imageWidth = iconWidth;
	imageOut->imageHeight = iconHeight;
	imageOut->imageChannels = iconChannels;
	imageOut->imagePixelDepth = (iconBitDepth / iconChannels);
	imageOut->imageDataSize = iconDataSize;
	imageOut->imageData = (icns_byte_t *)malloc(iconDataSize);
	if(!imageOut->imageData)
	{
		icns_print_err("icns_init_image: Unable to allocate memory block of size: %d ($s:%m)!\n",(int)iconDataSize);
		return ICNS_STATUS_NO_MEMORY;
	}
	memset(imageOut->imageData,0,iconDataSize);
	
	return ICNS_STATUS_OK;
}

//***************************** icns_free_image **************************//
// Deallocate data in an image structure

int icns_free_image(icns_image_t *imageIn)
{
	imageIn->imageWidth = 0;
	imageIn->imageHeight = 0;
	imageIn->imageChannels = 0;
	imageIn->imagePixelDepth = 0;
	imageIn->imageDataSize = 0;
	
	if(imageIn->imageData != NULL)
	{
		free(imageIn->imageData);
		imageIn->imageData = NULL;
	}
	
	return ICNS_STATUS_OK;
}

