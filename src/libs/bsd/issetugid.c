/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <unistd.h>


int
issetugid(void)
{
	// TODO: as long as we're effectively a single user system, this will do,
	//	but we might want to have real kernel support for this.
	return 1;
}
