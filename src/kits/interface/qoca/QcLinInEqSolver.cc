// $Id: QcLinInEqSolver.cc,v 1.24 2000/12/15 01:20:05 pmoulder Exp $

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
#include "qoca/QcLinInEqSolver.hh"
#include "qoca/QcTableauRowIterator.hh"
#include "qoca/QcTableauColIterator.hh"
#include "qoca/QcBasicVarIndexIterator.hh"
#include "qoca/QcParamVarIndexIterator.hh"
#include "qoca/QcVariableIndexIterator.hh"
#include "qoca/QcStructVarIndexIterator.hh"
#include "qoca/QcSparseMatrixRowIterator.hh"

void QcLinInEqSolver::AddDualCoeffs(QcLinPoly &lpoly, int p)
	// Used by Generate constraints to add the dual variable
	// coefficients to a linpoly corresponding to parameter p.
	// Assumes the dual variables have already been allocated.
{
  for (QcTableauColIterator varCoeffs( fTableau, p);
       !varCoeffs.AtEnd();
       varCoeffs.Increment())
    {
      if (!fTableau.IsRestricted( varCoeffs.getIndex()))
	continue;

      int cvi = fTableau.GetBasicVar (varCoeffs.getIndex());
      assert ((unsigned) cvi < fDuals.size());
      QcFloatRep *dualVar = fDuals.varPtr( cvi);

      // Register the new dual index
      if (!fSubVBiMap.IdentifierPtrPresent( dualVar))
	{
	  unsigned dualIndex = fSubTableau.AddDual();
	  fSubVBiMap.Update( dualVar, dualIndex);
	  QcFloatRep *primeVar = fSubVBiMap.getIdentifierPtr( cvi);
	  fDuals.push_back( primeVar, cvi);
		// This code is far from ideal for a number of reasons:
		// 1) It uses knowledge about how LinSimp is implemented -
		// specifically how the index for the new dual is allocated
		// on the fly.
		// 2) Looking up the handle for the corresponding variable
		// via the variable bimap seems a bit heavy handed.
		//
		// This will be rethought when the interfaces are revamped.
	}
      lpoly.add( varCoeffs.getValue(), dualVar);
    }

  dbg(assertInvar());
}


void QcLinInEqSolver::BeginEdit()
{
  fAI = QcTableau::fInvalidVariableIndex;
  fSubSystem.Restart();

  // Let fVarsByIndex be a version of fEditVars by index.
  {
    fVarsByIndex.resize(0);
    for (vector<QcFloat>::iterator evi = fEditVars.begin(), eviEnd = fEditVars.end();
	 evi != eviEnd;
	 ++evi)
      fVarsByIndex.push_back( fVBiMap.Index( *evi));
  }

  /* Make a copy of the main tableau coefficients in fCacheCoeffs to speed up
     GenerateKT.  This is performed only if the virtual tableau or rhs is
     used. */
  CacheTableau();

  // Ensure variable indices are the same by adding variables into the
  // sub-tableau.  This may be improved later by just copying the bimaps.
  PrepareVariables();

  // Calculates fDepPars, fDepBasicVars and fDepParVars.
  TransitiveClosure();

  // Place constraints with slack basic variables with dependant variable in
  // fDepParVars into fSubLS tableau.
  for (QcBasicVarIndexIterator bvIt( fTableau);
       !bvIt.AtEnd();
       bvIt.Increment())
    {
      if (!fTableau.IsSlack( bvIt.getIndex()))
	continue;

      /* Check if the current constraint has a dependant parameter variable. */
      unsigned c = bvIt.getConstraintBasicIn();
      assert (c < fTableau.getNRows());
      QcSparseMatrixRowIterator varCoeffs (fCacheCoeff, c);
      bool hasDepPar = false;
      while (!varCoeffs.AtEnd())
	{
	  if (IsDepVar( varCoeffs.getIndex()))
	    {
	      hasDepPar = true;
	      break;
	    }
	  varCoeffs.Increment();
	}

      /* If so, then add the solved form constraint into fSubLS tableau. */
      if (hasDepPar)
	{
	  QcLinPoly lpoly;
	  lpoly.add( fTableau.GetValue( c, bvIt.getIndex()),
		     fVBiMap.getIdentifierPtr( bvIt.getIndex()));
	  varCoeffs.SetToBeginOf( c);
	  while (!varCoeffs.AtEnd())
	    {
	      QcFloatRep *v = fVBiMap.getIdentifierPtr( varCoeffs.getIndex());
	      lpoly.add( varCoeffs.getValue(), v);
	      varCoeffs.Increment();
	    }

#ifdef qcRealTableauRHS
	  QcConstraint lc ("", lpoly, QcConstraintRep::coEQ,
			   fTableau.GetRHS(c));
#else
	  QcConstraint lc ("", lpoly, QcConstraintRep::coEQ,
			   fCacheRHS[c]);
#endif
	  dbg(bool added =)
	    fSubSystem.CopyConstraint( lc, bvIt.getIndex());
	  assert( added);
	}
    }

  // This needs the original constraints copied into
  // subsystem.  The dual variables are preallocated.
  // The indices are assigned below.
  CreateDualVars();

  // Create a variable for the desired value of each EditVars.
  CreateDesValVars();

  fAI = fSubTableau.AddArtificial (-1.0);
  fAV.SetWeight (0.0);	// Just in case it somehow get taken notice of.
  fDuals.push_back( fAV.pointer(), fAI);	// This shouldn't need to be accessed.
  fSubVBiMap.Update (fAV, fAI);

  // Add Edit Variables.
  fSystem.ClearEditVars();

  for (vector<unsigned int>::iterator viIt = fDepPars.begin();
       viIt != fDepPars.end(); ++viIt)
    {
      QcFloat &v = fSubVBiMap.Identifier (*viIt);
      fSystem.RegisterEditVar (v);
    }

  GenerateKT2();

  // Enter fDualIndex values for non slack variables
  {
    unsigned ssCols = fSubTableau.getNColumns();
    assert( ssCols <= fDuals.size()); // todo: this could probably be tightened.
    for (unsigned vi = 0; vi < ssCols; vi++)
      {
	QcFloatRep *dualVar = fDuals.varPtr( vi);
	int dualVarIx = fSubVBiMap.safeIndex( dualVar);
	if (dualVarIx >= 0)
	  fDuals.setIndex( vi, dualVarIx);
	// TODO: Consider making the above non-conditional.
      }
  }

  // The subsystem has both the dual and restricted primal constraints.
  LoadDesVarCache();
  MakeFeasibleComplex();
  qcAssert(fSubTableau.IsBasicFeasibleSolved());
  fSubSystem.InEqRawSolveComplex();
  fSubSystem.EqRawSolveComplex();
  // And now the variable values provide the solution.
  fSystem.EqSolve();	// Solve using slack values as well
  fEditVarsSetup = true;

  dbg(assertInvar());
}


void
QcLinInEqSolver::CacheTableau()
{
  fCacheCoeff.clear();
  fCacheCoeff.Resize( fTableau.getNRows(),
		      fTableau.getNColumns());

  for (QcParamVarIndexIterator vIt( fTableau);
       !vIt.AtEnd();
       vIt.Increment())
    {
      for (QcTableauColIterator colCoeffs( fTableau, vIt.getIndex());
	   !colCoeffs.AtEnd();
	   colCoeffs.Increment())
	{
	  fCacheCoeff.SetValue( colCoeffs.getIndex(), vIt.getIndex(),
				colCoeffs.getValue());
	}
    }

  // Make a copy of rhs to speed up GenerateKT.
#ifndef qcRealTableauRHS
  fCacheRHS.Resize( 0);
  fCacheRHS.Resize( tableau.getNRows());
  for (unsigned r = 0; r < tableau.getNRows(); r++)
    fCacheRHS[r] = tableau.GetRHS(r);
#endif
}

void QcLinInEqSolver::CreateDesValVars()
{
  fDesValVar.resize( fTableau.getNColumns());
  fDesValIndex.resize( fTableau.getNColumns());

  for (unsigned i = 0; i < fTableau.getNColumns(); i++)
    fDesValIndex[i] = QcTableau::fInvalidVariableIndex;

  for (vector<unsigned>::const_iterator i = fVarsByIndex.begin(), iEnd = fVarsByIndex.end();
       i != iEnd; i++)
    {
      unsigned const vi = *i;
      QcFloat v("desval var", fSubVBiMap.getIdentifierPtr( vi)->DesireValue());
      v.SetWeight(0.0);
      v.SuggestValue(v.Value());
      fDesValIndex[vi] = fSubTableau.AddDesValVar();
      fDesValVar[vi] = v;
      fSubVBiMap.Update(v, fDesValIndex[vi]);
      fDuals.push_back( v.pointer(), fDesValIndex[vi]);
      assert( v.Counter() > 1);
    }

  dbg(assertInvar());
}

void QcLinInEqSolver::CreateDualVars()
	// Create complementary variables for each variable in
	// the primal tableau.  The fDual array is used to locate these
	// variables.  N.B. Not all these variables need be used.
	// The rest of fDual is initialised in AddDualCoeffs.
{
  for (unsigned i = 0; i < fSubTableau.getNColumns(); i++)
    {
      QcFloat v( "Dual Var");
      fDuals.push_back( v.pointer(), QcTableau::fInvalidVariableIndex);
      assert( v.Counter() > 1);
    }
}

numT QcLinInEqSolver::Derivative1(QcFloat &p, int pIndex, QcLinPoly &lpoly)
  { return Derivative( p, pIndex, lpoly, false); }

numT QcLinInEqSolver::Derivative2(QcFloat &p, int pIndex, QcLinPoly &lpoly)
  { return Derivative( p, pIndex, lpoly, true); }

numT QcLinInEqSolver::Derivative(QcFloat &p, int pIndex, QcLinPoly &lpoly, bool phaseII)
	// Generate the expression (lpoly,rhs): df'/dp + dual(p).
	// where f' = least squares objective function with basic variables
	// eliminated and dual(p) is the column coeffs of the original
	// tableau for p applied to new dual variables.
	// The variable coefficients are accumulated in lpoly, the rhs value
	// is returned.
{
  qcAssertPre(p == fSubVBiMap.Identifier(pIndex));
  numT rhs = 0.0;
  numT pWeight2 = p.Weight() * 2;
  numT pAccumulator = pWeight2;

  for (QcBasicVarIndexIterator bvIt( fTableau);
       !bvIt.AtEnd();
       bvIt.Increment())
    {
      QcFloat &bv = fSubVBiMap.Identifier (bvIt.getIndex());
      numT bvWeight = bv.Weight();

      if (QcUtility::IsZero( bvWeight))
	continue;

      unsigned c = bvIt.getConstraintBasicIn();
      assert( c < fTableau.getNRows());
      numT pCoeff = fCacheCoeff.GetValue(c, pIndex);

      if (!QcUtility::IsZero( pCoeff))
	{
				// bv is basic in the solved form constraint with index c.
	  numT factor = bv.Weight() * pCoeff * 2;
	  pAccumulator += factor * pCoeff;

				// constraint c will be used to eliminate bv,
				// and pCoeff is the coefficient of p in the elimination.
	  for (QcSparseMatrixRowIterator varCoeffs( fCacheCoeff, c);
	       !varCoeffs.AtEnd();
	       varCoeffs.Increment())
	    {
	      if (varCoeffs.GetIndex() != pIndex)
		{
		  QcFloatRep *pv = fSubVBiMap.getIdentifierPtr( varCoeffs.GetIndex());
		  lpoly.add( factor * varCoeffs.GetValue(), pv);
		}
	    }

	  if (phaseII
	      && IsDepBasicVar( bvIt.getIndex())
	      && IsEditVar( bv))
	    {
#ifdef qcRealTableauRHS
	      rhs += factor * fTableau.GetRHS(c);
#else
	      rhs += factor * fCacheRHS[c];
#endif
	      lpoly.add( factor, fDesValVar[bvIt.getIndex()].pointer());
	    }
	  else
	    {
#ifdef qcRealTableauRHS
	      rhs += factor * (fTableau.GetRHS(c) - bv.DesireValue());
#else
	      rhs += factor * (fCacheRHS[c] - bv.GetDesValue());
#endif
	    }
	}
    }

  lpoly.Push(pAccumulator, p);
  AddDualCoeffs(lpoly, pIndex);

  if (phaseII
      && IsEditVar( p))
    {
      QcFloat &pDV = fDesValVar[pIndex];
      lpoly.Push(-pWeight2, pDV);
    }
  else
    rhs += pWeight2 * p.DesireValue();

  return rhs;
}

void QcLinInEqSolver::EndEdit()
{
  // The EndEdit in the QcSolver class is called so that the weight of all
  // the edit variable are set back to stay weight, and for ClearEditVars.
  QcSolver::EndEdit();

  vector<unsigned int>::iterator vIt;

  for (unsigned int vi = 0; vi < fDepBasicVars.size(); ++vi)
    if (fDepBasicVars[vi])
      fVBiMap.getIdentifierPtr( vi)->RestDesVal();

  for (vIt = fDepPars.begin(); vIt != fDepPars.end(); ++vIt)
    fVBiMap.getIdentifierPtr( *vIt)->RestDesVal();

  fDuals.clear();
}

void QcLinInEqSolver::GenerateKT1()
	// Generate the constraint df'/dp + dual(p) == 0 for each unconstrained
	// primal parameter p.  Then generate the constraint
	// df'/dp + dual(p) >= 0 for each constrained (i.e. slack) primal
	// parameter.  Add the new constraint directly to the subsystem.
	// Where f' = least squares objective function with basic variables
	// eliminated, and dual(p) is the column coeffs of the original
	// tableau for p applied to new dual variables.
{
  QcParamVarIndexIterator vIt( fTableau);

  while (!vIt.AtEnd())
    {
      unsigned const vi = vIt.getIndex();
      // The !IsFree test avoids entering a redundant constraint in
      // the subsystem (to save time).  There is no need to call
      // v.Set when the variable is free since this will be
      // done when fSystem.Solve() is called later.
      if (!fTableau.IsFree( vi)
	  && fTableau.IsConstrained( vi))
	{
	  // Primal slack parameters were given a weight of 0 in Solve.
	  QcLinPoly lpoly;
	  /* [pjm:] It makes a difference that this is `QcFloat p' rather than
	     `QcFloat &p', because p becomes unused during fSubSystem.CopyInEq
	     below.  Haven't checked whether this is OK. */
	  QcFloat p = fSubVBiMap.Identifier( vi);
	  numT rhs = Derivative1(p, vi, lpoly);
	  lpoly.add( 1.0, fAV); // artificial variable coeff
	  QcConstraint lc("", lpoly, QcConstraintRep::coGE, rhs);
	  dbg(bool added =)
	    fSubSystem.CopyInEq( lc); // new slack is made basic
	  assert( added);
	  fDuals.push_back( p.pointer(), vi);	// p corresponds to new slack
	  fDuals.setIndex( vi, fSubTableau.GetColumns() - 1);
	  // fDual[vi].SetName("Dual Slack Var");
	}
      vIt.Increment();
    }

  fSubTableau.SetBeginEq();
  vIt.Reset();

  while (!vIt.AtEnd())
    {
      unsigned const vi = vIt.getIndex();
      // The !IsFree test avoids entering a redundant constraint in
      // the subsystem (to save time).  There is no need to call
      // v.Set when the variable is free since this will be
      // done when fSystem.Solve() is called later.
      if (!fTableau.IsFree( vi)
	  && !fTableau.IsConstrained( vi))
	{
	  // Do the unrestricted primal parameters.
	  QcLinPoly lpoly;
	  QcFloat &viVar = fSubVBiMap.Identifier(vi);
	  numT rhs = Derivative1(viVar, vi, lpoly);
	  // No need to add the (-1) artificial variable coeff
	  // here since the primal parameter is unrestricted.
	  QcConstraint lc("", lpoly, QcConstraintRep::coEQ, rhs);
	  dbgPost(bool added =)
	    fSubSystem.AddConstraint(lc, viVar);
	  qcAssertPost( added);
	}
      vIt.Increment();
    }

  dbg(assertInvar());
}

void QcLinInEqSolver::GenerateKT2()
  // Generate the constraint df'/dp + dual(p) == 0 for each unconstrained
  // primal parameter p.  Then generate the constraint
  // df'/dp + dual(p) >= 0 for each constrained (i.e. slack) primal
  // parameter.  Add the new constraint directly to the subsystem.
  // Where f' = least squares objective function with basic variables
  // eliminated, and dual(p) is the column coeffs of the original
  // tableau for p applied to new dual variables.
{
  vector<unsigned int>::const_iterator viIt;

  for (viIt = fDepPars.begin(); viIt != fDepPars.end(); ++viIt)
    {
      unsigned const vi = *viIt;

      if (!fTableau.IsFree( vi)
	  && fTableau.IsConstrained( vi))
	{
	  // Primal slack parameters were given a weight of 0 in Solve.
	  QcLinPoly lpoly;
	  QcFloat &p = fSubVBiMap.Identifier( vi);
	  numT rhs = Derivative2(p, vi, lpoly);
	  lpoly.add( 1.0, fAV); // artificial variable coeff
	  QcConstraint lc("", lpoly, QcConstraintRep::coGE, rhs);
	  dbg(bool added =)
	    fSubSystem.CopyInEq( lc); // new slack is made basic
	  assert( added);
	  fDuals.push_back( p.pointer(), vi);	// p corresponds to new slack
	  fDuals.setIndex( vi, fSubTableau.GetColumns() - 1);
	  // fDual[vi].SetName("Dual Slack Var");
	}
    }

  fSubTableau.SetBeginEq();

  for (viIt = fDepPars.begin(); viIt != fDepPars.end(); ++viIt)
    {
      unsigned const vi = *viIt;

      if (!fTableau.IsFree( vi)
	  && !fTableau.IsConstrained( vi))
	{
	  // First do the unrestricted primal parameters.
	  QcLinPoly lpoly;
	  QcFloat &viItVar = fSubVBiMap.Identifier( vi);
	  numT rhs = Derivative2( viItVar, vi, lpoly);
	  // No need to add the (-1) artificial variable coeff
	  // here since the primal parameter is unrestricted.
	  QcConstraint lc("", lpoly, QcConstraintRep::coEQ, rhs);
	  dbgPost(bool consistent =)
	    fSubSystem.AddConstraint(lc, viItVar);
	  qcAssertPost(consistent);
	}
    }

  dbg(assertInvar());
}

void QcLinInEqSolver::LoadDesVarCache()
    // Required for desval variables.
{
	for (unsigned int i = 0; i < fDesValVar.size(); i++) {
		if (fDesValIndex[i] != QcTableau::fInvalidVariableIndex) {
			int vi = fDesValIndex[i];
			fSubTableau.SetDesireValue(vi,
               		fSubVBiMap.Identifier(i).DesireValue());
		}
	}
}

void QcLinInEqSolver::MakeFeasible()
{
	// Find the restricted row with most negative RHS
	numT mostNeg = 0.0;
	int mostNegIndex = QcTableau::fInvalidConstraintIndex;
	int rows = fSubTableau.GetRows();

	for (int ci = 0; ci < rows; ci++) {
		if (ci >= fSubTableau.GetBeginEq())
			break;

		if (fSubTableau.IsRestricted(ci)) {
			numT coeff = fSubTableau.GetRHS(ci);

			if (coeff < mostNeg) {
				mostNeg = coeff;
				mostNegIndex = ci;
			}
		}
	}

	if (!QcUtility::IsNegative(mostNeg))
		return;	// tableau is already feasible

	// Initialize the coefficients of fAV in all the constraints with 1.0.
   	fSubTableau.SetArtificial(-1.0);

	int justLeft = fSubTableau.GetBasicVar(mostNegIndex);
	fSubTableau.RestrictedPivot(mostNegIndex, fAI);
	qcAssert(fSubTableau.IsBasicFeasibleSolved());

	// We now have a subsystem tableau which is basic feasible.
	// But we wish to eliminate the artificial variable from it.
	// To do so we keep pivoting on the variable complementary to the
	// variable which just left the basis.
	int ci;

	while (fSubTableau.IsBasic(fAI)) {		// artificial variable is still basic
		unsigned vi = fDuals.varIx( justLeft);  // complementary variable
		// Select Pivot row
		qcAssert(fSubTableau.getNRestrictedRows() != 0);
		ci = fSubSystem.SelectExitVar(vi);
		qcAssert(ci >= 0);

		if (ci < 0) break;	// Better to give up than continue now.

		justLeft = fSubTableau.GetBasicVar(ci);
		#ifdef qcSafetyChecks
		bool pivoted = fSubTableau.RestrictedPivot(ci, vi);
		qcAssert(pivoted);
		#else
		fSubTableau.RestrictedPivot(ci, vi);
		#endif
	}
}

void QcLinInEqSolver::MakeFeasibleComplex()
{
	// Find the restricted row with most negative RHS
	numT mostNeg = 0.0;
	int mostNegIndex = QcTableau::fInvalidConstraintIndex;
	int rows = fSubTableau.GetRows();

	for (int ci = 0; ci < rows; ci++) {
		if (ci >= fSubTableau.GetBeginEq())
         		break;

		if (fSubTableau.IsRestricted(ci)) {
			numT coeff = fSubTableau.GetComplexRHS(ci);
			if (coeff < mostNeg) {
				mostNeg = coeff;
				mostNegIndex = ci;
			}
		}
	}

	if (!QcUtility::IsNegative(mostNeg))
		return;	// tableau is already feasible
	assert( mostNegIndex >= 0);

	// Initialize the coefficients of fAV in all the constraints with 1.0.
	fSubTableau.SetArtificial(-1.0);

	int justLeft = fSubTableau.GetBasicVar(mostNegIndex);
	fSubTableau.RestrictedPivot(mostNegIndex, fAI);
	/* FIXME: The assertion on the following line can fail (e.g. in test suite). */
	qcAssert(fSubTableau.IsBasicFeasibleSolved());

	// We now have a subsystem tableau which is basic feasible.
	// But we wish to eliminate the artificial variable from it.
	// To do so we keep pivoting on the variable complementary to the
	// variable which just left the basis.
	int ci;

	while (fSubTableau.IsBasic(fAI)) {		// artificial variable is still basic
		int vi = fDuals.varIx( justLeft);  // complementary variable
		// Select Pivot row
		qcAssert(fSubTableau.getNRestrictedRows() != 0);
		ci = fSubSystem.SelectExitVarComplex(vi);
		qcAssert(ci >= 0);

		if (ci < 0) break;	// Better to give up than continue now.

		justLeft = fSubTableau.GetBasicVar(ci);
		#ifdef qcSafetyChecks
		bool pivoted = fSubTableau.RestrictedPivot(ci, vi);
		qcAssert(pivoted);
		#else
		fSubTableau.RestrictedPivot(ci, vi);
		#endif
	}
}

numT QcLinInEqSolver::Objective()
{
  numT opt( 0);

  for (QcVariableIndexIterator vIt( fTableau);
       !vIt.AtEnd();
       vIt.Increment())
    {
      QcFloatRep *v = fVBiMap.getIdentifierPtr( vIt.getIndex());
      numT delta = v->Value() - v->DesireValue();
      opt += v->Weight() * delta * delta;
    }

  return opt;
}

void QcLinInEqSolver::PrepareVariables()
{
  qcAssertPre( fSubSystem.getVariables_begin() == fSubSystem.getVariables_end());

  for (unsigned vi = 0; vi < fTableau.getNColumns(); vi++)
    {
      QcFloat &v = fVBiMap.Identifier( vi);
      if (fTableau.IsSlack( vi))
	{
	  // TODO: Consider making slack vars QcFixedFloat's.
	  v.SuggestValue( 0);
	  fSubSystem.AddSlack( v);
	}
      else
	fSubSystem.AddVar( v);
    }
}

void QcLinInEqSolver::Print(ostream &os) const
{
	fSystem.Print(os);

	if (fEditVarsSetup) {
       	os << "Complementary pivot tableau: " << endl;
		fSubTableau.Print(os);
		os << endl;
       	os << "Sub-system notifier: " << endl;
		fSubNotifier.Print(os);
		os << endl;
	}
}

void QcLinInEqSolver::Resolve()
{
	if (!fEditVarsSetup)
		BeginEdit();

	// The subsystem has both the dual and restricted primal constraints.
	LoadDesVarCache();
	MakeFeasibleComplex();

	qcAssert(fSubTableau.IsBasicFeasibleSolved());
	fSubSystem.InEqRawSolveComplex();
	fSubSystem.EqRawSolveComplex();
	SubSysEqSolve();	// Solve using slack values as well

#if qcCheckPost
	checkSatisfied();
#endif
}

void QcLinInEqSolver::Solve()
  // IMPORTANT NOTE.
  // It is not necessary to call fSystem.Solve() before creating and solving
  // the subsystem since the subsystem is created without using the values
  // of any variables - just desired values and weights.
{
  fAI = QcTableau::fInvalidVariableIndex;
  fSubSystem.Restart();

  // Make a copy of the main tableua coefficients to speed up GenerateKT.
  // This is only performed if the virtual tableau or virtual rhs is used.
  CacheTableau();

  // Ensure variable indices are the same by adding variables into the
  // sub-tableau.  This may be improved later by just copying the bimaps.
  PrepareVariables();

  // Place constraints with slack basic variables into fSubTableau.
  for (QcBasicVarIndexIterator bvIt( fTableau);
       !bvIt.AtEnd();
       bvIt.Increment())
    {
      if (!fTableau.IsSlack( bvIt.getIndex()))
	continue;

      unsigned c = bvIt.getConstraintBasicIn();
      QcLinPoly lpoly;
      lpoly.add( fTableau.GetValue( c, bvIt.getIndex()),
		 fVBiMap.getIdentifierPtr( bvIt.getIndex()));
      for (QcSparseMatrixRowIterator varCoeffs( fCacheCoeff, c);
	   !varCoeffs.AtEnd();
	   varCoeffs.Increment())
	{
	  lpoly.add( varCoeffs.getValue(),
		     fVBiMap.getIdentifierPtr( varCoeffs.getIndex()));
	}
#ifdef qcRealTableauRHS
      QcConstraint lc ("", lpoly, QcConstraintRep::coEQ,
		       fTableau.GetRHS(c));
#else
      QcConstraint lc ("", lpoly, QcConstraintRep::coEQ,
		       fCacheRHS[c]);
#endif
#ifdef QcAssertions
      bool success =
#endif
	fSubSystem.CopyConstraint (lc, bvIt.getIndex());
      qcAssert (success);
    }

  // This needs the original constraints copied into
  // subsystem.  The dual variables are preallocated.
  // The indices are assigned below.
  CreateDualVars();

  fAI = fSubTableau.AddArtificial(-1.0);
  fAV.SetWeight(0.0);		// Just in case it somehow get taken notice of
  fDuals.push_back( fAV.pointer(), fAI);	// This shouldn't need to be accessed.
  fSubVBiMap.Update(fAV, fAI);

  // Add Kuhn-Tucker constraints to the SubSystem
  GenerateKT1();

  // Enter fDualIndex values for non slack variables
  {
    unsigned ssCols = fSubTableau.GetColumns();
    assert( fDuals.size() >= ssCols); // todo: could probably be tightened
    for (unsigned vi = 0; vi < ssCols; vi++)
      {
	QcFloatRep *dualVar = fDuals.varPtr( vi);
	int dualVarIx = fSubVBiMap.safeIndex(dualVar);
	if (dualVarIx >= 0)
	  fDuals.setIndex( vi, dualVarIx);
	// TODO: Consider making the above unconditional.
      }
  }

  // The subsystem has both the dual and restricted primal constraints.
  MakeFeasible();
  qcAssert(fSubTableau.IsBasicFeasibleSolved());
  fSubSystem.InEqRawSolve();
  fSubSystem.EqRawSolve();
  SubSysEqSolve();		// Solve using slack values as well

  // Rest variables by setting their desire value to the actual value
  // computed above.
  for (QcVariableIndexIterator vIt( fTableau);
       !vIt.AtEnd();
       vIt.Increment())
    {
      QcFloatRep *v = fVBiMap.getIdentifierPtr( vIt.getIndex());
      v->RestDesVal();
    }

  // And now the variable values provide the solution.
  fDuals.clear();

#if qcCheckPost
  checkSatisfied();
  dbg(assertInvar());
#endif
}

void QcLinInEqSolver::SubSysEqSolve()
	// Equivalent to LinEqSystem::Solve
{
	#ifdef qcSafetyChecks
	if (!fSubTableau.IsBasicFeasibleSolved())
		throw QcWarning(qcPlace,
			"tableau is not in basic feasible solved form");
	#endif

	QcVariableIndexIterator vIt(fTableau);

	while (!vIt.AtEnd()) {
		if (fTableau.IsBasic(vIt.getIndex())) {
			QcFloat &v = fVBiMap.Identifier(vIt.getIndex());
			int ci = fTableau.IsBasicIn(vIt.getIndex());
			QcTableauRowIterator varCoeffs(fTableau, ci);
			numT value = fTableau.GetRHS(ci);

			while (!varCoeffs.AtEnd()) {
				if (varCoeffs.getIndex() != vIt.getIndex()) {
					QcFloat &bv = fVBiMap.Identifier (varCoeffs.getIndex());
					value -= varCoeffs.getValue() * bv.Value();
				}
				varCoeffs.Increment();
			}

			v.SetValue(QcUtility::Zeroise(value));
		}
		vIt.Increment();
	}
}

void QcLinInEqSolver::TransitiveClosure()
	// Calculate fDepBasicVars and fDepPars as transitive closure of
	// variables in constraints which mention EditVars.
{
  vector<unsigned> depVars( fVarsByIndex);
  fTableau.TransitiveClosure( depVars);

  // Sift depVars into fDepPars, fDepBasicVars and fDepParVars.
  fDepBasicVars.resize( fTableau.getNColumns());
  fDepParVars.resize( fTableau.getNColumns());

  for (unsigned i = 0; i < fTableau.getNColumns(); i++)
    {
      /* effic: Check if this could be achieved with clear() before the above
         resize.  In particular, is `bool()' guaranteed to be zero, or is it a
         garbage value?  clear()/resize() is especially better if vector<bool>
         is implemented as a bit vector. */
      fDepBasicVars[i] = false;
      fDepParVars[i] = false;
    }

  fDepPars.resize(0);
  for (vector<unsigned>::const_iterator i = depVars.begin(), iEnd = depVars.end();
       i != iEnd; i++)
    {
      unsigned const vi = *i;
      assert( vi < fDepBasicVars.size());
      if (fTableau.IsBasic( vi))
	fDepBasicVars[vi] = true;
      else
	{
	  fDepPars.push_back( vi);
	  fDepParVars[vi] = true;
	}
    }
}
