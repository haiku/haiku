// $Id: QcLinEqSolver.cc,v 1.16 2001/01/30 01:32:08 pmoulder Exp $

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

#include "qoca/QcDefines.hh"
#include "qoca/QcDesireValueStore.hh"
#include "qoca/QcLinEqSolver.hh"
#include "qoca/QcParamVarIndexIterator.hh"
#include "qoca/QcTableauColIterator.hh"
#include "qoca/QcBasicVarIndexIterator.hh"
#include "qoca/QcSparseMatrixRowIterator.hh"
#include "qoca/QcVariableIndexIterator.hh"

#ifndef NDEBUG
void QcLinEqSolver::assertDeepInvar() const
{
  QcSolver::assertDeepInvar();
  fSystem.vAssertDeepInvar();
}

void QcLinEqSolver::vAssertDeepInvar() const
{
  assertDeepInvar();
}
#endif

void QcLinEqSolver::BeginEdit()
{
	vector<QcFloat>::iterator voiIt;
	vector<unsigned int>::iterator viIt;

	// build a version of fEditVars by index.
	fVarsByIndex.resize(0);
	for (voiIt = fEditVars.begin(); voiIt != fEditVars.end(); ++voiIt)
		fVarsByIndex.push_back(fVBiMap.Index(*voiIt));

	fSubSystem.Restart();
	TransitiveClosure();		// Calculates fDepPars and fDepBasicVars

	// Create a variable for the desired value of each EditVars.
	// Also set fSubSystem EditVars to this->EditVars.
	fDesValVar.resize( fTableau.getNColumns());
	for (unsigned i = 0; i < fVarsByIndex.size(); i++)
	  {
	    unsigned vi = fVarsByIndex[i];
	    assert( vi < fTableau.getNColumns());
	    fSubSystem.RegisterEditVar( fDesValVar[vi] = QcFloat( "desired value"));
	  }

	// Make a copy of a matrix of coefficients to speed up GenerateKT1.
 	QcSparseMatrix ccache;
	ccache.Resize(fTableau.GetRows(), fTableau.GetColumns());
	QcParamVarIndexIterator vIt(fTableau);

	while (!vIt.AtEnd()) {
		QcTableauColIterator colCoeffs (fTableau, vIt.getIndex());
   		while (!colCoeffs.AtEnd()) {
			ccache.SetValue(colCoeffs.getIndex(), vIt.getIndex(), colCoeffs.getValue());
			colCoeffs.Increment();
		}
		vIt.Increment();
	}

	#ifndef qcRealTableauRHS
	vector<numT> crhs;
	crhs.Resize(fTableau.GetRows());
	for (int r = 0; r < fTableau.GetRows(); r++)
		crhs[r] = fTableau.GetRHS(r);
	#endif

	// Generate subsystem KT constraint for each dependent parameter
	// Also set fSystem EditVars to fDepPars.
	fSystem.ClearEditVars();
	for (viIt = fDepPars.begin(); viIt != fDepPars.end(); ++viIt) {
		QcFloat &v = fVBiMap.Identifier(*viIt);
		// N.B. constraint may be needed even when v is free
		GenerateKT2(v, ccache
#ifndef qcRealTableauRHS
			    , crhs
#endif
			);
		fSystem.RegisterEditVar(v);
	}

	fSubSystem.BeginEdit();
	fEditVarsSetup = true;
	dbg(assertInvar());
}

void QcLinEqSolver::EndEdit()
{
	vector<unsigned int>::const_iterator vIt;

	for (unsigned int vi = 0; vi < fDepBasicVars.size(); ++vi)
		if (fDepBasicVars[vi])
			fVBiMap.Identifier(vi).RestVariable();

	for (vIt = fDepPars.begin(); vIt != fDepPars.end(); ++vIt)
		fVBiMap.Identifier(*vIt).RestVariable();

	QcSolver::EndEdit();
	dbg(assertInvar());
}

void QcLinEqSolver::GenerateKT1(QcFloat &p, QcSparseMatrix &ccache
#ifndef qcRealTableauRHS
				, vector<numT> &crhs
#endif
)
	// Generate the constraint df'/dp == 0
	// where f' = least squares objective function with basic variables
	// eliminated.  Add the constraint to the subsystem fSubSystem.
	// It is important that this routine does not rely on
	// the values of any of the variables being a solution of LinEq.
	// See note in QcLinEqSolver::Solve.
{
#ifndef qcRealTableauRHS
	qcAssertPre (crhs.size() == ccache.getNRows());
#endif

	QcLinPoly lpoly;		// N.B. lpoly is destroyed when this routine returns
	numT rhs = 0.0;
	int pIndex = fVBiMap.Index(p);
	numT pWeight2 = p.Weight() * 2;
	numT pAccumulator = pWeight2;

	QcBasicVarIndexIterator bvIt(fTableau);

	while (!bvIt.AtEnd()) {
		QcFloat &bv = fVBiMap.Identifier(bvIt.getIndex());
		unsigned c = bvIt.getConstraintBasicIn();	// use constraint c to eliminate bv
		assert (c < ccache.getNRows());
	 	numT pCoeff = ccache.GetValue(c, pIndex);
		numT factor = bv.Weight() * pCoeff * 2;
		pAccumulator += factor * pCoeff;

		QcSparseMatrixRowIterator varCoeffs(ccache, c);
		while (!varCoeffs.AtEnd()) {
			QcFloat &pv = fVBiMap.Identifier(varCoeffs.GetIndex());
			int pvIndex = varCoeffs.GetIndex();

			if (pvIndex!=pIndex)
				lpoly += QcLinPolyTerm(factor * varCoeffs.GetValue(), pv);

			varCoeffs.Increment();
		}
		#ifdef qcRealTableauRHS
		rhs += factor * (fTableau.GetRHS(c) - bv.DesireValue());
		#else
		rhs += factor * (crhs[c] - bv.GetDesValue());
		#endif
		bvIt.Increment();
	}

	lpoly.Push(pAccumulator, p);
	rhs += pWeight2 * p.DesireValue();

	QcConstraint kt("", lpoly, QcConstraintRep::coEQ, rhs);

	#ifdef qcSafetyChecks
	if (!fSubSystem.AddConstraint(kt))
		throw QcWarning("QcLinEqSolver::GenerateKT1: subsystem inconsistant");
	#else
	fSubSystem.AddConstraint(kt);
	#endif
}

void QcLinEqSolver::GenerateKT2(QcFloat &p, QcSparseMatrix &ccache
#ifndef qcRealTableauRHS
				, vector<numT> &crhs
#endif
				)
	// Generate the constraint df'/dp == 0 where parameters of interest
	// have variable desired values and where f' = least squares
	// objective function with basic variables eliminated.
	// Add the constraint to the subsystem fSubSystem.
{
#ifndef qcRealTableauRHS
	qcAssertPre (crhs.size() == ccache.getNRows());
#endif

	QcLinPoly lpoly;	// N.B. lpoly is destroyed when this routine returns
	numT rhs = 0.0;
	int pIndex = fVBiMap.Index(p);
	numT pWeight2 = p.Weight() * 2;
	numT pAccumulator = pWeight2;
	QcFloat &pDV = fDesValVar[pIndex];

	QcBasicVarIndexIterator bvIt(fTableau);

	while (!bvIt.AtEnd()) {
		unsigned bvi = bvIt.getIndex();
		QcFloat &bv = fVBiMap.Identifier(bvi);
		unsigned c = bvIt.getConstraintBasicIn();	// use constraint c to eliminate bv
		assert (c < ccache.getNRows());
		numT pCoeff = ccache.GetValue(c, pIndex);
		numT factor = bv.Weight() * pCoeff * 2;
		pAccumulator += factor * pCoeff;

	 	QcSparseMatrixRowIterator varCoeffs(ccache, c);
		while (!varCoeffs.AtEnd()) {
			QcFloat &pv = fVBiMap.Identifier(varCoeffs.GetIndex());
			int pvIndex = varCoeffs.GetIndex();

			if (pvIndex!=pIndex)
				lpoly += QcLinPolyTerm(factor * varCoeffs.GetValue(), pv);

			varCoeffs.Increment();
		}
		if (IsDepBasicVar(bvi) && IsEditVar(bv)) {
			#ifdef qcRealTableauRHS
			rhs += factor * fTableau.GetRHS(c);
			#else
			rhs += factor * crhs[c];
			#endif
			lpoly += QcLinPolyTerm(factor, fDesValVar[bvi]);
		} else {
			#ifdef qcRealTableauRHS
			rhs += factor * (fTableau.GetRHS(c) - bv.DesireValue());
			#else
			rhs += factor * (crhs[c] - bv.GetDesValue());
			#endif
		}
		bvIt.Increment();
	}

	lpoly.Push(pAccumulator, p);

	if (IsEditVar(p))
		lpoly.Push(-pWeight2, pDV);
	else
		rhs += pWeight2 * p.DesireValue();

	QcConstraint kt("", lpoly, QcConstraintRep::coEQ, rhs);

	#ifdef qcSafetyChecks
	if (!fSubSystem.AddConstraint(kt))
		throw QcWarning("QcLinEqSolver::GenerateKT2: subsystem inconsistant");
	#else
	fSubSystem.AddConstraint(kt);
	#endif
}

bool QcLinEqSolver::IsDepPar(int vi) const
{
	if (vi < 0) /* TODO: We can probably assertPre (vi > 0).  Similarly for other IsDepPar implementations. */
		return false;

	vector<unsigned int>::const_iterator it;
	for (it = fDepPars.begin(); it != fDepPars.end(); ++it)
		if ((*it) == (unsigned) vi)
			return true;
	return false;
}

numT QcLinEqSolver::Objective()
{
	numT opt = 0.0;
	QcVariableIndexIterator vIt(fTableau);

	while (!vIt.AtEnd()) {
		QcFloat &v = fVBiMap.Identifier(vIt.getIndex());
		numT delta = v.Value() - v.DesireValue();
		opt += v.Weight() * delta * delta;
		vIt.Increment();
	}

	return opt;
}

void QcLinEqSolver::Resolve()
{
	if (!fEditVarsSetup)
		BeginEdit();

	// Setup the desired value variables.
	assert( fVarsByIndex.size() == fEditVars.size());
	for (unsigned i = 0; i < fVarsByIndex.size(); i++)
	  {
	    unsigned vi = fVarsByIndex[i];
	    assert( vi < fDesValVar.size());
	    fDesValVar[vi].SuggestValue( fEditVars[i].DesireValue());
	  }

	fSubSystem.Resolve();

	/* effic: The handling of saveDesireValue (both here and in Solve) could
	   be improved: we do a lot of unnecessary construction and
	   deconstruction. */
	vector<QcDesireValueStore> saveDesireValue;

	// The dependent variables have their desired values altered
	// according to the value found by the subsystem solve.
	for (vector<unsigned int>::const_iterator vIt = fDepPars.begin();
	     vIt != fDepPars.end();
	     ++vIt)
	  {
	    QcFloatRep *vr = fVBiMap.getIdentifierPtr( *vIt);
	    saveDesireValue.push_back( QcDesireValueStore( vr));
	    if (!vr->RestDesVal_changed())
	      saveDesireValue.pop_back();
	  }

	fSystem.Resolve();

	// Restore the altered desired values.
	for (vector<QcDesireValueStore>::iterator irdv = saveDesireValue.begin(), irdvEnd = saveDesireValue.end();
	     irdv != irdvEnd;
	     irdv++)
	  irdv->Restore();

#if qcCheckPost
	checkSatisfied();
	dbg(assertInvar());
#endif
}

void QcLinEqSolver::Solve()
{
	// Make a copy of a matrix of coefficients to speed up GenerateKT1.
 	QcSparseMatrix ccache;
	ccache.Resize(fTableau.GetRows(), fTableau.GetColumns());
	QcParamVarIndexIterator vIt(fTableau);

	while (!vIt.AtEnd()) {
		QcTableauColIterator colCoeffs (fTableau, vIt.getIndex());
		while (!colCoeffs.AtEnd()) {
			ccache.SetValue (colCoeffs.getIndex(), vIt.getIndex(), colCoeffs.getValue());
			colCoeffs.Increment();
		}
		vIt.Increment();
	}

	#ifndef qcRealTableauRHS
	vector<numT> crhs;
	crhs.Resize(tableau.GetRows());
	for (int r = 0; r < fTableau.GetRows(); r++)
		crhs[r] = fTableau.GetRHS(r);
	#endif

	// Generate a subsystem constraint for each parametric variable.
	fSubSystem.Restart();
	vIt.Reset();

	while (!vIt.AtEnd()) {
		unsigned vi = vIt.getIndex();
		// The test avoids entering a redundant constraint in
		// the subsystem to save time.  Also, there is no need to
		// call v.Set when the variable is free since this will be
		// done when LinEq::Solve() is called later.
		if (!fTableau.IsFree(vi))
			GenerateKT1(fVBiMap.Identifier(vi), ccache
#ifndef qcRealTableauRHS
				    , crhs
#endif
				);
		vIt.Increment();
	}

	fSubSystem.Solve();

	vector<QcDesireValueStore> saveDesireValue;

	// At this point we have final values for the non-free parametric
	// variables.  Set the desired values and solve the original system
	// again to get the values for the basic variables.
	// Make a record of the altered desired values.
	for (QcBasicVarIndexIterator bvIt( fSubSystem.GetTableau());
	     !bvIt.AtEnd();
	     bvIt.Increment())
	  {
	    unsigned ix = bvIt.getIndex();
	    assert( fSubSystem.GetTableau().IsBasic( ix));
	    QcFloatRep *vr = fSubSystem.GetVBiMap().getIdentifierPtr( ix);
	    saveDesireValue.push_back( QcDesireValueStore( vr));
	    if (!vr->RestDesVal_changed())
	      saveDesireValue.pop_back();
	  }

	fSystem.Solve();

	// Restore the altered desired values
	for (vector<QcDesireValueStore>::iterator irdv = saveDesireValue.begin(), irdvEnd = saveDesireValue.end();
	     irdv != irdvEnd;
	     irdv++)
	  irdv->Restore();

	// And now the variable values provide the solution.

#if qcCheckPost
	checkSatisfied();
#endif
}

void QcLinEqSolver::TransitiveClosure()
{
  vector<unsigned int> depVars(fVarsByIndex);
  fSystem.TransitiveClosure(depVars);

  // Sift depVars into fDepPars and fDepBasicVars.
  fDepBasicVars.resize( fTableau.getNColumns());

  for (unsigned int i = 0; i < fDepBasicVars.size(); i++)
    fDepBasicVars[i] = false;

  fDepPars.resize(0);

  for (unsigned i = 0; i < depVars.size(); i++)
    {
      unsigned vi = depVars[i];

      if (fTableau.IsBasic( vi))
	fDepBasicVars[vi] = true;
      else
	fDepPars.push_back( vi);
    }
}

void QcLinEqSolver::Print(ostream &os) const
{
	fSystem.Print(os);

	if (fEditVarsSetup) {
		os << "Subsystem:" << endl;
		fSubSystem.Print(os);
		os << endl << "fDepBasicVars is:";

		for (unsigned int i = 0; i < fDepBasicVars.size(); i++)
			if (fDepBasicVars[i])
				os << i << ", ";

		os << endl << "fDepPars is:";

		for (unsigned int i = 0; i < fDepPars.size(); i++)
			os << fDepPars[i] << ", ";
	}
}
