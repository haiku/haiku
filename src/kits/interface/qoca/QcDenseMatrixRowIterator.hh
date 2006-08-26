// $Id: QcDenseMatrixRowIterator.hh,v 1.5 2001/01/30 01:32:07 pmoulder Exp $

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

#ifndef __QcDenseMatrixRowIteratorH
#define __QcDenseMatrixRowIteratorH

#include "qoca/QcDenseMatrixIterator.hh"

class QcDenseMatrixRowIterator : public QcDenseMatrixIterator
{
public:
	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
	QcDenseMatrixRowIterator(const QcDenseMatrix &m, unsigned row);

	//-----------------------------------------------------------------------//
	// Iteration functions.                                                  //
	//-----------------------------------------------------------------------//
	virtual void Advance();
	virtual bool AtEnd() const
  		{ return fIndex == fMatrix.getNColumns(); }
};

inline QcDenseMatrixRowIterator::QcDenseMatrixRowIterator(
		const QcDenseMatrix &m, unsigned row)
    	: QcDenseMatrixIterator(m, row)
{
	qcAssertPre (row < m.getNRows());

	Reset();
}

inline void QcDenseMatrixRowIterator::Advance()
{
	qcAssertPre (fIndex <= fMatrix.getNColumns());

	while (fIndex < fMatrix.getNColumns()) {
		fValue = fMatrix.fElements[fVector][fIndex];
		if (!QcUtility::IsZero( fValue))
			break;
		fIndex++;
	}
}

#endif
