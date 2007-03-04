// AreaUtils.cpp
//
// Copyright (c) 2003, Ingo Weinhold (bonefish@cs.tu-berlin.de)
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// 
// Except as contained in this notice, the name of a copyright holder shall
// not be used in advertising or otherwise to promote the sale, use or other
// dealings in this Software without prior written authorization of the
// copyright holder.

#include <OS.h>

#include "AreaUtils.h"
#include "Debug.h"
#include "Misc.h"

#ifndef USE_STANDARD_FUNCTIONS
#define USE_STANDARD_FUNCTIONS 0
#endif


// area_info_for
static
status_t
area_info_for(void *address, area_info *info)
{
	status_t error = B_OK;
	if (address) {
		// get the area ID for the ptr
		area_id area = area_for(address);
		// check if supplied pointer points to the beginning of the area
		if (area >= 0)
			error = get_area_info(area, info);
		else
			error = area;
	} else
		error = B_BAD_VALUE;
	return error;
}

// calloc
void *
AreaUtils::calloc(size_t nmemb, size_t size)
{
//PRINT(("AreaUtils::calloc(%lu, %lu)\n", nmemb, size));
	return AreaUtils::malloc(nmemb * size);
}

// free
void 
AreaUtils::free(void *ptr)
{
//PRINT(("AreaUtils::free(%p)\n", ptr));
#if USE_STANDARD_FUNCTIONS
	return ::free(ptr);
#else
	if (ptr) {
		// get the area for the pointer
		area_info info;
		if (area_info_for(ptr, &info) == B_OK) {
			if (ptr == info.address) {
				// everything is fine, delete the area
				delete_area(info.area);
			} else {
				INFORM(("WARNING: AreaUtils::free(%p): area begin is %p."
						"Ignored.\n", ptr, info.address));
			}
		}
	}
#endif
}

// malloc
void *
AreaUtils::malloc(size_t size)
{
//PRINT(("AreaUtils::malloc(%lu)\n", size));
#if USE_STANDARD_FUNCTIONS
	return ::malloc(size);
#else
	void *address = NULL;
	if (size > 0) {
		// round to multiple of page size
		size = (size + B_PAGE_SIZE - 1) / B_PAGE_SIZE * B_PAGE_SIZE;
		// create an area
#if USER
		area_id area = create_area("AreaUtils::malloc", &address,
								   B_ANY_ADDRESS, size, B_NO_LOCK,
								   B_WRITE_AREA | B_READ_AREA);
#else
		area_id area = create_area("AreaUtils::malloc", &address,
								   B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK,
								   B_READ_AREA | B_WRITE_AREA);
#endif
		if (area < 0)
			address = NULL;
	}
	return address;
#endif
}

// realloc
void *
AreaUtils::realloc(void * ptr, size_t size)
{
//PRINT(("AreaUtils::realloc(%p, %lu)\n", ptr, size))
#if USE_STANDARD_FUNCTIONS
	return ::realloc(ptr, size);
#else
	void *newAddress = NULL;
	if (size == 0) {
		AreaUtils::free(ptr);
	} else if (ptr) {
		// get the area for the pointer
		area_info info;
		if (area_info_for(ptr, &info) == B_OK) {
			if (ptr == info.address) {
				// round to multiple of page size
				size = (size + B_PAGE_SIZE - 1) / B_PAGE_SIZE * B_PAGE_SIZE;
				if (size == info.size) {
					// nothing to do
					newAddress = ptr;
				} else if (resize_area(info.area, size) == B_OK) {
					// resizing the area went fine
					newAddress = ptr;
				} else {
					// resizing the area failed: we need to allocate a new one
					newAddress = AreaUtils::malloc(size);
					if (newAddress) {
						memcpy(newAddress, ptr, min(size, info.size));
						delete_area(info.area);
					}
				}
			} else {
				INFORM(("WARNING: AreaUtils::realloc(%p): area begin is %p."
						"Ignored.\n", ptr, info.address));
			}
		}
	}
	return newAddress;
#endif
}

