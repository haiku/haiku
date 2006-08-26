// $Id: QcDelCoreTableau.hh,v 1.9 2001/01/30 01:32:07 pmoulder Exp $

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
//----------------------------------------------------------------------------//
// MRowIndex and ARowIndex record associations between rows of fA and fM      //
// which are used when a constraint is deleted or not yet in the basis (means //
// the fM[*,Index] is a unit vector, element fM[MRowIndex,Index] is 1.0 and   //
// and the other elements are zero).  MRowIndex is the row of fM              //
// corresponding to column fM[*,Index] of fM and row fA[Index,*] of fA.       //
// ARowIndex is the row of fA corresponding to row fM[Index,*].  Unused       //
// entries in ARowIndex and MRowIndex are set to -1.  Hence MRowIndex and     //
// ARowIndex are used to represent inverse indexing functions on a partial    //
// domain.                                                                    //
// EXCEPTION: this rule is currently violated by Pivot.                       //
//                                                                            //
// QcDelCoreTableau and QcCoreTableau are only ever used as part of a         //
// tableau, but it is not a tableau itself.  Deriving a tableau from it       //
// requires the use of multiple inheritance.  Currently we are trying to      //
// avoid multiple inheritance due to the increased risk running into compiler //
// bugs and to facilitate conversion to Java.                                 //
//============================================================================//

#ifndef __QcDelCoreTableauH
#define __QcDelCoreTableauH

#include "qoca/QcCoreTableau.hh"
#include "qoca/QcDenseMatrix.hh"
#include "qoca/QcBiMapNotifier.hh"
#include "qoca/QcSparseMatrixColIterator.hh"
#include "qoca/QcQuasiRowColStateVector.hh"
#include "qoca/QcSparseCoeff.hh"

class QcDelCoreTableau : public QcCoreTableau
{
public:
	#ifdef qcDenseQuasiInverse
	QcDenseMatrix fM; 		// fRows x fRows
	#else
	QcSparseMatrix fM;		// fRows x fRows
	#endif

	QcSparseMatrix fA;		// fRows x fCols

	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
	QcDelCoreTableau(QcQuasiRowColStateVector &rcs) : QcCoreTableau(rcs) {}
	QcDelCoreTableau(unsigned hintRows, unsigned hintCols, QcQuasiRowColStateVector &rcs)
			: QcCoreTableau(rcs)
		{ Initialize(hintRows, hintCols); }

#ifndef NDEBUG
  void assertInvar() const;
  void assertDeepInvar() const;

  virtual void vAssertDeepInvar() const
  {
    assertDeepInvar();
  }
#endif

	//-----------------------------------------------------------------------//
	// Enquiry methods.                                                      //
	//-----------------------------------------------------------------------//
	virtual numT GetRHS(unsigned ci) const;
		// Get RHS for solved form constraint ci.
	virtual numT GetRHSVirtual(unsigned ci) const;
	virtual numT GetValue(unsigned ci, unsigned vi) const;
		// Get coefficient for variable vi in constraint ci.
		// GetVal and GetRHS can be used to access the coefficients
		// of deleted constraints (zero is returned).  This behaviour is
		// required to satisfy the RowCol interface amongst other reasons.

  /** Returns (fM &times fA)[ci, vi], i.e.&nbsp;\sum_i&nbsp;fM[ci, i] &times;
      fA[i, vi].  As with <tt>GetValue</tt>, deleted rows are treated like
      zeroes.

      @precondition <tt>ci &lt; fRows</tt>
      @precondition <tt>ci &lt; fColumns</tt>
      @postcondition <tt>isZeroized( ret)</tt>
  **/
	virtual numT GetValueVirtual(unsigned ci, unsigned vi) const;

	bool IsFree(unsigned vi) const
		// Indicates if a particular variable is in use (i.e. has a
		// non-zero coefficient in any solved form (or original) constraint.
		{ return QcSparseMatrixColIterator(fA, vi).AtEnd(); }

	bool IsValidSolvedForm() const;
		// For debugging purposes to ensure that the real solved form is
		// consistant with the virtual solved form.

	//-----------------------------------------------------------------------//
	// Manipulation functions.                                               //
	//-----------------------------------------------------------------------//
	virtual unsigned AddRow(QcRowAdaptor &varCoeffs, numT rhs);
		// Add a new row, allocating any new columns indicated.
		// The new row index is returned.
	virtual void AddScaledRow(unsigned destRow, unsigned srcRow, numT factor);
	virtual void ChangeRHS(unsigned ci, numT rhs);
		// Change the RHS of the constraint ci.

	virtual void DecreaseColumns(QcBiMapNotifier &note);
		// Get rid of the last (empty) column.
	virtual void DecreaseRows(QcBiMapNotifier &note);
		// Get rid of the last (deleted) row.
	virtual int DeleteRow(unsigned ci);
		// Deletes a row given an original constraint index by setting all
		// the coefficients to zero and marking the row as deleted.
		// Derived classes use this method to delete constraints
		// which are found to be inconsistant after a successful AddEq
		// (i.e. unable to successfully pivot) and also used to remove a
		// constraint after is has been removed from the basis.
	virtual unsigned IncreaseColumns();
		// Creates a new non-basic variable.
	virtual unsigned IncreaseRows();
		// Creates a new redundant constraint.
	virtual void Initialize(unsigned hintRows, unsigned hintCols);
		// Call this after construction
	virtual void Normalize(unsigned ci);
		// Normalize the corresponding row fM and fSF if the rhs is negative.
		// This makes rhs positive.
	virtual void PivotQuasiInverse(unsigned row, unsigned col);
		// Pivot on the quasi inverse matrix.
	virtual void Restart(); // Erase everything ready to start afresh.
	virtual void Restore();
		// The tableau is reinitialised as if all the original constraints
		// had been added to a fresh tableau using AddEq but with no
		// Elimination or Pivoting having been done.
	virtual void ScaleRow(unsigned row, numT factor);
	virtual void CopyColumn(unsigned destCol, unsigned srcCol);

	void TransitiveClosure(vector<unsigned int> &vars) const;
		// Compute the transitive closure of a set of variables, using the
		// relation "belong to the same original constraint".

	//-----------------------------------------------------------------------//
	// Utility functions.                                                    //
	//-----------------------------------------------------------------------//
	virtual void Print(ostream &os) const;
};

inline void QcDelCoreTableau::AddScaledRow(unsigned destRow, unsigned srcRow,
					   numT factor)
{
	QcCoreTableau::AddScaledRow(destRow, srcRow, factor);
	fM.AddScaledRow(destRow, srcRow, factor);
}

#ifndef NDEBUG
inline void
QcDelCoreTableau::assertInvar() const
{
  assert( (fA.GetRows() == fRows)
	  && (fA.GetColumns() == fColumns)
	  && (fM.GetRows() == fRows)
	  && (fM.GetColumns() == fRows));
}

inline void
QcDelCoreTableau::assertDeepInvar() const
{
  QcCoreTableau::assertDeepInvar();
  fA.assertDeepInvar();
  fM.assertDeepInvar();
}
#endif

inline int QcDelCoreTableau::DeleteRow(unsigned ci)
{
	int quasiRow = QcCoreTableau::DeleteRow(ci);
	fM.ZeroColumn(ci);
	fM.ZeroRow(quasiRow);
	fA.ZeroRow(ci);
	return quasiRow;
}

inline numT QcDelCoreTableau::GetRHS(unsigned ci) const
{
	#ifdef qcRealTableauRHS
	return QcCoreTableau::GetRHS(ci);
	#else
	return GetRHSVirtual(ci);
	#endif
}

inline numT QcDelCoreTableau::GetValue(unsigned ci, unsigned vi) const
{
	#ifdef qcRealTableauCoeff
	return QcCoreTableau::GetValue(ci, vi);
	#else
	return GetValueVirtual(ci, vi);
	#endif
}

inline void QcDelCoreTableau::Initialize(unsigned hintRows, unsigned hintCols)
{
	QcCoreTableau::Initialize(hintRows, hintCols);
	fM.Reserve(hintRows, hintRows);
	fA.Reserve(hintRows, hintCols);
}

inline void QcDelCoreTableau::Normalize(unsigned ci)
{
	if (QcUtility::IsNegative(GetRHS(ci))) {
		fM.NegateRow(ci);
		fSF.NegateRow(ci);
	}
}

inline void QcDelCoreTableau::ScaleRow(unsigned row, numT factor)
{
	QcCoreTableau::ScaleRow(row, factor);
	fM.ScaleRow(row, factor);
}

inline void QcDelCoreTableau::CopyColumn(unsigned destCol, unsigned srcCol)
{
	QcCoreTableau::CopyColumn( destCol, srcCol);
	fA.CopyColumn( destCol, srcCol);
}

#ifndef qcNoStream
inline ostream &operator<<(ostream &os, const QcDelCoreTableau &t)
{
	t.Print(os);
	return os;
}
#endif

#endif
