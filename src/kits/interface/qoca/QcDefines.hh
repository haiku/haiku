// $Id: QcDefines.hh,v 1.16 2001/01/31 07:37:13 pmoulder Exp $

//============================================================================//
// Written by Alan Finlay and Sitt Sen Chok                                   //
//----------------------------------------------------------------------------//
// The QOCA implementation is free software, but it is Copyright (C)          //
// 1994-1999 Monash University.  It is distributed under the terms of the GNU //
// General Public License.  See the file COPYING for copying permission.      //
//                                                                            //
// The QOCA toolkit and runtime are distributed under the terms of the GNU    //
// Library General Public License.  See the file COPYING.LIB for copying      //
// permissions for those files.                                               //
//                                                                            //
// If those licencing arrangements are not satisfactory, please contact us!   //
// We are willing to offer alternative arrangements, if the need should arise.//
//                                                                            //
// THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED OR  //
// IMPLIED.  ANY USE IS AT YOUR OWN RISK.                                     //
//                                                                            //
// Permission is hereby granted to use or copy this program for any purpose,  //
// provided the above notices are retained on all copies.  Permission to      //
// modify the code and to distribute modified code is granted, provided the   //
// above notices are retained, and a notice that the code was modified is     //
// included with the above copyright notice.                                  //
//============================================================================//

#ifndef __QcDefinesH
#define __QcDefinesH

#define qcQuantum			64
#define qcHashTableInitCapacity 	256
#define qcVectorInitCapacity 		256
#define qcMatrixInitRowCapacity 	256
#define qcMatrixInitColCapacity 	256
#define qcTableauInitRowCapacity	256
#define qcTableauInitColCapacity	256
#define qcMaxLoopCycles			10000
#define qcOneMegaBytes 			1048576
#define qcOneKiloBytes 			1024

#ifdef USE_RATIONALS
# define NUMT_IS_PJM_MPQ 1
#else
# define NUMT_IS_DOUBLE 1
#endif
#include <qoca/arith.H>

#define qcCheckInput 1

/* Simply #defining qcOneDirectionalSparseMatrix (i.e. use
   singly-linked lists for the sparse matrices) without any other
   changes is probably a bad idea, due to removal costs.  However,
   using one directional linked lists for the row iterating linked
   lists is just about feasible, since usually we reach the element by
   traversing the row linked lists anyway, so we should already have a
   pointer to the previous element as an auto variable.

   Alternatively, don't immediately unlink things.  (In this case, do
   consider whether it's safe to unlink from one list without
   unlinking from the other.) */
//#define qcOneDirectionalSparseMatrix

//#define qcDenseQuasiInverse

// Note: currently still crashes if defined.
//#define qcSafetyChecks	1

/* qcUseSafeIterator is of limited use; consider removing the code used ifdef
   qcUseSafeIterator.  Use it if you don't trust the sparse iterators.  However,
   note that it doesn't actually test the correctness of the sparse iterator, it
   just uses a dense one instead.  Note that if sparse iterators are erroneous,
   then the program will probably still go wrong. */
//#define qcUseSafeIterator

#ifndef NDEBUG
# define qcCheckPre 1
# define qcCheckPost 1
# define qcCheckInternalPre 1
# define qcCheckInternalPost 1
#endif
#define qcRealTableauCoeff
#define qcRealTableauRHS
#define qcUseExceptions

#ifdef qcUseExceptions
# include "qoca/QcException.hh"
#endif

#define qcInvalidInt -1
	// Used in QcBimap.h and must be the value used for invalid
	// int and int for this reason.

#define FUNCTION_H
	// STL function.h creates problems due to dumb template operators.

#include <float.h>

#if qcCheckInternalPre || qcCheckInternalPost
# define qcCheckInternalInvar 1
#endif

#ifndef NDEBUG
# define qcBoundsChecks 1
#endif

#ifdef qcSafetyChecks
# define qcBoundsChecks 1
# define QcAssertions 1
#endif

#if defined(qcSafetyChecks) || qcCheckPre || qcCheckPost || qcCheckInternalPre || qcCheckInternalPost
# include <assert.h>
#endif

#ifdef NDEBUG
# define dbg(_expr)
#else
# define dbg(_expr) _expr
#endif

#define showplace() do { cout << "DBG: " << __FILE__ << ':' << __LINE__ << endl; } while(false)

#define qcPlace __FILE__,__LINE__

#ifdef QcAssertions
//# define qcAssert(i) ((i)? 0: (throw QcWarning(qcPlace, "Assertion failed"), 0))
# define qcAssert(i) assert(i)
#else
# define qcAssert(i)
#endif

#if qcCheckPre
/* TODO: We could prepend a "Pre..." / "Post..." string. */
# define qcAssertExternalPre(_p) assert(_p)
# define qcAssertPre(_p) assert(_p)
# define qcAssertExpensivePre(_p) assert(_p)
# define dbgPre(_expr) _expr
#else
# define qcAssertExternalPre(_p)	do { } while(0)
# define qcAssertPre(_p) do { } while(0)
# define dbgPre(_expr)
#endif
#if qcCheckPost
# define qcAssertExternalPost(_p)	assert(_p)
# define qcAssertPost(_p) assert(_p)
# define dbgPost(_expr) _expr
#else
# define qcAssertExternalPost(_p)	do { } while(0)
# define qcAssertPost(_p) do { } while(0)
# define dbgPost(_expr)
#endif

#if qcCheckInternalPre
# define qcDurchfall(_s) assert(0 && "Durchfall on " _s)
#else
# define qcDurchfall(_s) do { } while(0)
#endif

#if qcCheckInternalInvar
# define qcAssertInvar(_p) assert(_p)
#else
# define qcAssertInvar(_p) do { } while(0)
#endif

#ifndef UNUSED
# define UNUSED(_var) ((void) (_var))
#endif

#define CAST(_newtype, _expr) \
  (\
    assert (dynamic_cast<_newtype> (_expr) != 0), \
    (_newtype) (_expr) \
  )

#ifndef qcNoStream
# include <iostream.h>
#endif

/* TODO: Maybe use autoconf to detect this.  (OTOH, I imagine that
   most such primitive environments would be non-Un*x anyway.) */
#ifdef qcDefineBool
# include <bool.h>	// Use this while using STL and no built in bool
#endif

	// This is needed if we use the STL provided with Borland C++
#ifdef __BCPLUSPLUS__
# define RWSTD_NO_NAMESPACE
#endif

#endif
