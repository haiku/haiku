/*------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		areafunc.c
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	C functions for interfacing with the BGET pool allocator
//  
//------------------------------------------------------------------------------*/
#include <OS.h>
#include "bget.h"
#include <stdio.h>

void set_area_buffer_management(void);
void * expand_area_storage(long size);
void contract_area_storage(void *buffer);

void set_area_buffer_management(void)
{
	bectl(NULL, expand_area_storage, contract_area_storage, B_PAGE_SIZE);
}

void * expand_area_storage(long size)
{
	long areasize=0;
	area_id a;
	int *parea;
	
	if(size<B_PAGE_SIZE)
		areasize=B_PAGE_SIZE;
	else
	{
		if((size % B_PAGE_SIZE)!=0)
			areasize=((long)(size/B_PAGE_SIZE)+1)*B_PAGE_SIZE;
		else
			areasize=size;
	}
	a=create_area("bitmap_area",(void**)&parea,B_ANY_ADDRESS,areasize,
			B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if(a==B_BAD_VALUE ||
	   a==B_NO_MEMORY ||
	   a==B_ERROR)
	{
	   	printf("PANIC: BitmapManager couldn't allocate area!!\n");
	   	return NULL;
	}

	return parea;
}

void contract_area_storage(void *buffer)
{
	area_id trash=area_for(buffer);

	if(trash==B_ERROR)
		return;
	delete_area(trash);
}
