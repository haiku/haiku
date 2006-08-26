// $Id: QcCoreTableau.hh,v 1.13 2001/01/30 01:32:07 pmoulder Exp $

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
//----------------------------------------------------------------------------//


#ifndef __QcCoreTableauH
#define __QcCoreTableauH

#include "qoca/QcSolvedFormMatrix.hh"
#include "qoca/QcQuasiRowColStateVector.hh"
#include "qoca/QcOrigRowStateVector.hh"
#include "qoca/QcUtility.hh"

class QcRowAdaptor;

/** At this level it is important to understand the relationship between rows
    and columns of fM and fA in the quasi representation.  Each solved form
    constraint coefficient is represented in a decomposed form.  Rows of fM
    correspond to constraints and a CIndex constraint iterator Index field
    indicates which row of fM this is.  For each constraint there is a row of fA
    and both a row and a column of fM.  However, once a constraint has been
    assigned a basic variable (via Pivot), there is no association between the
    corresponding row of fM and any particular row of fA.  The constraint coeff
    (ci,vi) is the dot product of a row of fM and a column of fA (ignoring
    deleted entries).  The column is given by vi directly.  The row of fM is
    ci->MRowIndex.  The QuasiRowState structure provides information about rows
    of fA as well as rows of fM.  For example ARowDeleted.
                                                                            
    ARowDeleted(ci) means row ci of fA, row MRowIndex(ci) of fM and column ci of
    fM corrrespond to a deleted constraint.  ARowDeleted(ci) &rArr;
    MRowDeleted(MRowIndex(ci)).
**/
class QcCoreTableau
{
public:
	/** Solved form. */
	QcSolvedFormMatrix fSF;

	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
	QcCoreTableau( QcQuasiRowColStateVector &rcs);
	QcCoreTableau( unsigned hintRows, unsigned hintCols,
		       QcQuasiRowColStateVector &rcs);

	virtual ~QcCoreTableau() { }

#ifndef NDEBUG
  void assertInvar() const;

  void assertDeepInvar() const
  {
    assertInvar();
    fSF.assertDeepInvar();
  }

  virtual void vAssertDeepInvar() const
  {
    assertDeepInvar();
  }
#endif


	//-----------------------------------------------------------------------//
	// Query functions.                                                      //
	//-----------------------------------------------------------------------//

	/** Get RHS for solved form constraint <tt>ci</tt>.
	    If <tt>GetMRowDeleted(ci))</tt> then return 0.

	    @precondition <tt>ci &lt; getNRows()</tt>
	**/
	virtual numT GetRHS(unsigned ci) const;

	/** Get coefficient for variable vi in constraint ci.
	    If <tt>GetMRowDeleted(ci))</tt> then return 0.

	    <P>(One reason that GetRHS and GetValue work for deleted constraints
	    (returning 0) is to satisfy the RowCol interface.)

	    @precondition <tt>ci &lt; getNRows()</tt>
	    @precondition <tt>vi &lt; getNColumns()</tt>
	**/
	virtual numT GetValue(unsigned ci, unsigned vi) const;

	unsigned GetAllocRows() const
		{ return fAllocRows; }

	unsigned GetAllocCols() const
		{ return fAllocColumns; }

	unsigned GetColumns() const
		{ return fColumns; }

	bool GetARowDeleted(unsigned ci) const
		{ return fOrigRowState.GetARowDeleted(ci); }

	int GetARowIndex(unsigned ci) const
		{ return fRowColState.fRowState->GetARowIndex(ci); }

	bool GetMRowDeleted(unsigned ci) const
		{ return fRowColState.fRowState->GetMRowDeleted(ci); }

	int GetMRowIndex(unsigned ci) const
		{ return fOrigRowState.GetMRowIndex(ci); }

	numT GetRawRHS(unsigned ci) const	// The orginal constraint RHS
		{ return fOrigRowState.GetRHS(ci); }  // Don't confuse this with GetRHS

	QcQuasiRowColStateVector &GetRowColState() const
		{ return fRowColState; }

	unsigned GetRows() const
		{ return fRows; }

	bool IsValidOCIndex(int ci) const
		{ return ((unsigned) ci < fRows && !GetARowDeleted(ci)); }

	bool IsValidCIndex(int ci) const
		{ return ((unsigned) ci < fRows && !GetMRowDeleted(ci)); }

	bool IsValidVIndex(int vi) const 
		{ return ((unsigned) vi < fColumns); }

	//-----------------------------------------------------------------------//
	// Manipulation functions.                                               //
	//-----------------------------------------------------------------------//

	/** Add a new row, allocating any new columns indicated.

	    @return The new row index.
	**/
	virtual unsigned AddRow(QcRowAdaptor &varCoeffs, numT rhs);

	virtual void AddScaledRow(unsigned destRow, unsigned srcRow, numT factor)
		{ fSF.AddScaledRow(destRow, srcRow, factor); }

	virtual void ChangeRHS(unsigned ci, numT rhs)
		// Change the RHS of the constraint ci.
		{ SetRawRHS(ci, rhs); }

	virtual int DeleteRow(unsigned ci);
		// Deletes a row given an original constraint index by setting all
		// the coefficients to zero and marking the row as deleted.
		// Derived classes use this method to delete constraints
		// which are found to be inconsistant after a successful AddEq
		// (i.e. unable to successfully pivot).

	virtual unsigned IncreaseColumns();	// Creates a new non-basic variable

	virtual unsigned IncreaseRows();	     // Creates a new redundant constraint
	virtual void Initialize(unsigned hintRows, unsigned hintCols);
    		// Call this after construction.

#if 0
	virtual void LinkRow(unsigned ci)
		// Add row ci to the row list.
		{ fRowColState.fRowState->LinkRow(ci); }
#endif

	/** Force the RHS of row <tt>ci</tt> to be non-negative, by negating the
	    row iff its RHS was negative.

	    @precondition <tt>ci &lt; getNRows()</tt>
	**/
	virtual void Normalize(unsigned ci);

	virtual void Restart(); 		// Erase everything ready to start afresh.
	virtual void Restore();
		// The tableau is reinitialised as if all the original constraints
		// had been added to a fresh tableau using AddEq but with no
		// Elimination or Pivoting having been done.

	virtual void ScaleRow(unsigned row, numT factor)
		{ fSF.ScaleRow(row, factor); }

	virtual void CopyColumn(unsigned destCol, unsigned srcCol)
		{ fSF.CopyColumn(destCol, srcCol); }

	void SetARowDeleted(unsigned ci, bool d)
		{ fOrigRowState.SetARowDeleted(ci, d); }

	void SetARowIndex(unsigned ci, int i)
		{ fRowColState.fRowState->SetARowIndex(ci, i); }

	void SetMRowDeleted(unsigned ci, bool d)
		{ fRowColState.fRowState->SetMRowDeleted(ci, d); }

	void SetMRowIndex(unsigned ci, int i)
		{ fOrigRowState.SetMRowIndex(ci, i); }

	void SetRawRHS(unsigned ci, numT rhs) // The orginal constraint RHS
		{ fOrigRowState.SetRHS(ci, rhs); }

	//-----------------------------------------------------------------------//
	// Utility functions.                                                    //
	//-----------------------------------------------------------------------//
	virtual void Print(ostream &os) const;

protected:
	QcQuasiRowColStateVector &fRowColState;
	QcOrigRowStateVector fOrigRowState;	// 0..fRow-1
	unsigned fRows;
	unsigned fColumns;
	unsigned fAllocRows;
	unsigned fAllocColumns;
};

inline QcCoreTableau::QcCoreTableau(QcQuasiRowColStateVector &rcs)
		: fRowColState(rcs)
{
}

inline QcCoreTableau::QcCoreTableau(unsigned hintRows, unsigned hintCols,
				    QcQuasiRowColStateVector &rcs)
	: fRowColState(rcs)
{
	Initialize(hintRows, hintCols);
}

inline numT QcCoreTableau::GetRHS(unsigned ci) const
{
	qcAssertPre (ci < fRows);

	if (GetMRowDeleted(ci))
		return 0.0;

	return fSF.GetRHS(ci);
}

inline numT QcCoreTableau::GetValue(unsigned ci, unsigned vi) const
{
	qcAssertPre (ci < fRows);
	qcAssertPre (vi < fColumns);

	if (GetMRowDeleted(ci))
		return 0.0;

	return fSF.GetValue(ci, vi);
}

inline void QcCoreTableau::Normalize(unsigned ci)
{
	if (QcUtility::IsNegative(GetRHS(ci)))
		fSF.NegateRow( ci);
}

#ifndef qcNoStream
inline ostream &operator<<(ostream &os, const QcCoreTableau &t)
{
	t.Print(os);
	return os;
}
#endif

#endif
