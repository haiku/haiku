// $Id: QcLinInEqSystem.hh,v 1.9 2000/12/15 07:06:20 pmoulder Exp $

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

#ifndef __QcInEqSystemH
#define __QcInEqSystemH

#include "qoca/QcDelLinEqSystem.hh"
#include "qoca/QcLinInEqTableau.hh"

class QcLinInEqSystem : public QcDelLinEqSystem
{
public:
	QcLinInEqTableau &fTableau;

	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
	QcLinInEqSystem();
	QcLinInEqSystem(unsigned hintRows, unsigned hintCols);
	QcLinInEqSystem(QcLinInEqTableau &tab);

	//-----------------------------------------------------------------------//
	// Variable management methods (see QcSolver.java for descriptions)      //
	//-----------------------------------------------------------------------//
	virtual bool RemoveVar(QcFloat &v);
	virtual void RestSolver();

	//-----------------------------------------------------------------------//
	// High Level Edit Variable Interface for use by solver clients.         //
	//-----------------------------------------------------------------------//
	virtual void BeginEdit();

	//-----------------------------------------------------------------------//
	// Constraint management methods                                         //
	//-----------------------------------------------------------------------//
	virtual bool AddConstraint(QcConstraint &c);
		// Extracts the various components and enters them in the tableau
		// as well as updating the bimapnotifier for the tableau
		// if successful.  The return value indicates success or failure.
		// If AddConstraint fails the state of the tableau is unchanged.
	virtual bool AddConstraint(QcConstraint &c, QcFloat &hint);
	virtual bool AddInEq(QcConstraint &c);
		// Used for adding inequalities only.  Always pivots on the new slack
		// variable (hence making it basic).
	virtual bool ApplyHints(int cs);
		// This version of applyHints is similar to QcLinEqSystem::applyHints
		// only it is aware of slack variables and calls SimplexI if none of
		// the slack variables can be made basic with a single pivot.
		// This is a convenience function to pivot an independent new
		// solved form constraint according to the PivotHints.
	virtual void BeginAddConstraint();
		// Marks the beginning of a batch of AddConstraint operation.
	virtual bool EndAddConstraint();
		// Marks the end of a batch of AddConstraint operation.
	virtual bool Reset();
		// While this routine is in progress the tableau is slowly
		// rebuilt from row 0 forwards retaining the original constraint
		// matrix fA.
		// The implementation used here only relies on the rest
		// of the tableau methods being able to cope with multiple
		// undetermined constraints.
		// Reset starts by calling Restore to establish a state equivalent
		// to the original constraints having been all entered using AddEq
		// and nothing else apart from the deleted entries being left alone.
	virtual bool RemoveConstraint(QcConstraint &c);

	//-----------------------------------------------------------------------//
	// Constraint Solving methods                                            //
	//-----------------------------------------------------------------------//
	virtual void EqSolve();
  		// Equivalent to QcLinEqSystem::Solve
	virtual void Resolve();
		// This is the same as QcLinEqSystem::resolve.
	virtual void Solve();

protected:
	vector<unsigned int> fArtVars;
	
	virtual int AddToTableau(QcConstraint &c);
		// Does first part of addConstraint, calling the correct tableau
		// function.
	virtual void InitObjective();
		// Build objective function SIGMA(x.weight * x) for structural
		// variables x.  Replace basic variables by their parametric
		// representation but don't include any non-slack parameters.
	virtual void RawSolve();
		// Evaluate basic Variables from parameters

	//-----------------------------------------------------------------------//
	// Advanced methods.                                                     //
	//-----------------------------------------------------------------------//
public:
	virtual bool DualSimplexII();
		// As for SimplexII only this algorithm uses the dual algorithm and
		// needs an initially optimal (but infeasible) solution.
		// Maintains optimality while searching for a feasible solution.
	virtual int SelectEntryVar();
		// Returns parameter for SimplexII to make basic.
		// Or returns invalid index if there are no candidates.
		// Any constrained parameter with negative objective coefficient will
		// do.  But to implement Bland's anti-cycling rule, we choose the
		// parameter with the lowest index.


	/** Select the pivot row for pivot variable <tt>vi</tt> (slack parameter)
	    maintaining Basic Feasible Solved Form.
	    Returns <tt>fInvalidConstraintIndex</tt> if no such pivot is possible.
	    This version implement's Bland's anticycling rule.
	    Used by <tt>SimplexII</tt>, <tt>BeginEdit</tt> and in derived classes.

	    @precondition <tt>vi &lt; fTableau.getNColumns()</tt>
	**/
	virtual int SelectExitVar (unsigned vi);

	virtual int SelectDualEntryVar(int ei);
		// Select a slack parameter to enter the basis which will approach
		// feasibility. Used by DualSimplexII.
	virtual int SelectDualExitVar();
		// Select the basic variable to leave the basis so that optimality
		// is maintained (in the narrow sense - feasibility is not assumed).
		// And feasibility is approached.
		// Returns InvalidCIndex if no such pivot is possible (i.e. optimal
		// feasible).
	virtual void SetObjective(QcLinPoly &obj);
  		// Simplex objective minimise function

	/** Uses the first phase of the revised simplex algorithm to obtain a
	    basic feasible solution for the new constraint with index
	    <tt>cs</tt>.  Returns false only if the problem is not satisfiable
	    (or if it does an "unreasonable" number of iterations,
	    viz.&nbsp;<tt>qcMaxLoopCycles</tt>).  <tt>SimplexI</tt> tries to
	    find a basic feasible solution when adding a new row to a tableau
	    previously in basic feasible form.  It has not been possible to find
	    an unconstrained variable to make basic.  Hence a restricted row is
	    being created.  The slackCoeffs vector parameter does not include
	    the new slack variable, this has already been tried in Applyhints.
	    A precondition for this call is that the tableau is basic feasible
	    apart from the new row <tt>cs</tt> which is being added.

	    @precondition <tt>cs &lt; fTableau.getNRows()</tt>
	**/
	virtual bool SimplexI (unsigned cs);

	virtual bool SimplexII();
		// Uses the second phase of the revised simplex algorithm to
		// find a solution minimising the objecive function.
		// Perform simplex optimisation on the restricted rows of the tableau.
		// Should only be called if there are some restricted rows.
		// The objective function should be initialised before this call.
		// Bland's anti-cycling rule is implemented and depends upon tableau
		// column iterators providing coeffs in order of increasing
		// variable index.

protected:
	virtual void LoadDesValCache();
		// Used for Solve

	bool CannotIncrease(numT coeff, numT rhs);
		// Used by SimplexI and applied to rhs = QcLinTableau::getRHS.
		// The sign of coeff and rhs is critical hence the extra check.

private:
	bool doAddConstraint(QcConstraint &c, int hintIx);
};

inline QcLinInEqSystem::QcLinInEqSystem()
	: QcDelLinEqSystem(*new QcLinInEqTableau(0, 0, *new QcBiMapNotifier())),
	  fTableau((QcLinInEqTableau &)QcDelLinEqSystem::fTableau)
{
}

inline QcLinInEqSystem::QcLinInEqSystem(unsigned hintRows, unsigned hintCols)
	: QcDelLinEqSystem(*new QcLinInEqTableau(hintRows, hintCols,
						 *new QcBiMapNotifier())),
	  fTableau((QcLinInEqTableau &)QcDelLinEqSystem::fTableau)
{
}

inline QcLinInEqSystem::QcLinInEqSystem(QcLinInEqTableau &tab)
	: QcDelLinEqSystem(tab),
	  fTableau(tab)
{
}

inline void QcLinInEqSystem::BeginAddConstraint()
{
	QcSolver::BeginAddConstraint();
	fArtVars.clear();
}

inline bool QcLinInEqSystem::CannotIncrease(numT coeff, numT rhs)
	// Used by SimplexI and applied to rhs = LinTableau::GetRHS.
	// The sign of coeff and rhs is critical hence the extra check
{
	qcAssert(!QcUtility::IsZero(coeff));

	if (QcUtility::IsNearZero(coeff))
		return false;

	if (rhs < 0.0)
		return (coeff < 0.0);
		// rhs is zeroised already, otherwise use `if qcIsNeg(rhs)'
	else
		return (coeff > 0.0);
}

inline bool QcLinInEqSystem::RemoveVar(QcFloat &v)
{
	UNUSED(v);
	throw QcWarning("Error: Should not call QcLinInEqSystem::RemoveVar");
	return false;
}

inline bool QcLinInEqSystem::RemoveConstraint(QcConstraint &c)
{
	UNUSED(c);
	throw QcWarning("Error: Should not call QcLinInEqSystem::removeConstraint");
	return false;
}

inline void QcLinInEqSystem::SetObjective(QcLinPoly &obj)
{
	QcRowAdaptor objIt(obj, fVBiMap, fTableau);
	fTableau.SetObjective(objIt);
}

#endif /* !__QcInEqSystemH */
