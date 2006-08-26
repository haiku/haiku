// $Id: QcMatrix.hh,v 1.16 2001/01/30 01:32:08 pmoulder Exp $

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

#ifndef __QcMatrixH
#define __QcMatrixH

#include "qoca/QcDefines.hh"
//#include "qoca/QcMatrixIterator.hh"

class QcMatrix
{
public:
	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
	QcMatrix();
	virtual ~QcMatrix() {}

#ifndef NDEBUG
	void assertInvar() const
	{
		assert (fRows <= fAllocRows);
		assert ((int) fAllocRows >= 0);
		assert (fColumns <= fAllocColumns);
		assert ((int) fAllocColumns >= 0);
	}
#endif

	//-----------------------------------------------------------------------//
	// Enquiry functions.                                                    //
	//-----------------------------------------------------------------------//
	virtual numT GetValue(unsigned row, unsigned col) const = 0;

	unsigned GetColumns() const
		{ return fColumns; }

	unsigned getNColumns() const
		{ return fColumns; }

	unsigned GetRows() const
		{ return fRows; }

	unsigned getNRows() const
		{ return fRows; }

	//-----------------------------------------------------------------------//
	// Matrix size management functions.                                     //
	//-----------------------------------------------------------------------//

  /** Ensure that future resizes up to these limits will take place in constant
      time -- i.e.&nbsp;preallocate if necessary.  Never shrinks the allocation:
      equivalent to <tt>Reserve(MAX(rows, fAllocRows), MAX(cols,
      fAllocColumns))</tt>.  <tt>Reserve(fAllocRows, fAllocColumns)</tt> is a
      no-op.
  **/
	virtual void Reserve(unsigned rows, unsigned cols) = 0;

  void clear()
    {
      Resize( 0, 0);
    }

	virtual void Resize(unsigned rows, unsigned cols) = 0;
		// Change the size of the used part of the array up or down.
		// Increases may take time up to O(rows * cols).
		// New elements are initialised to zero.

	//-----------------------------------------------------------------------//
	// Basic matrix data management functions.                               //
	//-----------------------------------------------------------------------//
	//virtual void CopyRow(unsigned row, QcMatrixIterator &i) = 0;
  //virtual void IncreaseValue(unsigned row, unsigned col, numT inc) = 0;
	virtual void SetValue(unsigned row, unsigned col, numT val) = 0;
  //virtual void SwapColumns(unsigned c1, unsigned c2) = 0;
  //virtual void SwapRows(unsigned r1, unsigned r2) = 0;
	virtual void Zero() = 0;
	virtual void ZeroColumn(unsigned col) = 0;
	virtual void ZeroRow(unsigned row) = 0;

	//-----------------------------------------------------------------------//
	// Numerical computation functions.                                      //
	//-----------------------------------------------------------------------//

  // /** Equivalent to <tt>AddScaledRow(destRow, srcRow, 1.0)</tt>. */
  // virtual void AddRow(unsigned destRow, unsigned srcRow) = 0;


	/** Set destRow to ((old destRow) + ((old srcRow) * factor)).

	    @precondition <tt>destRow &lt; getNRows()</tt>
	    @precondition <tt>srcRow &lt; getNRows()</tt>
	    @precondition <tt>QcUtility::isFinite(factor)</tt>

	    @return <tt>!QcUtility::IsZero(factor)</tt>
	**/
	virtual bool AddScaledRow( unsigned destRow, unsigned srcRow,
				   numT factor) = 0;


	/** Equivalent to <tt>ScaleRow(row, 1.0/factor)</tt>, but with sometimes
	    slightly less round-off error. */
	virtual void FractionRow(unsigned row, numT factor) = 0;


  /** Do a pivot operation on <tt>row</tt>, <tt>col</tt>.

      <p>In the following conditions, M<sub>i, j</sub> means
      <tt>getValue(i,&nbsp;j)</tt>, and p means old M<sub>row, col</sub>.

      @precondition <tt>!IsZero(p)</tt>
      @precondition <tt>row &lt; getNRows()</tt>
      @precondition <tt>col &lt; getNColumns()</tt>

      <p>The last two postconditions follow from the first two.

      @postcondition row <tt>row</tt> is 1/p times its old value

      @postcondition &forall;[i: i &ne <tt>row</tt>] row i is its old
      value minus M<sub>i, col</sub> times the new row <tt>row</tt>.

      @postcondition getValue(row, col) = 1
      @postcondition &forall;[i &ne; row] M<sub>i, col</sub> = 0
  **/
  virtual void Pivot(unsigned row, unsigned col) = 0;

	/** Set row to old row * factor. */
	virtual bool ScaleRow(unsigned row, numT factor) = 0;

	//-----------------------------------------------------------------------//
	// Utility functions.                                                    //
	//-----------------------------------------------------------------------//
	virtual void Print(ostream &os) const;

protected:
	unsigned fAllocRows;		// Used only by Reserve
	unsigned fAllocColumns; 	// Used only by Reserve
	unsigned fRows; 		// Number of rows
	unsigned fColumns;		// Number of columns
};

inline QcMatrix::QcMatrix()
		: fAllocRows(0), fAllocColumns(0), fRows(0), fColumns(0)
{
}

inline void QcMatrix::Print(ostream &os) const
{
	for (unsigned i = 0; i < fRows; i++) {
		os << i << ":";
		for (unsigned j = 0; j < fColumns; j++)
			os << "\t" << GetValue(i, j);
		os << endl;
	}
}

#ifndef qcNoStream
inline ostream &operator<<(ostream &os, const QcMatrix &m)
{
	m.Print(os);
	return os;
}
#endif

#endif
