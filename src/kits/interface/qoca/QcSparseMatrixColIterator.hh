// $Id: QcSparseMatrixColIterator.hh,v 1.9 2001/01/31 10:04:51 pmoulder Exp $

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

#ifndef __QcSparseMatrixColIteratorH
#define __QcSparseMatrixColIteratorH

#include <qoca/QcUtility.hh>
#include <qoca/QcSparseMatrixIterator.hh>

/** Iterator over the non-zero elements of a column of a
    <tt>QcSparseMatrix</tt>.

    <p>Note: there is no order guarantee.

    <p>Also note that it's best not to write to the matrix while iterating over
    it.
**/
class QcSparseMatrixColIterator : public QcSparseMatrixIterator
{
	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
public:
	QcSparseMatrixColIterator(const QcSparseMatrix &m, unsigned col);

	//-----------------------------------------------------------------------//
	// Iteration functions.                                                  //
	//-----------------------------------------------------------------------//
public:
	bool AtEnd() const
		// Returns true iff the iterator is past the end of its column.
		{ return fIndex == ~0u; }
	virtual void Increment();
	virtual void Reset();
};

inline QcSparseMatrixColIterator::QcSparseMatrixColIterator(
		const QcSparseMatrix &m, unsigned col)
  : QcSparseMatrixIterator(m, col)
{
  qcAssertPre (col < m.getNColumns());

  Reset();
}

inline void QcSparseMatrixColIterator::Increment()
{
	qcAssertPre (!AtEnd());

	fCurrent = fCurrent->getNextInCol();
	fValue = fCurrent->getValue();
	fIndex = fCurrent->getRowNr();
}

inline void QcSparseMatrixColIterator::Reset()
{
	fCurrent = fMatrix->getFirstInCol( fVector);
	fValue = fCurrent->getValue();
	fIndex = fCurrent->getRowNr();
}

#endif
