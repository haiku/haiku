/*
File:       icns_io.c
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

/***************************** ICNS_MEMCPY **************************/
#if HAVE_UNALIGNED_MEMCPY == 0
__attribute__ ((noinline)) void *icns_memcpy( void *dst, const void *src, size_t num ) {
 return memcpy(dst,src,num);
}
#endif

/***************************** ICNS_READ_UNALIGNED_BE **************************/
/* NOTE: only accessible to icns_io.c */
#define ICNS_READ_UNALIGNED_BE(val, addr, size)    icns_read_be(&(val), (addr), (size))
static inline void icns_read_be(void *outp, void *inp, int size)
{
	icns_byte_t	b[8] = {0,0,0,0,0,0,0,0};
		
	if(outp == NULL)
		return;
	
	if(inp == NULL)
		return;

	memcpy(&b, inp, size);

	#ifdef ICNS_DEBUG
	int i = 0;
	printf("Reading %d bytes: ",size);
	for(i = 0; i < size; i++)
		printf("0x%02X ",b[i]);
	printf("\n");
	#endif
		
	switch(size)
	{
	case 1:
		*((uint8_t *)(outp)) = b[0];
		break;
	case 2:
		*((uint16_t *)(outp)) = b[1]|b[0]<< 8;
		break;
	case 4:
		*((uint32_t *)(outp)) = b[3]|b[2]<<8| \
		                        b[1]<<16|b[0]<<24;
		break;
	case 8:
		*((uint64_t *)(outp)) = b[7]|b[6]<<8| \
		                        b[5]<<16|b[4]<<24| \
					(uint64_t)b[3]<<32|(uint64_t)b[2]<<40| \
					(uint64_t)b[1]<<48|(uint64_t)b[0]<<56;
		break;
	
	// This is a special case needed by icns_read_macbinary_resource_fork
	case 3:
		*((uint32_t *)(outp)) = ((uint16_t)b[2]|(uint16_t)b[1]<<8|(uint16_t)b[0]<<16) & 0x00FFFFFF;
		break;
	
	default:
		break;
	}
}

#define ICNS_READ_UNALIGNED_LE(val, addr, size)    icns_read_le(&(val), (addr), (size))
static inline void icns_read_le(void *outp, void *inp, int size)
{
	icns_byte_t	b[8] = {0,0,0,0,0,0,0,0};
		
	if(outp == NULL)
		return;
	
	if(inp == NULL)
		return;

	memcpy(&b, inp, size);

	#ifdef ICNS_DEBUG
	int i = 0;
	printf("Reading %d bytes: ",size);
	for(i = 0; i < size; i++)
		printf("0x%02X ",b[i]);
	printf("\n");
	#endif
		
	switch(size)
	{
	case 1:
		*((uint8_t *)(outp)) = b[0];
		break;
	case 2:
		*((uint16_t *)(outp)) = b[0]|b[1]<< 8;
		break;
	case 4:
		*((uint32_t *)(outp)) = b[0]|b[1]<<8| b[2]<<16|b[3]<<24;
		break;
	case 8:
		*((uint64_t *)(outp)) = b[0]|b[1]<<8| b[2]<<16|b[3]<<24| (uint64_t)b[4]<<32|(uint64_t)b[5]<<40| (uint64_t)b[6]<<48|(uint64_t)b[7]<<56;
		break;
	
	// This is a special case needed by icns_read_macbinary_resource_fork
	case 3:
		*((uint32_t *)(outp)) = ((uint16_t)b[0]|(uint16_t)b[1]<<8|(uint16_t)b[2]<<16) & 0x00FFFFFF;
		break;
	
	default:
		break;
	}
}

/***************************** ICNS_WRITE_UNALIGNED_BE **************************/
/* NOTE: only accessible to icns_io.c */
#define ICNS_WRITE_UNALIGNED_BE(addr, val, size)    icns_write_be((addr), &(val), (size))
static inline void icns_write_be(void *outp, void *inp, int size)
{
	icns_byte_t	b[8] = {0,0,0,0,0,0,0,0};
	
	if(outp == NULL)
		return;
	
	if(inp == NULL)
		return;
	
	switch(size)
	{
	case 1:
		{
		uint8_t v = *((uint8_t *)inp);
		b[0] = v;
		}
		break;
	case 2:
		{
		uint16_t v = *((uint16_t *)inp);
		b[0] = v >> 8;
		b[1] = v;
		}
		break;
	case 4:
		{
		uint32_t v = *((uint32_t *)inp);
		b[0] = v >> 24;
		b[1] = v >> 16;
		b[2] = v >> 8;
		b[3] = v;
		}
		break;
	case 8:
		{
		uint64_t v = *((uint64_t *)inp);
		b[0] = v >> 56;
		b[1] = v >> 48;
		b[2] = v >> 40;
		b[3] = v >> 32;
		b[4] = v >> 24;
		b[5] = v >> 16;
		b[6] = v >> 8;
		b[7] = v;
		}
		break;
	default:
		break;
	}
	
	#ifdef ICNS_DEBUG
	int i = 0;
	printf("Writing %d bytes: ",size);
	for(i = 0; i < size; i++)
		printf("0x%02X ",b[i]);
	printf("\n");
	#endif
	
	memcpy(outp, &b, size);
}

/***************************** icns_write_family_to_file **************************/

int icns_write_family_to_file(FILE *dataFile,icns_family_t *iconFamilyIn)
{
	int		error = ICNS_STATUS_OK;
	icns_size_t	blockSize = 0;
	icns_size_t	blockCount = 0;
	icns_size_t	blocksWritten = 0;
	icns_size_t	dataWritten = 0;
	icns_size_t	dataSize = 0;
	icns_byte_t	*dataPtr = NULL;
	
	if( dataFile == NULL )
	{
		icns_print_err("icns_write_family_to_file: NULL file pointer!\n");
		return ICNS_STATUS_NULL_PARAM;
	}	
	
	if( iconFamilyIn == NULL )
	{
		icns_print_err("icns_write_family_to_file: NULL icns family!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	error = icns_export_family_data(iconFamilyIn,&dataSize,&dataPtr);
	
	if(error != ICNS_STATUS_OK)
		return error;
	
	#ifdef ICNS_DEBUG
	printf("Writing icon family data to file...\n");
	printf("  total data size: %d (0x%08X)\n",(int)dataSize,dataSize);
	#endif
	
	blockSize = 1024;
	blockCount = dataSize / blockSize;
	
	blocksWritten = fwrite ( dataPtr , blockSize , blockCount , dataFile );
	
	if(blocksWritten < blockCount)
	{
			icns_print_err("icns_write_family_to_file: Error writing icns to file!\n");
			return ICNS_STATUS_IO_WRITE_ERR;
	}
	
	dataWritten = (blockCount * blockSize);
	blockSize = dataSize - (blockCount * blockSize);
	blocksWritten = fwrite ( dataPtr + dataWritten , blockSize , 1 , dataFile );
	
	if(blocksWritten != 1)
	{
		icns_print_err("icns_write_family_to_file: Error writing icns to file!\n");
		return ICNS_STATUS_IO_WRITE_ERR;
	}
	
	free(dataPtr);
	
	return ICNS_STATUS_OK;
}


/***************************** icns_read_family_from_file **************************/

int icns_read_family_from_file(FILE *dataFile,icns_family_t **iconFamilyOut)
{
	int	      error = ICNS_STATUS_OK;
	icns_uint32_t dataSize = 0;
	void          *dataPtr = NULL;
	
	if( dataFile == NULL )
	{
		icns_print_err("icns_read_family_from_file: NULL file pointer!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	if( iconFamilyOut == NULL )
	{
		icns_print_err("icns_read_family_from_file: NULL icns family ref!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	if(fseek(dataFile,0,SEEK_END) == 0)
	{
		dataSize = ftell(dataFile);
		rewind(dataFile);
		
		dataPtr = (void *)malloc(dataSize);

		if( (error == 0) && (dataPtr != NULL) )
		{
			if(fread( dataPtr, sizeof(char), dataSize, dataFile) != dataSize)
			{
				free( dataPtr );
				dataPtr = NULL;
				dataSize = 0;
				error = ICNS_STATUS_IO_READ_ERR;
				icns_print_err("icns_read_family_from_file: Error occured reading file!\n");
				goto exception;
			}
		}
		else
		{
			icns_print_err("icns_read_family_from_file: Unable to allocate memory block of size: %d!\n",(int)dataSize);
			error = ICNS_STATUS_NO_MEMORY;
			goto exception;
		}
	}
	else
	{
		error = ICNS_STATUS_IO_READ_ERR;
		icns_print_err("icns_read_family_from_file: Error occured seeking to end of file!\n");
		goto exception;
	}
	
	if(error == 0)
	{
		// Attempt 1 - try to import as an 'icns' file
		if(icns_icns_header_check(dataSize,dataPtr))
		{
			#ifdef ICNS_DEBUG
			printf("Trying to read from icns file...\n");
			#endif
			if((error = icns_parse_family_data(dataSize,dataPtr,iconFamilyOut)))
			{
				icns_print_err("icns_read_family_from_file: Error parsing icon family data!\n");
				*iconFamilyOut = NULL;	
			}
			else // Success!
			{
				// icns_parse_family_data points to allocated memory
				// clear these out so they won't be freed at the end
				dataSize = 0;
				dataPtr = NULL;
			}
		}
		// Attempt 2 - try to import from an 'icns' resource in a big endian macintosh resource file
		else if(icns_rsrc_header_check(dataSize,dataPtr,ICNS_BE_RSRC))
		{
			#ifdef ICNS_DEBUG
			printf("Trying to find icns data in resource file...\n");
			#endif
			if((error = icns_find_family_in_mac_resource(dataSize,dataPtr,ICNS_BE_RSRC,iconFamilyOut)))
			{
				icns_print_err("icns_read_family_from_file: Error reading macintosh resource file!\n");
				*iconFamilyOut = NULL;	
			}
		}
		// Attempt 3 - try to import from an 'icns' resource in a little endian macintosh resource file
		else if(icns_rsrc_header_check(dataSize,dataPtr,ICNS_LE_RSRC))
		{
			#ifdef ICNS_DEBUG
			printf("Trying to find icns data in resource file...\n");
			#endif
			if((error = icns_find_family_in_mac_resource(dataSize,dataPtr,ICNS_LE_RSRC,iconFamilyOut)))
			{
				icns_print_err("icns_read_family_from_file: Error reading macintosh resource file!\n");
				*iconFamilyOut = NULL;	
			}
		}
		// Attempt 4 - try to import from an 'icns' resource in a macbinary resource fork
		else if(icns_macbinary_header_check(dataSize,dataPtr))
		{
			icns_size_t	resourceSize;
			icns_byte_t	*resourceData;
			#ifdef ICNS_DEBUG
			printf("Trying to find icns data in macbinary resource fork...\n");
			#endif
			if((error = icns_read_macbinary_resource_fork(dataSize,dataPtr,NULL,NULL,&resourceSize,&resourceData)))
			{
				icns_print_err("icns_read_family_from_file: Error reading macbinary resource fork!\n");
				*iconFamilyOut = NULL;	
			}
			
			if(error == 0)
			{
				if((error = icns_find_family_in_mac_resource(resourceSize,resourceData,ICNS_BE_RSRC,iconFamilyOut)))
				{
					icns_print_err("icns_read_family_from_file: Error reading icns data from macbinary resource fork!\n");
					*iconFamilyOut = NULL;	
				}
			}
			
			if(resourceData != NULL)
			{
				free(resourceData);
				resourceData = NULL;
			}
		}
		// Attempt 5 - try to import from an 'icns' resource in a apple encoded resource fork
		else if(icns_apple_encoded_header_check(dataSize,dataPtr))
		{
			icns_size_t	resourceSize;
			icns_byte_t	*resourceData;
			#ifdef ICNS_DEBUG
			printf("Trying to find icns data in apple encoded resource fork...\n");
			#endif
			if((error = icns_read_apple_encoded_resource_fork(dataSize,dataPtr,NULL,NULL,&resourceSize,&resourceData)))
			{
				icns_print_err("icns_read_family_from_file: Error reading macbinary resource fork!\n");
				*iconFamilyOut = NULL;	
			}
			
			if(error == 0)
			{
				if((error = icns_find_family_in_mac_resource(resourceSize,resourceData,ICNS_BE_RSRC,iconFamilyOut)))
				{
					icns_print_err("icns_read_family_from_file: Error reading icns data from macbinary resource fork!\n");
					*iconFamilyOut = NULL;	
				}
			}
			
			if(resourceData != NULL)
			{
				free(resourceData);
				resourceData = NULL;
			}
		}
		// All attempts failed
		else
		{
			icns_print_err("icns_read_family_from_file: Error reading icns file - all parsing methods failed!\n");
			*iconFamilyOut = NULL;
			error = ICNS_STATUS_INVALID_DATA;
		}

	}
	
exception:
	
	if(dataPtr != NULL)
	{
		free(dataPtr);
		dataPtr = NULL;
	}
	
	return error;
}

/***************************** icns_read_family_from_rsrc **************************/

int icns_read_family_from_rsrc(FILE *dataFile,icns_family_t **iconFamilyOut)
{
	int	      error = ICNS_STATUS_OK;
	icns_uint32_t dataSize = 0;
	void          *dataPtr = NULL;
	
	if( dataFile == NULL )
	{
		icns_print_err("icns_read_family_from_rsrc: NULL file pointer!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	if( iconFamilyOut == NULL )
	{
		icns_print_err("icns_read_family_from_rsrc: NULL icns family ref!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	if(fseek(dataFile,0,SEEK_END) == 0)
	{
		dataSize = ftell(dataFile);
		rewind(dataFile);
		
		dataPtr = (void *)malloc(dataSize);

		if( (error == 0) && (dataPtr != NULL) )
		{
			if(fread( dataPtr, sizeof(char), dataSize, dataFile) != dataSize)
			{
				free( dataPtr );
				dataPtr = NULL;
				dataSize = 0;
				error = ICNS_STATUS_IO_READ_ERR;
				icns_print_err("icns_read_family_from_rsrc: Error occured reading file!\n");
				goto exception;
			}
		}
		else
		{
			icns_print_err("icns_read_family_from_rsrc: Unable to allocate memory block of size: %d!\n",(int)dataSize);
			error = ICNS_STATUS_NO_MEMORY;
			goto exception;
		}
	}
	else
	{
		error = ICNS_STATUS_IO_READ_ERR;
		icns_print_err("icns_read_family_from_rsrc: Error occured seeking to end of file!\n");
		goto exception;
	}
	
	if(error == 0)
	{
		// Read from big endian resource file
		if(icns_rsrc_header_check(dataSize,dataPtr,ICNS_BE_RSRC))
		{
			#ifdef ICNS_DEBUG
			printf("Trying to find icns data in big endian mac resource file...\n");
			#endif
			if((error = icns_find_family_in_mac_resource(dataSize,dataPtr,ICNS_BE_RSRC,iconFamilyOut)))
			{
				icns_print_err("icns_read_family_from_rsrc: Error reading macintosh resource file!\n");
				*iconFamilyOut = NULL;	
			}
		}
		// Read from little endian resource file
		else if(icns_rsrc_header_check(dataSize,dataPtr,ICNS_LE_RSRC))
		{
			#ifdef ICNS_DEBUG
			printf("Trying to find icns data in little endian mac resource file...\n");
			#endif
			if((error = icns_find_family_in_mac_resource(dataSize,dataPtr,ICNS_LE_RSRC,iconFamilyOut)))
			{
				icns_print_err("icns_read_family_from_rsrc: Error reading macintosh resource file!\n");
				*iconFamilyOut = NULL;	
			}
		}
		// All attempts failed
		else
		{
			icns_print_err("icns_read_family_from_rsrc: Error reading rsrc file - all parsing methods failed!\n");
			*iconFamilyOut = NULL;
			error = ICNS_STATUS_INVALID_DATA;
		}

	}
	
exception:
	
	if(dataPtr != NULL)
	{
		free(dataPtr);
		dataPtr = NULL;
	}
	
	return error;
}


/***************************** icns_export_family_data **************************/

int icns_export_family_data(icns_family_t *iconFamily,icns_size_t *dataSizeOut,icns_byte_t **dataPtrOut)
{
	int		error = ICNS_STATUS_OK;
	icns_type_t	dataType = ICNS_NULL_TYPE;
	icns_size_t	dataSize = 0;
	icns_byte_t	*dataPtr = NULL;
	
	if(iconFamily == NULL)
	{
		icns_print_err("icns_export_family_data: icon family is NULL\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	if(dataPtrOut == NULL)
	{
		icns_print_err("icns_export_family_data: data ref is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	#ifdef ICNS_DEBUG
	printf("Writing icns family to data...\n");
	#endif
	
	// Check the data type
	if(iconFamily->resourceType != ICNS_FAMILY_TYPE)
	{
		icns_print_err("icns_export_family_data: Invalid type in header! (%d)\n",dataSize);
		*dataPtrOut = NULL;
		return ICNS_STATUS_INVALID_DATA;
	}
	else
	{
		dataType = iconFamily->resourceType;
	}
	
	// Check the data size
	if(iconFamily->resourceSize < 8)
	{
		icns_print_err("icns_export_family_data: Invalid size in header! (%d)\n",dataSize);
		*dataPtrOut = NULL;
		return ICNS_STATUS_INVALID_DATA;
	}
	else
	{
		dataSize = iconFamily->resourceSize;
	}
	
	#ifdef ICNS_DEBUG
	{
		char typeStr[5];
		printf("  data type is '%s'\n",icns_type_str(dataType,typeStr));
		printf("  data size is %d\n",dataSize);
	}
	#endif
	
	// Allocate a new block of memory for the outgoing data
	dataPtr = (icns_byte_t *)malloc(dataSize);
	
	if(dataPtr == NULL)
	{
		icns_print_err("icns_import_family_data: Unable to allocate memory block of size: %d!\n",dataSize);
		error = ICNS_STATUS_NO_MEMORY;
	}
	else
	{
		memcpy( dataPtr, iconFamily, dataSize);
	}
	
	if(error == 0)
	{
		unsigned long	dataOffset = 0;
		icns_type_t	elementType = ICNS_NULL_TYPE;
		icns_size_t	elementSize = 0;
		
		ICNS_WRITE_UNALIGNED_BE(dataPtr, dataType, sizeof(icns_type_t));
		ICNS_WRITE_UNALIGNED_BE(dataPtr + 4, dataSize, sizeof(icns_size_t));
		
		// Skip past the icns header
		dataOffset = sizeof(icns_type_t) + sizeof(icns_size_t);
		
		// Iterate through the icns resource, converting the 'size' values to big endian
		while( ((dataOffset+8) < dataSize) && (error == 0) )
		{
			ICNS_READ_UNALIGNED(elementType, dataPtr+dataOffset,sizeof(icns_type_t));
			ICNS_READ_UNALIGNED(elementSize, dataPtr+dataOffset+4,sizeof(icns_size_t));
			
			#ifdef ICNS_DEBUG
			{
				char typeStr[5];
				printf("  checking element type... type is %s\n",icns_type_str(elementType,typeStr));
				printf("  checking element size... size is %d\n",elementSize);
			}
			#endif
			
			if( (elementSize < 8) || (dataOffset+elementSize > dataSize) )
			{
				icns_print_err("icns_export_family_data: Invalid element size! (%d)\n",elementSize);
				error = ICNS_STATUS_INVALID_DATA;
				goto exception;
			}
			
			// Reset the values to big endian
			ICNS_WRITE_UNALIGNED_BE( dataPtr+dataOffset, elementType, sizeof(icns_type_t));
			ICNS_WRITE_UNALIGNED_BE( dataPtr+dataOffset+4, elementSize, sizeof(icns_size_t));
			
			// Move on to the next element
			dataOffset += elementSize;
		}
		
	}
	
exception:
	
	if(error != 0)
	{
		*dataSizeOut = 0;
		*dataPtrOut = NULL;
	}
	else
	{
		*dataSizeOut = dataSize;
		*dataPtrOut = dataPtr;
	}
	
	return error;
}

/***************************** icns_import_family_data **************************/

int icns_import_family_data(icns_size_t dataSize,icns_byte_t *dataPtr,icns_family_t **iconFamilyOut)
{
	int error = ICNS_STATUS_OK;
	icns_byte_t *iconFamilyData;
	
	if(dataSize < 8)
	{
		icns_print_err("icns_import_family_data: data size is %d - missing icns header!\n",(int)dataSize);
		return ICNS_STATUS_INVALID_DATA;
	}
	
	if(dataPtr == NULL)
	{
		icns_print_err("icns_import_family_data: data is NULL\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	if(iconFamilyOut == NULL)
	{
		icns_print_err("icns_import_family_data: icon family ref is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	// icns_parse_family_data is destructive, so we allocate a new block of memory
	iconFamilyData = malloc(dataSize);
	
	if(iconFamilyData != NULL)
	{
		memcpy( iconFamilyData , dataPtr,dataSize);
		if((error = icns_parse_family_data(dataSize,iconFamilyData,iconFamilyOut)))
		{
			icns_print_err("icns_import_family_data: Error parsing icon family!\n");
			*iconFamilyOut = NULL;
		}
	}
	else
	{
		icns_print_err("icns_import_family_data: Unable to allocate memory block of size: %d!\n",(int)dataSize);
		error = ICNS_STATUS_NO_MEMORY;
		*iconFamilyOut = NULL;
	}
			
	return error;
}

/***************************** icns_parse_family_data **************************/

int icns_parse_family_data(icns_size_t dataSize,icns_byte_t *dataPtr,icns_family_t **iconFamilyOut)
{
	int		error = ICNS_STATUS_OK;
	icns_type_t	resourceType = ICNS_NULL_TYPE;
	icns_size_t	resourceSize = 0;
	
	if(dataSize < 8)
	{
		icns_print_err("icns_parse_family_data: data size is %d - missing icns header!\n",(int)dataSize);
		return ICNS_STATUS_INVALID_DATA;
	}
	
	if(dataPtr == NULL)
	{
		icns_print_err("icns_parse_family_data: data is NULL\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	if(iconFamilyOut == NULL)
	{
		icns_print_err("icns_parse_family_data: icon family ref is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	ICNS_READ_UNALIGNED_BE(resourceType, dataPtr,sizeof(icns_type_t));
	ICNS_READ_UNALIGNED_BE(resourceSize, dataPtr + 4,sizeof(icns_size_t));
	
	#ifdef ICNS_DEBUG
	{
		char typeStr[5];
		printf("Reading icns family from data...\n");
		printf("  resource type is '%s'\n",icns_type_str(resourceType,typeStr));
		printf("  resource size is %d\n",resourceSize);
	}
	#endif
	
	if(resourceType == ICNS_FAMILY_TYPE)
	{
		if( dataSize == resourceSize )
		{
			// 'Fix' the values for working with the data later
			ICNS_WRITE_UNALIGNED(dataPtr, resourceType, sizeof(icns_type_t));
			ICNS_WRITE_UNALIGNED(dataPtr + 4, resourceSize, sizeof(icns_size_t));
		}
		else
		{
			icns_print_err("icns_parse_family_data: Invalid icon family resource size! (%d)\n",resourceSize);
			error = ICNS_STATUS_INVALID_DATA;
			goto exception;
		}
	}
	else
	{
		char typeStr[5];
		icns_print_err("icns_parse_family_data: Invalid icon family resource type! ('%s')\n",icns_type_str(resourceType,typeStr));
		error = ICNS_STATUS_INVALID_DATA;
		goto exception;
	}
	
	if(error == 0)
	{
		unsigned long	dataOffset = 0;
		icns_type_t	elementType = ICNS_NULL_TYPE;
		icns_size_t	elementSize = 0;
		
		// Skip past the icns header
		dataOffset = sizeof(icns_type_t) + sizeof(icns_size_t);
		
		// Iterate through the icns resource, converting the 'type' and 'size' values to native endian
		while( ((dataOffset+8) < resourceSize) && (error == 0) )
		{
			ICNS_READ_UNALIGNED_BE(elementType, dataPtr+dataOffset,sizeof(icns_type_t));
			ICNS_READ_UNALIGNED_BE(elementSize, dataPtr+dataOffset+4,sizeof(icns_size_t));
			
			#ifdef ICNS_DEBUG
			{
				char typeStr[5];
				printf("  checking element type... type is '%s'\n",icns_type_str(elementType,typeStr));
				printf("  checking element size... size is %d\n",elementSize);
			}
			#endif
			
			if( (elementSize < 8) || (dataOffset+elementSize > resourceSize) )
			{
				icns_print_err("icns_parse_family_data: Invalid element size! (%d)\n",elementSize);
				error = ICNS_STATUS_INVALID_DATA;
				goto exception;
			}
			
			// 'Fix' the value's endianess for working with with them
			ICNS_WRITE_UNALIGNED( dataPtr+dataOffset, elementType, sizeof(icns_type_t));
			ICNS_WRITE_UNALIGNED( dataPtr+dataOffset+4, elementSize, sizeof(icns_size_t));
			
			// Move on to the next element
			dataOffset += elementSize;
		}
		
	}
	
	if(error == 0)
		*iconFamilyOut = (icns_family_t *)(dataPtr);
	
exception:
	
	return error;
}

/***************************** icns_find_family_in_mac_resource **************************/

int icns_find_family_in_mac_resource(icns_size_t resDataSize, icns_byte_t *resData, icns_rsrc_endian_t fileEndian, icns_family_t **dataOut)
{
	icns_bool_t	error = ICNS_STATUS_OK;
	icns_bool_t	found = 0;
	icns_uint32_t	count = 0;
	
	icns_sint32_t	resHeadDataOffset = 0;
	icns_sint32_t	resHeadMapOffset = 0;
	icns_sint32_t	resHeadDataSize = 0;
	icns_sint32_t	resHeadMapSize = 0;
	
	icns_sint16_t	resMapAttributes = 0;
	icns_sint16_t	resMapTypeOffset = 0;
	icns_sint16_t	resMapNameOffset = 0;
	icns_sint16_t	resMapNumTypes = 0;
	
	icns_type_t	getResType = ICNS_FAMILY_TYPE;
	
	icns_size_t	iconDataSize = 0;
	icns_byte_t	*iconData = NULL;
	
	#ifdef ICNS_DEBUG
	printf("Parsing resource data...\n");
	printf("  total data size: %d (0x%08X)\n",resDataSize,resDataSize);
	#endif
	
	if(resDataSize < 16)
	{
		// rsrc header is 16 bytes - We cannot have a file of a smaller size.
		icns_print_err("icns_find_family_in_mac_resource: Unable to decode rsrc data! - Data size too small.\n");
		return ICNS_STATUS_INVALID_DATA;
	}
	
	// Load Resource Header to if we are dealing with a raw resource fork.
	if(fileEndian == ICNS_BE_RSRC) {
		ICNS_READ_UNALIGNED_BE(resHeadDataOffset, (resData+0),sizeof( icns_sint32_t));
		ICNS_READ_UNALIGNED_BE(resHeadMapOffset, (resData+4),sizeof( icns_sint32_t));
		ICNS_READ_UNALIGNED_BE(resHeadDataSize, (resData+8),sizeof( icns_sint32_t));
		ICNS_READ_UNALIGNED_BE(resHeadMapSize, (resData+12),sizeof( icns_sint32_t));
	} else if(fileEndian == ICNS_LE_RSRC) {
		ICNS_READ_UNALIGNED_LE(resHeadDataOffset, (resData+0),sizeof( icns_sint32_t));
		ICNS_READ_UNALIGNED_LE(resHeadMapOffset, (resData+4),sizeof( icns_sint32_t));
		ICNS_READ_UNALIGNED_LE(resHeadDataSize, (resData+8),sizeof( icns_sint32_t));
		ICNS_READ_UNALIGNED_LE(resHeadMapSize, (resData+12),sizeof( icns_sint32_t));
	}
	
	#ifdef ICNS_DEBUG
	printf("  data offset: %d (0x%08X)\n",resHeadDataOffset,resHeadDataOffset);
	printf("  map offset: %d (0x%08X)\n",resHeadMapOffset,resHeadMapOffset);
	printf("  data size: %d (0x%08X)\n",resHeadDataSize,resHeadDataSize);
	printf("  map size: %d (0x%08X)\n",resHeadMapSize,resHeadMapSize);
	#endif
	
	// Check to see if file is not a raw resource file
	if( (resHeadMapOffset+resHeadMapSize != resDataSize) || (resHeadDataOffset+resHeadDataSize != resHeadMapOffset) )
	{
		icns_print_err("icns_find_family_in_mac_resource: Invalid resource header!\n");
		error = ICNS_STATUS_INVALID_DATA;
		goto exception;
	}

	if(resHeadMapOffset+28 > resDataSize)
	{
		icns_print_err("icns_find_family_in_mac_resource: Invalid resource header!\n");
		error = ICNS_STATUS_INVALID_DATA;
		goto exception;
	}
	
	// Load Resource Map
	if(fileEndian == ICNS_BE_RSRC) {
		ICNS_READ_UNALIGNED_BE(resMapAttributes, (resData+resHeadMapOffset+0+22),sizeof( icns_sint16_t));
		ICNS_READ_UNALIGNED_BE(resMapTypeOffset, (resData+resHeadMapOffset+2+22),sizeof( icns_sint16_t));
		ICNS_READ_UNALIGNED_BE(resMapNameOffset, (resData+resHeadMapOffset+4+22),sizeof( icns_sint16_t));
		ICNS_READ_UNALIGNED_BE(resMapNumTypes, (resData+resHeadMapOffset+6+22),sizeof( icns_sint16_t));
	} else 	if(fileEndian == ICNS_LE_RSRC) {
		ICNS_READ_UNALIGNED_LE(resMapAttributes, (resData+resHeadMapOffset+0+22),sizeof( icns_sint16_t));
		ICNS_READ_UNALIGNED_LE(resMapTypeOffset, (resData+resHeadMapOffset+2+22),sizeof( icns_sint16_t));
		ICNS_READ_UNALIGNED_LE(resMapNameOffset, (resData+resHeadMapOffset+4+22),sizeof( icns_sint16_t));
		ICNS_READ_UNALIGNED_LE(resMapNumTypes, (resData+resHeadMapOffset+6+22),sizeof( icns_sint16_t));
	}
	
	// 0 == 1 here, so fix that
	resMapNumTypes = resMapNumTypes + 1;
	
	if( (resHeadMapOffset+resMapTypeOffset+2+(count*8)) > resDataSize)
	{
		icns_print_err("icns_find_family_in_mac_resource: Invalid resource map!\n");
		error = ICNS_STATUS_INVALID_DATA;
		goto exception;
	}
	
	#ifdef ICNS_DEBUG
	printf("  parsing resource map...\n");
	#endif
	
	for(count = 0; count < resMapNumTypes && found == 0; count++)
	{
		icns_type_t	resType = ICNS_NULL_TYPE;
		icns_sint16_t	resNumItems = 0;
		icns_sint16_t	resOffset = 0;
		
		ICNS_READ_UNALIGNED_BE(resType, (resData+resHeadMapOffset+resMapTypeOffset+2+(count*8)),sizeof( icns_type_t));
		
		if(fileEndian == ICNS_BE_RSRC) {
			ICNS_READ_UNALIGNED_BE(resNumItems, (resData+resHeadMapOffset+resMapTypeOffset+6+(count*8)),sizeof( icns_sint16_t));
			ICNS_READ_UNALIGNED_BE(resOffset, (resData+resHeadMapOffset+resMapTypeOffset+8+(count*8)),sizeof( icns_sint16_t));
		} else 	if(fileEndian == ICNS_LE_RSRC) {
			ICNS_READ_UNALIGNED_LE(resNumItems, (resData+resHeadMapOffset+resMapTypeOffset+6+(count*8)),sizeof( icns_sint16_t));
			ICNS_READ_UNALIGNED_LE(resOffset, (resData+resHeadMapOffset+resMapTypeOffset+8+(count*8)),sizeof( icns_sint16_t));
		}

		// 0 == 1 here, so fix that
		resNumItems = resNumItems + 1;
		
		#ifdef ICNS_DEBUG
		{
			char typeStr[5];
			printf("    found %d items of type '%s'\n",resNumItems, icns_type_str(resType,typeStr));
		}
		#endif
		
		if(resType == getResType)
		{
			icns_sint16_t	resID = 0;
			icns_sint8_t	resAttributes = 0;
			icns_sint16_t	resNameOffset = 0;
			icns_sint8_t	resNameLength = 0;
			char		resName[256] = {0};
			icns_sint32_t	resItemDataOffset = 0;
			icns_sint32_t	resItemDataSize = 0;
			icns_byte_t	*resItemData = NULL;
			
			found = 1;
			
			if(fileEndian == ICNS_BE_RSRC) {
				ICNS_READ_UNALIGNED_BE(resID, (resData+resHeadMapOffset+resMapTypeOffset+resOffset),sizeof( icns_sint16_t));
				ICNS_READ_UNALIGNED_BE(resNameOffset, (resData+resHeadMapOffset+resMapTypeOffset+resOffset+2),sizeof( icns_sint16_t));
				ICNS_READ_UNALIGNED_BE(resAttributes, (resData+resHeadMapOffset+resMapTypeOffset+resOffset+4),sizeof( icns_sint8_t));
			} else	if(fileEndian == ICNS_LE_RSRC) {
				ICNS_READ_UNALIGNED_LE(resID, (resData+resHeadMapOffset+resMapTypeOffset+resOffset),sizeof( icns_sint16_t));
				ICNS_READ_UNALIGNED_LE(resNameOffset, (resData+resHeadMapOffset+resMapTypeOffset+resOffset+2),sizeof( icns_sint16_t));
				ICNS_READ_UNALIGNED_LE(resAttributes, (resData+resHeadMapOffset+resMapTypeOffset+resOffset+4),sizeof( icns_sint8_t));
			}

			// Read in the resource name, if it exists (-1 indicates it doesn't)
			if(resNameOffset != -1)
			{
				if(fileEndian == ICNS_BE_RSRC) {
					ICNS_READ_UNALIGNED_BE(resNameLength, (resData+resHeadMapOffset+resMapNameOffset+resNameOffset),sizeof( icns_sint8_t));
				} else if(fileEndian == ICNS_LE_RSRC) {
					ICNS_READ_UNALIGNED_LE(resNameLength, (resData+resHeadMapOffset+resMapNameOffset+resNameOffset),sizeof( icns_sint8_t));
				}

				if(resNameLength > 0)
					memcpy(&resName[0],(resData+resHeadMapOffset+resMapNameOffset+resNameOffset+1),resNameLength);

				resName[resNameLength] = 0;
			}
			
			// Read three byte int starting at resHeadMapOffset+resMapTypeOffset+resOffset+5
			if(fileEndian == ICNS_BE_RSRC) {
				ICNS_READ_UNALIGNED_BE(resItemDataOffset, (resData+resHeadMapOffset+resMapTypeOffset+resOffset+5), 3);
			} else	if(fileEndian == ICNS_LE_RSRC) {
				ICNS_READ_UNALIGNED_LE(resItemDataOffset, (resData+resHeadMapOffset+resMapTypeOffset+resOffset+5), 3);
			}
			
			#ifdef ICNS_DEBUG
			printf("    data offset is: %d (0x%08X)\n",resItemDataOffset,resItemDataOffset);
			printf("    actual offset is: %d (0x%08X)\n",resHeadDataOffset+resItemDataOffset,resHeadDataOffset+resItemDataOffset);
			#endif

			if(fileEndian == ICNS_BE_RSRC) {
				ICNS_READ_UNALIGNED_BE(resItemDataSize, (resData+resHeadDataOffset+resItemDataOffset),sizeof( icns_sint32_t));
			} else	if(fileEndian == ICNS_LE_RSRC) {
				ICNS_READ_UNALIGNED_LE(resItemDataSize, (resData+resHeadDataOffset+resItemDataOffset),sizeof( icns_sint32_t));
			}

			#ifdef ICNS_DEBUG
			printf("    data size is: %d\n",resItemDataSize);
			#endif
			
			if( (resHeadDataOffset+resItemDataOffset) > resDataSize )
			{
				icns_print_err("icns_find_family_in_mac_resource: Resource icns id# %d has invalid data offset!\n",resID);
				error = ICNS_STATUS_INVALID_DATA;
				goto exception;
			}
			
			if( (resItemDataSize <= 0) || (resItemDataSize > (resDataSize-(resHeadDataOffset+resItemDataOffset+4))) )
			{
				char typeStr[5];
				icns_print_err("icns_find_family_in_mac_resource: Resource type '%s' id# %d has invalid size!\n",icns_type_str(resType,typeStr),resID);
				icns_print_err("icns_find_family_in_mac_resource: (size %d not within range of %d to %d)\n",resItemDataSize,1,(resDataSize-(resHeadDataOffset+resItemDataOffset+4)));
				error = ICNS_STATUS_INVALID_DATA;
				goto exception;
			}
			
			resItemData = (icns_byte_t*)malloc(resItemDataSize);
			
			if(resItemData != NULL)
			{
				memcpy( resItemData ,(resData+resHeadDataOffset+resItemDataOffset+4),resItemDataSize);
				
				iconDataSize = resItemDataSize;
				iconData = resItemData;
			}
			else
			{
				icns_print_err("icns_find_family_in_mac_resource: Unable to allocate memory block of size: %d!\n",resItemDataSize);
				error = ICNS_STATUS_NO_MEMORY;
				iconData = NULL;
			}
		}
	}

	if(found)
	{
		if(error == 0)
		{
			if(iconData != NULL)
			{
				if((error = icns_parse_family_data(iconDataSize,iconData,dataOut)))
				{
					icns_print_err("icns_parse_family_data: Error parsing icon family data!\n");
					*dataOut = NULL;	
				}
			}
			else
			{
				icns_print_err("icns_find_family_in_mac_resource: Found icon data is NULL!\n");
				error = ICNS_STATUS_NULL_PARAM;
			}
		}
	}
	else
	{
		char typeStr[5];		
		icns_print_err("icns_find_family_in_mac_resource: Unable to find data of type '%s' in resource file!\n",icns_type_str(getResType,typeStr));
		error = ICNS_STATUS_DATA_NOT_FOUND;
	}
	
exception:

	return error;
}


//**************** icns_read_macbinary_resource_fork *******************//
// Parses a MacBinary file resource fork
// Returns the resource fork type, creator, size, and data

int icns_read_macbinary_resource_fork(icns_size_t dataSize,icns_byte_t *dataPtr,icns_type_t *dataTypeOut, icns_type_t *dataCreatorOut,icns_size_t *parsedResSizeOut,icns_byte_t **parsedResDataOut)
{
	// This code is based off information from the MacBinaryIII specification at
	// http://web.archive.org/web/*/www.lazerware.com/formats/macbinary/macbinary_iii.html
	
	int		error = ICNS_STATUS_OK;
	icns_type_t	dataType = ICNS_NULL_TYPE;
	icns_type_t	dataCreator = ICNS_NULL_TYPE;
	icns_bool_t	isValid = 0;
	icns_sint16_t	secondHeaderLength = 0;
	icns_sint32_t   fileDataPadding = 0;
	icns_sint32_t   fileDataSize = 0;
	icns_sint32_t   resourceDataSize = 0;
	icns_sint32_t   resourceDataStart = 0;
	icns_byte_t	*resourceDataPtr = NULL;
	
	if(dataPtr == NULL)
	{
		icns_print_err("icns_read_macbinary_resource_fork: macbinary data is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	if(dataTypeOut != NULL)
		*dataTypeOut = ICNS_NULL_TYPE;
	
	if(dataCreatorOut != NULL)
		*dataCreatorOut = ICNS_NULL_TYPE;
	
	if(parsedResSizeOut == NULL)
	{
		icns_print_err("icns_read_macbinary_resource_fork: parsedResSizeOut is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	else
	{
		*parsedResSizeOut = 0;
	}

	if(parsedResDataOut == NULL)
	{
		icns_print_err("icns_read_macbinary_resource_fork: parsedResSizeOut is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	else
	{
		*parsedResDataOut = NULL;
	}
	
	if(dataSize < 128)
	{
		// MacBinary header is 128 bytes - We cannot have a file of a smaller size.
		icns_print_err("icns_read_macbinary_resource_fork: Unable to decode MacBinary data! - Data size too small.\n");
		return ICNS_STATUS_INVALID_DATA;
	}
	
	// Read headers
	ICNS_READ_UNALIGNED_BE(dataType, (dataPtr+65),sizeof( icns_type_t));
	ICNS_READ_UNALIGNED_BE(dataCreator, (dataPtr+69),sizeof( icns_type_t));

	// Checking for valid MacBinary data...
	if(dataType == ICNS_MACBINARY_TYPE)
	{
		// Valid MacBinary III file
		isValid = 1;
	}
	else
	{
		icns_sint8_t	byte00 = 0;
		icns_sint8_t	byte74 = 0;
		icns_sint8_t	byte82 = 0;
		
		isValid = 1;
		
		ICNS_READ_UNALIGNED_BE(byte00, (dataPtr+0),sizeof( icns_sint8_t));
		ICNS_READ_UNALIGNED_BE(byte74, (dataPtr+74),sizeof( icns_sint8_t));
		ICNS_READ_UNALIGNED_BE(byte74, (dataPtr+82),sizeof( icns_sint8_t));
		
		// Bytes 0, 74, and 82 should all be zero in a valid MacBinary file
		if( ( byte00 != 0 ) || ( byte74 != 0 ) || ( byte82 != 0 ) )
			isValid = 0;
	}
	
	if( !isValid )
	{
		icns_print_err("icns_read_macbinary_resource_fork: Invalid MacBinary data!\n");
		return ICNS_STATUS_INVALID_DATA;
	}
	
	// If mac file type is requested, pass it up
	if(dataTypeOut != NULL)
		*dataTypeOut = dataType;

	// If mac file creator is requested, pass it up
	if(dataCreatorOut != NULL)
		*dataCreatorOut = dataCreator;
	
	// Start MacBinary Parsing routines
	
	// Load up the data lengths
	ICNS_READ_UNALIGNED_BE(secondHeaderLength, (dataPtr+120),sizeof( icns_sint16_t));
	ICNS_READ_UNALIGNED_BE(fileDataSize, (dataPtr+83),sizeof( icns_sint32_t));
	ICNS_READ_UNALIGNED_BE(resourceDataSize, (dataPtr+87),sizeof( icns_sint32_t));
	
	// Calculate extra padding length for forks
	fileDataPadding = (((fileDataSize + 127) >> 7) << 7) - fileDataSize;
	
	// Calculate starting offsets for data
	resourceDataStart = fileDataSize + fileDataPadding + 128;
	
	// Check that we are not reading invalid memory
	if( (resourceDataStart < 128) || (resourceDataStart > dataSize) ) {
		icns_print_err("icns_read_macbinary_resource_fork: Invalid resource data start!\n");
	       	return ICNS_STATUS_INVALID_DATA;
	}
	if( (resourceDataSize < 16) || (resourceDataSize > (dataSize-128))  ) {
		icns_print_err("icns_read_macbinary_resource_fork: Invalid resource data size!\n");
		return ICNS_STATUS_INVALID_DATA;
	}
	if( (resourceDataStart+resourceDataSize > dataSize) ) {
		icns_print_err("icns_read_macbinary_resource_fork: Invalid resource data location!\n");
		return ICNS_STATUS_INVALID_DATA;
	}

	resourceDataPtr = (icns_byte_t *)malloc(resourceDataSize);
	
	if(resourceDataPtr == NULL)
	{
		icns_print_err("icns_read_macbinary_resource_fork: Unable to allocate memory block of size: %d!\n",(int)resourceDataSize);
		return ICNS_STATUS_NO_MEMORY;
	}

	memcpy(resourceDataPtr,(dataPtr+resourceDataStart),resourceDataSize);
	
	*parsedResSizeOut = resourceDataSize;
	*parsedResDataOut = resourceDataPtr;

	return error;
}


//**************** icns_read_apple_encoded_resource_fork *******************//
// Parses a resource fork from an Apple Single or Apple Double file
// Returns the resource fork type, creator, size, and data

int icns_read_apple_encoded_resource_fork(icns_size_t dataSize,icns_byte_t *dataPtr,icns_type_t *dataTypeOut, icns_type_t *dataCreatorOut,icns_size_t *parsedResSizeOut,icns_byte_t **parsedResDataOut)
{
	icns_uint32_t magic = 0;
	icns_byte_t   version[4] = {0,0,0,0};
	icns_byte_t   filler[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	icns_uint16_t	entries = 0;
	icns_uint16_t	curEntry = 0;
	icns_uint32_t	entryID = 0;
	icns_uint32_t	entryOffset = 0;
	icns_uint32_t	entryLen = 0;
	icns_type_t	fileType = ICNS_NULL_TYPE;
	icns_type_t	fileCreator = ICNS_NULL_TYPE;
	icns_sint32_t   resourceDataSize = 0;
	icns_sint32_t   resourceDataStart = 0;
	icns_byte_t	*resourceDataPtr = NULL;

	if(dataPtr == NULL)
	{
		icns_print_err("icns_read_apple_encoded_resource_fork: apple encoded data is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	if(dataTypeOut != NULL)
		*dataTypeOut = ICNS_NULL_TYPE;
	
	if(dataCreatorOut != NULL)
		*dataCreatorOut = ICNS_NULL_TYPE;
	
	if(parsedResSizeOut == NULL)
	{
		icns_print_err("icns_read_apple_encoded_resource_fork: parsedResSizeOut is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	else
	{
		*parsedResSizeOut = 0;
	}

	if(parsedResDataOut == NULL)
	{
		icns_print_err("icns_read_apple_encoded_resource_fork: parsedResSizeOut is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	else
	{
		*parsedResDataOut = NULL;
	}
	
	if(dataSize < 26)
	{
		// AppleSingle/AppleDouble header is 26 bytes - We cannot have a file of a smaller size.
		icns_print_err("icns_read_apple_encoded_resource_fork: Unable to decode apple encoded data! - Data size too small.\n");
		return ICNS_STATUS_INVALID_DATA;
	}

	ICNS_READ_UNALIGNED_BE(magic, (dataPtr+0),sizeof(icns_uint32_t));
	ICNS_READ_UNALIGNED(version[0], (dataPtr+4), 4);
	ICNS_READ_UNALIGNED(filler[0], (dataPtr+8), 16);
	ICNS_READ_UNALIGNED_BE(entries, (dataPtr+24),sizeof(icns_uint16_t));

	#ifdef ICNS_DEBUG
	printf("    magic is: 0x%08X\n",magic);
	printf("    version is: %02X%02X%02X%02X\n",version[0],version[1],version[2],version[3]);
	printf("    filler is: ");
	printf("%02X%02X%02X%02X",filler[0],filler[1],filler[2],filler[3]);
	printf("%02X%02X%02X%02X",filler[4],filler[5],filler[6],filler[7]);
	printf("%02X%02X%02X%02X",filler[8],filler[9],filler[10],filler[11]);
	printf("%02X%02X%02X%02X",filler[12],filler[13],filler[14],filler[15]);
	printf("\n");
	printf("    entries count: %d\n",entries);
	#endif

	if(dataSize < (26 + (entries * 12) + 8) )
	{
		icns_print_err("icns_read_apple_encoded_resource_fork: Unable to decode apple encoded data! - Data size too small.\n");
		return ICNS_STATUS_INVALID_DATA;
	}

	if( (magic != ICNS_APPLE_SINGLE_MAGIC) && (magic != ICNS_APPLE_DOUBLE_MAGIC) )
	{
		icns_print_err("icns_read_apple_encoded_resource_fork: Unable to decode apple encoded data! - invalid magic bytes.\n");
		return ICNS_STATUS_INVALID_DATA;
	}

	ICNS_READ_UNALIGNED(fileType, (dataPtr+26+(entries*12)+0),sizeof( icns_type_t));
	ICNS_READ_UNALIGNED(fileCreator, (dataPtr+26+(entries*12)+4),sizeof( icns_type_t));

	// If mac file type is requested, pass it up
	if(dataTypeOut != NULL)
		*dataTypeOut = fileType;

	// If mac file creator is requested, pass it up
	if(dataCreatorOut != NULL)
		*dataCreatorOut = fileCreator;

	for (curEntry = 0; curEntry < entries; curEntry++)
	{
		ICNS_READ_UNALIGNED_BE(entryID, (dataPtr+26+(curEntry*12)+0),sizeof(icns_uint32_t));
		ICNS_READ_UNALIGNED_BE(entryOffset, (dataPtr+26+(curEntry*12)+4),sizeof(icns_uint32_t));
		ICNS_READ_UNALIGNED_BE(entryLen, (dataPtr+26+(curEntry*12)+8),sizeof(icns_uint32_t));

		#ifdef ICNS_DEBUG
		printf("    decoding entry...\n");
		printf("    entryID is: %d\n",entryID);
		printf("    entryOffset is: %d\n",entryOffset);
		printf("    entryLen is: %d",entryLen);
		#endif

		switch (entryID) {
		case ICNS_APPLE_ENC_DATA:
			// Do nothing
			break;
		case ICNS_APPLE_ENC_RSRC:
			resourceDataStart = entryOffset;
			resourceDataSize = entryLen;
			break;
		}
	}

	if( (resourceDataStart < 38) || (resourceDataStart > dataSize) ) {
		icns_print_err("icns_read_apple_encoded_resource_fork: Invalid resource data start!\n");
	       	return ICNS_STATUS_INVALID_DATA;
	}
	if( (resourceDataSize < 16) || (resourceDataSize > (dataSize-38))  ) {
		icns_print_err("icns_read_apple_encoded_resource_fork: Invalid resource data size!\n");
		return ICNS_STATUS_INVALID_DATA;
	}

	resourceDataPtr = (icns_byte_t *)malloc(resourceDataSize);

	if(resourceDataPtr == NULL)
	{
		icns_print_err("icns_read_macbinary_resource_fork: Unable to allocate memory block of size: %d!\n",(int)resourceDataSize);
		return ICNS_STATUS_NO_MEMORY;
	}

	memcpy(resourceDataPtr,(dataPtr+resourceDataStart),resourceDataSize);

	*parsedResSizeOut = resourceDataSize;
	*parsedResDataOut = resourceDataPtr;

	return ICNS_STATUS_OK;	
}


//**************** icns_icns_header_check *******************//
icns_bool_t icns_icns_header_check(icns_size_t dataSize,icns_byte_t *dataPtr)
{
	icns_type_t	resourceType = ICNS_NULL_TYPE;
	icns_size_t	resourceSize = 0;
	
	if(dataSize < 8)
		return 0;
	
	if(dataPtr == 0)
		return 0;
	
	ICNS_READ_UNALIGNED_BE(resourceType, dataPtr,sizeof(icns_type_t));
	ICNS_READ_UNALIGNED_BE(resourceSize, dataPtr + 4,sizeof(icns_size_t));
	
	if(resourceType != ICNS_FAMILY_TYPE)
		return 0;
	
	if(dataSize != resourceSize )
		return 0;
	
	return 1;
}

//**************** icns_rsrc_header_check *******************//
icns_bool_t icns_rsrc_header_check(icns_size_t dataSize,icns_byte_t *dataPtr,icns_rsrc_endian_t fileEndian)
{
	icns_sint32_t	resHeadDataOffset = 0;
	icns_sint32_t	resHeadMapOffset = 0;
	icns_sint32_t	resHeadDataSize = 0;
	icns_sint32_t	resHeadMapSize = 0;
	
	if(dataSize < 16)
		return 0;
	
	// Load Resource Header to if we are dealing with a raw little endian resource fork.
	if(fileEndian == ICNS_BE_RSRC) {
		ICNS_READ_UNALIGNED_BE(resHeadDataOffset, (dataPtr+0),sizeof( icns_sint32_t));
		ICNS_READ_UNALIGNED_BE(resHeadMapOffset, (dataPtr+4),sizeof( icns_sint32_t));
		ICNS_READ_UNALIGNED_BE(resHeadDataSize, (dataPtr+8),sizeof( icns_sint32_t));
		ICNS_READ_UNALIGNED_BE(resHeadMapSize, (dataPtr+12),sizeof( icns_sint32_t));
	} else if(fileEndian == ICNS_LE_RSRC) {
		ICNS_READ_UNALIGNED_LE(resHeadDataOffset, (dataPtr+0),sizeof( icns_sint32_t));
		ICNS_READ_UNALIGNED_LE(resHeadMapOffset, (dataPtr+4),sizeof( icns_sint32_t));
		ICNS_READ_UNALIGNED_LE(resHeadDataSize, (dataPtr+8),sizeof( icns_sint32_t));
		ICNS_READ_UNALIGNED_LE(resHeadMapSize, (dataPtr+12),sizeof( icns_sint32_t));
	} else {
		return 0;
	}

	// Check to see if file is not a raw resource file
	if( (resHeadMapOffset+resHeadMapSize != dataSize) || (resHeadDataOffset+resHeadDataSize != resHeadMapOffset) )
		return 0;

	if(resHeadMapOffset+28 > dataSize)
		return 0;
	
	return 1;
}

//**************** icns_macbinary_header_check *******************//
icns_bool_t icns_macbinary_header_check(icns_size_t dataSize,icns_byte_t *dataPtr)
{
	icns_bool_t	isValid = 0;
	icns_type_t	dataType = ICNS_NULL_TYPE;
	icns_type_t	dataCreator = ICNS_NULL_TYPE;
	icns_sint16_t	secondHeaderLength = 0;
	icns_sint32_t   fileDataPadding = 0;
	icns_sint32_t   fileDataSize = 0;
	icns_sint32_t   resourceDataSize = 0;
	icns_sint32_t   resourceDataStart = 0;
	
	if(dataPtr == NULL)
		return 0;
	
	if(dataSize < 128)
		return 0;
	
	ICNS_READ_UNALIGNED_BE(dataType, (dataPtr+65),sizeof( icns_type_t));
	ICNS_READ_UNALIGNED_BE(dataCreator, (dataPtr+69),sizeof( icns_type_t));

	// Checking for valid MacBinary data...
	if(dataType == ICNS_MACBINARY_TYPE)
	{
		// Valid MacBinary III file
		isValid = 1;
	}
	else
	{
		icns_sint8_t	byte00 = 0;
		icns_sint8_t	byte74 = 0;
		icns_sint8_t	byte82 = 0;
		
		isValid = 1;
		
		ICNS_READ_UNALIGNED_BE(byte00, (dataPtr+0),sizeof( icns_sint8_t));
		ICNS_READ_UNALIGNED_BE(byte74, (dataPtr+74),sizeof( icns_sint8_t));
		ICNS_READ_UNALIGNED_BE(byte74, (dataPtr+82),sizeof( icns_sint8_t));
		
		// Bytes 0, 74, and 82 should all be zero in a valid MacBinary file
		if( ( byte00 != 0 ) || ( byte74 != 0 ) || ( byte82 != 0 ) )
			isValid = 0;
	}
	
	if( !isValid )
		return 0;
	
	ICNS_READ_UNALIGNED_BE(secondHeaderLength, (dataPtr+120),sizeof( icns_sint16_t));
	ICNS_READ_UNALIGNED_BE(fileDataSize, (dataPtr+83),sizeof( icns_sint32_t));
	ICNS_READ_UNALIGNED_BE(resourceDataSize, (dataPtr+87),sizeof( icns_sint32_t));
	
	fileDataPadding = (((fileDataSize + 127) >> 7) << 7) - fileDataSize;
	
	resourceDataStart = fileDataSize + fileDataPadding + 128;
	
	if( (resourceDataStart < 128) || (resourceDataStart > dataSize) )
	       	return 0;
	
	if( (resourceDataSize < 16) || (resourceDataSize > (dataSize-128))  )
		return 0;
	
	if( (resourceDataStart+resourceDataSize > dataSize) )
		return 0;
	
	return 1;
}

//**************** icns_apple_encoded_header_check *******************//

icns_bool_t icns_apple_encoded_header_check(icns_size_t dataSize,icns_byte_t *dataPtr)
{
	icns_uint32_t magic = 0;
	icns_byte_t   version[4] = {0,0,0,0};
	icns_byte_t   filler[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	icns_uint16_t	entries = 0;
	
	if(dataPtr == NULL)
		return 0;	
	
	if(dataSize < 26)
		return 0;

	ICNS_READ_UNALIGNED_BE(magic, (dataPtr+0),sizeof(icns_uint32_t));
	ICNS_READ_UNALIGNED(version[0], (dataPtr+4), 4);
	ICNS_READ_UNALIGNED(filler[0], (dataPtr+8), 16);
	ICNS_READ_UNALIGNED_BE(entries, (dataPtr+24),sizeof(icns_uint16_t));


	if( (magic != ICNS_APPLE_SINGLE_MAGIC) && (magic != ICNS_APPLE_DOUBLE_MAGIC) )
		return 0;

	if(dataSize < (26 + (entries * 12) + 8) )
		return 0;

	return 1;
}
