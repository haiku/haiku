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
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "Packet.h"

#define PACKET
#ifdef PACKET
  #define TRACE printf
#else
  #define TRACE(a...)
#endif


Packet::Packet(size_t init_size)
 :	fBuffer(malloc(init_size))
 ,	fBufferSize(0)
 ,	fBufferSizeMax(init_size)
 ,	fTimeStamp(0)
{
}


Packet::Packet(const Packet &clone)
{
 	fBufferSize = clone.Size();
	fBufferSizeMax = fBufferSize;
	fBuffer = malloc(fBufferSize);
	fTimeStamp = clone.TimeStamp();
	memcpy(fBuffer, clone.Data(), fBufferSize);
}


Packet::Packet(const void *data, size_t size, bigtime_t time_stamp)
 :	fBuffer(malloc(size))
 ,	fBufferSize(size)
 ,	fBufferSizeMax(size)
 ,	fTimeStamp(time_stamp)
{
	memcpy(fBuffer, data, size);
}


Packet::~Packet()
{
	free(fBuffer);
}


void
Packet::AddData(const void *data, size_t size)
{
	if (fBufferSize + size > fBufferSizeMax) {
		fBufferSizeMax = (fBufferSize + size + 8191) & ~8191;
		fBuffer = realloc(fBuffer, fBufferSizeMax);
	}
	
	memcpy((char *)fBuffer + fBufferSize, data, size);
	fBufferSize += size;
}
