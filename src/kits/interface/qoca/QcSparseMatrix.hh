// $Id: QcSparseMatrix.hh,v 1.17 2001/01/31 10:47:44 pmoulder Exp $

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

#ifndef __QcSparseMatrixH
#define __QcSparseMatrixH

#include <vector.h>
#include "qoca/QcDefines.hh"
#include "qoca/QcMatrix.hh"
#include "qoca/QcSparseMatrixElement.hh"
#include "qoca/QcSparseCoeff.hh"

class QcSparseMatrixIterator;
class QcSparseMatrixRowIterator;
class QcSparseMatrixColIterator;

/** A matrix of <tt>numT</tt>s whose representation is optimized for containing many zero elements.

    <p>["Optimized" is used a little loosely in the above statement, but that is
    at least the intent.]

    <p>Representation: There are two arrays, <tt>fRowHeads</tt> and
    <tt>fColHeads</tt>.

    <p>&forall;[j] <tt>fColHeads[j]</tt> points to a "sentinel"
    element of a circular bidirectional unsorted linked list of the
    non-zero elements of column j.  The sentinel element is marked
    with row number ~0u.

    <p><tt>fRowHeads</tt> is similar, but the list is sorted by column number.

    <p>Proposal for better representation:
    <ul>

    <li>Use hash tabling (see below) to allow lookups to be O(1) (approximately).

    </ul>

    <p>The reason for still having linked lists is for more efficient iteration
    over non-zero elements of a specified row or column.  The cost of iterating
    over a hashtable in STL is O(size of table), which can be a relatively large
    multiplier.  Unsorted linked lists are cheap to maintain, the main cost
    being memory usage (four words per stored element for doubly-linked lists).

    <p>Singly-linked lists are not worth the extra programming effort.  [In
    favor of singly-linked lists: (i) We save two words per stored element.
    (ii) Deletions are relatively rare, the main cases being <tt>clear()</tt>
    (which isn't a problem because we're iterating over everything anyway) and
    pivoting.  However, there may be some correctness issues due to the changing
    of the address of the elements <em>following</em> the deleted item.  Also
    consider that the following item could be the sentinel.]

    <p>Regarding choice of backing for O(1) lookup: The choices are:
    <ol>
    <li>Dense matrix.
    <li>Single hashtable mapping from pair(row number, column number) to element.
    <li>Array of fAllocRows hashtables mapping from column number to element.  (Or the other way around.)
    </ol>

    <p>Option one is best for relatively small problems, especially since STL
    hashtables have a minimum size of 53 elements (see __stl_prime_list in
    stl_hashtable.h).  It's also the right choice if we don't care about memory
    consumption.

    <p>Option 3 has a better-than-random hash function (namely the column number
    itself): there will in many problems be no clashes at all, since regularity
    in the column numbers works in our favour.

    <p>Option 2 is more memory efficient, though finding a good hash function is
    harder.  That said, an array of hash tables is conceptually equivalent to a
    large single hash table, which suggests that a single hash table should be
    at least almost as good as an array of hash tables.
**/
class QcSparseMatrix : public QcMatrix
{
public:
  friend class QcSparseMatrixRowIterator;
  friend class QcSparseMatrixColIterator;

	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
	QcSparseMatrix()
	  : QcMatrix(), fRowHeads( 0), fColHeads( 0)
	{ }

	virtual ~QcSparseMatrix();

#ifndef NDEBUG
  void assertInvar() const;
  void assertDeepInvar() const { assertInvar(); }
  virtual void vAssertDeepInvar() const;
#endif

	//-----------------------------------------------------------------------//
	// Enquiry functions.                                                    //
	//-----------------------------------------------------------------------//
	virtual numT GetValue(unsigned row, unsigned col) const;

  inline QcSparseMatrixElement const *
  getFirstInRow( unsigned row) const
  {
    qcAssertPre( row < fRows);
    return fRowHeads[ row]->getNextInRow();
  }

  inline QcSparseMatrixElement const *
  getFirstInCol( unsigned col) const
  {
    qcAssertPre( col < fColumns);
    return fColHeads[ col]->getNextInCol();
  }

	//-----------------------------------------------------------------------//
	// Matrix size management functions.                                     //
	//-----------------------------------------------------------------------//
	virtual void Reserve(unsigned rows, unsigned cols);
	virtual void Resize(unsigned rows, unsigned cols);
		// Change the size of the used part of the array up or down.
		// Increases may take time up to O(rows * cols).

	//-----------------------------------------------------------------------//
	// Basic matrix data management functions.                               //
	//-----------------------------------------------------------------------//
	void CopyRow(unsigned destRow, QcSparseMatrix const &srcMatrix, unsigned srcRow);

	virtual void CopyColumn(unsigned d, unsigned s);
  //virtual void SwapRows(unsigned r1, unsigned r2);
	virtual void SetValue(unsigned row, unsigned col, numT val);
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
  /** Equivalent to ScaleRow(row, -1), but faster. */
  void NegateRow( unsigned row);

private:
  //-----------------------------------------------------------------------//
  // Sparse matrix specific functions.                                     //
  //-----------------------------------------------------------------------//

  /** Disposes each row's sparse elements with column index greater than or
      equal to <tt>from</tt>.  Doesn't touch <tt>fColHeads</tt>, i.e. the
      pointers are left dangling.
  **/
  void TrimRows(unsigned from);


  /** Exactly analagous to <tt>TrimRow</tt>.  I.e. same explanation applies
      except swapping the word `row' with the word `column' throughout. */
  void TrimColumn(unsigned col, unsigned from);


private:
  QcSparseMatrixElement **fRowHeads;
  QcSparseMatrixElement **fColHeads;
};

#endif
