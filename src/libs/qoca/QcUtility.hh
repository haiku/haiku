// $Id: QcUtility.hh,v 1.10 2000/12/13 07:18:32 pmoulder Exp $

//============================================================================//
// Written by Sitt Sen Chok                                                   //
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

#ifndef __QcUtilityH
#define __QcUtilityH

#include "qoca/QcDefines.hh"

class QcUtility
{
#if NUMT_IS_DOUBLE
private:
  #define             _QC_EPS	  2.273736754432321e-13		// 2 ** -42
  static double const qcEps     = _QC_EPS;
  static double const qcNearEps = 9.313225746154785e-10;   // 2 ** -30
  static double const qcVaguelyNearEps = 9.5367431640625e-7; // 2 ** -20

public:
  /** Highest <tt>numT</tt> <var>x</var> s.t.&nbsp;<tt>IsZero(x)</tt>.
      This is a positive (non-zero) number iff numT is floating point. */
  static double const qcMaxZeroVal = _QC_EPS * (1 - DBL_EPSILON / 2);
  #undef _QC_EPS

public:
	//-----------------------------------------------------------------------//
	// We need some conventions about when small values are converted        //
	// to 0.0 or we will be performing unnecessary isZero tests.             //
	// Generally we don't want to test and "zeroise" values every time       //
	// a calculation is done (e.g. matrix row operations) since the values   //
	// may never need to be compared with zero anyway.  Better to take       //
	// care where efficiency may be impacted to ensure the test is only      //
	// done once, and where there is no efficiency impact, be cautious       //
	// and do isZero tests which are not strictly necessary - with           //
	// a comment to this effect.  Note that sparse iterators which by        //
	// definition do not return zero coefficients should do the isZero       //
	// test, clients do not need to repeat this test.                        //
	//-----------------------------------------------------------------------//

  static bool IsVaguelyZero(numT val)
    { return fabs( val) < QcUtility::qcVaguelyNearEps; }

  static bool IsNearZero(numT val)
    { return fabs( val) < QcUtility::qcNearEps; }

  static bool IsZero(double val)
    { return fabs( val) < QcUtility::qcEps; }

  static bool IsOne(double val)
    { return QcUtility::IsZero( val - 1.0); }

  static bool IsNegative(numT val)
    { return (val <= -QcUtility::qcEps); }

  static bool IsPositive(numT val)
    { return (val >= QcUtility::qcEps); }

  static double Zeroise(double val)
    {
      return (QcUtility::IsZero( val)
	      ? 0.0
	      : val);
    }

#ifndef NDEBUG
  static bool IsZeroised(double val)
    {
      return (val == 0 || !QcUtility::IsZero( val));
    }
#endif

  /** Return true iff x is neither NaN nor +/- infinity. */
  static bool isFinite( double x)
  {
#ifdef isfinite
    return isfinite( x);
#else
    return x * 0.0 == 0.0;
#endif
  }
#elif NUMT_IS_PJM_MPQ
public:
  static unsigned long const qcMaxZeroVal = 0ul;

  static bool IsNegative(numT val)
    { return val < 0; }
  static bool IsPositive(numT val)
    { return val > 0; }
  static bool IsZero(numT val)
    { return val == 0; }
  static bool IsNearZero(numT val)
    { return val == 0; }
  static bool IsVaguelyZero(numT val)
    { return val == 0; }
  static bool IsOne(numT val)
    { return val == 1; }
  static numT Zeroise(numT val)
    { return val; }
  static bool isFinite(numT val)
    { return val.isFinite(); }

# ifndef NDEBUG
  static bool IsZeroised(numT val)
    {
      UNUSED(val);
      return true;
    }
# endif

#else
# error "Unknown numT"
#endif
};

#endif
