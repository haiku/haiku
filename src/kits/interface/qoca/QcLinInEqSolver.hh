// $Id: QcLinInEqSolver.hh,v 1.17 2001/01/04 05:20:11 pmoulder Exp $

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
// This interface needs to be read in conjuction with that of Solver.         //
//============================================================================//

#ifndef __QcInEqSolverH
#define __QcInEqSolverH

#include "qoca/QcDelLinInEqSystem.hh"
#include "qoca/QcCompPivotSystem.hh"
#include "qoca/QcIneqSolverBase.H"

class QcLinInEqSolver
  : public QcIneqSolverBase
{
public:
	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
	QcLinInEqSolver();
	QcLinInEqSolver(unsigned hintNumConstraints, unsigned hintNumVariables);

#ifndef NDEBUG
	void assertInvar() const
	{
	  assert(    (fCacheCoeff.getNRows() == 0)
		  || (fCacheCoeff.getNRows() == fTableau.getNRows()));
	  assert(    (fCacheCoeff.getNColumns() == 0)
		  || (fCacheCoeff.getNColumns() == fTableau.getNColumns()));

	  unsigned pvs = fDepParVars.size();
	  assert( pvs == fDepBasicVars.size());
	  /* The following may be wrong: I haven't checked that fDep*Vars get
	     cleared after use.  Note that they are set up in BeginEdit, and
	     still used by EndEdit.  I think this will break if someone calls
	     AddVar between BeginEdit and EndEdit.  fixme: Look at the AddVar
	     case.  Also consider having all users of fDep*Vars assert(
	     fEditVarsSetup).  [Can't do this yet: fEditVarsSetup is set to true
	     only at end of BeginEdit.]  And see what happens with
	     AddConstraint: it looks as if it can go badly: e.g. it doesn't seem
	     to clear fEditVarsSetup. */
	  assert( pvs == 0 || pvs == fTableau.getNColumns());
	}
#endif

  bool IsDepBasicVar(unsigned vi) const
  {
    qcAssertPre( vi < fDepBasicVars.size());
    return fDepBasicVars[vi];
  }

  bool IsDepVar(unsigned vi) const
  {
    qcAssertPre( (vi < fDepParVars.size())
		 && (vi < fDepBasicVars.size()));
    return fDepParVars[vi] || fDepBasicVars[vi];
  }

	//-----------------------------------------------------------------------//
	// High Level Edit Variable Interface for use by solver clients.         //
	//-----------------------------------------------------------------------//
	virtual void BeginEdit();
	virtual void EndEdit();

	//-----------------------------------------------------------------------//
	// Constraint Solving methods                                            //
	//-----------------------------------------------------------------------//
	virtual void Solve();
		// IMPORTANT NOTE.
		// It is not necessary to call fSystem.solve() before creating and
		// solving the subsystem since the subsystem is created without using
		// the values of any variables - just desired values and weights.
	virtual void Resolve();

protected:
  /** Current value of objective function. */
  numT Objective();

	//-----------------------------------------------------------------------//
	// Sub-system specific functions.                                        //
	//-----------------------------------------------------------------------//
	void AddDualCoeffs(QcLinPoly &lpoly, int p);
		// Used by Generate constraints to add the dual variable
		// coefficients to a linpoly corresponding to parameter p.
		// Assumes the dual variables have already been allocated.
	void CacheTableau();
	void CreateDesValVars();
	void CreateDualVars();
		// Create complementary variables for each variable in
		// the primal tableau.  The fDual array is used to locate these
		// variables.  N.B. Not all these variables need be used.
		// The rest of fDual is initialised in AddDualCoeffs.

	numT Derivative1(QcFloat &p, int pIndex, QcLinPoly &lpoly);
	numT Derivative2(QcFloat &p, int pIndex, QcLinPoly &lpoly);
	numT Derivative(QcFloat &p, int pIndex, QcLinPoly &lpoly, bool phaseII);
		// Generate the expression (lpoly,rhs): df'/dp + dual(p).
		// where f' = least squares objective function with basic variables
		// eliminated and dual(p) is the column coeffs of the original
		// tableau for p applied to new dual variables.
		// The variable coefficients are accumulated in lpoly, the rhs value
		// is returned.

	void GenerateKT1();
		// Generate the constraint df'/dp + dual(p) == 0 for each unconstrained
		// primal parameter p.  Then generate the constraint
		// df'/dp + dual(p) >= 0 for each constrained (i.e. slack) primal
		// parameter.  Add the new constraint directly to the subsystem.
		// Where f' = least squares objective function with basic variables
		// eliminated, and dual(p) is the column coeffs of the original
		// tableau for p applied to new dual variables.
	void GenerateKT2();
		// Generate the constraint df'/dp + dual(p) == 0 for each unconstrained
		// primal parameter p.  Then generate the constraint
		// df'/dp + dual(p) >= 0 for each constrained (i.e. slack) primal
		// parameter.  Add the new constraint directly to the subsystem.
		// Where f' = least squares objective function with basic variables
		// eliminated, and dual(p) is the column coeffs of the original
		// tableau for p applied to new dual variables.
	void LoadDesVarCache();
  		// Required for desval variables.
	void MakeFeasible();
		// Make the subsystem tableau feasible
	void MakeFeasibleComplex();
		// Make the subsystem tableau feasible
	void PrepareVariables();
	void SubSysEqSolve();

public:
	virtual void Print(ostream &os) const;
	virtual void Restart();
	void TransitiveClosure();
		// Calculate fDepBasicVars and fDepPars as transitive closure of
		// variables in constraints which mention EditVars.

private:
  class DualHolder
  {
  public:

    DualHolder()
      : fDual(),
	fDualIndex()
    { }

  private:
    DualHolder( DualHolder const &tmpl);

#ifndef NDEBUG
    void assertInvar() const
      {
	assert( fDual.size() == fDualIndex.size());
      }
#endif

  public:
    ~DualHolder()
      { }

    unsigned size() const
      { return fDual.size(); }

    void clear()
      {
	fDual.clear();
	fDualIndex.clear();
      }

    void push_back( QcNullableFloat &var, unsigned vi)
      {
	qcAssertPre( var.pointer() != 0);
	fDual.push_back( var);
	fDualIndex.push_back( vi);
	dbg(assertInvar());
      }

    void push_back( QcFloatRep *var, unsigned vi)
      {
	qcAssertPre( var != 0);
	fDual.push_back( QcNullableFloat( var));
	fDualIndex.push_back( vi);
	dbg(assertInvar());
      }

    QcFloatRep *varPtr( unsigned ix) const
      {
	qcAssertPre( ix < fDual.size());
	QcFloatRep *ret = fDual[ix].pointer();
	qcAssertPost( ret != 0);
	return ret;
      }

    unsigned varIx( unsigned ix) const
      {
	qcAssertPre( ix < fDualIndex.size());
	return fDualIndex[ix];
      }

    void setIndex( unsigned ix, unsigned varIx)
      {
	qcAssertPre( ix < fDualIndex.size());
	fDualIndex[ix] = varIx;
      }

  private:
    vector<QcNullableFloat> fDual;			// The complementary variables.
    vector<unsigned int> fDualIndex;	// The complementary variable indices.
  };


protected:
  /** Each variable in the dual problem (<tt>fSubSystem</tt>) has a
      complementary variable.  This correspondance is that for all j:

      <ul>
      <li>If variable j in the primal problem is a parameter, then, there is a
      corresponding dual constraint, and <tt>fDuals.var(j)</tt> is its slack
      variable.

      <li>Otherwise (i.e. if variable j in the primal problem is a basic
      variable), there is a corresponding dual variable <tt>fDuals.var(j)</tt>
      (as used in <tt>AddDualCoeffs</tt>).

      </ul>

      <p>The total number of variables in the dual problem is hence (primal
      parametric vars + 2 &times; primal basic vars + artificial var).  The artificial
      variable is entered as fDual[fAI], though this entry is not used anywhere
      except for debug state output.

      <p>For each i, <tt>fDuals.varIx(i)</tt> provides the index of
      <tt>fDuals.var(i)</tt> in <tt>fSubSystem</tt>.
  **/
  DualHolder fDuals;

  /** Artificial variable used for complementary pivot. */
  QcFloat fAV;

  /** Index of artificial variable in fSubSystem. */
  int fAI;

  vector<unsigned int> fDepPars;
  vector<unsigned int> fVarsByIndex;
  vector<bool> fDepBasicVars;
  vector<bool> fDepParVars;
  vector<QcFloat> fDesValVar;
  vector<int> fDesValIndex;

  QcVariableBiMap &fVBiMap;
  const QcBiMapNotifier &fNotifier;
  QcConstraintBiMap &fOCBiMap;
  QcDelLinInEqTableau &fTableau;

  QcCompPivotSystem fSubSystem;
  QcCompPivotTableau &fSubTableau;

  QcVariableBiMap &fSubVBiMap;
  const QcBiMapNotifier &fSubNotifier;
  QcConstraintBiMap &fSubOCBiMap;

private:
  QcSparseMatrix fCacheCoeff;

};


inline QcLinInEqSolver::QcLinInEqSolver()
	: QcIneqSolverBase(),
	  fAV("Artificial Var"),
	  fAI(QcTableau::fInvalidVariableIndex),
	  fVBiMap(fSystem.GetVBiMap()),
	  fNotifier(fSystem.GetNotifier()),
	  fOCBiMap(fSystem.GetOCBiMap()),
	  fTableau((QcDelLinInEqTableau &)fSystem.GetTableau()),
	  fSubTableau((QcCompPivotTableau &)fSubSystem.GetTableau()),
	  fSubVBiMap(fSubSystem.GetVBiMap()),
	  fSubNotifier(fSubSystem.GetNotifier()),
	  fSubOCBiMap(fSubSystem.GetOCBiMap()),
	  fCacheCoeff()
{
}

inline QcLinInEqSolver::QcLinInEqSolver(unsigned hintNumConstraints,
					unsigned hintNumVariables)
	: QcIneqSolverBase( hintNumConstraints, hintNumVariables),
	  fAV("Artificial Var"),
	  fAI(QcTableau::fInvalidVariableIndex),
	  fVBiMap(fSystem.GetVBiMap()),
	  fNotifier(fSystem.GetNotifier()),
	  fOCBiMap(fSystem.GetOCBiMap()),
	  fTableau((QcDelLinInEqTableau &)fSystem.GetTableau()),
	  fSubSystem(hintNumConstraints, hintNumVariables),
	  fSubTableau((QcCompPivotTableau &)fSubSystem.GetTableau()),
	  fSubVBiMap(fSubSystem.GetVBiMap()),
	  fSubNotifier(fSubSystem.GetNotifier()),
	  fSubOCBiMap(fSubSystem.GetOCBiMap()),
	  fCacheCoeff()
{
}

inline void QcLinInEqSolver::Restart()
{
	fSystem.Restart();
	fSubSystem.Restart();
}

#endif

/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/
