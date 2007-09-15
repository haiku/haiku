// $Id: QcDenseTableauColIterator.hh,v 1.5 2000/11/24 12:24:47 pmoulder Exp $

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

#ifndef __QcDenseTableauColIteratorH
#define __QcDenseTableauColIteratorH

#include "qoca/QcDenseTableauIterator.hh"

class QcDenseTableauColIterator : public QcDenseTableauIterator
{
public:
	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//

	/** @precondition <tt>col &lt; tab.getNColumns()</tt> */
	QcDenseTableauColIterator (const QcLinEqTableau &tab, unsigned col);

	//-----------------------------------------------------------------------//
	// Iteration functions.                                                  //
	//-----------------------------------------------------------------------//
	virtual void Advance();
};

inline QcDenseTableauColIterator::QcDenseTableauColIterator(
		const QcLinEqTableau &tab, unsigned col)
	: QcDenseTableauIterator(tab, col, tab.getNRows())
{
	qcAssertPre (col < fTableau.getNColumns());

	Reset();
	dbg(assertInvar());
}

inline void QcDenseTableauColIterator::Advance()
{
	assert (fIndex <= fEnd);
	assert (fVector < fTableau.getNColumns());
	assert (fEnd <= fTableau.getNRows());
	while (fIndex < fEnd) {
		fValue = fTableau.GetValue(fIndex, fVector);
		if (!QcUtility::IsZero(fValue))
			break;
		fIndex++;
	}
	dbg(assertInvar());
}
#endif
