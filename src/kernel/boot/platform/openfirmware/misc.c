/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <boot/platform.h>
#include <boot/stdio.h>

#include "openfirmware.h"


bool
platform_user_menu_requested(void)
{
	// ToDo: implement this for real - trigger on space?
	return false;
}

