// $Id: QcDenseMatrixIterator.hh,v 1.5 2000/11/24 12:16:21 pmoulder Exp $

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

#ifndef __QcDenseMatrixIteratorH
#define __QcDenseMatrixIteratorH

#include "qoca/QcMatrixIterator.hh"
#include "qoca/QcDenseMatrix.hh"

class QcDenseMatrixIterator : public QcMatrixIterator
{
public:
	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
	QcDenseMatrixIterator(const QcDenseMatrix &m, unsigned vec);

	//-----------------------------------------------------------------------//
	// Iteration functions.                                                  //
	//-----------------------------------------------------------------------//
	virtual void Advance() = 0;
	virtual void Increment();
	virtual void Reset();

protected:
	const QcDenseMatrix &fMatrix;
};

inline QcDenseMatrixIterator::QcDenseMatrixIterator(const QcDenseMatrix &m,
						    unsigned vec)
	: QcMatrixIterator(vec), fMatrix(m)
{
}

inline void QcDenseMatrixIterator::Increment()
{
  qcAssertPre (!AtEnd());

  fIndex++;
  Advance();
}

inline void QcDenseMatrixIterator::Reset()
{
	fIndex = 0;
	Advance();
}

#endif
