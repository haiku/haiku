/*
File:       icns_rle24.c
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
//***************************** icns_decode_rle24_data ****************************//
// Decode a rgb 24 bit rle encoded data stream into 32 bit argb (alpha is ignored)

int icns_decode_rle24_data(icns_size_t rawDataSize, icns_byte_t *rawDataPtr,icns_size_t expectedPixelCount, icns_size_t *dataSizeOut, icns_byte_t **dataPtrOut)
{
	icns_uint8_t	colorOffset = 0;
	icns_byte_t	colorValue = 0;
	icns_uint8_t	runLength = 0;
	icns_uint32_t	dataOffset = 0;
	icns_uint32_t	pixelOffset = 0;
	icns_uint32_t	i = 0;
	icns_byte_t	*destIconData = NULL;	// Decompressed Raw Icon Data
	icns_uint32_t	destIconDataSize = 0;
	
	if(rawDataPtr == NULL)
	{
		icns_print_err("icns_decode_rle24_data: rle decoder data in ptr is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	if(dataSizeOut == NULL)
	{
		icns_print_err("icns_decode_rle24_data: rle decoder data out size ref is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	if(dataPtrOut == NULL)
	{
		icns_print_err("icns_decode_rle24_data: rle decoder data out ptr is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	// Calculate required data storage (pixels * 4 channels)
	destIconDataSize = expectedPixelCount * 4;
	
	#ifdef ICNS_DEBUG
		printf("Compressed RLE data size is %d\n",(int)rawDataSize);
		printf("Decompressed will be %d bytes (%d pixels)\n",(int)destIconDataSize,(int)expectedPixelCount);
	#endif
	
	if( (*dataSizeOut != destIconDataSize) || (*dataPtrOut == NULL) )
	{
		if(*dataPtrOut != NULL)
			free(*dataPtrOut);
		
		// Allocate the block for the decoded memory and set to 0
		destIconData = (icns_byte_t *)malloc(destIconDataSize);
		if(!destIconData)
		{
			icns_print_err("icns_decode_rle24_data: Unable to allocate memory block of size: %d ($s:%m)!\n",(int)destIconDataSize);
			return ICNS_STATUS_NO_MEMORY;
		}
		memset(destIconData,0,destIconDataSize);
	}
	else
	{
		destIconData = *dataPtrOut;
	}
	
	#ifdef ICNS_DEBUG
		printf("Decoding RLE data into RGB pixels...\n");
	#endif

	// What's this??? In the 128x128 icons, we need to start 4 bytes
	// ahead. There is often a NULL padding here for some reason. If
	// we don't, the red channel will be off by 2 pixels, or worse
	if( *((icns_uint32_t*)rawDataPtr) == 0x00000000 )
	{
		#ifdef ICNS_DEBUG
		printf("4 byte null padding found in rle data!\n");
		#endif
		dataOffset = 4;
	}
	else
	{
		dataOffset = 0;
	}
	
	// Data is stored in red run, green run,blue run
	// So we decompress to pixel format RGBA
	// RED:   byte[0], byte[4], byte[8]  ...
	// GREEN: byte[1], byte[5], byte[9]  ...
	// BLUE:  byte[2], byte[6], byte[10] ...
	// ALPHA: byte[3], byte[7], byte[11] do nothing with these bytes
	for(colorOffset = 0; colorOffset < 3; colorOffset++)
	{
		pixelOffset = 0;
		while((pixelOffset < expectedPixelCount) && (dataOffset < rawDataSize))
		{
			if( (rawDataPtr[dataOffset] & 0x80) == 0)
			{
				// Top bit is clear - run of various values to follow
				runLength = (0xFF & rawDataPtr[dataOffset++]) + 1; // 1 <= len <= 128
				for(i = 0; (i < runLength) && (pixelOffset < expectedPixelCount) && (dataOffset < rawDataSize); i++) {
					destIconData[(pixelOffset * 4) + colorOffset] = rawDataPtr[dataOffset++];
					pixelOffset++;
				}
			}
			else
			{
				// Top bit is set - run of one value to follow
				runLength = (0xFF & rawDataPtr[dataOffset++]) - 125; // 3 <= len <= 130
				// Set the value to the color shifted to the correct bit offset
				colorValue = rawDataPtr[dataOffset++];
				
				for(i = 0; (i < runLength) && (pixelOffset < expectedPixelCount); i++) {
					destIconData[(pixelOffset * 4) + colorOffset] = colorValue;
					pixelOffset++;
				}
			}
		}
	}
	
	*dataSizeOut = destIconDataSize;
	*dataPtrOut = destIconData;
	
	return ICNS_STATUS_OK;
}

//***************************** icns_encode_rle24_data *******************************************//
// Encode an 32 bit argb data stream into a 24 bit rgb rle encoded data stream (alpha is ignored)

int icns_encode_rle24_data(icns_size_t dataSizeIn, icns_byte_t *dataPtrIn,icns_size_t *dataSizeOut, icns_byte_t **dataPtrOut)
{
	unsigned long	dataInCount = 0;
	unsigned long 	dataInChanSize = 0;
	icns_byte_t	*rgbaDataPtr = NULL;
	icns_sint8_t	*dataTemp = NULL;
	unsigned long	dataTempCount = 0;
	icns_uint8_t	colorOffset = 0;
	icns_uint8_t	dataByte = 0;
	icns_uint8_t	*dataRun = NULL;
	icns_bool_t	runType = 0;
	icns_uint8_t	runLength = 0; // Runs will never go over 130, one byte is ok
	int		runCount = 0;
	
	if(dataPtrIn == NULL)
	{
		icns_print_err("icns_encode_rle24_data: rle encoder data in ptr is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	if(dataSizeOut == NULL)
	{
		icns_print_err("icns_encode_rle24_data: rle encoder data out size ref is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}
	
	if(dataPtrOut == NULL)
	{
		icns_print_err("icns_encode_rle24_data: rle encoder data out ptr ref is NULL!\n");
		return ICNS_STATUS_NULL_PARAM;
	}

	// Assumptions of what icns rle data is all about:
	// A) Each channel is encoded indepenent of the next.
	// B) An encoded channel looks like this:
	//    0xRL 0xCV 0xCV 0xRL 0xCV - RL is run-length and CV is color value.
	// C) There are two types of runs
	//    1) Run of same value - high bit of RL is set
	//    2) Run of differing values - high bit of RL is NOT set
	// D) 0xRL also has two ranges
	//    1) for set high bit RL, 3 to 130
	//    2) for clr high bit RL, 1 to 128
	// E) 0xRL byte is therefore set as follows:
	//    1) for same values, RL = RL - 1
	//    2) different values, RL = RL + 125
	//    3) both methods will automatically set the high bit appropriately
	// F) 0xCV byte are set accordingly
	//    1) for differing values, run of all differing values
	//    2) for same values, only one byte of that values
	// Estimations put the absolute worst case scenario as the
	// final compressed data being slightly LARGER. So we need to be
	// careful about allocating memory. (Did I miss something?)
	// tests seem to indicate it will never be larger than the original
	
	// This block is for the new RLE encoded data - make it 25% larger
	dataTemp = (icns_sint8_t *)malloc(dataSizeIn + (dataSizeIn / 4));
	if(dataTemp == NULL)
	{
		icns_print_err("icns_encode_rle24_data: Unable to allocate memory block of size: %d!\n",(int)dataSizeIn);
		return ICNS_STATUS_NO_MEMORY;
	}
	memset(dataTemp,0,dataSizeIn);
	
	// This block is for a run of RLE encoded data
	dataRun = (icns_uint8_t *)malloc(140);
	if(dataRun == NULL)
	{
		icns_print_err("icns_encode_rle24_data: Unable to allocate memory block of size: %d!\n",140);
		free(dataTemp);
		return ICNS_STATUS_NO_MEMORY;
	}
	memset(dataRun,0,140);
	
	// Set the data ptr
	rgbaDataPtr = (icns_byte_t *)dataPtrIn;
	
	// There's always going to be 4 channels in this
	// so we want our counter to increment through
	// channels, not bytes....
	dataInChanSize = dataSizeIn / 4;
	
	// Move forward 4 bytes for 128 size - who knows why this should be
	if(dataSizeIn >= 65536)
		dataTempCount = 4;
	
	// Data is stored in red run, green run,blue run
	// So we compress from pixel format RGBA
	// RED:   byte[0], byte[4], byte[8]  ...
	// GREEN: byte[1], byte[5], byte[9]  ...
	// BLUE:  byte[2], byte[6], byte[10] ...
	// ALPHA: byte[3], byte[7], byte[11] do nothing with these bytes
	for(colorOffset = 0; colorOffset < 3; colorOffset++)
	{
		runCount = 0;
		
		// Set the first byte of the run...
		dataRun[0] = *(rgbaDataPtr+colorOffset);
		
		// Start with a runlength of 1 for the first byte
		runLength = 1;
		
		// Assume that the run will be different for now... We can change this later
		runType = 0; // 0 for low bit (different), 1 for high bit (same)	
		
		// Start one byte ahead
		for(dataInCount = 1; dataInCount < dataInChanSize; dataInCount++)
		{
			dataByte = *(rgbaDataPtr+colorOffset+(dataInCount*4));
			
			if(runLength < 2)
			{
				// Simply append to the current run
				dataRun[runLength++] = dataByte;
			}
			else if(runLength == 2)
			{
				// Decide here if the run should be same values or different values
				
				// If the last three values were the same, we can change to a same-type run
				if( (dataByte == dataRun[runLength-1]) && (dataByte == dataRun[runLength-2]) )
					runType = 1;
				else
					runType = 0;
				
				dataRun[runLength++] = dataByte;
			}
			else // Greater than or equal to 2
			{
				if(runType == 0 && runLength < 128) // Different type run
				{
					// If the new value matches both of the last two values, we have a new
					// same-type run starting with the previous two bytes
					if( (dataByte == dataRun[runLength-1]) && (dataByte == dataRun[runLength-2]) )
					{
						// Set the RL byte
						*(dataTemp+dataTempCount) = runLength - 3;
						dataTempCount++;
						
						// Copy 0 to runLength-2 bytes to the RLE data here
						memcpy( dataTemp+dataTempCount , dataRun , runLength - 2 );
						dataTempCount = dataTempCount + (runLength - 2);
						runCount++;
						
						// Set up the new same-type run
						dataRun[0] = dataRun[runLength-2];
						dataRun[1] = dataRun[runLength-1];
						dataRun[2] = dataByte;
						runLength = 3;
						runType = 1;
					}
					else // They don't match, so we can proceed
					{
						dataRun[runLength++] = dataByte;
					}
				}
				else if(runType == 1 && runLength < 130) // Same type run
				{
					// If the new value matches both of the last two values, we
					// can safely continue
					if( (dataByte == dataRun[runLength-1]) && (dataByte == dataRun[runLength-2]) )
					{
						dataRun[runLength++] = dataByte;
					}
					else // They don't match, so we need to start a new run
					{
						// Set the RL byte
						*(dataTemp+dataTempCount) = runLength + 125;
						dataTempCount++;
						
						// Only copy the first byte, since all the remaining values are identical
						*(dataTemp+dataTempCount) = dataRun[0];
						dataTempCount++;
						runCount++;
						
						// Copy 0 to runLength bytes to the RLE data here
						dataRun[0] = dataByte;
						runLength = 1;
						runType = 0;
					}
				}
				else // Exceeded run limit, need to start a new one
				{
					if(runType == 0)
					{
						// Set the RL byte low
						*(dataTemp+dataTempCount) = runLength - 1;
						dataTempCount++;
						
						// Copy 0 to runLength bytes to the RLE data here
						memcpy( dataTemp+dataTempCount , dataRun , runLength );
						dataTempCount = dataTempCount + runLength;
					}
					else if(runType == 1)
					{
						// Set the RL byte high
						*(dataTemp+dataTempCount) = runLength + 125;
						dataTempCount++;
						
						// Only copy the first byte, since all the remaining values are identical
						*(dataTemp+dataTempCount) = dataRun[0];
						dataTempCount++;
					}
					
					runCount++;
					
					// Copy 0 to runLength bytes to the RLE data here
					dataRun[0] = dataByte;
					runLength = 1;
					runType = 0;
				}
			}
		}
		
		// Copy the end of the last run
		if(runLength > 0)
		{
			if(runType == 0)
			{
				// Set the RL byte low
				*(dataTemp+dataTempCount) = runLength - 1;
				dataTempCount++;
				
				// Copy 0 to runLength bytes to the RLE data here
				memcpy( dataTemp+dataTempCount , dataRun , runLength );
				dataTempCount = dataTempCount + runLength;
			}
			else if(runType == 1)
			{
				// Set the RL byte high
				*(dataTemp+dataTempCount) = runLength + 125;
				dataTempCount++;
				
				// Only copy the first byte, since all the remaining values are identical
				*(dataTemp+dataTempCount) = dataRun[0];
				dataTempCount++;
			}
			
			runCount++;
		}
	}
	
	free(dataRun);
	
	// This block is for the final encoded data
	(*dataPtrOut) = (icns_uint8_t *)malloc(dataTempCount);
	if((*dataPtrOut) == NULL)
	{
		icns_print_err("icns_encode_rle24_data: Unable to allocate memory block of size: %d!\n",dataTempCount);
		free(dataTemp);
		return ICNS_STATUS_NO_MEMORY;
	}
	
	*dataSizeOut = dataTempCount;
	memcpy( (*dataPtrOut), dataTemp, dataTempCount);

	free(dataTemp);
	
	return ICNS_STATUS_OK;
}

