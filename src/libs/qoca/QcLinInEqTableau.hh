// $Id: QcLinInEqTableau.hh,v 1.18 2001/01/30 01:32:08 pmoulder Exp $

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

#ifndef __QcLinInEqTableauH
#define __QcLinInEqTableauH

#include "qoca/QcLinInEqRowColStateVector.hh"
#include "qoca/QcDelLinEqTableau.hh"
#include "qoca/QcRowAdaptor.hh"

class QcLinInEqTableau : public QcDelLinEqTableau
{
protected:
	QcLinInEqRowColStateVector *fRowColState;
	QcLinInEqColStateVector *fColState;

public:
	//-----------------------------------------------------------------------//
	// Constructors.                                                         //
	//-----------------------------------------------------------------------//
	QcLinInEqTableau(unsigned hintRows, unsigned hintCols, QcBiMapNotifier &n);
	QcLinInEqTableau(QcCoreTableau &tab, QcBiMapNotifier &n);

	//-----------------------------------------------------------------------//
	// Query functions.                                                      //
	//-----------------------------------------------------------------------//
	virtual numT EvalBasicVar(unsigned vi) const;
		// Use the desired value cache and tableau coeffs to evaluate
		// the basic variable with index vi.
	virtual numT GetRHS(unsigned ci) const;
		// This version of getRHS for QcLinInEqTableau is needed because the
		// sign of the RHS value is critical for deciding feasibility.
		// Can still use QcLinEqTableau::getRHS where sign is not critical.

	int GetColCondition(unsigned vi) const
		{ return fColState->GetCondition(vi); }

	numT GetObjValue(unsigned vi) const
		{ return fColState->GetObjValue(vi); }

	/** Returns the number of restricted rows, i.e.&nbsp;n({i: <tt>IsRestricted(i)</tt>}).
	    @postcondition 0 &le; ret &le; <tt>getNRows()</tt>
	**/
	unsigned getNRestrictedRows() const;

	bool IsArtificial(unsigned vi) const
		{ return fColState->IsArtificial(vi); }

	/** Returns true iff &forall;[unrestricted undeleted rows <tt>ci</tt>]
	    <tt>GetRHS(ci)</tt> &ge; 0. */
	bool IsBasicFeasible() const;

	bool IsBasicFeasibleSolved() const
  		{ return (IsSolved() && IsBasicFeasible()); }

	bool IsBasicOptimal() const;
  		// But not necessarily feasible.

	bool IsBasicOptimalSolved() const
		{ return (IsSolved() && IsBasicOptimal()); }

	/** See documentation of <tt>QcLinInEqColState::fConstrained</tt>. */ 
	bool IsConstrained(unsigned vi) const
		{ return fColState->IsConstrained(vi); }

	bool IsDesire(unsigned vi) const
		{ return fColState->IsDesire(vi); }

	bool IsDual(unsigned vi) const
		{ return fColState->IsDual(vi); }

	bool IsError(unsigned vi) const
		{ return fColState->IsError(vi); }

  /** Indicates the constraint is solved and has a constrained basic
      var.  But not necessarily optimal.  True iff ci's row condition
      is fRegular, and ci's basic var is constrained.
  **/
  bool IsRestricted(unsigned ci) const;

	bool IsSlack(unsigned vi) const
		{ return fColState->IsSlack(vi); }

	bool IsStructural(unsigned vi) const
		{ return fColState->IsStructural(vi); }

	//-----------------------------------------------------------------------//
	// Manipulation functions.                                               //
	//-----------------------------------------------------------------------//
	virtual int AddArtificial(numT coeff);
		// Adds an artificial variable with given coefficient to all
		// original constraints in the tableau.  Used for complementary pivot.

  /** Identical to addLtEq, except uses a negative slack variable coeff. */
  virtual int AddGtEq(QcRowAdaptor &varCoeffs, numT rhs);

  /** Does the same as AddEq but also introduces a new slack variable so the
      varCoeffs will represent a less-than-or-equal-to constraint.  The slack
      variable is added after any other new regular variables.

      @postcondition ret &ge; 0
  **/
  virtual int AddLtEq(QcRowAdaptor &varCoeffs, numT rhs);

	virtual unsigned NewVariable();
		// Create a new non basic structural variable and return its index.
	virtual int RemoveEq(unsigned ci);
	virtual bool RemoveVarix(unsigned vi);


  /** Adds a restricted dual variable.  Used for complementary pivot.

      @postcondition 0 &le; ret &lt; new <tt>getNColumns()</tt>.

      @return The index of the new variable.
  **/
  unsigned AddDual();


	int AddError();
		// Add an error (slack) variable.
	unsigned AddSlack();
		// Add an error (slack) variable.
	void EliminateObjective();
		// Eliminate basic variable coeffs from objective function.
		// Only constrained parameters are included.  Constant terms have
		// no effect on the objective function so are not included.
#if PROVIDE_LoadArtificial
	int LoadArtificial(const QcLinInEqTableau &other);
		// Restarts the tableau and loads it with copies of the solved form
		// constraints from the other tableau.  Also an artificial variable
		// is introduced and its index is returned.
#endif

	void SetConstrained(unsigned vi, bool c)
		{ fColState->SetConstrained(vi, c); }

	void SetColCondition(unsigned vi, int c)
		{ fColState->SetCondition(vi, c); }

	void SetObjective(QcRowAdaptor &objCoeffs);
		// Loads the objective function coefficients and calls eliminateObj.

	void SetObjValue(unsigned vi, numT obj)
  		{ fColState->SetObjValue(vi, obj); }

	bool SimplexPivot(unsigned ci, unsigned vi);
		// This does a QcLinEqTableau::Pivot but also maintains the objective
		// function coefficients for restricted rows.

	//-----------------------------------------------------------------------//
	// Utility functions.                                                    //
	//-----------------------------------------------------------------------//
	virtual void Print(ostream &os) const
  		{ QcLinEqTableau::Print(os); }

protected:
	//-----------------------------------------------------------------------//
	// Constructors.                                                         //
	//-----------------------------------------------------------------------//
	QcLinInEqTableau(QcBiMapNotifier &n);
};

inline QcLinInEqTableau::QcLinInEqTableau(unsigned hintRows, unsigned hintCols,
					  QcBiMapNotifier &n)
	: QcDelLinEqTableau(*new QcDelCoreTableau(hintRows, hintCols,
						  *new QcLinInEqRowColStateVector()),
			    n),
	  fRowColState((QcLinInEqRowColStateVector *)QcLinEqTableau::fRowColState),
	  fColState((QcLinInEqColStateVector *)&fRowColState->GetColState())
{
}

inline QcLinInEqTableau::QcLinInEqTableau(QcCoreTableau &tab, QcBiMapNotifier &n)
	: QcDelLinEqTableau(tab, n),
	  fRowColState((QcLinInEqRowColStateVector *)QcLinEqTableau::fRowColState),
	  fColState((QcLinInEqColStateVector *)&fRowColState->GetColState())
{
}

inline QcLinInEqTableau::QcLinInEqTableau(QcBiMapNotifier &n)
		: QcDelLinEqTableau(n)
{
}

inline unsigned QcLinInEqTableau::AddDual()
{
	unsigned vi = IncreaseColumns();
	SetColCondition(vi, QcLinInEqColState::fDual);
	SetConstrained(vi, true);
	return vi;
}

inline int QcLinInEqTableau::AddError()
{
	unsigned vi = NewVariable();
	SetColCondition(vi,
    		(QcLinInEqColState::fSlack |
		QcLinInEqColState::fStructural |
		QcLinInEqColState::fError));
	SetConstrained(vi, true);
	return vi;
}

inline numT QcLinInEqTableau::GetRHS(unsigned ci) const
{
  numT rhs = QcLinEqTableau::GetRHS(ci);

  /* [pjm:] TODO: Should this be QcUtility::IsZero? */
  if (QcUtility::IsNearZero(rhs))
    {
#ifndef NDEBUG
      if (!QcUtility::IsZero (rhs))
	{
	  cerr << "DBG: N.B.: found near-zero non-zero RHS.\n";
	  //*(char *)0=0;
	  //return rhs;
	}
#endif
      return 0.0;
    }
  return rhs;
}

inline bool QcLinInEqTableau::IsRestricted(unsigned ci) const
{
	return GetRowCondition(ci) == QcLinEqRowState::fRegular &&
    		IsConstrained(GetBasicVar(ci));
}

inline unsigned QcLinInEqTableau::NewVariable()
{
	unsigned vi = QcLinEqTableau::NewVariable();
	return vi;
}

inline int QcLinInEqTableau::RemoveEq(unsigned ci)
{
	UNUSED(ci);
	throw QcWarning("Error: Should not call QcLinInEqTableau::RemoveEq");
  	return fInvalidConstraintIndex;
}

inline bool QcLinInEqTableau::RemoveVarix(unsigned vi)
{
	UNUSED(vi);
	throw QcWarning("Error: Should not call QcLinInEqTableau::RemoveVarix");
	return false;
}

#endif /* !__QcLinInEqTableauH */
