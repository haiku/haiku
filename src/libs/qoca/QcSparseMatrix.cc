// $Id: QcSparseMatrix.cc,v 1.23 2001/01/31 11:06:14 pmoulder Exp $

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

#include <qoca/QcDefines.hh>
#include <qoca/QcSparseMatrixColIterator.hh>
#include <qoca/QcSparseMatrixRowIterator.hh>
#include <qoca/QcSparseMatrix.hh>
#include <qoca/QcUtility.hh>

#ifndef MAX
# define MAX(_a, _b) ((_a) > (_b) ? (_a) : (_b))
#endif

QcSparseMatrix::~QcSparseMatrix()
{
  TrimRows( 0);

  if(fAllocRows)
    {
      for(QcSparseMatrixElement **ri = fRowHeads, **riEnd = ri + fAllocRows;
	  ri < riEnd; ri++)
	delete *ri;
      delete [] fRowHeads;
    }

  if(fAllocColumns)
    {
      for(QcSparseMatrixElement **ci = fColHeads, **ciEnd = ci + fAllocColumns;
	  ci < ciEnd; ci++)
	delete *ci;

      delete [] fColHeads;
    }
}

#ifndef NDEBUG
void
QcSparseMatrix::assertInvar() const
{
  assert( (fAllocColumns == 0) == (fColHeads == 0));
  assert( (fAllocRows == 0) == (fRowHeads == 0));
  QcMatrix::assertInvar();
  /* &forall;[i &isin; [fColumns, fAllocColumns)] fColHeads[i] points to an
     allocated element (though that element's pointers are allowed to be
     garbage). */
}

void
QcSparseMatrix::vAssertDeepInvar() const
{
  assertDeepInvar();
}
#endif

/* Note: On the only largish example we've tried, AddScaledRow takes up 46% of
   the CPU time.  Any speed improvements (other than removing things that
   disappear with -DNDEBUG, such as qcAssertPre, assert, dbg(...)) are likely to
   have a noticeable effect on overall program speed. */
bool QcSparseMatrix::AddScaledRow(unsigned destRow, unsigned srcRow, numT factor)
{
  qcAssertPre( QcUtility::isFinite( factor));
  qcAssertPre( srcRow < fRows);
  qcAssertPre( destRow < fRows);

  if(QcUtility::IsZero( factor))
    return false;

  qcAssertPre( destRow != srcRow);
  /* If we are called with destRow == srcRow then should call ScaleRow. */

  QcSparseMatrixElement *de = fRowHeads[ destRow];
  QcSparseMatrixElement *prevCol = de;		// Link from previous column
  de = de->fNextColumn;
  unsigned col;
  for(QcSparseMatrixElement const *se = fRowHeads[ srcRow]->fNextColumn;
      (col = se->fColumn) != ~0u; se = se->fNextColumn)
    {
      numT increment = se->fValue * factor;
      if(QcUtility::IsZero( increment))
	continue;

      // Find existing element or insertion point
      while(de->fColumn < col)
	{ 
	  prevCol = de;
	  de = de->fNextColumn;
	}

      assert( de->fColumn >= col);

      if(de->fColumn == col)
	{	// found existing
	  de->fValue += increment;

	  if(QcUtility::IsZero( de->fValue))
	    {
	      // Row linkage.
	      de->fPrevColumn->fNextColumn = de->fNextColumn;
	      de->fNextColumn->fPrevColumn = de->fPrevColumn;

	      // Column linkage.
	      de->fPrevRow->fNextRow = de->fNextRow;
	      de->fNextRow->fPrevRow = de->fPrevRow;

	      QcSparseMatrixElement *de_nxt = de->fNextColumn;
	      delete de;		// Free memory.
	      de = de_nxt;
	    }
	  else
	    {
	      prevCol = de;
	      de = de->fNextColumn;
	    }
	}
      else
	{		 // Allocate a new element
	  QcSparseMatrixElement *ne = new QcSparseMatrixElement( increment, destRow, col);

	  // Insert new element into destRow.
	  ne->fNextColumn = de;
	  de->fPrevColumn = ne;

	  prevCol->fNextColumn = ne;
	  ne->fPrevColumn = prevCol;

	  // Insert new element into column.
	  {
	    QcSparseMatrixElement *head = fColHeads[ col];

	    ne->fPrevRow = head;
	    ne->fNextRow = head->fNextRow;
	    head->fNextRow = ne;
	    ne->fNextRow->fPrevRow = ne;
	  }

	  prevCol = ne;
	  assert( de == prevCol->fNextColumn);
	}

      assert( (de->fColumn > se->fColumn)
	      && (de->fPrevColumn == prevCol));
      /* Note that prevCol can still be the sentinel (if we added to make zero),
         in which case prevCol->fColumn == ~0u. */
      assert( (prevCol->fColumn + 1 <= se->fColumn + 1)
	      && (prevCol->fNextColumn == de));
    }

  return true;
}

void QcSparseMatrix::CopyRow(unsigned destRow, QcSparseMatrix const &srcMatrix, unsigned srcRow)
{
  ZeroRow (destRow);
  
  /* effic: Construct the row in place instead of using SetValue. */
  for (QcSparseMatrixRowIterator i( srcMatrix, srcRow);
       !i.AtEnd();
       i.Increment())
    SetValue (destRow, i.getIndex(), i.getValue());
}

void QcSparseMatrix::FractionRow(unsigned row, numT factor)
{
  /* The reason we don't do simply `ScaleRow( row, recip( factor))'
     is to reduce round-off error. */

  qcAssertPre( row < fRows);

  if(QcUtility::IsOne( factor))
    return;

  QcSparseMatrixElement *e = fRowHeads[ row]->fNextColumn;
  while(!e->isRowHead())
    {
      e->fValue /= factor;
      e = e->fNextColumn;
    }
}

numT QcSparseMatrix::GetValue(unsigned row, unsigned col) const
{
  qcAssertPre( row < fRows);
  qcAssertPre( col < fColumns);

  unsigned eCol;
  QcSparseMatrixElement *e = fRowHeads[ row];
  do
    e = e->fNextColumn;
  while((eCol = e->fColumn) < col);

  return (eCol == col
	  ? e->fValue
	  : numT( 0));
}


void
QcSparseMatrix::Pivot(unsigned row, unsigned col)
{
  qcAssertPre( row < getNRows());
  qcAssertPre( col < getNColumns());

  numT p = GetValue(row, col);
  qcAssertPre( !QcUtility::IsZero( p));

  ScaleRow( row, recip( p));

  // Cache column coefficients because it is problematic due to the possible
  // change of column coefficients.
  {
    vector<QcSparseCoeff> coeffCache;

    for (QcSparseMatrixColIterator varCoeffs(*this, col);
	 !varCoeffs.AtEnd();
	 varCoeffs.Increment())
      {
	if(varCoeffs.getIndex() != row)
	  coeffCache.push_back( QcSparseCoeff( varCoeffs.getValue(),
					       varCoeffs.getIndex()));
      }

    for (vector<QcSparseCoeff>::iterator colIt = coeffCache.begin();
	 colIt != coeffCache.end();
	 colIt++)
      AddScaledRow( colIt->getIndex(), row, -colIt->getValue());
  }


#ifdef qcCheckInternalPost
  QcSparseMatrixColIterator i( *this, col);
  qcAssertPost( i.getIndex() == row);
  qcAssertPost( QcUtility::IsOne( i.getValue()));
  i.Increment();
  qcAssertPost( i.AtEnd());
#endif
}


void QcSparseMatrix::Reserve(unsigned rows, unsigned cols)
	// This routing must cope with any of rows, cols, fRows, fColumns,
	// fAllocRows, fAllocColumns being 0.
	// Only this routine should change fAllocRows and fAllocColumns
	// apart from them being initialised to 0 by the constructor.
{
	qcAssertPre ((int) rows >= 0);
	qcAssertPre ((int) cols >= 0);

	QcSparseMatrixElement **e;
	unsigned i;

	assert( fAllocRows >= fRows);
	assert( fAllocColumns >= fColumns);

	// Copy existing column references and allocate new references
	if (cols > fAllocColumns) {
		e = new QcSparseMatrixElement*[cols];		// Allocate a new column vector

		// Copy old columns.
		assert( (fAllocColumns == 0) == (fColHeads == 0));
		if (fAllocColumns != 0) {
			memcpy( e, fColHeads,
			        fAllocColumns * sizeof(QcSparseMatrixElement *));
			delete [] fColHeads; 	// Free the old column vector
		}

		fColHeads = e;     		// matrix now uses the new col vector

		for (i = fAllocColumns; i < cols; i++)
		  {
		    QcSparseMatrixElement *head = new QcSparseMatrixElement( numT( 1), ~0u, i);
		    fColHeads[ i] = head;
		  }

		fAllocColumns = cols;
	}

	// Copy existing row references and allocate new references
	if (rows > fAllocRows) {
		e = new QcSparseMatrixElement*[rows];		// Allocate a new row vector

		// Copy old rows.
		if (fAllocRows != 0) {
			memcpy( e, fRowHeads, fAllocRows * sizeof( QcSparseMatrixElement *));
			delete [] fRowHeads; 	// Free the old row vector
		}

		fRowHeads = e;			// matrix now uses the new row vector

		for (i = fAllocRows; i < rows; i++)
		  fRowHeads[ i] = new QcSparseMatrixElement( numT( 1), i, ~0u); 	// Initialise new rows

		fAllocRows = rows;
	}

	qcAssertPost (fAllocRows >= rows);
	qcAssertPost (fAllocColumns >= cols);
	dbg(assertInvar());
}

#ifndef MIN
# define MIN(_a, _b) ((_a) < (_b) ? (_a) : (_b))
#endif

void QcSparseMatrix::Resize(unsigned rows, unsigned cols)
	// Note this routine is called by a constructor, be careful
	// calling any virtual members.
	// Only this routine should change fRows and fColumns.
{
  /* Dispose of surplus elements.  Simplistic algorithm since QOCA does not
     normally require resize downwards (except to zero, in which case the
     current algorithm is almost optimal anyway), so it's not worth finding an
     efficient algorithm -- especially as preliminary investigations indicate
     that efficient implementation is complex due to the fNextRow and
     fNextColumn pointers spanning the clipping region and that one can do
     better than scanning all the elements. */

  // Sweep rows.
  if (cols < fColumns)
    {
      TrimRows( cols);
      fColumns = cols; // Because fColHeads[i in [cols..fColumns)] now contain garbage pointers.
    }

  // Sweep columns.
  if (rows < fRows)
    {
      for (unsigned j = fColumns; j--;)
	TrimColumn( j, rows);
      fRows = rows;
    }

  // To aid in correctness analysis, note that after each sweep the
  // matrix is consistent: the sweeps simply produce empty rows or
  // columns outside the clipping region.

  // Add new rows and columns
  if ((rows > fAllocRows)
      || (cols > fAllocColumns))
    Reserve( MAX( rows, 2 * fAllocRows),
	     MAX( cols, 2 * fAllocColumns));

  // Initialise newly exposed columns
  for (unsigned i = fColumns; i < cols; i++)
    {
      QcSparseMatrixElement *head = fColHeads[i];
      head->fNextRow = head->fPrevRow = head;
    }
  fColumns = cols;

  // Initialise newly exposed rows
  for (unsigned i = fRows; i < rows; i++)
    {
      QcSparseMatrixElement *head = fRowHeads[ i];
      head->fNextColumn = head->fPrevColumn = head;
    }
  fRows = rows;

  dbg(assertInvar());
}

bool QcSparseMatrix::ScaleRow(unsigned row, numT factor)
{
  qcAssertPre( row < fRows);

  if(QcUtility::IsOne( factor))
    return false;

  QcSparseMatrixElement *e = fRowHeads[ row]->fNextColumn;
  while(!e->isRowHead())
    {
      e->fValue *= factor;
      e = e->fNextColumn;
    }

  return true;
}

void QcSparseMatrix::NegateRow(unsigned row)
{
  qcAssertPre( row < fRows);

  QcSparseMatrixElement *e = fRowHeads[ row]->fNextColumn;
  while(!e->isRowHead())
    {
      make_neg( e->fValue);
      e = e->fNextColumn;
    }
}

void QcSparseMatrix::SetValue(unsigned row, unsigned col, numT val)
	// Note that SetValue currently search the row for the insertion point.
	// They could just as easily be implemented to search the column.
	// Which is best depends on the application and may not be matter.
{
  qcAssertPre( row < fRows);
  qcAssertPre( col < fColumns);

  // Find existing element or insertion point.
  QcSparseMatrixElement *e = fRowHeads[ row];	// element cursor
  QcSparseMatrixElement *prevCol;		// Link from previous column

  do
    {
      prevCol = e;
      e = e->fNextColumn;
    }
  while(e->fColumn < col);

  assert( e->getColNr() >= col);
  assert( prevCol->fNextColumn == e);

  if(e->fColumn == col)
    {	// found existing
      if(QcUtility::IsZero( val))
	{
	  e->fPrevColumn->fNextColumn = e->fNextColumn;
	  e->fNextColumn->fPrevColumn = e->fPrevColumn;

	  e->fPrevRow->fNextRow = e->fNextRow;
	  e->fNextRow->fPrevRow = e->fPrevRow;

	  delete e;		// Free memory.
	}
      else
	e->fValue = val;		// update existing
    }
  else if(!QcUtility::IsZero( val))
    {	// Allocate a new element.
      QcSparseMatrixElement *ne = new QcSparseMatrixElement( val, row, col);

      // Insert new element into row.
      ne->fNextColumn = e;
      e->fPrevColumn = ne;

      prevCol->fNextColumn = ne;
      ne->fPrevColumn = prevCol;

      // Insert new element into column.
      {
	QcSparseMatrixElement *head = fColHeads[ col];

	ne->fPrevRow = head;
	ne->fNextRow = head->fNextRow;
	head->fNextRow = ne;
	ne->fNextRow->fPrevRow = ne;
      }
    }
}

void QcSparseMatrix::CopyColumn(unsigned destCol, unsigned srcCol)
{
  ZeroColumn( destCol);

  for(QcSparseMatrixColIterator colIt( *this, srcCol);
      !colIt.AtEnd();
      colIt.Increment())
    SetValue( colIt.getIndex(), destCol, colIt.getValue());
}

#if 0 /* unused */
void QcSparseMatrix::SwapRows(unsigned r1, unsigned r2)
{
	numT temp;

	/* effic: Avoid use of SetValue,GetValue. */
	for (unsigned i = 0; i < fColumns; i++) {
		temp = GetValue(r1, i);
		SetValue(r1, i, GetValue(r2, i));
		SetValue(r2, i, temp);
	}
}
#endif

void QcSparseMatrix::TrimColumn(unsigned col, unsigned from)
{
	qcAssertPre(col < fColumns);
	qcAssertPre(from < fAllocRows);

	QcSparseMatrixElement *e1 = fColHeads[ col];
	QcSparseMatrixElement *last = e1;
	e1 = e1->fNextRow;

	while (!e1->isColHead()) {
		QcSparseMatrixElement *e2 = e1->fNextRow;

		if (e1->fRow >= from)
         		delete e1;		// Free memory.
		else
         		last = e1;

		e1 = e2;
	}

	assert( e1 == fColHeads[ col]);
	last->fNextRow = e1;		// fixup dangling pointer
}

void QcSparseMatrix::Zero()
{
  if(fColumns != 0)
    TrimRows( 0);

  // Fix up after TrimRows.
  for(unsigned i = 0; i < fColumns; i++)
    {
      QcSparseMatrixElement *head = fColHeads[ i];
      head->fNextRow = head->fPrevRow = head;
    }
}

void QcSparseMatrix::TrimRows(unsigned from)
{
  qcAssertPre( from < fAllocColumns);

  for(QcSparseMatrixElement **ri = fRowHeads, **riEnd = ri + fRows;
      ri < riEnd; ri++)
    {
      QcSparseMatrixElement *head = *ri;

      /* Go backwards: no need to iterate over elements we aren't deleting. */
      QcSparseMatrixElement *e1 = head->fPrevColumn;
      while(e1->fColumn + 1 > from) // Note that e1->fColumn can be ~0u.
	{
	  QcSparseMatrixElement *savedPrevCol = e1->fPrevColumn;
	  delete e1;		// Free memory.
	  e1 = savedPrevCol;
	}

      e1->fNextColumn = head;
      head->fPrevColumn = e1;
    }
}

void QcSparseMatrix::ZeroColumn(unsigned col)
{
  qcAssertPre( col < fColumns);

  QcSparseMatrixElement *head = fColHeads[ col];
  QcSparseMatrixElement *ecol = head->fNextRow; // element cursor

  while(ecol != head)
    {				// Remove the column elements from rows
      ecol->fPrevColumn->fNextColumn = ecol->fNextColumn;
      ecol->fNextColumn->fPrevColumn = ecol->fPrevColumn;

      QcSparseMatrixElement *temp = ecol;
      ecol = ecol->fNextRow;
      delete temp;
    }

  head->fPrevRow = head->fNextRow = head;
}

void QcSparseMatrix::ZeroRow(unsigned row)
{
  qcAssertPre( row < fRows);

  QcSparseMatrixElement *head = fRowHeads[ row];
  QcSparseMatrixElement *erow = head->fNextColumn;

  while(erow != head)
    {				// Remove the column elements from rows
      erow->fPrevRow->fNextRow = erow->fNextRow;
      erow->fNextRow->fPrevRow = erow->fPrevRow;

      QcSparseMatrixElement *temp = erow;
      erow = erow->fNextColumn;
      delete temp;
    }

  head->fPrevColumn = head->fNextColumn = head;
}
