/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon driver

	some utility functions
*/

#include "OS.h"
#include "utils.h"

// get ceil( log2( size ))
int radeon_log2( uint32 x )
{
	int res;
	uint32 tmp;

	for( res = 0, tmp = x ; tmp > 1 ; ++res )
		tmp >>= 1;

	if( (x & ((1 << res) - 1)) != 0 )
		++res;

	return res;
}
