/*
File:       icns_element.c
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


//***************************** icns_get_element_from_family **************************//
// Parses requested data from an icon family - puts it into a icon element

int icns_get_element_from_family(icns_family_t *iconFamily,icns_type_t iconType,icns_element_t **iconElementOut)
{
	int		error = ICNS_STATUS_OK;
	int		foundData = 0;
	icns_type_t	iconFamilyType = ICNS_NULL_TYPE;
	icns_size_t	iconFamilySize = 0;
	icns_element_t	*iconElement = NULL;
	icns_type_t	elementType = ICNS_NULL_TYPE;
	icns_size_t	elementSize = 0;
	icns_uint32_t	dataOffset = 0;
	
	if(iconFamily == NULL)
	{
		icns_print_err("icns_get_element_from_family: icns family is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	if(iconElementOut == NULL)
	{
		icns_print_err("icns_get_element_in_family: icns element out is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	else
	{
		*iconElementOut = NULL;
	}
	
	if(iconFamily->resourceType != ICNS_FAMILY_TYPE)
	{
		icns_print_err("icns_get_element_from_family: Invalid icns family!\n");
		return ICNS_STATUS_INVALID_DATA;
	}
	
	ICNS_READ_UNALIGNED(iconFamilyType, &(iconFamily->resourceType),sizeof( icns_type_t));
	ICNS_READ_UNALIGNED(iconFamilySize, &(iconFamily->resourceSize),sizeof( icns_size_t));
	
	#ifdef ICNS_DEBUG
	{
		char typeStr[5];
		printf("Looking for icon element of type: '%s'\n",icns_type_str(iconType,typeStr));
		printf("  icon family type check: '%s'\n",icns_type_str(iconFamilyType,typeStr));
		printf("  icon family size check: %d\n",iconFamilySize);
	}
	#endif
	
	dataOffset = sizeof(icns_type_t) + sizeof(icns_size_t);
	
	while ( (foundData == 0) && (dataOffset < iconFamilySize) )
	{
		iconElement = ((icns_element_t*)(((icns_byte_t*)iconFamily)+dataOffset));
		
		if( iconFamilySize < (dataOffset+sizeof(icns_type_t)+sizeof(icns_size_t)) )
		{
			icns_print_err("icns_get_element_from_family: Corrupted icns family!\n");
			return ICNS_STATUS_INVALID_DATA;		
		}
		
		ICNS_READ_UNALIGNED(elementType, &(iconElement->elementType),sizeof( icns_type_t));
		ICNS_READ_UNALIGNED(elementSize, &(iconElement->elementSize),sizeof( icns_size_t));
		
		#ifdef ICNS_DEBUG
		{
			char typeStr[5];
			printf("element data...\n");
			printf("  type: '%s'%s\n",icns_type_str(elementType,typeStr),(elementType == iconType) ? " - match!" : " ");
			printf("  size: %d\n",(int)elementSize);
		}
		#endif
		
		if( (elementSize < 8) || ((dataOffset+elementSize) > iconFamilySize) )
		{
			icns_print_err("icns_get_element_from_family: Invalid element size! (%d)\n",elementSize);
			return ICNS_STATUS_INVALID_DATA;
		}
		
		if(elementType == iconType)
			foundData = 1;
		else
			dataOffset += elementSize;
	}
	
	if(foundData)
	{
		*iconElementOut = malloc(elementSize);
		if(*iconElementOut == NULL)
		{
			icns_print_err("icns_get_element_from_family: Unable to allocate memory block of size: %d!\n",elementSize);
			return ICNS_STATUS_NO_MEMORY;
		}
		memcpy( *iconElementOut, iconElement, elementSize);
	}
	else
	{
		icns_print_err("icns_get_element_from_family: Unable to find requested icon data!\n");
		error = ICNS_STATUS_DATA_NOT_FOUND;
	}
	
	return error;
}

//***************************** icns_set_element_in_family **************************//
// Adds/updates the icns element of it's type in the icon family

int icns_set_element_in_family(icns_family_t **iconFamilyRef,icns_element_t *newIconElement)
{
	int		error = ICNS_STATUS_OK;
	int		foundData = 0;
	int		copiedData = 0;
	icns_family_t	*iconFamily = NULL;
	icns_type_t	iconFamilyType = ICNS_NULL_TYPE;
	icns_size_t	iconFamilySize = 0;
	icns_element_t	*iconElement = NULL;
	icns_type_t	newElementType = ICNS_NULL_TYPE;
	icns_size_t	newElementSize = 0;
	icns_type_t	elementType = ICNS_NULL_TYPE;
	icns_size_t	elementSize = 0;
	icns_uint32_t	dataOffset = 0;
	icns_size_t	newIconFamilySize = 0;
	icns_family_t	*newIconFamily = NULL;
	icns_uint32_t	newDataOffset = 0;
	icns_uint32_t	elementOrder = 0;
	icns_uint32_t   newElementOrder = 0;

	
	if(iconFamilyRef == NULL)
	{
		icns_print_err("icns_set_element_in_family: icns family reference is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	iconFamily = *iconFamilyRef;
	
	if(iconFamily == NULL)
	{
		icns_print_err("icns_set_element_in_family: icns family is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	#ifdef ICNS_DEBUG
	printf("Setting element in icon family...\n");
	#endif
	
	if(iconFamily->resourceType != ICNS_FAMILY_TYPE)
	{
		icns_print_err("icns_set_element_in_family: Invalid icns family!\n");
		error = ICNS_STATUS_INVALID_DATA;
	}
	
	ICNS_READ_UNALIGNED(iconFamilyType, &(iconFamily->resourceType),sizeof( icns_type_t));
	ICNS_READ_UNALIGNED(iconFamilySize, &(iconFamily->resourceSize),sizeof( icns_size_t));
	
	#ifdef ICNS_DEBUG
	{
		char typeStr[5];
		printf("  family type '%s'\n",icns_type_str(iconFamilyType,typeStr));
		printf("  family size: %d (0x%08X)\n",(int)iconFamilySize,iconFamilySize);
	}
	#endif
	
	if(newIconElement == NULL)
	{
		icns_print_err("icns_set_element_in_family: icns element is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	// Retrieve first, then swap. May help with problems on some arch	
	ICNS_READ_UNALIGNED(newElementType, &(newIconElement->elementType),sizeof( icns_type_t));
	ICNS_READ_UNALIGNED(newElementSize, &(newIconElement->elementSize),sizeof( icns_size_t));
	
	#ifdef ICNS_DEBUG
	{
		char typeStr[5];
		printf("  element type '%s'\n",icns_type_str(newElementType,typeStr));
		printf("  element size: %d (0x%08X)\n",(int)newElementSize,newElementSize);
	}
	#endif
	
	dataOffset = sizeof(icns_type_t) + sizeof(icns_size_t);
	
	while ( (foundData == 0) && (dataOffset < iconFamilySize) )
	{
		iconElement = ((icns_element_t*)(((char*)iconFamily)+dataOffset));
		ICNS_READ_UNALIGNED(elementType, &(iconElement->elementType),sizeof( icns_type_t));
		ICNS_READ_UNALIGNED(elementSize, &(iconElement->elementSize),sizeof( icns_size_t));
		
		if(elementType == newElementType)
			foundData = 1;
		else
			dataOffset += elementSize;
	}
	
	if(foundData)
		newIconFamilySize = iconFamilySize - elementSize + newElementSize;
	else
		newIconFamilySize = iconFamilySize + newElementSize;
	
	#ifdef ICNS_DEBUG
	printf("  new family type 'icns'\n");
	printf("  new family size: %d (0x%08X)\n",(int)newIconFamilySize,newIconFamilySize);
	#endif
	
	newIconFamily = malloc(newIconFamilySize);
	
	if(newIconFamily == NULL)
	{
		icns_print_err("icns_set_element_in_family: Unable to allocate memory block of size: %d!\n",newIconFamilySize);
		return ICNS_STATUS_NO_MEMORY;
	}
	
	newIconFamily->resourceType = ICNS_FAMILY_TYPE;
	newIconFamily->resourceSize = newIconFamilySize;
	
	newDataOffset = sizeof(icns_type_t) + sizeof(icns_size_t);
	dataOffset = sizeof(icns_type_t) + sizeof(icns_size_t);
	
	copiedData = 0;
	
	newElementOrder = icns_get_element_order(newElementType);
	
	while ( dataOffset < iconFamilySize )
	{
		iconElement = ((icns_element_t*)(((char*)iconFamily)+dataOffset));
		ICNS_READ_UNALIGNED(elementType, &(iconElement->elementType),sizeof( icns_type_t));
		ICNS_READ_UNALIGNED(elementSize, &(iconElement->elementSize),sizeof( icns_size_t));
		elementOrder = icns_get_element_order(elementType);
		
		if(!copiedData && (elementType == newElementType))
		{
			memcpy( ((char *)(newIconFamily))+newDataOffset , (char *)newIconElement, newElementSize);
			newDataOffset += newElementSize;
			copiedData = 1;
		}
		else if(!copiedData && !foundData && newElementOrder < elementOrder)
		{
			memcpy( ((char *)(newIconFamily))+newDataOffset , (char *)newIconElement, newElementSize);
			newDataOffset += newElementSize;
			copiedData = 1;
			
			memcpy( ((char *)(newIconFamily))+newDataOffset , ((char *)(iconFamily))+dataOffset, elementSize);
			newDataOffset += elementSize;
		}
		else
		{
			memcpy( ((char *)(newIconFamily))+newDataOffset , ((char *)(iconFamily))+dataOffset, elementSize);
			newDataOffset += elementSize;
		}
		
		dataOffset += elementSize;
	}
	
	if(!copiedData)
	{
		memcpy( ((char *)(newIconFamily))+newDataOffset , (char *)newIconElement, newElementSize);
		newDataOffset += newElementSize;
	}
	
	*iconFamilyRef = newIconFamily;
	
	free(iconFamily);
	
	return error;
}

//***************************** icns_add_element_in_family **************************//
// Adds/updates the icns element of it's type in the icon family
// A convenience alias to icns_set_element_in_family

int icns_add_element_in_family(icns_family_t **iconFamilyRef,icns_element_t *newIconElement)
{
	if(iconFamilyRef == NULL)
	{
		icns_print_err("icns_add_element_in_family: icns family reference is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	if(newIconElement == NULL)
	{
		icns_print_err("icns_add_element_in_family: icon element is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	return icns_set_element_in_family(iconFamilyRef,newIconElement);
}

//***************************** icns_remove_element_in_family **************************//
// Parses requested data from an icon family - puts it into a "raw" image format

int icns_remove_element_in_family(icns_family_t **iconFamilyRef,icns_type_t iconElementType)
{
	int		error = ICNS_STATUS_OK;
	int		foundData = 0;
	icns_family_t	*iconFamily = NULL;
	icns_type_t	iconFamilyType = ICNS_NULL_TYPE;
	icns_size_t	iconFamilySize = 0;
	icns_element_t	*iconElement = NULL;
	icns_type_t	elementType = ICNS_NULL_TYPE;
	icns_size_t	elementSize = 0;
	icns_uint32_t	dataOffset = 0;

	icns_size_t	newIconFamilySize = 0;
	icns_family_t	*newIconFamily = NULL;
	icns_uint32_t	newDataOffset = 0;	
	
	if(iconFamilyRef == NULL)
	{
		icns_print_err("icns_remove_element_in_family: icon family reference is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	iconFamily = *iconFamilyRef;
	
	if(iconFamily == NULL)
	{
		icns_print_err("icns_remove_element_in_family: icon family is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	if(iconFamily->resourceType != ICNS_FAMILY_TYPE)
	{
		icns_print_err("icns_remove_element_in_family: Invalid icon family!\n");
		error = ICNS_STATUS_INVALID_DATA;
	}
	
	ICNS_READ_UNALIGNED(iconFamilyType, &(iconFamily->resourceType),sizeof( icns_type_t));
	ICNS_READ_UNALIGNED(iconFamilySize, &(iconFamily->resourceSize),sizeof( icns_size_t));
	
	dataOffset = sizeof(icns_type_t) + sizeof(icns_size_t);
	
	while ( (foundData == 0) && (dataOffset < iconFamilySize) )
	{
		iconElement = ((icns_element_t*)(((char*)iconFamily)+dataOffset));
		ICNS_READ_UNALIGNED(elementType, &(iconElement->elementType),sizeof( icns_type_t));
		ICNS_READ_UNALIGNED(elementSize, &(iconElement->elementSize),sizeof( icns_size_t));
		
		if(elementType == iconElementType)
			foundData = 1;
		else
			dataOffset += elementSize;
	}
	
	if(!foundData)
	{
		icns_print_err("icns_remove_element_in_family: Unable to find requested icon data for removal!\n");
		return ICNS_STATUS_DATA_NOT_FOUND;
	}

	
	newIconFamilySize = iconFamilySize - elementSize;
	newIconFamily = malloc(newIconFamilySize);
	
	if(newIconFamily == NULL)
	{
		icns_print_err("icns_remove_element_in_family: Unable to allocate memory block of size: %d!\n",newIconFamilySize);
		return ICNS_STATUS_NO_MEMORY;
	}
	
	newIconFamily->resourceType = ICNS_FAMILY_TYPE;
	newIconFamily->resourceSize = newIconFamilySize;
	
	newDataOffset = sizeof(icns_type_t) + sizeof(icns_size_t);
	dataOffset = sizeof(icns_type_t) + sizeof(icns_size_t);
	
	while ( dataOffset < iconFamilySize )
	{
		iconElement = ((icns_element_t*)(((char*)iconFamily)+dataOffset));
		ICNS_READ_UNALIGNED(elementType, &(iconElement->elementType),sizeof( icns_type_t));
		ICNS_READ_UNALIGNED(elementSize, &(iconElement->elementSize),sizeof( icns_size_t));
		
		if(elementType != iconElementType)
		{
			memcpy( ((char *)(newIconFamily))+newDataOffset , ((char *)(iconFamily))+dataOffset, elementSize);
			newDataOffset += elementSize;
		}
		
		dataOffset += elementSize;
	}
	
	*iconFamilyRef = newIconFamily;

	free(iconFamily);
	
	return error;
}



//***************************** icns_new_element_from_image **************************//
// Creates a new icon element from an image
int icns_new_element_from_image(icns_image_t *imageIn,icns_type_t iconType,icns_element_t **iconElementOut)
{
	return icns_new_element_from_image_or_mask(imageIn,iconType,0,iconElementOut);
}

//***************************** icns_new_element_from_mask **************************//
// Creates a new icon element from an image
int icns_new_element_from_mask(icns_image_t *imageIn,icns_type_t iconType,icns_element_t **iconElementOut)
{
	return icns_new_element_from_image_or_mask(imageIn,iconType,1,iconElementOut);
}


//***************************** icns_new_element_from_image_or_mask **************************//
// Creates a new icon element from an image
int icns_new_element_from_image_or_mask(icns_image_t *imageIn,icns_type_t iconType,icns_bool_t isMask,icns_element_t **iconElementOut)
{
	icns_element_t	*newElement = NULL;
	icns_size_t	newElementSize = 0;
	
	if(iconElementOut == NULL)
	{
		icns_print_err("icns_new_element_with_image_or_mask: Icon element reference is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	else
	{
		*iconElementOut = NULL;
	}
	
	newElementSize = sizeof(icns_type_t) + sizeof(icns_size_t);
	newElement = (icns_element_t *)malloc(newElementSize);
	if(newElement == NULL)
	{
		icns_print_err("icns_new_element_with_image_or_mask: Unable to allocate memory block of size: %d!\n",(int)newElementSize);
		return ICNS_STATUS_NO_MEMORY;
	}
	
	newElement->elementType = iconType;
	newElement->elementSize = newElementSize;
	*iconElementOut = newElement;
	
	return icns_update_element_with_image_or_mask(imageIn,isMask,iconElementOut);
}

//***************************** icns_update_element_from_image **************************//
// Updates an icon element with the given image
int icns_update_element_with_image(icns_image_t *imageIn,icns_element_t **iconElement)
{
	return icns_update_element_with_image_or_mask(imageIn,0,iconElement);
}

//***************************** icns_update_element_from_mask **************************//
// Updates an icon element with the given mask
int icns_update_element_with_mask(icns_image_t *imageIn,icns_element_t **iconElement)
{
	return icns_update_element_with_image_or_mask(imageIn,1,iconElement);
}

//***************************** icns_update_element_with_image_or_mask **************************//
// Updates an icon element with the given image or mask
int icns_update_element_with_image_or_mask(icns_image_t *imageIn,icns_bool_t isMask,icns_element_t **iconElement)
{
	int		        error = ICNS_STATUS_OK;
	icns_type_t             iconType;
	icns_icon_info_t 	iconInfo;

	// Finally, done with all the preliminary checks
	icns_size_t	imageDataSize = 0;
	icns_byte_t	*imageDataPtr = NULL;
	
	// For use to easily track deallocation if we use rle24, or jp2
	icns_size_t	newDataSize = 0;
	icns_byte_t	*newDataPtr = NULL;
	
	
	if(imageIn == NULL)
	{
		icns_print_err("icns_update_element_with_image_or_mask: Icon image is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	if(iconElement == NULL)
	{
		icns_print_err("icns_update_element_with_image_or_mask: Icon element reference is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	if(*iconElement == NULL)
	{
		icns_print_err("icns_update_element_with_image_or_mask: Icon element is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	iconType = (*iconElement)->elementType;
	
	if(iconType == ICNS_NULL_DATA) {
		icns_print_err("icns_update_element_with_image_or_mask: Invalid icon type!\n");
		return ICNS_STATUS_INVALID_DATA;
	}
	
	// Determine what the height and width ought to be, to check the incoming image
	iconInfo = icns_get_image_info_for_type(iconType);
	
	// Check the image width, height, and pixel depth
	
	if(imageIn->imageWidth != iconInfo.iconWidth)
	{
		icns_print_err("icns_update_element_with_image_or_mask: Invalid input image width: %d\n",imageIn->imageWidth);
		return ICNS_STATUS_INVALID_DATA;
	}
	
	if(imageIn->imageWidth != iconInfo.iconWidth)
	{
		icns_print_err("icns_update_element_with_image_or_mask: Invalid input image width: %d\n",imageIn->imageWidth);
		return ICNS_STATUS_INVALID_DATA;
	}
	
	if(imageIn->imageHeight != iconInfo.iconHeight)
	{
		icns_print_err("icns_update_element_with_image_or_mask: Invalid input image height: %d\n",imageIn->imageHeight);
		return ICNS_STATUS_INVALID_DATA;
	}
	
	if(imageIn->imagePixelDepth != (iconInfo.iconBitDepth/iconInfo.iconChannels))
	{
		icns_print_err("icns_update_element_with_image_or_mask: libicns does't support bit depth conversion yet.\n");
		return ICNS_STATUS_INVALID_DATA;
	}
	
	// Check the image data size and data pointer
	
	if(imageIn->imageDataSize == 0)
	{
		icns_print_err("icns_update_element_with_image_or_mask: Invalid input image data size: %d\n",(int)imageIn->imageDataSize);
		return ICNS_STATUS_INVALID_DATA;
	}
	
	if(imageIn->imageData == NULL)
	{
		icns_print_err("icns_update_element_with_image_or_mask: Invalid input image data\n");
		return ICNS_STATUS_INVALID_DATA;
	}
		
	switch(iconType)
	{
	case ICNS_1024x1024_32BIT_ARGB_DATA:
	case ICNS_256x256_32BIT_ARGB_DATA:
	case ICNS_512x512_32BIT_ARGB_DATA:
		error = icns_image_to_jp2(imageIn,&newDataSize,&newDataPtr);
		imageDataSize = newDataSize;
		imageDataPtr = newDataPtr;
		break;
	case ICNS_128X128_32BIT_DATA:
	case ICNS_48x48_32BIT_DATA:
	case ICNS_32x32_32BIT_DATA:
	case ICNS_16x16_32BIT_DATA:
		newDataSize = imageIn->imageDataSize;
		
		// Note: icns_encode_rle24_data allocates memory that must be freed later
		if( (error = icns_encode_rle24_data(imageIn->imageDataSize,imageIn->imageData,&newDataSize,&newDataPtr)) )
		{
			icns_print_err("icns_update_element_with_image_or_mask: Error rle encoding image data.\n");
			error = ICNS_STATUS_INVALID_DATA;
		}
		
		imageDataSize = newDataSize;
		imageDataPtr = newDataPtr;
		break;
	// Note that ICNS_NNxNN_1BIT_DATA == ICNS_NNxNN_1BIT_MASK
	// so we do not need to put them here twice
	case ICNS_48x48_1BIT_DATA:
	case ICNS_32x32_1BIT_DATA:
	case ICNS_16x16_1BIT_DATA:
	case ICNS_16x12_1BIT_DATA:
		{
			icns_byte_t	*existingData = NULL;
			icns_size_t	existingDataSize = 0;
			icns_size_t	existingDataOffset = 0;
			
			if(imageIn->imageDataSize != iconInfo.iconRawDataSize)
			{
				icns_print_err("icns_update_element_with_image_or_mask: Invalid input image data size: %d!\n",imageIn->imageDataSize);
				return ICNS_STATUS_INVALID_DATA;
			}
			
			newDataSize = iconInfo.iconRawDataSize * 2;
			newDataPtr = (icns_byte_t *)malloc(newDataSize);
			if(newDataPtr == NULL)
			{
				icns_print_err("icns_update_element_with_image_or_mask: Unable to allocate memory block of size: %d!\n",newDataSize);
				error = ICNS_STATUS_NO_MEMORY;
				goto exception;
			}
			
			memset (newDataPtr,0,newDataSize);
			
			existingData = (icns_byte_t*)(*iconElement);
			existingDataOffset = sizeof(icns_type_t) + sizeof(icns_size_t);
			existingDataSize = (*iconElement)->elementSize - existingDataOffset;
			
			// No icon data
			if(existingDataSize == 0)
			{
				if(isMask == 0) {
					memcpy(newDataPtr,imageIn->imageData,imageIn->imageDataSize);
				} else {
					memcpy(newDataPtr,imageIn->imageData + iconInfo.iconRawDataSize,imageIn->imageDataSize);
				}
			}
			// Only image data
			else if(existingDataSize == iconInfo.iconRawDataSize)
			{
				if(isMask == 0) {
					memcpy(newDataPtr,imageIn->imageData,imageIn->imageDataSize);
				} else {
					memcpy(newDataPtr,existingData+existingDataOffset,iconInfo.iconRawDataSize);
					memcpy(newDataPtr,imageIn->imageData + iconInfo.iconRawDataSize,imageIn->imageDataSize);
				}
			}
			// Image + Mask data
			else if(existingDataSize == newDataSize)
			{
				if(isMask == 0) {
					memcpy(newDataPtr,imageIn->imageData,imageIn->imageDataSize);
					memcpy(newDataPtr,existingData+existingDataOffset+iconInfo.iconRawDataSize,iconInfo.iconRawDataSize);
				} else {
					memcpy(newDataPtr,existingData+existingDataOffset,iconInfo.iconRawDataSize);
					memcpy(newDataPtr,imageIn->imageData + iconInfo.iconRawDataSize,imageIn->imageDataSize);
				}
			}
			else
			{
				icns_print_err("icns_update_element_with_image_or_mask: Unexpected element data size: %d!\n",existingDataSize);
				error = ICNS_STATUS_INVALID_DATA;
				goto exception;
			}
			
			imageDataSize = newDataSize;
			imageDataPtr = newDataPtr;
		}
		break;
	default:
		imageDataSize = imageIn->imageDataSize;
		imageDataPtr = imageIn->imageData;
		break;
	}
	
	if(error == ICNS_STATUS_OK)
	{
		icns_element_t	*newElement = NULL;
		icns_size_t	newElementSize = 0;
		icns_type_t	newElementType = ICNS_NULL_DATA;
		icns_size_t	newElementHeaderSize = 0;
		
		if(imageDataSize == 0)
		{
			icns_print_err("icns_update_element_with_image_or_mask: Image data size should not be 0!\n");
			error = ICNS_STATUS_INVALID_DATA;
			goto exception;
		}
		
		if(imageDataPtr == NULL)
		{
			icns_print_err("icns_update_element_with_image_or_mask: Image data should not be NULL!\n");
			error = ICNS_STATUS_INVALID_DATA;
			goto exception;
		}

		// Set up and create the new element
		newElementHeaderSize = sizeof(icns_type_t) + sizeof(icns_size_t);
		newElementSize = newElementHeaderSize + imageDataSize;
		newElementType = iconType;
		
		newElement = (icns_element_t *)malloc(newElementSize);
		
		if(newElement == NULL)
		{
			icns_print_err("icns_update_element_with_image_or_mask: Unable to allocate memory block of size: %d!\n",(int)newElementSize);
			error = ICNS_STATUS_NO_MEMORY;
			goto exception;
		}
		
		// Set up the element info
		newElement->elementType = newElementType;
		newElement->elementSize = newElementSize;
		
		#ifdef ICNS_DEBUG
		{
			char typeStr[5];
			printf(" updated element data...\n");
			printf("  type: '%s'\n",icns_type_str(newElementType,typeStr));
			printf("  size: %d\n",(int)newElementSize);
		}
		#endif		
		
		// Copy in the image data
		memcpy(newElement->elementData,imageDataPtr,imageDataSize);
		
		// Free the old element...
		if(*iconElement != NULL)
			free(*iconElement);
		
		// and move the pointer to the new element
		*iconElement = newElement;
	}
	
exception:
	
	// We might have allocated new memory earlier...
	if(newDataPtr != NULL)
	{
		free(newDataPtr);
		newDataPtr = NULL;
	}
	
	return error;
}
