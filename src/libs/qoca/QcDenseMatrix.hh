// $Id: QcDenseMatrix.hh,v 1.10 2001/01/30 01:32:07 pmoulder Exp $

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

#ifndef __QcDenseMatrixH
#define __QcDenseMatrixH

#include "qoca/QcDefines.hh"
#ifdef qcBoundsChecks
# include <math.h> // NAN
#endif
#include "qoca/QcMatrix.hh"

class QcDenseMatrixIterator;

class QcDenseMatrix : public QcMatrix
{
public:
	numT **fElements;

	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
	QcDenseMatrix();
	virtual ~QcDenseMatrix();

	//-----------------------------------------------------------------------//
	// Enquiry functions.                                                    //
	//-----------------------------------------------------------------------//
	virtual numT GetValue(unsigned row, unsigned col) const;

	//-----------------------------------------------------------------------//
	// Matrix size management functions.                                     //
	//-----------------------------------------------------------------------//
	virtual void Reserve(unsigned rows, unsigned cols);
		// Ensure that future resizes up to these limits will take
		// place in constant time - i.e. preallocate if necessary.
	virtual void Resize(unsigned rows, unsigned cols);
		// Change the size of the used part of the array up or down.
		// Increases may take time up to O(rows * cols).

	//-----------------------------------------------------------------------//
	// Basic matrix data management functions.                               //
	//-----------------------------------------------------------------------//

	//virtual void IncreaseValue(unsigned row, unsigned col, numT inc)
	//	{ fElements[row][col] += inc; }

	virtual void SetValue(unsigned row, unsigned col, numT val);
	virtual void SwapColumns(unsigned c1, unsigned c2);

#ifdef KEEP_DENSE_SHADOW
  void CopyColumn(unsigned destCol, unsigned srcCol);
  void CopyRow(unsigned destRow, QcDenseMatrix const &srcMatrix, unsigned srcRow);
#endif
	//virtual void SwapRows(unsigned r1, unsigned r2);

	virtual void Zero();
	virtual void ZeroColumn(unsigned col);
	virtual void ZeroRow(unsigned row);

	//-----------------------------------------------------------------------//
	// Numerical computation functions.                                      //
	//-----------------------------------------------------------------------//
  //virtual void AddRow(unsigned destRow, unsigned srcRow);
	virtual bool AddScaledRow(unsigned destRow, unsigned srcRow, numT factor);
	virtual void FractionRow(unsigned row, numT factor);
	virtual void Pivot(unsigned row, unsigned col);
	virtual bool ScaleRow(unsigned row, numT factor);
};

inline QcDenseMatrix::QcDenseMatrix()
  : QcMatrix(), fElements(0)
{
}

inline numT QcDenseMatrix::GetValue(unsigned row, unsigned col) const
{
  qcAssertPre (row < fRows);
  qcAssertPre (col < fColumns);

  return fElements[row][col];
}

inline void QcDenseMatrix::SetValue(unsigned row, unsigned col, numT val)
{
  qcAssertPre (row < fRows);
  qcAssertPre (col < fColumns);

  fElements[row][col] = val;
}

#endif /* !__QcDenseMatrixH */
