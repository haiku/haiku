/*
 * Copyright 2009, Colin Günther, coling@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <compat/sys/libkern.h>


uint32_t
arc4random(void)
{
	return random();
}

