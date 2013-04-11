/*
 * Copyright 2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include <util/Random.h>

#include <OS.h>


static uint32	fast_last	= 0;
static uint32	last		= 0;

// In the following functions there are race conditions when many threads
// attempt to update static variable last. However, since such conflicts
// are non-deterministic it is not a big problem.


// A simple linear congruential generator
unsigned int
fast_random_value()
{
	if (fast_last == 0)
		fast_last = system_time();

	uint32 random = fast_last * 1103515245 + 12345;
	fast_last = random;
	return (random >> 16) & 0x7fff;
}


// Taken from "Random number generators: good ones are hard to find",
// Park and Miller, Communications of the ACM, vol. 31, no. 10,
// October 1988, p. 1195.
unsigned int
random_value()
{
	if (last == 0)
		last = system_time();

	uint32 hi = last / 127773;
	uint32 lo = last % 127773;

	int32 random = 16807 * lo - 2836 * hi;
	if (random <= 0)
		random += MAX_RANDOM_VALUE;
	last = random;
	return random % (MAX_RANDOM_VALUE + 1);
}

