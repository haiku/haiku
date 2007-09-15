// $Id: QcSparseMatrixIterator.hh,v 1.8 2001/01/31 04:53:11 pmoulder Exp $

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

#ifndef __QcSparseMatrixIteratorH
#define __QcSparseMatrixIteratorH

#include "qoca/QcMatrixIterator.hh"
#include "qoca/QcSparseMatrix.hh"

class QcSparseMatrix;
class QcSparseMatrixElement;

/** Iterator over the non-zero elements of a row or column of a
    <tt>QcSparseMatrix</tt>.

    <p>Note: there is no order guarantee.

    <p>Also note that it's best not to write to the matrix while iterating over
    it.
**/
class QcSparseMatrixIterator : public QcMatrixIterator
{
	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
public:
	QcSparseMatrixIterator(const QcSparseMatrix &m, unsigned vec);

protected:
	const QcSparseMatrix *fMatrix;
  	QcSparseMatrixElement const *fCurrent;
};

inline QcSparseMatrixIterator::QcSparseMatrixIterator(const QcSparseMatrix &m,
						      unsigned vec)
	: QcMatrixIterator(vec),
	  fMatrix(&m)
{
}

#endif /* !__QcSparseMatrixIteratorH */
