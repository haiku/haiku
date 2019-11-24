///-*-C++-*-//////////////////////////////////////////////////////////////////
//
// Hoard: A Fast, Scalable, and Memory-Efficient Allocator
//        for Shared-Memory Multiprocessors
// Contact author: Emery Berger, http://www.cs.utexas.edu/users/emery
//
// Copyright (c) 1998-2000, The University of Texas at Austin.
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Library General Public License as
// published by the Free Software Foundation, http://www.fsf.org.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
//////////////////////////////////////////////////////////////////////////////
#ifndef _HEAPSTATS_H_
#define _HEAPSTATS_H_

#include "config.h"

//#include <stdio.h>
//#include <assert.h>


class heapStats {
	public:
		heapStats(void)
			: U(0), A(0)
#if HEAP_STATS
			, Umax(0), Amax(0)
#endif
		{
		}

		inline const heapStats & operator=(const heapStats & p);

		inline void incStats(int updateU, int updateA);
		inline void incUStats(void);

		inline void decStats(int updateU, int updateA);
		inline void decUStats(void);
		inline void decUStats(int &Uout, int &Aout);

		inline void getStats(int &Uout, int &Aout);

#if HEAP_STATS
		inline int getUmax(void);
		inline int getAmax(void);
#endif

	private:
		// U and A *must* be the first items in this class --
		// we will depend on this to atomically update them.

		int U;						// Memory in use.
		int A;						// Memory allocated.

#if HEAP_STATS
		int Umax;
		int Amax;
#endif
};


inline void
heapStats::incStats(int updateU, int updateA)
{
	assert(updateU >= 0);
	assert(updateA >= 0);
	assert(U <= A);
	assert(U >= 0);
	assert(A >= 0);
	U += updateU;
	A += updateA;

#if HEAP_STATS
	Amax = MAX(Amax, A);
	Umax = MAX(Umax, U);
#endif

	assert(U <= A);
	assert(U >= 0);
	assert(A >= 0);
}


inline void
heapStats::incUStats(void)
{
	assert(U < A);
	assert(U >= 0);
	assert(A >= 0);
	U++;

#if HEAP_STATS
	Umax = MAX(Umax, U);
#endif

	assert(U >= 0);
	assert(A >= 0);
}


inline void
heapStats::decStats(int updateU, int updateA)
{
	assert(updateU >= 0);
	assert(updateA >= 0);
	assert(U <= A);
	assert(U >= updateU);
	assert(A >= updateA);
	U -= updateU;
	A -= updateA;
	assert(U <= A);
	assert(U >= 0);
	assert(A >= 0);
}


inline void
heapStats::decUStats(int &Uout, int &Aout)
{
	assert(U <= A);
	assert(U > 0);
	assert(A >= 0);
	U--;
	Uout = U;
	Aout = A;
	assert(U >= 0);
	assert(A >= 0);
}


inline void
heapStats::decUStats(void)
{
	assert(U <= A);
	assert(U > 0);
	assert(A >= 0);
	U--;
}


inline void
heapStats::getStats(int &Uout, int &Aout)
{
	assert(U >= 0);
	assert(A >= 0);
	Uout = U;
	Aout = A;
	assert(U <= A);
	assert(U >= 0);
	assert(A >= 0);
}


#if HEAP_STATS
inline int
heapStats::getUmax(void)
{
	return Umax;
}


inline int
heapStats::getAmax(void)
{
	return Amax;
}
#endif // HEAP_STATS

#endif // _HEAPSTATS_H_
