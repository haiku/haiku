// $Id: QcSparseCoeff.hh,v 1.5 2001/01/31 07:37:13 pmoulder Exp $

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

#ifndef __QcSparseCoeffH
#define __QcSparseCoeffH

#include <iostream>
using namespace std;

class QcSparseCoeff
{
public:
  //-----------------------------------------------------------------------//
  // Constructor                                                           //
  //-----------------------------------------------------------------------//
  QcSparseCoeff()
    : fValue()
  {
    dbg(fIndex = -1);
  }

  QcSparseCoeff(numT v, int i)
    : fValue(v), fIndex(i)
  {
    qcAssertPre( i >= 0);
  }

  numT getValue() const
  {
    assert( fIndex >= 0);
    return fValue;
  }

  unsigned getIndex() const
  {
    assert( fIndex >= 0);
    return (unsigned) fIndex;
  }

	//-----------------------------------------------------------------------//
	// Utility function                                                      //
	//-----------------------------------------------------------------------//
  	void Print(ostream &os) const
		{ os << "[" << fIndex << "," << fValue << "]"; }


private:
	numT fValue;
	int fIndex;
};

#endif
