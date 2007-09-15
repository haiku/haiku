// $Id: QcDenseTableauIterator.hh,v 1.6 2000/12/06 05:32:56 pmoulder Exp $

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
// THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY	EXPRESSED OR  //
// IMPLIED.  ANY USE IS AT YOUR OWN RISK.                                     //
//                                                                            //
// Permission is hereby granted to use or copy this program for any purpose,  //
// provided the above notices are retained on all copies.  Permission to      //
// modify the code and to distribute modified code is granted, provided the   //
// above notices are retained, and a notice that the code was modified is     //
// included with the above copyright notice.                                  //
//============================================================================//

#ifndef __QcDenseTableauIteratorH
#define __QcDenseTableauIteratorH

#include "qoca/QcLinEqTableau.hh"
#include "qoca/QcIterator.hh"

class QcDenseTableauIterator : public QcIterator
{
public:
	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
	QcDenseTableauIterator(const QcLinEqTableau &tab,
			       unsigned vec, unsigned end);

#ifndef NDEBUG
	void assertInvar() const
	{
		assert (fIndex <= fEnd);
	}
#endif

	//-----------------------------------------------------------------------//
	// Enquiry functions.                                                    //
	//-----------------------------------------------------------------------//
	int GetIndex() const
     	{ return (int) fIndex; }

	unsigned getIndex() const
	{ return fIndex; }

	numT GetValue() const
	{
		qcAssertPre (!AtEnd());
		return fValue;
	}

	numT getValue() const
	{
		qcAssertPre (!AtEnd());
		return fValue;
	}

	//------------------------------------------------------------------------//
	// Iteration functions.                                                   //
	//------------------------------------------------------------------------//
	virtual void Advance() = 0;
	virtual void Increment();
	virtual void Reset();
  	virtual void SetToBeginOf(unsigned i);
	virtual bool AtEnd() const
		{ return fIndex == fEnd; }

protected:
	unsigned fIndex;
	numT fValue;
	unsigned fVector;
	unsigned fEnd;
	const QcLinEqTableau &fTableau;
};

inline QcDenseTableauIterator::QcDenseTableauIterator(
		const QcLinEqTableau &tab, unsigned vec, unsigned end)
	: fIndex (0),
	  fValue(),
	  fVector (vec),
	  fEnd (end),
	  fTableau (tab)
{
	dbg(assertInvar());
}

inline void QcDenseTableauIterator::Increment()
{
	qcAssertPre (!AtEnd());
	fIndex++;
	Advance();
}

inline void QcDenseTableauIterator::Reset()
{
	fIndex = 0;
	Advance();
}

inline void QcDenseTableauIterator::SetToBeginOf(unsigned i)
{
	fVector = i;
	Reset();
}

#endif
