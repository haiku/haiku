// $Id: QcLinEqSolver.hh,v 1.12 2001/01/30 01:32:08 pmoulder Exp $

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
// This interface needs to be read in conjuction with that of Solver.         //
//============================================================================//

#ifndef __QcEqSolverH
#define __QcEqSolverH

#include "qoca/QcDelLinEqSystem.hh"

class QcLinEqSolver : public QcSolver
{
protected:
	QcDelLinEqSystem fSystem;
	#if defined(qcRealTableauCoeff) || defined(qcRealTableauRHS)
	QcLinEqSystem fSubSystem;
	#else
	QcDelLinEqSystem fSubSystem;
	#endif
	vector<QcFloat> fDesValVar;
	vector<unsigned int> fDepPars;
	vector<unsigned int> fVarsByIndex;
	vector<bool> fDepBasicVars;

  	QcVariableBiMap &fVBiMap;
	const QcBiMapNotifier &fNotifier;
  	QcConstraintBiMap &fOCBiMap;
	QcDelLinEqTableau &fTableau;
  
public:
	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
	QcLinEqSolver();
	QcLinEqSolver(unsigned hintNumConstraints, unsigned hintNumVariables);

#ifndef NDEBUG
  void assertInvar() const { }
  void assertDeepInvar() const;
  virtual void vAssertDeepInvar() const;
#endif
    
	//-----------------------------------------------------------------------//
	// Variable management methods                                           //
	//-----------------------------------------------------------------------//
	virtual void AddVar(QcFloat &v)
  		{ fSystem.AddVar(v); }

	virtual void SuggestValue(QcFloat &v, numT desval)
  		{ fSystem.SuggestValue(v, desval); }

	virtual bool RemoveVar(QcFloat &v)
  		{ return fSystem.RemoveVar(v); }

	virtual void RestSolver()
  		{ fSystem.RestSolver(); }

	//-----------------------------------------------------------------------//
	// Enquiry functions for variables                                       //
	//-----------------------------------------------------------------------//
	virtual bool IsRegistered(QcFloat const &v) const
  		{ return fSystem.IsRegistered(v); }

	virtual bool IsFree(QcFloat const &v) const
  		{ return fSystem.IsFree(v); }

	virtual bool IsBasic(QcFloat const &v) const
  		{ return fSystem.IsBasic(v); }

	bool IsDepBasicVar(unsigned vi) const
  	{
	  qcAssertPre( vi < fDepBasicVars.size());
	  return fDepBasicVars[vi];
	}

	bool IsDepPar(int vi) const;

	//-----------------------------------------------------------------------//
	// High Level Edit Variable Interface for use by solver clients.         //
	//-----------------------------------------------------------------------//
	virtual void BeginEdit();
	virtual void EndEdit();

	//-----------------------------------------------------------------------//
	// Constraint management methods                                         //
	//-----------------------------------------------------------------------//
	virtual bool AddConstraint(QcConstraint &c);
	virtual bool AddConstraint(QcConstraint &c, QcFloat &hint);

	virtual bool ChangeConstraint(QcConstraint &oldc, numT rhs);

	virtual bool RemoveConstraint(QcConstraint &c);

	virtual bool Reset()
     	{ return fSystem.Reset(); }

	//-----------------------------------------------------------------------//
	// Constraint Solving methods                                            //
	//-----------------------------------------------------------------------//
	virtual void Solve();
		// IMPORTANT NOTE.
		// It is is not necessary to call QcLinEqSystem::solve() before
		// creating and solving the subsystem since the subsystem
		// is created without using the values of any variables - just
		// desired values and weights.
	virtual void Resolve();

protected:
	numT Objective();

	//-----------------------------------------------------------------------//
	// Sub-system specific functions.                                        //
	//-----------------------------------------------------------------------//
	void GenerateKT1(QcFloat &p, QcSparseMatrix &ccache
#ifndef qcRealTableauRHS
			 , vector<numT> &crhs
#endif
			);
		// Generate the constraint df'/dp == 0
		// where f' = least squares objective function with basic variables
		// eliminated.  Add the constraint to the subsystem fSubSystem.
		// It is important that this routine does not rely on
		// the values of any of the variables being a solution of LinEq.
		// See note in QcLinEqSolver::Solve.

	void GenerateKT2(QcFloat &p, QcSparseMatrix &ccache
#ifndef qcRealTableauRHS
			 , vector<numT> &crhs
#endif
			);
		// Generate the constraint df'/dp == 0 where parameters of interest
		// have variable desired values and where f' = least squares
		// objective function with basic variables eliminated.
		// Add the constraint to the subsystem fSubSystem.

	void TransitiveClosure();
		// Calculates dependent basic variables and parameters as transitive
		// closure of VOI using the relation "belongs to same constraint".

	//-----------------------------------------------------------------------//
	// Utility functions.                                                    //
	//-----------------------------------------------------------------------//
  	QcConstraint GetConstraint(const char *n)
		{ return fSystem.GetConstraint(n); }

  	QcFloat GetVariable(const char *n)
  		{ return fSystem.GetVariable(n); }

  	void GetVariableSet(vector<QcFloat> &vars)
  		{ fSystem.GetVariableSet(vars); }

	virtual void QcLinEqSolver::Print(ostream &os) const;
};

inline QcLinEqSolver::QcLinEqSolver()
		: QcSolver(),
		fVBiMap(fSystem.GetVBiMap()),
		fNotifier(fSystem.GetNotifier()),
		fOCBiMap(fSystem.GetOCBiMap()),
		fTableau((QcDelLinEqTableau &)fSystem.GetTableau())
{
}

inline QcLinEqSolver::QcLinEqSolver(unsigned hintNumConstraints, unsigned hintNumVariables)
	: QcSolver(),
	  fSystem(hintNumConstraints, hintNumVariables),
	  fSubSystem(hintNumConstraints, hintNumVariables),
	  fVBiMap(fSystem.GetVBiMap()),
	  fNotifier(fSystem.GetNotifier()),
	  fOCBiMap(fSystem.GetOCBiMap()),
	  fTableau((QcDelLinEqTableau &)fSystem.GetTableau())
{
}

inline bool QcLinEqSolver::AddConstraint(QcConstraint &c)
{
	bool success = fSystem.AddConstraint(c);
	if (success) {
#if qcCheckPost
		addCheckedConstraint( c.pointer());
#endif
		if (fAutoSolve)
			Solve();
	}
	return success;
}

inline bool QcLinEqSolver::AddConstraint(QcConstraint &c, QcFloat &hint)
{
	bool success = fSystem.AddConstraint(c, hint);
	if (success) {
#if qcCheckPost
		addCheckedConstraint( c.pointer());
#endif
		if (fAutoSolve)
			Solve();
	}
	return success;
}

inline bool QcLinEqSolver::ChangeConstraint(QcConstraint &oldc, numT rhs)
{
	bool success = fSystem.ChangeConstraint(oldc, rhs);
#if qcCheckPost
	if (success)
		changeCheckedConstraint( oldc.pointer(), rhs);
#endif
	return success;
}

inline bool QcLinEqSolver::RemoveConstraint(QcConstraint &c)
{
	bool success = fSystem.RemoveConstraint(c);
	if (success) {
#if qcCheckPost
		removeCheckedConstraint( c.pointer());
#endif
		if (fAutoSolve)
			Solve();
	}
	return success;
}

#endif
