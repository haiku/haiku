#ifndef _UTILS_H
#define _UTILS_H

extern int log2( uint32 x );

static inline int RoundDiv( int num, int den )
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

// macros for fix-point calculation
#define FIX_SHIFT 32
#define FIX_SCALE (1LL << FIX_SHIFT)

#endif
