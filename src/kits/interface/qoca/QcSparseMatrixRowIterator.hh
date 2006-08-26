// $Id: QcSparseMatrixRowIterator.hh,v 1.7 2001/01/31 10:04:51 pmoulder Exp $

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

#ifndef __QcSparseMatrixRowIteratorH
#define __QcSparseMatrixRowIteratorH

#include <qoca/QcUtility.hh>
#include <qoca/QcSparseMatrixIterator.hh>

/** An iterator over the non-zero elements of a row of a <tt>QcSparseMatrix</tt>. */
class QcSparseMatrixRowIterator : public QcSparseMatrixIterator
{
public:
	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
	QcSparseMatrixRowIterator(const QcSparseMatrix &m, unsigned col);

	//-----------------------------------------------------------------------//
	// Iteration functions.                                                  //
	//-----------------------------------------------------------------------//

  /** Returns <tt>true</tt> iff the iterator is past the end of its row. */
  bool AtEnd() const
    { return fIndex == ~0u; }

	virtual void Increment();
	virtual void Reset();

};

inline QcSparseMatrixRowIterator::QcSparseMatrixRowIterator(
		const QcSparseMatrix &m, unsigned row)
	: QcSparseMatrixIterator(m, row)
{
	qcAssertPre (row < m.getNRows());

	Reset();
}

inline void QcSparseMatrixRowIterator::Increment()
{
	qcAssertPre (!AtEnd());

  	fCurrent = fCurrent->getNextInRow();
	fValue = fCurrent->getValue();
	fIndex = fCurrent->getColNr();
}

inline void QcSparseMatrixRowIterator::Reset()
{
  	fCurrent = fMatrix->getFirstInRow( fVector);
	fValue = fCurrent->getValue();
	fIndex = fCurrent->getColNr();
}

#endif
