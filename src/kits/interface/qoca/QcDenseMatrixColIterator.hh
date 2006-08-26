// $Id: QcDenseMatrixColIterator.hh,v 1.5 2000/11/24 12:16:21 pmoulder Exp $

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

#ifndef __QcDenseMatrixColIteratorH
#define __QcDenseMatrixColIteratorH

#include "qoca/QcDenseMatrixIterator.hh"
#include "qoca/QcUtility.hh"

class QcDenseMatrixColIterator : public QcDenseMatrixIterator
{
public:
	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
	QcDenseMatrixColIterator(const QcDenseMatrix &m, unsigned col);

	//-----------------------------------------------------------------------//
	// Iteration functions.                                                  //
	//-----------------------------------------------------------------------//
  	virtual void Advance();
	virtual bool AtEnd() const
		// Returns true iff the iterator is past the end of its row or column.
  		{ return fIndex == fMatrix.getNRows(); }
};

inline QcDenseMatrixColIterator::QcDenseMatrixColIterator(
		const QcDenseMatrix &m, unsigned col)
	: QcDenseMatrixIterator(m, col)
{
	qcAssertPre (col < m.getNColumns());

	Reset();
}

inline void QcDenseMatrixColIterator::Advance()
{
	qcAssertPre (fIndex <= fMatrix.getNRows());

	while (fIndex < fMatrix.getNRows()) {
		fValue = fMatrix.fElements[fIndex][fVector];
		if (!QcUtility::IsZero(fValue))
			break;
		fIndex++;
	}
}

#endif /* !__QcDenseMatrixColIteratorH */
