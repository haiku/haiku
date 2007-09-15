// $Id: QcLinEqTableau.hh,v 1.18 2001/01/30 01:32:08 pmoulder Exp $

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

#ifndef __QcLinEqTableauH
#define __QcLinEqTableauH

#include <deque>
#include <vector>
#include "qoca/QcTableau.hh"
#include "qoca/QcSparseCoeff.hh"
#include "qoca/QcDelCoreTableau.hh"
#include "qoca/QcLinEqRowColStateVector.hh"

class QcLinEqTableau : public QcTableau
{
protected:
	QcLinEqRowColStateVector *fRowColState;
	QcLinEqRowStateVector *fRowState;
	QcLinEqColStateVector *fColState;
	QcCoreTableau *fCoreTableau;

public:
  	deque<unsigned int> fPivotHints;
		// During an eliminate operation, fPivotHints is erased and then
		// the indices of parametric variables that do not have zero
		// solved form coeffs are recorded.  These are likely to be good
		// candidates for the pivot operation.  Note that it is still
		// necessary to check that the coeff is non-zero, since
		// parametric coeffs may be modified by eliminate after the coeff
		// is recorded, this check is intended to be done using pivot.

	//-----------------------------------------------------------------------//
	// Constructors.                                                         //
	//-----------------------------------------------------------------------//
	QcLinEqTableau(unsigned hintRows, unsigned hintCols, QcBiMapNotifier &n);
	QcLinEqTableau(QcCoreTableau &tab, QcBiMapNotifier &n);
	virtual ~QcLinEqTableau();

#ifndef NDEBUG
  void assertInvar() const { }
  void assertDeepInvar() const;
  virtual void vAssertDeepInvar() const;
#endif

	//-----------------------------------------------------------------------//
	// Data Structure access functions.                                      //
	//-----------------------------------------------------------------------//
   	QcCoreTableau &GetCoreTableau() const
		{ return *fCoreTableau; }

	QcLinEqRowColStateVector &GetRowColState() const
		{ return *fRowColState; }

	QcLinEqRowStateVector &GetRowState() const
		{ return *fRowState; }

	QcLinEqColStateVector &GetColState() const
		{ return *fColState; }

	//-----------------------------------------------------------------------//
	// Query functions.                                                      //
	//-----------------------------------------------------------------------//

  /** Get RHS for solved-form constraint <tt>ci</tt>.

      <p><tt>ci</tt> is allowed to be a deleted constraint, in which case zero
      is returned.
  **/
  virtual numT GetRHS(unsigned ci) const
    { return fCoreTableau->GetRHS(ci); }


  /** Get coefficient for variable <tt>vi</tt> in solved-form constraint <tt>ci</tt>.

      <p><tt>ci</tt> is allowed to be a deleted constraint, in which case zero
      is returned.

      @precondition <tt>ci &lt; getNRows()</tt>
      @precondition <tt>vi &lt; getNColumns()</tt>
  **/
  virtual numT GetValue(unsigned ci, unsigned vi) const;


	/** Find the value of the basic variable with index <tt>vi</tt>, using
	    the desired value cache and tableau coeffs.

	    @precondition <tt>IsBasic(vi)</tt>
	**/
	virtual numT EvalBasicVar(unsigned vi) const;

	int GetColumns() const
  		{ return fCoreTableau->GetColumns(); }

	unsigned getNColumns() const
  		{ return fCoreTableau->GetColumns(); }

	bool GetARowDeleted(unsigned ci) const
		{ return fCoreTableau->GetARowDeleted(ci); }

	int GetARowIndex(unsigned ci) const
		{ return fCoreTableau->GetARowIndex(ci); }

	bool GetMRowDeleted(unsigned ci) const
		{ return fCoreTableau->GetMRowDeleted(ci); }

	int GetMRowIndex(unsigned ci) const
  		{ return fCoreTableau->GetMRowIndex(ci); }

	int GetBasicVar(unsigned ci) const
  		{ return fRowState->GetBasicVar(ci); }

	numT GetDesireValue(unsigned vi) const
  		{ return fColState->GetDesireValue(vi); }

	int GetRowCondition(unsigned ci) const
  		{ return fRowState->GetCondition(ci); }

	int GetRows() const
  		{ return fCoreTableau->GetRows(); }

	unsigned getNRows() const
  		{ return fCoreTableau->GetRows(); }

#if 0 /* unused */
	numT GetSparsenessInBasic() const;
	numT GetSparsenessExBasic() const;
#endif

	int IsBasicIn(unsigned vi) const
  		{ return fColState->IsBasicIn(vi); }

	bool IsBasic(unsigned vi) const
		{ return fColState->IsBasic(vi); }

	bool IsRedundant(unsigned ci) const
		{ return (GetRowCondition(ci) == QcLinEqRowState::fRedundant); }

  /** Indicates whether the tableau is in solved form.
      Returns true iff every constraint is determined (i.e. !IsUndetermined).
  **/
	bool IsSolved() const;

	bool IsUndetermined(unsigned ci) const;

	bool IsValidCIndex(int ci) const
  		{ return fCoreTableau->IsValidCIndex(ci); }

	bool IsValidOCIndex(int ci) const
		{ return fCoreTableau->IsValidOCIndex(ci); }

	bool IsValidVIndex(int vi)
  		{ return fCoreTableau->IsValidVIndex(vi); }

	//-----------------------------------------------------------------------//
	// Manipulation functions.                                               //
	//-----------------------------------------------------------------------//

	/** Create a new invalid constraint, and add any new variables
	    indicated.

	    @return The new original constraint index.

	    @postcondition ret &ge; 0
	    @postcondition ret &lt; getNRows()
	**/
	virtual int AddEq(QcRowAdaptor &varCoeffs, numT rhs)
	{
	  int ret = fCoreTableau->AddRow( varCoeffs, rhs);
	  qcAssertPost( (unsigned) ret < getNRows());
	  return ret;
	}

	virtual void ChangeRHS(unsigned ci, numT rhs)
		// Change the RHS of the constraint ci.
  		{ fCoreTableau->ChangeRHS(ci, rhs); }

	virtual void DeleteRow(int co);
		// Delete an undetermined constraint given its original row index

	virtual int Eliminate(unsigned ci);		// Call after addEq
		// see QcTableau.hh for doc.

	/** Create a new non-basic variable. */
	virtual unsigned IncreaseColumns();

	/** Create a new constraint. */
	virtual unsigned IncreaseRows()
		{ return fCoreTableau->IncreaseRows(); }

	virtual unsigned NewVariable()
		// Create a new non basic variable and return its index.  This
		// new variable will have zero coefficient in all existing constraints.
		// There is no necessity to explicitely call this function.  It just
		// may is convenient to determine in advance the variable index for
		// a new variable rather than let AddEq to assign one.
  		{ return IncreaseColumns(); }

	virtual bool Pivot(unsigned ci, unsigned vi);
		// The parameters indicate a solved form coeff. Pivot can only be
		// applied to a normalised or regular constraint.  Get the pivot
		// element and check it is non-zero, this catches redundant and
		// deleted constaints as well (Pivot should not be called for these).
		// To save checking that the pivot element is non-zero before calling
		// Pivot, and Pivot has to look at this in any case, it is expected
		// that Pivot be used to do this check.

	virtual void Restart()
		// Erase everything ready to start afresh. This also restarts
		// fRowColState.
		{ fCoreTableau->Restart(); }

	virtual void Restore();
		// By setting all the undeleted constraints to normalised, the tableau
		// data structures can remain in a consistent state through this Reset.
		// It is tempting to set fRows to -1 and bring it back up to its
		// original value while solving each constraint in turn.  But this is
		// an invitation to disaster since other routines are entitled to
		// assume fRows is an upper bound for constraint indices and the
		// deleted rows information in fMRowIndex and fIsDeleted would need to
		// violate this. Hence the safe implementation used here only relies
		// on the rest of the tableau methods being able to cope with multiple
		// normalised constraints. Restore establishes a state equivalent to
		// the original constraints having been all entered using AddRow and
		// nothing else apart from the deleted entries being left alone.


	/** Normalize the corresponding row in fM if the rhs is negative.  This
	    makes rhs positive.

	    @precondition <tt>ci &lt; getNRows()</tt>
	**/
	void Normalize (unsigned ci)
	{
	  qcAssertPre (ci < getNRows());
	  fCoreTableau->Normalize (ci);
	}

	void SetARowDeleted(int ci, bool d)
		{ fCoreTableau->SetARowDeleted(ci, d); }

	void SetARowIndex(int ci, int i)
		{ fCoreTableau->SetARowIndex(ci, i); }

	void SetDesireValue(int vi, numT dv) 	// must be setup by client
		{ fColState->SetDesireValue(vi, dv); }

	void SetMRowDeleted(int ci, bool d)
		{ fCoreTableau->SetMRowDeleted(ci, d); }

	void SetMRowIndex(int ci, int i)
		{ fCoreTableau->SetMRowIndex(ci, i); }

	void SetBasicVar(int ci, int i)
		{ fRowState->SetBasicVar(ci, i); }

	void SetRowCondition(int ci, int s)
		{ fRowState->SetCondition(ci, s); }

	void SetBasicIn(int vi, int i)
		{ fColState->SetBasicIn(vi, i); }

	void SetBasic(int vi, bool b)
		{ fColState->SetBasic(vi, b); }

	void Unsolve(unsigned ci);
		// Given a solved form constraint index, for a regular constraint,
		// Unsolve removes it from the basis so that it looks like a newly
		// entered row after Eliminate but before a Pivot. However the
		// bookeeping information associating this solved form constraint with
		// an original constraint is not done by Unsolve.

	//-----------------------------------------------------------------------//
	// Utility function.                                                     //
	//-----------------------------------------------------------------------//
	virtual void Print(ostream &os) const;

protected:
	//-----------------------------------------------------------------------//
	// Constructors.                                                         //
	//-----------------------------------------------------------------------//
	QcLinEqTableau(QcBiMapNotifier &n);
};

inline QcLinEqTableau::QcLinEqTableau(unsigned hintRows, unsigned hintCols,
				      QcBiMapNotifier &n)
	: QcTableau(n),
	  fRowColState(new QcLinEqRowColStateVector()),
	  fRowState(&fRowColState->GetRowState()),
	  fColState(&fRowColState->GetColState()),
	  fCoreTableau(new QcCoreTableau(hintRows, hintCols, *fRowColState))
{
}

inline QcLinEqTableau::QcLinEqTableau(QcCoreTableau &tab, QcBiMapNotifier &n)
	: QcTableau(n),
	  fRowColState((QcLinEqRowColStateVector *)&tab.GetRowColState()),
	  fRowState(&fRowColState->GetRowState()),
	  fColState(&fRowColState->GetColState()),
	  fCoreTableau(&tab)
{
}

inline QcLinEqTableau::QcLinEqTableau(QcBiMapNotifier &n)
		: QcTableau(n)
{
}

inline QcLinEqTableau::~QcLinEqTableau()
{
	delete fRowColState;
	delete fCoreTableau;
}

inline void QcLinEqTableau::DeleteRow(int co)
{
	SetRowCondition(fCoreTableau->GetMRowIndex(co), QcLinEqRowState::fInvalid);
	fCoreTableau->DeleteRow(co);
}

inline unsigned QcLinEqTableau::IncreaseColumns()
{
	unsigned i = fCoreTableau->IncreaseColumns();
	return i;
}

inline bool QcLinEqTableau::IsUndetermined(unsigned ci) const
{
  int status = GetRowCondition( ci);
  return ((status == QcLinEqRowState::fInvalid)
	  || (status == QcLinEqRowState::fNormalised));
}

#endif /* !__QcLinEqTableauH */
