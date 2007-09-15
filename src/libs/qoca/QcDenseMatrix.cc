// $Id: QcDenseMatrix.cc,v 1.12 2001/01/30 01:32:07 pmoulder Exp $

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

#include "qoca/QcDefines.hh"
#include "qoca/QcDenseMatrix.hh"
#include "qoca/QcDenseMatrixColIterator.hh"

QcDenseMatrix::~QcDenseMatrix()
{
	if (fAllocColumns != 0) 
		for (unsigned i = 0; i < fAllocRows; i++)
			delete[] fElements[i];

	if (fAllocRows != 0)
    		delete[] fElements;
}

#if 0 /* unused */
void QcDenseMatrix::AddRow(unsigned destRow, unsigned srcRow)
{
  qcAssertPre (srcRow < fRows);
  qcAssertPre (destRow < fRows);

  for (unsigned i = 0; i < fColumns; i++)
    fElements[destRow][i] += fElements[srcRow][i];
}
#endif

bool QcDenseMatrix::AddScaledRow(unsigned destRow, unsigned srcRow, numT factor)
{
  qcAssertPre( QcUtility::isFinite( factor));
  qcAssertPre( srcRow < fRows);
  qcAssertPre( destRow < fRows);

  if (QcUtility::IsZero( factor))
    return false;

  for (unsigned i = 0; i < fColumns; i++)
    fElements[destRow][i] += fElements[srcRow][i] * factor;

  return true;
}

void QcDenseMatrix::FractionRow(unsigned row, numT factor)
{
	qcAssertPre (row < fRows);

	if (QcUtility::IsOne(factor))
		return;

	for (unsigned i = 0; i < fColumns; i++)
		fElements[row][i] /= factor;
}

void QcDenseMatrix::Pivot(unsigned row, unsigned col)
{
  qcAssertPre( row < getNRows());
  qcAssertPre( col < getNColumns());

  numT p = fElements[row][col];
  qcAssertPre( !QcUtility::IsZero( p));

  ScaleRow( row, recip( p));

  for (QcDenseMatrixColIterator colItr( *this, col);
       !colItr.AtEnd();
       colItr.Increment())
    {
      if (colItr.getIndex() != row)
	AddScaledRow( colItr.getIndex(), row, -colItr.getValue());
    }

#ifdef qcCheckInternalPost
  qcAssertPost( QcUtility::IsOne( GetValue( row, col)));
  for (unsigned i = getNRows(); i--;)
    if (i != row)
      qcAssertPost( QcUtility::IsZero( GetValue( i, col)));
#endif
}

void QcDenseMatrix::Reserve(unsigned rows, unsigned cols)
{
	numT **e;
	numT *e1 = 0;

	qcAssert(fAllocRows >= fRows);
	qcAssert(fAllocColumns >= fColumns);

	// Add new columns to existing rows
	if (cols > fAllocColumns) {
		// Extend rows which need copying of values
		for (unsigned i = 0; i < fRows; i++) {
			if (fAllocColumns > 0)
				e1 = fElements[i];

			fElements[i] = new numT[cols];
			memcpy(fElements[i], e1, sizeof(numT) * fColumns);

			if (fAllocColumns > 0)
				delete [] e1;
		}

		// Extend rows which are presently allocated but unused
		for (unsigned i = fRows; i < fAllocRows; i++) {
			if (fAllocColumns > 0)
				delete[] fElements[i];
			fElements[i] = new numT[cols];
		}
		fAllocColumns = cols;
	}

	// Copy existing and Allocate new rows
	if (rows > fAllocRows) {
		e = new numT*[rows];		// Allocate a new row vector

		// Copy old rows.
		if (fAllocRows > 0) {
			memcpy(e, fElements, sizeof(numT) * fAllocRows);
			delete[] fElements; 	// free the old row vector
		}

		fElements = e;			// matrix now uses the new row vector

		// Allocate new rows
		for (unsigned i = fAllocRows; i < rows; i++)
			fElements[i] = new numT[fAllocColumns];

		fAllocRows = rows;
	}

	qcAssertPost (fAllocRows >= rows);
	qcAssertPost (fAllocColumns >= cols);
	dbg(assertInvar());
}

void QcDenseMatrix::Resize(unsigned rows, unsigned cols)
{
	if (rows > fAllocRows || cols > fAllocColumns)
		Reserve(rows, cols);

	// Initialise newly exposed columns of existing rows
	for (unsigned i = 0; i < fRows; i++)
		for (unsigned j = fColumns; j < cols; j++)
			fElements[i][j] = 0.0;

	// Initialise newly exposed rows
	for (unsigned i = fRows; i < rows; i++)
		for (unsigned j = 0; j < cols; j++)
			fElements[i][j] = 0.0;

	fRows = rows;
	fColumns = cols;
	dbg(assertInvar());
}

bool QcDenseMatrix::ScaleRow(unsigned row, numT factor)
{
	if (QcUtility::IsOne( factor))
		return false;

	for (unsigned i = 0; i < fColumns; i++)
		fElements[row][i] *= factor;

	return true;
}

void QcDenseMatrix::SwapColumns(unsigned c1, unsigned c2)
{
	numT temp;

	for (unsigned i = 0; i < fRows; i++) {
		temp = fElements[i][c1];
		fElements[i][c1] = fElements[i][c2];
		fElements[i][c2] = temp;
	}
}

#ifdef KEEP_DENSE_SHADOW
void QcDenseMatrix::CopyColumn(unsigned destCol, unsigned srcCol)
{
  qcAssertPre( destCol < getNColumns());
  qcAssertPre( srcCol < getNColumns());

  for (unsigned i = 0; i < fRows; i++)
    fElements[i][destCol] = fElements[i][srcCol];
}

void QcDenseMatrix::CopyRow(unsigned destRow,
			    QcDenseMatrix const &srcMatrix, unsigned srcRow)
{
  qcAssertPre( destRow < getNRows());
  qcAssertPre( srcRow < getNRows());

  for (unsigned j = 0; j < fColumns; j++)
    fElements[destRow][j] = srcMatrix.fElements[srcRow][j];
}
#endif

#if 0 /* unused */
void QcDenseMatrix::SwapRows(unsigned r1, unsigned r2)
{
	numT temp;

	for (unsigned i = 0; i < fColumns; i++) {
		temp = fElements[r1][i];
		fElements[r1][i] = fElements[r2][i];
		fElements[r2][i] = temp;
    }
}
#endif /* unused */

void QcDenseMatrix::Zero()
{
	for (unsigned i = 0; i < fRows; i++)
		for (unsigned j = 0; j < fColumns; j++)
			fElements[i][j] = 0.0;
}

void QcDenseMatrix::ZeroColumn(unsigned col)
{
	for (unsigned i = 0; i < fRows; i++)
		fElements[i][col] = 0.0;
}

void QcDenseMatrix::ZeroRow(unsigned row)
{
	for (unsigned i = 0; i < fColumns; i++)
		fElements[row][i] = 0.0;
}
