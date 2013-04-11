/*
 * Copyright 2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include <util/Random.h>

#include <OS.h>


static uint32	sFastLast	= 0;
static uint32	sLast		= 0;

// In the following functions there are race conditions when many threads
// attempt to update static variable last. However, since such conflicts
// are non-deterministic it is not a big problem.


// A simple linear congruential generator
unsigned int
fast_random_value()
{
	if (sFastLast == 0)
		sFastLast = system_time();

	uint32 random = sFastLast * 1103515245 + 12345;
	sFastLast = random;
	return (random >> 16) & 0x7fff;
}


// Taken from "Random number generators: good ones are hard to find",
// Park and Miller, Communications of the ACM, vol. 31, no. 10,
// October 1988, p. 1195.
unsigned int
random_value()
{
	if (sLast == 0)
		sLast = system_time();

	uint32 hi = sLast / 127773;
	uint32 lo = sLast % 127773;

	int32 random = 16807 * lo - 2836 * hi;
	if (random <= 0)
		random += MAX_RANDOM_VALUE;
	sLast = random;
	return random % (MAX_RANDOM_VALUE + 1);
}

