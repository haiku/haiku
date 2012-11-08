/*
File:       icns_utils.c
Copyright (C) 2001-2012 Mathew Eis <mathew@eisbox.net>

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
#include <stdarg.h>


#include "icns.h"
#include "icns_internals.h"


/********* This variable is intentionally global ************/
/********* scope is the internals of the icns library *******/
#ifdef ICNS_DEBUG
icns_bool_t	gShouldPrintErrors = 1;
#else
icns_bool_t	gShouldPrintErrors = 0;
#endif

icns_uint32_t icns_get_element_order(icns_type_t iconType)
{
	// Note: 1 bit mask is 'excluded' as
	// 1 bit data and mask ID's are equal
	// data stored in the same element
	switch(iconType)
	{
	case ICNS_ICON_VERSION:
		return 100;
	case ICNS_1024x1024_32BIT_ARGB_DATA:
		return 23;
	case ICNS_512x512_32BIT_ARGB_DATA:
		return 22;
	case ICNS_256x256_32BIT_ARGB_DATA:
		return 21;
	case ICNS_128X128_8BIT_MASK:
		return 20;
	case ICNS_128X128_32BIT_DATA:
		return 19;
	case ICNS_48x48_8BIT_MASK:
		return 18;
	case ICNS_48x48_32BIT_DATA:
		return 17;
	case ICNS_48x48_8BIT_DATA:
		return 16;
	case ICNS_48x48_4BIT_DATA:
		return 15;
	case ICNS_48x48_1BIT_DATA:
		return 14;
	case ICNS_32x32_8BIT_MASK:
		return 13;
	case ICNS_32x32_32BIT_DATA:
		return 12;
	case ICNS_32x32_8BIT_DATA:
		return 11;
	case ICNS_32x32_4BIT_DATA:
		return 10;
	case ICNS_32x32_1BIT_DATA:
		return 9;
	case ICNS_16x16_8BIT_MASK:
		return 8;
	case ICNS_16x16_32BIT_DATA:
		return 7;
	case ICNS_16x16_8BIT_DATA:
		return 6;
	case ICNS_16x16_4BIT_DATA:
		return 5;
	case ICNS_16x16_1BIT_DATA:
		return 4;
	case ICNS_16x12_8BIT_DATA:
		return 3;
	case ICNS_16x12_4BIT_DATA:
		return 2;
	case ICNS_16x12_1BIT_DATA:
		return 1;
	case ICNS_TABLE_OF_CONTENTS:
		return 0;
	default:
		return 1000;
	}
	
	return 1000;
}

icns_type_t icns_get_mask_type_for_icon_type(icns_type_t iconType)
{
	switch(iconType)
	{
	// Obviously the TOC type has no mask
	case ICNS_TABLE_OF_CONTENTS:
		return ICNS_NULL_MASK;

	// Obviously the version type has no mask
	case ICNS_ICON_VERSION:
		return ICNS_NULL_MASK;
		
	// 32-bit image types > 256x256 - no mask (mask is already in image)
	case ICNS_1024x1024_32BIT_ARGB_DATA:
		return ICNS_NULL_MASK;
	case ICNS_512x512_32BIT_ARGB_DATA:
		return ICNS_NULL_MASK;			
	case ICNS_256x256_32BIT_ARGB_DATA:
		return ICNS_NULL_MASK;		
		
	// 32-bit image types - 8-bit mask type
	case ICNS_128X128_32BIT_DATA:
		return ICNS_128X128_8BIT_MASK;	
	case ICNS_48x48_32BIT_DATA:
		return ICNS_48x48_8BIT_MASK;	
	case ICNS_32x32_32BIT_DATA:
		return ICNS_32x32_8BIT_MASK;
	case ICNS_16x16_32BIT_DATA:
		return ICNS_16x16_8BIT_MASK;
		
	// 8-bit image types - 1-bit mask types
	case ICNS_48x48_8BIT_DATA:
		return ICNS_48x48_1BIT_MASK;
	case ICNS_32x32_8BIT_DATA:
		return ICNS_32x32_1BIT_MASK;
	case ICNS_16x16_8BIT_DATA:
		return ICNS_16x16_1BIT_MASK;
	case ICNS_16x12_8BIT_DATA:
		return ICNS_16x12_1BIT_MASK;
		
	// 4 bit image types - 1-bit mask types
	case ICNS_48x48_4BIT_DATA:
		return ICNS_48x48_1BIT_MASK;
	case ICNS_32x32_4BIT_DATA:
		return ICNS_32x32_1BIT_MASK;
	case ICNS_16x16_4BIT_DATA:
		return ICNS_16x16_1BIT_MASK;
	case ICNS_16x12_4BIT_DATA:
		return ICNS_16x12_1BIT_MASK;
		
	// 1 bit image types - 1-bit mask types
	case ICNS_48x48_1BIT_DATA:
		return ICNS_48x48_1BIT_MASK;
	case ICNS_32x32_1BIT_DATA:
		return ICNS_32x32_1BIT_MASK;
	case ICNS_16x16_1BIT_DATA:
		return ICNS_16x16_1BIT_MASK;
	case ICNS_16x12_1BIT_DATA:
		return ICNS_16x12_1BIT_MASK;
	default:
		return ICNS_NULL_MASK;
	}
		
	return ICNS_NULL_MASK;
}

icns_icon_info_t icns_get_image_info_for_type(icns_type_t iconType)
{
	icns_icon_info_t iconInfo;
	
	memset(&iconInfo,0,sizeof(iconInfo));
	
	if(iconType == ICNS_NULL_TYPE)
	{
		icns_print_err("icns_get_image_info_for_type: Unable to parse NULL type!\n");
		return iconInfo;
	}
	
	/*
	#ifdef ICNS_DEBUG
	{
		char typeStr[5];
		printf("Retrieving info for type '%s'...\n",icns_type_str(iconType,typeStr));
	}
	#endif
	*/
	
	iconInfo.iconType = iconType;
	
	switch(iconType)
	{
	// TOC type
	case ICNS_TABLE_OF_CONTENTS:
		iconInfo.isImage = 0;
		iconInfo.isMask = 0;
		iconInfo.iconWidth = 0;
		iconInfo.iconHeight = 0;
		iconInfo.iconChannels = 0;
		iconInfo.iconPixelDepth = 0;
		iconInfo.iconBitDepth = 0;
		break;
	// Version type
	case ICNS_ICON_VERSION:
		iconInfo.isImage = 0;
		iconInfo.isMask = 0;
		iconInfo.iconWidth = 0;
		iconInfo.iconHeight = 0;
		iconInfo.iconChannels = 0;
		iconInfo.iconPixelDepth = 0;
		iconInfo.iconBitDepth = 0;
		break;
	// 32-bit image types
	case ICNS_1024x1024_32BIT_ARGB_DATA:
		iconInfo.isImage = 1;
		iconInfo.isMask = 0;
		iconInfo.iconWidth = 1024;
		iconInfo.iconHeight = 1024;
		iconInfo.iconChannels = 4;
		iconInfo.iconPixelDepth = 8;
		iconInfo.iconBitDepth = 32;
		break;
	case ICNS_512x512_32BIT_ARGB_DATA:
		iconInfo.isImage = 1;
		iconInfo.isMask = 0;
		iconInfo.iconWidth = 512;
		iconInfo.iconHeight = 512;
		iconInfo.iconChannels = 4;
		iconInfo.iconPixelDepth = 8;
		iconInfo.iconBitDepth = 32;
		break;
	case ICNS_256x256_32BIT_ARGB_DATA:
		iconInfo.isImage = 1;
		iconInfo.isMask = 0;
		iconInfo.iconWidth = 256;
		iconInfo.iconHeight = 256;
		iconInfo.iconChannels = 4;
		iconInfo.iconPixelDepth = 8;
		iconInfo.iconBitDepth = 32;
		break;
	case ICNS_128X128_32BIT_DATA:
		iconInfo.isImage = 1;
		iconInfo.isMask = 0;
		iconInfo.iconWidth = 128;
		iconInfo.iconHeight = 128;
		iconInfo.iconChannels = 4;
		iconInfo.iconPixelDepth = 8;
		iconInfo.iconBitDepth = 32;
		break;
	case ICNS_48x48_32BIT_DATA:
		iconInfo.isImage = 1;
		iconInfo.isMask = 0;
		iconInfo.iconWidth = 48;
		iconInfo.iconHeight = 48;
		iconInfo.iconChannels = 4;
		iconInfo.iconPixelDepth = 8;
		iconInfo.iconBitDepth = 32;	
		break;
	case ICNS_32x32_32BIT_DATA:
		iconInfo.isImage = 1;
		iconInfo.isMask = 0;
		iconInfo.iconWidth = 32;
		iconInfo.iconHeight = 32;
		iconInfo.iconChannels = 4;
		iconInfo.iconPixelDepth = 8;
		iconInfo.iconBitDepth = 32;
		break;
	case ICNS_16x16_32BIT_DATA:
		iconInfo.isImage = 1;
		iconInfo.isMask = 0;
		iconInfo.iconWidth = 16;
		iconInfo.iconHeight = 16;
		iconInfo.iconChannels = 4;
		iconInfo.iconPixelDepth = 8;
		iconInfo.iconBitDepth = 32;
		break;
		
	// 8-bit mask types
	case ICNS_128X128_8BIT_MASK:
		iconInfo.isImage = 0;
		iconInfo.isMask = 1;
		iconInfo.iconWidth = 128;
		iconInfo.iconHeight = 128;
		iconInfo.iconChannels = 1;
		iconInfo.iconPixelDepth = 8;
		iconInfo.iconBitDepth = 8;
		break;
	case ICNS_48x48_8BIT_MASK:
		iconInfo.isImage = 0;
		iconInfo.isMask = 1;
		iconInfo.iconWidth = 48;
		iconInfo.iconHeight = 48;
		iconInfo.iconChannels = 1;
		iconInfo.iconPixelDepth = 8;
		iconInfo.iconBitDepth = 8;
		break;
	case ICNS_32x32_8BIT_MASK:
		iconInfo.isImage = 0;
		iconInfo.isMask = 1;
		iconInfo.iconWidth = 32;
		iconInfo.iconHeight = 32;
		iconInfo.iconChannels = 1;
		iconInfo.iconPixelDepth = 8;
		iconInfo.iconBitDepth = 8;
		break;
	case ICNS_16x16_8BIT_MASK:
		iconInfo.isImage = 0;
		iconInfo.isMask = 1;
		iconInfo.iconWidth = 16;
		iconInfo.iconHeight = 16;
		iconInfo.iconChannels = 1;
		iconInfo.iconPixelDepth = 8;
		iconInfo.iconBitDepth = 8;
		break;
	
	// 8-bit image types
	case ICNS_48x48_8BIT_DATA:
		iconInfo.isImage = 1;
		iconInfo.isMask = 0;
		iconInfo.iconWidth = 48;
		iconInfo.iconHeight = 48;
		iconInfo.iconChannels = 1;
		iconInfo.iconPixelDepth = 8;
		iconInfo.iconBitDepth = 8;
		break;
	case ICNS_32x32_8BIT_DATA:
		iconInfo.isImage = 1;
		iconInfo.isMask = 0;
		iconInfo.iconWidth = 32;
		iconInfo.iconHeight = 32;
		iconInfo.iconChannels = 1;
		iconInfo.iconPixelDepth = 8;
		iconInfo.iconBitDepth = 8;
		break;
	case ICNS_16x16_8BIT_DATA:
		iconInfo.isImage = 1;
		iconInfo.isMask = 0;
		iconInfo.iconWidth = 16;
		iconInfo.iconHeight = 16;
		iconInfo.iconChannels = 1;
		iconInfo.iconPixelDepth = 8;
		iconInfo.iconBitDepth = 8;
		break;
	case ICNS_16x12_8BIT_DATA:
		iconInfo.isImage = 1;
		iconInfo.isMask = 0;
		iconInfo.iconWidth = 16;
		iconInfo.iconHeight = 12;
		iconInfo.iconChannels = 1;
		iconInfo.iconPixelDepth = 8;
		iconInfo.iconBitDepth = 8;
		break;
		
	// 4 bit image types
	case ICNS_48x48_4BIT_DATA:
		iconInfo.isImage = 1;
		iconInfo.isMask = 0;
		iconInfo.iconWidth = 48;
		iconInfo.iconHeight = 48;
		iconInfo.iconChannels = 1;
		iconInfo.iconPixelDepth = 4;
		iconInfo.iconBitDepth = 4;
		break;
	case ICNS_32x32_4BIT_DATA:
		iconInfo.isImage = 1;
		iconInfo.isMask = 0;
		iconInfo.iconWidth = 32;
		iconInfo.iconHeight = 32;
		iconInfo.iconChannels = 1;
		iconInfo.iconPixelDepth = 4;
		iconInfo.iconBitDepth = 4;
		break;
	case ICNS_16x16_4BIT_DATA:
		iconInfo.isImage = 1;
		iconInfo.isMask = 0;
		iconInfo.iconWidth = 16;
		iconInfo.iconHeight = 16;
		iconInfo.iconChannels = 1;
		iconInfo.iconPixelDepth = 4;
		iconInfo.iconBitDepth = 4;
		break;
	case ICNS_16x12_4BIT_DATA:
		iconInfo.isImage = 1;
		iconInfo.isMask = 0;
		iconInfo.iconWidth = 16;
		iconInfo.iconHeight = 12;
		iconInfo.iconChannels = 1;
		iconInfo.iconPixelDepth = 4;
		iconInfo.iconBitDepth = 4;
		break;
		
	// 1 bit image types - same as mask typess
	case ICNS_48x48_1BIT_DATA:
		iconInfo.isImage = 1;
		iconInfo.isMask = 1;
		iconInfo.iconWidth = 48;
		iconInfo.iconHeight = 48;
		iconInfo.iconChannels = 1;
		iconInfo.iconPixelDepth = 1;
		iconInfo.iconBitDepth = 1;
		break;
	case ICNS_32x32_1BIT_DATA:
		iconInfo.isImage = 1;
		iconInfo.isMask = 1;
		iconInfo.iconWidth = 32;
		iconInfo.iconHeight = 32;
		iconInfo.iconChannels = 1;
		iconInfo.iconPixelDepth = 1;
		iconInfo.iconBitDepth = 1;
		break;
	case ICNS_16x16_1BIT_DATA:
		iconInfo.isImage = 1;
		iconInfo.isMask = 1;
		iconInfo.iconWidth = 16;
		iconInfo.iconHeight = 16;
		iconInfo.iconChannels = 1;
		iconInfo.iconPixelDepth = 1;
		iconInfo.iconBitDepth = 1;
		break;
	case ICNS_16x12_1BIT_DATA:
		iconInfo.isImage = 1;
		iconInfo.isMask = 1;
		iconInfo.iconWidth = 16;
		iconInfo.iconHeight = 12;
		iconInfo.iconChannels = 1;
		iconInfo.iconPixelDepth = 1;
		iconInfo.iconBitDepth = 1;
		break;
	default:
		{
			char typeStr[5];
			icns_print_err("icns_get_image_info_for_type: Unable to parse icon type '%s'\n",icns_type_str(iconType,typeStr));
			iconInfo.iconType = ICNS_NULL_TYPE;
		}
		break;
	}
	
	iconInfo.iconRawDataSize = iconInfo.iconHeight * iconInfo.iconWidth * iconInfo.iconBitDepth / ICNS_BYTE_BITS;
	
	/*
	#ifdef ICNS_DEBUG
	{
		char typeStr[5];
		printf("  type is: '%s'\n",icns_type_str(iconInfo.iconType));
		printf("  width is: %d\n",iconInfo.iconWidth);
		printf("  height is: %d\n",iconInfo.iconHeight);
		printf("  channels are: %d\n",iconInfo.iconChannels);
		printf("  pixel depth is: %d\n",iconInfo.iconPixelDepth);
		printf("  bit depth is: %d\n",iconInfo.iconBitDepth);
		printf("  data size is: %d\n",(int)iconInfo.iconRawDataSize);
	}
	#endif
	*/
	
	return iconInfo;
}

icns_type_t	icns_get_type_from_image_info(icns_icon_info_t iconInfo)
{
	// Give our best effort to returning a type from the given information
	// But there is only so much we can't work with...
	if( (iconInfo.isImage == 0) && (iconInfo.isMask == 0) )
		return ICNS_NULL_TYPE;
	
	if( (iconInfo.iconWidth == 0) || (iconInfo.iconHeight == 0) )
	{
		// For some really small sizes, we can tell from just the data size...
		switch(iconInfo.iconRawDataSize)
		{
		case 24:
			// Data is too small to have both
			if( (iconInfo.isImage == 1) && (iconInfo.isMask == 1) )
				return ICNS_NULL_TYPE;
			if( iconInfo.isImage == 1 )
				return ICNS_16x12_1BIT_DATA;
			if( iconInfo.isMask == 1 )
				return ICNS_16x12_1BIT_MASK;
			break;
		case 32:
			// Data is too small to have both
			if( (iconInfo.isImage == 1) && (iconInfo.isMask == 1) )
				return ICNS_NULL_TYPE;
			if( iconInfo.isImage == 1 )
				return ICNS_16x16_1BIT_DATA;
			if( iconInfo.isMask == 1 )
				return ICNS_16x16_1BIT_MASK;
			break;
		default:
			return ICNS_NULL_TYPE;
		}
	}
	
	// We need the bit depth to determine the type for sizes < 128
	if( (iconInfo.iconBitDepth == 0) && (iconInfo.iconWidth < 128 || iconInfo.iconHeight < 128) )
	{
		if(iconInfo.iconPixelDepth == 0 || iconInfo.iconChannels == 0)
			return ICNS_NULL_TYPE;
		else
			iconInfo.iconBitDepth = iconInfo.iconPixelDepth * iconInfo.iconChannels;
	}
	
	// Special case for these mini icons
	if(iconInfo.iconWidth == 16 && iconInfo.iconHeight == 12)
	{
		switch(iconInfo.iconBitDepth)
		{
		case 1:
			if(iconInfo.isImage == 1)
				return ICNS_16x12_1BIT_DATA;
			if(iconInfo.isMask == 1)
				return ICNS_16x12_1BIT_MASK;
			break;
		case 4:
			return ICNS_16x12_4BIT_DATA;
			break;
		case 8:
			return ICNS_16x12_8BIT_DATA;
			break;
		default:
			return ICNS_NULL_TYPE;
			break;
		}
	}
	
	// Width must equal hieght from here on...
	if(iconInfo.iconWidth != iconInfo.iconHeight)
		return ICNS_NULL_TYPE;
	
	
	switch(iconInfo.iconWidth)
	{
	case 16:
		switch(iconInfo.iconBitDepth)
		{
		case 1:
			if(iconInfo.isImage == 1)
				return ICNS_16x16_1BIT_DATA;
			if(iconInfo.isMask == 1)
				return ICNS_16x16_1BIT_MASK;
			break;
		case 4:
			return ICNS_16x16_4BIT_DATA;
			break;
		case 8:
			if(iconInfo.isImage == 1)
				return ICNS_16x16_8BIT_DATA;
			if(iconInfo.isMask == 1)
				return ICNS_16x16_8BIT_MASK;
			break;
		case 32:
			return ICNS_16x16_32BIT_DATA;
			break;
		default:
			return ICNS_NULL_TYPE;
			break;
		}
		break;
	case 32:
		switch(iconInfo.iconBitDepth)
		{
		case 1:
			if(iconInfo.isImage == 1)
				return ICNS_32x32_1BIT_DATA;
			if(iconInfo.isMask == 1)
				return ICNS_32x32_1BIT_MASK;
			break;
		case 4:
			return ICNS_32x32_4BIT_DATA;
			break;
		case 8:
			if(iconInfo.isImage == 1)
				return ICNS_32x32_8BIT_DATA;
			if(iconInfo.isMask == 1)
				return ICNS_32x32_8BIT_MASK;
			break;
		case 32:
			return ICNS_32x32_32BIT_DATA;
			break;
		default:
			return ICNS_NULL_TYPE;
			break;
		}
		break;
	case 48:
		switch(iconInfo.iconBitDepth)
		{
		case 1:
			if(iconInfo.isImage == 1)
				return ICNS_48x48_1BIT_DATA;
			if(iconInfo.isMask == 1)
				return ICNS_48x48_1BIT_MASK;
			break;
		case 4:
			return ICNS_48x48_4BIT_DATA;
			break;
		case 8:
			if(iconInfo.isImage == 1)
				return ICNS_48x48_8BIT_DATA;
			if(iconInfo.isMask == 1)
				return ICNS_48x48_8BIT_MASK;
			break;
		case 32:
			return ICNS_48x48_32BIT_DATA;
			break;
		default:
			return ICNS_NULL_TYPE;
			break;
		}
		break;
	case 128:
		if(iconInfo.isImage == 1 || iconInfo.iconBitDepth == 32)
			return ICNS_128X128_32BIT_DATA;
		if(iconInfo.isMask == 1 || iconInfo.iconBitDepth == 8)
			return ICNS_128X128_8BIT_MASK;
		break;
	case 256:
		return ICNS_256x256_32BIT_ARGB_DATA;
		break;
	case 512:
		return ICNS_512x512_32BIT_ARGB_DATA;
		break;
	case 1024:
		return ICNS_1024x1024_32BIT_ARGB_DATA;
		break;
		
	}
	
	return ICNS_NULL_TYPE;
}

icns_type_t	icns_get_type_from_image(icns_image_t iconImage)
{
	icns_icon_info_t		iconInfo;
	
	iconInfo.iconWidth = iconImage.imageWidth;
	iconInfo.iconHeight = iconImage.imageHeight;
	iconInfo.iconChannels = iconImage.imageChannels;
	iconInfo.iconPixelDepth = iconImage.imagePixelDepth;
	iconInfo.iconBitDepth = iconInfo.iconPixelDepth * iconInfo.iconChannels;
	iconInfo.iconRawDataSize = iconImage.imageDataSize;
	
	if(iconInfo.iconBitDepth == 1)
	{
		int	flatSize = ((iconInfo.iconWidth * iconInfo.iconHeight) / 8);
		if(iconInfo.iconRawDataSize == (flatSize * 2) )
		{
		iconInfo.isImage = 1;
		iconInfo.isMask = 1;
		}
		else
		{
		iconInfo.isImage = 1;
		iconInfo.isMask = 0;
		}
	}
	else
	{
		iconInfo.isImage = 1;
		iconInfo.isMask = 0;
	}
	
	return icns_get_type_from_image_info(iconInfo);
}

icns_type_t	icns_get_type_from_mask(icns_image_t iconImage)
{
	icns_icon_info_t		iconInfo;
	
	iconInfo.iconWidth = iconImage.imageWidth;
	iconInfo.iconHeight = iconImage.imageHeight;
	iconInfo.iconChannels = iconImage.imageChannels;
	iconInfo.iconPixelDepth = iconImage.imagePixelDepth;
	iconInfo.iconBitDepth = iconInfo.iconPixelDepth * iconInfo.iconChannels;
	iconInfo.iconRawDataSize = iconImage.imageDataSize;
	
	if(iconInfo.iconBitDepth == 1)
	{
		int	flatSize = ((iconInfo.iconWidth * iconInfo.iconHeight) / 8);
		if(iconInfo.iconRawDataSize == (flatSize * 2) )
		{
		iconInfo.isImage = 1;
		iconInfo.isMask = 1;
		}
		else
		{
		iconInfo.isImage = 0;
		iconInfo.isMask = 1;
		}
	}
	else
	{
		iconInfo.isImage = 0;
		iconInfo.isMask = 1;
	}
	
	return icns_get_type_from_image_info(iconInfo);
}

icns_bool_t icns_types_equal(icns_type_t typeA,icns_type_t typeB)
{
	if(memcmp(&typeA, &typeB, sizeof(icns_type_t)) == 0)
		return 1;
	else
		return 0;
}

// This is is largely for conveniance and readability
icns_bool_t icns_types_not_equal(icns_type_t typeA,icns_type_t typeB)
{
	if(memcmp(&typeA, &typeB, sizeof(icns_type_t)) != 0)
		return 1;
	else
		return 0;
}

const char * icns_type_str(icns_type_t type, char *strbuf)
{
	if(strbuf != NULL)
	{
		uint32_t v = *((uint32_t *)(&type));
		strbuf[0] = v >> 24;
		strbuf[1] = v >> 16;
		strbuf[2] = v >> 8;
		strbuf[3] = v;
		strbuf[4] = 0;
		return (const char *)strbuf;
	}
	return NULL;
}

void icns_set_print_errors(icns_bool_t shouldPrint)
{
	#ifdef ICNS_DEBUG
		if(shouldPrint == 0) {
			icns_print_err("Debugging enabled - error message status cannot be disabled!\n");
		}
	#else
		gShouldPrintErrors = shouldPrint;
	#endif
}

void icns_print_err(const char *template, ...)
{
	va_list ap;
	
	#ifdef ICNS_DEBUG
	printf ( "libicns: ");
	va_start (ap, template);
	vprintf (template, ap);
	va_end (ap);
	#else
	if(gShouldPrintErrors)
	{
		fprintf (stderr, "libicns: ");
		va_start (ap, template);
		vfprintf (stderr, template, ap);
		va_end (ap);
	}
	#endif
}

