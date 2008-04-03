/* 
** Copyright 2008, James Woodcock. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <unistd.h>
#include <kernel/OS.h>

int
getpagesize(void)
{
	return B_PAGE_SIZE;
}
