/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <util/Random.h>

#include <errno.h>

#include <Debug.h>


unsigned int
random_value()
{
	unsigned int value;
	int status = getentropy(&value, sizeof(value));
	if (status != 0)
		SERIAL_PRINT(("kernelland_emu random_value() error: %s\n", strerror(errno)));

	return value;
}
