/*
	Copyright (c) 2002/03, Thomas Kurschel


	Part of Radeon accelerant

	Utility functions
*/

#ifndef _UTILS_H
#define _UTILS_H

#ifdef __cplusplus
extern "C" {
#endif


extern int radeon_log2( uint32 x );

static inline int RoundDiv( int num, int den )
{
	return (num + (den / 2)) / den;
}

static inline int32 RoundDiv64( int64 num, int32 den )
{
	return (num + (den / 2)) / den;
}

static inline int ceilShiftDiv( int num, int shift )
{
	return (num + (1 << shift) - 1) >> shift;
}

static inline int ceilDiv( int num, int den )
{
	return (num + den - 1) / den;
}

// macros for fixed-point calculation
#define FIX_SHIFT 32
#define FIX_SCALE (1LL << FIX_SHIFT)

#ifdef __cplusplus
}
#endif

#endif
