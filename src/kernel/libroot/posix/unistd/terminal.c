/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <unistd.h>
#include <stdio.h>


int
isatty(int file)
{
	// ToDo: please fix me!
	return file < 3;
}

