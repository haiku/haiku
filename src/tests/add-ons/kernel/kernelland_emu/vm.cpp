/*
 * Copyright 2002-2023, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de.
 *		Axel Dörfler, axeld@pinc-software.de.
 */

#include <string.h>

#include <KernelExport.h>

#include <vm/vm_page.h>


extern "C" status_t
user_memcpy(void *to, const void *from, size_t size)
{
	memcpy(to, from, size);
	return B_OK;
}


extern "C" ssize_t
user_strlcpy(char *to, const char *from, size_t size)
{
	return strlcpy(to, from, size);
}


extern "C" status_t
user_memset(void* target, char data, size_t length)
{
	memset(target, data, length);
	return B_OK;
}


page_num_t
vm_page_num_pages(void)
{
	return 65536;
		// TODO: 256 MB. Return real value?
}
