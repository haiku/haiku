/*
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of 
 * the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include "pes.h"


status_t
pes_extract(const uint8 *pes_data, size_t pes_size, const uint8 **data, size_t *size)
{
	if (pes_size <= 9)
		return B_ERROR;
	
	if (pes_data[0] || pes_data[1] || pes_data[2] != 1)
		return B_ERROR;
		
	size_t header_size = 9 + pes_data[8];
	
	if (pes_size <= header_size)
		return B_ERROR;
		
//	printf("header size %d\n", header_size);
	
	pes_data += header_size;
	pes_size -= header_size;

	*data = pes_data;
	*size = pes_size;
	
	return B_OK;
}


status_t
pes_stream_id(const uint8 *pes_data, size_t pes_size, int *stream_id)
{
	if (pes_size <= 9)
		return B_ERROR;
	
	if (pes_data[0] || pes_data[1] || pes_data[2] != 1)
		return B_ERROR;

	*stream_id = pes_data[3];

	return B_OK;	
}

