// $Id: QcMatrixIterator.hh,v 1.9 2001/01/30 01:32:08 pmoulder Exp $

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

#ifndef __QcMatrixIteratorH
#define __QcMatrixIteratorH

#include <qoca/QcUtility.hh>
#include <qoca/QcIterator.hh>

class QcMatrixIterator : public QcIterator
{
public:
	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
	QcMatrixIterator(unsigned vec);

#ifndef NDEBUG
	void assertInvar() const
	{
		assert ((int) fIndex >= 0);
	}
#endif

	//-----------------------------------------------------------------------//
	// Enquiry functions.                                                    //
	//-----------------------------------------------------------------------//
	int GetIndex() const
	{
		qcAssertPre (!AtEnd());
		qcAssertPost( int( fIndex) >= 0);
		return (int) fIndex;
	}

	unsigned getIndex() const
	{
		qcAssertPre (!AtEnd());
		qcAssertPost ((int) fIndex >= 0);
		return fIndex;
	}

	numT GetValue() const
	{
		qcAssertPre( !AtEnd());
		qcAssertPost( !QcUtility::IsZero( fValue));
		return fValue;
	}

	numT getValue() const
	{
		qcAssertPre( !AtEnd());
		qcAssertPost( !QcUtility::IsZero( fValue));
		return fValue;
	}

	//-----------------------------------------------------------------------//
	// Iteration functions.                                                  //
	//-----------------------------------------------------------------------//
  	void SetToBeginOf(unsigned vec);

protected:
	unsigned fVector;
	unsigned fIndex;
	numT fValue;
};

inline QcMatrixIterator::QcMatrixIterator(unsigned vec)
	: QcIterator(),
	  fVector(vec),
	  fValue()
{
}

inline void QcMatrixIterator::SetToBeginOf(unsigned vec)
{
	fVector = vec;
	Reset();
}

#endif
