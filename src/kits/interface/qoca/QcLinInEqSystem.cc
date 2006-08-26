// $Id: QcLinInEqSystem.cc,v 1.18 2001/01/31 07:37:13 pmoulder Exp $

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
// Unlike identifiers, indexes start from 0 since they are used as array and  //
// matrix subscripts in the tableau.                                          //
//============================================================================//

#include "qoca/QcDefines.hh"
#include "qoca/QcLinInEqSystem.hh"
#include "qoca/QcBasicVarIndexIterator.hh"
#include "qoca/QcVariableIndexIterator.hh"
#include "qoca/QcConstraintIndexIterator.hh"
#include "qoca/QcStructVarIndexIterator.hh"
#include "qoca/QcParamVarIndexIterator.hh"
#include "qoca/QcTableauRowIterator.hh"
#include "qoca/QcTableauColIterator.hh"
#include "qoca/QcDenseTableauRowIterator.hh"

bool QcLinInEqSystem::AddConstraint(QcConstraint &c)
{
	return doAddConstraint(c, -1);
}

bool QcLinInEqSystem::AddConstraint(QcConstraint &c, QcFloat &hint)
{
#if qcCheckPre
	int hintIx = fVBiMap.safeIndex(hint);
	qcAssertExternalPre(hintIx >= 0);
#else
	int hintIx = fVBiMap.Index(hint);
#endif
	return doAddConstraint(c, hintIx);
}

bool QcLinInEqSystem::doAddConstraint(QcConstraint &c, int hintIx)
{
	// If in batch mode and the one of more preceeding add-constraint
	// operations failed, then should not proceed to do any more constraint
	// addition.
	if (fBatchAddConst && fBatchAddConstFail)
		return false;

	bool success = true;
	int co = AddToTableau(c);
	// co is now the original constraint index
	int cs = fTableau.Eliminate(co);
	// cs is solved form constraint index

	if (hintIx >= 0) {
		fTableau.fPivotHints.resize(0);
		fTableau.fPivotHints.push_back(hintIx);
	}

	if (cs == QcTableau::fInvalidConstraintIndex) {
		fInconsistant.push_back(c);
		fEditVarsSetup = false;
		return false;
	}

	if (fBatchAddConst) {
		// Check if it has unrestricted variables.
		QcTableauRowIterator varCoeffs(fTableau, cs);

		while (!varCoeffs.AtEnd()) {
			if (!fTableau.IsSlack (varCoeffs.getIndex()))
				break;
			varCoeffs.Increment();
		}

		// If there is, then do the normal add constraint.
		if (!varCoeffs.AtEnd()) {
			/* TODO: It's not clear that the "hintIx < 0 || " part
			   is deliberate. */
			if (hintIx < 0 || !fTableau.IsRedundant(cs)) {
				success = ApplyHints(cs);
				if (!success)
					fTableau.DeleteRow(co);
			}
		} else {
			// If the constraint has a new slack variable.
			if (c.Operator() != QcConstraintRep::coEQ) {
				numT rhs = fTableau.GetRHS(cs);
				numT value = fTableau.GetValue(cs, fTableau.GetColumns() - 1);

				if (QcUtility::IsNegative(value)
				    ? !QcUtility::IsPositive(rhs)
				    : !QcUtility::IsNegative(rhs)) {
					fTableau.Pivot(cs, fTableau.GetColumns() - 1);
						// Should consider just setting the state of the
						// row and column instead of calling Pivot.  This
						// is because we know the variable to be make
						// basic is not in any other rows.
				} else {
					int vi = fTableau.AddArtificial(
							QcUtility::IsPositive(rhs) ? 1.0 : -1.0);
					fArtVars.push_back(vi);
					fTableau.Pivot(cs, vi);
						// May scale row by -1 to make the newly added
						// artifical variable to +1.  This method may
						// require explicit management of column and row
						// state.
				}
			}
		}

		if (success) {
			fOCBiMap.Update(c, co);
			fBatchConstraints.push_back(c);
		} else
			fBatchAddConstFail = true;
	} else {
		if (!fTableau.IsRedundant(cs)) {
			success = ApplyHints(cs);
			if (!success)
				fTableau.DeleteRow(co);
		}

		if (success)
			fOCBiMap.Update(c, co);
	}

	fEditVarsSetup = false;
	return success;
}

bool QcLinInEqSystem::AddInEq(QcConstraint &c)
{
	bool success = true;

	#ifdef qcSafetyChecks
	if (c.Operator() == QcConstraintRep::coEQ)
		throw QcWarning("QcLinInEqSystem::AddInEq supplied with an equation");
	#endif

	int co = AddToTableau(c);

	if (co < 0)
		return false;

	int cs = fTableau.Eliminate(co);	 // cs is solved form constraint index

	if (cs == QcTableau::fInvalidConstraintIndex) {
		fInconsistant.push_back(c);
		success = false;
	} else if (!fTableau.IsRedundant(cs)) {	// Pivot on new slack variable
		success = fTableau.Pivot(cs, fTableau.GetColumns() - 1);

		// Should get the slack var index returned from AddToTableau
		// rather than just knowing how it is implemented.
		if (!success)
			fTableau.DeleteRow(co);
	}

	if (success)
		fOCBiMap.Update(c, co);

	fEditVarsSetup = false;
	return success;
}

bool QcLinInEqSystem::ApplyHints(int cs)
{
	bool pivoted = false;

	if (cs < 0 || cs >= fTableau.GetRows() || fTableau.IsRedundant(cs))
		throw QcWarning("ApplyHints called for invalid constraint",
			cs, fTableau.GetRows() - 1);
	else {
		for (unsigned int i = 0; i < fTableau.fPivotHints.size() && !pivoted; i++) {
			int hint = fTableau.fPivotHints[i];
			if (!fTableau.IsConstrained(hint))
				pivoted = fTableau.Pivot(cs, hint);
		}

		vector<QcSparseCoeff> slackCoeffs;

		// If PivotHints failed, this means all the pivot hints variables
		// have zero coeffs or are constrained (slack).
		// Now try all the rest of the parametric variables with non-zero
		// coeffs in the solved form constraint.  First try non-slack
		// variables, saving the indices of any slack parameters for later use.
		if (!pivoted)
		  {
#ifdef qcUseSafeIterator
		    {
		      QcDenseTableauRowIterator varCoeffs(fTableau, cs);

		      while (!varCoeffs.AtEnd() && !pivoted)
			{
				if (!fTableau.IsBasic(varCoeffs.getIndex())) {
					if (fTableau.IsConstrained(varCoeffs.getIndex()))
						slackCoeffs.push_back(
							QcSparseCoeff(varCoeffs.getValue(), varCoeffs.getIndex()));
					else
						pivoted = fTableau.Pivot(cs, varCoeffs.getIndex());
				}
				varCoeffs.Increment();
			}
		    }
#else
		    // Cache tableau column coefficients since it is possible that
		    // the iterator may be invalidated during iteration.
		    {
		      QcTableauRowIterator varCoeffs(fTableau, cs);
		      vector<QcSparseCoeff> coeffCache;

		      while (!varCoeffs.AtEnd())
			{
			  coeffCache.push_back( QcSparseCoeff( varCoeffs.getValue(),
							       varCoeffs.getIndex()));
			  varCoeffs.Increment();
			}

		      for (vector<QcSparseCoeff>::iterator cIt = coeffCache.begin();
			   (cIt != coeffCache.end()) && !pivoted;
			   cIt++)
			{
			  QcSparseCoeff &coeff = (*cIt);
			  unsigned const coeff_ix = coeff.getIndex();

			  if (!fTableau.IsBasic( coeff_ix))
			    {
			      if (fTableau.IsConstrained( coeff_ix))
				slackCoeffs.push_back( QcSparseCoeff( coeff.getValue(), coeff_ix));
			      else
				pivoted = fTableau.Pivot( cs, coeff_ix);
			    }
			}
		    }
#endif
		  }

		if (!pivoted)
		  {	// try the new slack variable
		    numT rhs = fTableau.GetRHS(cs);
		    qcAssert(slackCoeffs.size() != 0);
		    QcSparseCoeff &s = slackCoeffs.back();
		    slackCoeffs.pop_back();

		    if (s.getIndex() == fTableau.getNColumns() - 1)
		      {	// new slack belongs to cs
			qcAssert( QcUtility::IsOne( fabs( s.getValue())));

			if (QcUtility::IsNegative( s.getValue())
			    ? !QcUtility::IsPositive( rhs)
			    : !QcUtility::IsNegative( rhs))
			  {
			    pivoted = fTableau.Pivot( cs, s.getIndex());
			    qcAssert(pivoted);
			  }
		      }

		    // Call SimplexI here as have rhs and new slack is popped off.
		    if (!pivoted)
		      pivoted = SimplexI(cs);
		  }
	}

	return pivoted;
}

/** Does first part of addConstraint, calling the correct tableau
    function.

    @return <tt>fTableau.add*</tt>
    @precondition <tt>c</tt> is non-null, and has a valid relation (eq, le, ge).
    @postcondition ret &ge; 0
**/
int QcLinInEqSystem::AddToTableau(QcConstraint &c)
{
	int co;
	QcRowAdaptor ra(c.LinPoly(), fVBiMap, fTableau);

	switch (c.Operator()) {
		case QcConstraintRep::coEQ:
			co = fTableau.AddEq(ra, c.RHS());
			goto out;

		case QcConstraintRep::coLE:
			co = fTableau.AddLtEq(ra, c.RHS());
			/* TODO: Consider making this a QcFixedFloat.
			   (Similarly below.) */
			fVBiMap.Update(
				QcFloat("", 0.0, 0.0, 0.0, true), fTableau.GetColumns() - 1);
			goto out;

		case QcConstraintRep::coGE:
			co = fTableau.AddGtEq(ra, c.RHS());
			fVBiMap.Update(
				QcFloat("", 0.0, 0.0, 0.0, true), fTableau.GetColumns() - 1);
			goto out;

		default:
			throw QcWarning("AddConstraint: invalid constraint operator");
			abort();
	}

 out:
	qcAssertPost( co >= 0);
	return co;
}

void QcLinInEqSystem::BeginEdit()
	// This is the same as LinEqSystem::BeginEdit except where noted with the
	// comment ++++.
{
	vector<QcFloat>::iterator voiIt;

	#ifdef qcSafetyChecks
	qcAssert(fTableau.IsBasicFeasibleSolved());				// ++++
	#endif

	// build a version of fEditVars by index.
	vector<unsigned int>::iterator viIt, viIt2;
	fVarsByIndex.resize(0);

	for (voiIt = fEditVars.begin(); voiIt != fEditVars.end(); ++voiIt)
		if (IsRegistered(*voiIt))
			fVarsByIndex.push_back(fVBiMap.Index(*voiIt));

	// try to make all variables of interest into parameters.
	// but not at the expense of creating a new restricted row.
	for (viIt = fVarsByIndex.begin(); viIt != fVarsByIndex.end(); ++viIt) {
		int vi = *viIt;
		if (fTableau.IsBasic(vi)) {
			int ci = fTableau.IsBasicIn(vi);

          	#ifdef qcUseSafeIterator
			{
			  QcDenseTableauRowIterator varCoeffs(fTableau, ci);

			  while (!varCoeffs.AtEnd()) {
				bool reject = fTableau.IsConstrained(varCoeffs.getIndex());		// ++++

				for (viIt2 = fVarsByIndex.begin();
                   		viIt2 != fVarsByIndex.end(); ++viIt2)
					if (varCoeffs.getIndex() == *viIt2)
						reject = true;

				if (!reject) {
					#ifdef qcSafetyChecks
					qcAssert(fTableau.IsBasic(vi));
					qcAssert(fTableau.GetBasicVar(ci) == vi);
					qcAssert(!fTableau.IsBasic(varCoeffs.getIndex()));
					qcAssert(fVBiMap.Identifier(varCoeffs.getIndex()) != *voiIt);
					#endif
					#ifdef qcSafetyChecks
					bool pivoted = fTableau.Pivot(ci, varCoeffs.getIndex());
					qcAssert(pivoted);
					#else
					fTableau.Pivot(ci, varCoeffs.getIndex());
					#endif
					break;
				}
				varCoeffs.Increment();
			  }
			}
               #else
			// Cache tableau column coefficients since it is possible that
			// the iterator may be invalidated during iteration.
			{
			  QcTableauRowIterator varCoeffs(fTableau, ci);
			  vector<QcSparseCoeff> coeffCache;

			  while (!varCoeffs.AtEnd())
			    {
			      coeffCache.push_back( QcSparseCoeff( varCoeffs.getValue(),
								   varCoeffs.getIndex()));
			      varCoeffs.Increment();
			    }

			  for (vector<QcSparseCoeff>::iterator cIt = coeffCache.begin();
			       cIt != coeffCache.end();
			       cIt++)
			    {
			      QcSparseCoeff const &coeff = (*cIt);
			      unsigned const coeff_ix = coeff.getIndex();

			      bool reject = fTableau.IsConstrained( coeff_ix);		// ++++
			      for (viIt2 = fVarsByIndex.begin();
				   !reject && (viIt2 != fVarsByIndex.end());
				   ++viIt2)
				if (coeff_ix == *viIt2)
				  reject = true;

			      if (!reject)
				{
				  qcAssert( fTableau.IsBasic( vi));
				  qcAssert( fTableau.GetBasicVar( ci) == vi);
				  qcAssert( !fTableau.IsBasic( coeff_ix));
				  qcAssert( fVBiMap.Identifier( coeff_ix) != *voiIt);
				  dbg(bool pivoted =)
				    fTableau.Pivot(ci, coeff_ix);
				  assert( pivoted);
				  break;
				}
			    }
			}
               #endif
		}
	}

	Solve();		// Compute new solution here.

	// setup fParsOfInterest
	fParsOfInterest.resize(0);

	for (viIt = fVarsByIndex.begin(); viIt != fVarsByIndex.end(); ++viIt)
		if (!fTableau.IsBasic(*viIt) && !IsParOfInterest(*viIt))
			fParsOfInterest.push_back(*viIt);

	// setup fDepVars
	fDepVars.resize(0);

	// setup fDepVarCoeffs
	// extend fDepVarCoeffs so 0..fTableau.GetColumns()-1 are legal indices.
	fDepVarCoeffs.resize(fTableau.GetColumns());
	fDepVarBaseVals.resize(fTableau.GetColumns());

	for (unsigned int i = 0; i < fDepVarCoeffs.size(); i++)
		fDepVarCoeffs[i].resize(0);

	QcBasicVarIndexIterator bvIt(fTableau);
	while (!bvIt.AtEnd()) {
		unsigned bvi = bvIt.getIndex();
		qcAssert(fTableau.IsBasic(bvi));
		unsigned ci = bvIt.getConstraintBasicIn();
		numT rhs = fTableau.GetRHS (ci);
		fDepVarBaseVals[bvi] = rhs;
		QcTableauRowIterator varCoeffs(fTableau, ci);
		while (!varCoeffs.AtEnd()) {
			/* TODO: This code occurs elsewhere too (minus the dodgy formatting).
			   See if it can be merged to a routine. */
			if (IsParOfInterest (varCoeffs.getIndex())) {
				if (!IsDepVar(bvi)) fDepVars.push_back(bvi);
					fDepVarCoeffs[bvi].push_back(
						QcSparseCoeff (varCoeffs.getValue(),
							       varCoeffs.getIndex()));
			} else if (bvi != varCoeffs.getIndex()) {
				if (fTableau.IsStructural (varCoeffs.getIndex())) { // ++++
					numT val = fVBiMap.Identifier (varCoeffs.getIndex()).Value();
					fDepVarBaseVals[bvIt.getIndex()] -= val * varCoeffs.getValue();
				}
			}
			varCoeffs.Increment();
		}
		bvIt.Increment();
	}

	fEditVarsSetup = true;
}

bool QcLinInEqSystem::DualSimplexII()
{
	int n = 1;

	#ifdef qcSafetyChecks
	qcAssert(fTableau.IsBasicOptimalSolved());
	unsigned nRestrictedRows = fTableau.getNRestrictedRows();
	qcAssert( nRestrictedRows != 0);
	#endif

	// This loop contains 2 exits, a "return true" and a "return false".
	while (true) {
		#ifdef qcSafetyChecks
		qcAssert(nRestrictedRows == fTableau.getNRestrictedRows());

		for (int vi = 0; vi < fTableau.GetColumns(); vi++)
			if (fTableau.GetObjValue(vi) != 0.0) {
				qcAssert(!fTableau.IsBasic(vi));
				qcAssert(fTableau.IsConstrained(vi));
			}

		qcAssert(fTableau.IsBasicOptimalSolved());

		if (n == 1000)
			throw QcWarning(
				"QcLinInEqSystem::DualSimplexII appears to be cycling");
		#endif

		// May be better than inf loop
		if (++n == qcMaxLoopCycles)
			return false;

		// Find pivot row.
		int ei = SelectDualExitVar();

		// Feasible
		if (ei < 0)
			return true;

		#ifdef qcSafetyChecks
		qcAssert(fTableau.IsConstrained(fTableau.GetBasicVar(ei)));
		#endif

		// Select parameter to make basic
		int vi = SelectDualEntryVar(ei);

		if (vi < 0)
			return false;

		// Do a full pivot for the now.  Later we may choose to implement a
		// slack pivot which only eliminates the new basic slack variable
		// from the restricted portion of the tableau.  This would violate
		// some of the integrity constraints for lineqtableau - so more
		// thought is needed first (maybe use two separate tableaus).
		fEditVarsSetup = false;	// for when called by Resolve
		fTableau.SimplexPivot(ei, vi);
	}
}

bool QcLinInEqSystem::EndAddConstraint()
{
	QcSolver::EndAddConstraint();

	if (fBatchAddConstFail)
		return false;

	if (fArtVars.size() == 0)
		return true;
		
	// Set objective function based on the added artifical
	// variables, and perform Simplex phase II.

	// First, initialize the objective function.
	for (unsigned c = 0; c < fTableau.getNColumns(); c++)
	  fTableau.SetObjValue (c, 0.0);

	// Then, set the objective function base on the artifical
	// variables.
	vector<unsigned int>::iterator aIt = fArtVars.begin();

	while (aIt != fArtVars.end()) {
		QcFloat &v = fVBiMap.Identifier(*aIt);
		fTableau.SetObjValue((*aIt), v.Weight());
		aIt++;
	}

	fTableau.EliminateObjective();
	SimplexII();

	// Remove the added artifical variables from the tableau.
	aIt = fArtVars.begin();

	while (aIt != fArtVars.end()) {
		unsigned vi = (*aIt);

		if (!fTableau.IsBasic(vi))
			fTableau.RemoveVarix( vi);
		else {
			int r = fTableau.IsBasicIn (vi);
			int maxcol = -1;
			{
			  numT max = QcUtility::qcMaxZeroVal;
			  QcTableauRowIterator iR (fTableau, r);

			  while (!iR.AtEnd())
			    {
			      numT absVal = fabs (iR.getValue());
			      if ((max < absVal)
				  && (iR.getIndex() != vi))
				{
				  max = absVal;
				  maxcol = iR.getIndex();
				  assert (maxcol >= 0);
				}

			      iR.Increment();
			    }
			}

			// If there is at least one non-zero coefficient in the
			// current column.
			if (maxcol != -1) {
				fTableau.Pivot(r, maxcol);
				fTableau.RemoveVarix( vi);
			} else {
				if (QcUtility::IsZero(fTableau.GetRHS(r))) {
					fTableau.SetRowCondition(r, QcLinEqRowState::fRedundant);
					fTableau.RemoveVarix( vi);
				} else {
					fTableau.SetRowCondition(r, QcLinEqRowState::fInvalid);
				}
			}
		}
		aIt++;
	}
	return true;
}

void QcLinInEqSystem::EqSolve()
{
	#ifdef qcSafetyChecks
	if (!fTableau.IsBasicFeasibleSolved())
		throw QcWarning(qcPlace,
          	"tableau is not in basic feasible solved form");
	#endif
    
	// The following loop is equiavalent to LinEqSystem::LoadDesValCache()
	// N.B. QcLinEqTableau::EvalBasicVar needs caching of non-structural
	// variables hence QcLinInEqSystem::LoadDesValCache() will not do.
	QcVariableIndexIterator vIt(fTableau);

	while (!vIt.AtEnd()) {
		unsigned vi = vIt.getIndex();
		fTableau.SetDesireValue(vi,
         		fVBiMap.Identifier(vi).DesireValue());
		vIt.Increment();
	}

	vIt.Reset();

	while (!vIt.AtEnd()) {
		unsigned vi = vIt.getIndex();
		QcFloat &v = fVBiMap.Identifier(vi);

		if (fTableau.IsBasic(vi)) {
			numT value = fTableau.QcLinEqTableau::EvalBasicVar(vi);
			v.SetValue(QcUtility::Zeroise(value));
		} else
			v.SetVariable();

		vIt.Increment();
	}
}

void QcLinInEqSystem::InitObjective()
{
  // Build objective function SIGMA(x.weight * x) for structural
  // variables x.  Replace basic variables by their parametric
  // representation but don't include any non-slack parameters.
  QcStructVarIndexIterator vIt (fTableau);
  while (!vIt.AtEnd())
    {
      unsigned vi = vIt.getIndex();
      // TODO: The Java version has an isStructural test as well as isBasic.
      if (fTableau.IsBasic (vi))
	{
	  QcFloat &v = fVBiMap.Identifier (vi);
	  fTableau.SetObjValue (vi, v.Weight());
	}
      else
	fTableau.SetObjValue (vi, 0.0);
      vIt.Increment();
    }

  fTableau.EliminateObjective();

#ifdef qcSafetyChecks
  for (unsigned i = 0; i < fTableau.getNColumns(); i++)
    if (fTableau.GetObjValue (i) != 0.0)
      {
	qcAssert (!fTableau.IsBasic (i));
	qcAssert (fTableau.IsConstrained (i));
      }
#endif
}

void QcLinInEqSystem::LoadDesValCache()
{
  QcStructVarIndexIterator vIt (fTableau);
  while (!vIt.AtEnd())
    {
      unsigned vi = vIt.getIndex();
      fTableau.SetDesireValue (vi, fVBiMap.Identifier(vi).DesireValue());
      vIt.Increment();
    }
}

void QcLinInEqSystem::RawSolve()
{
	#ifdef qcSafetyChecks
	if (!fTableau.IsBasicFeasibleSolved())
		throw QcWarning(qcPlace,
         		"tableau is not in basic feasible solved form");
	#endif

	LoadDesValCache();
	QcStructVarIndexIterator vIt(fTableau);

	while (!vIt.AtEnd()) {
		unsigned vi = vIt.getIndex();
		QcFloat &v = fVBiMap.Identifier(vi);

		if (fTableau.IsBasic(vi)) {
			numT value = fTableau.EvalBasicVar(vi);
			v.SetValue(QcUtility::Zeroise(value));
		} else
			v.SetVariable();
		vIt.Increment();
	}
}

bool QcLinInEqSystem::Reset()
{
	bool result = true;

	// Record the columns of the previous basic variables so to make them
	// basic after this Reset() method.
	bool *rbasic = new bool[fTableau.GetColumns()];
	bool *urbasic = new bool[fTableau.GetColumns()];

	for (int c = 0; c < fTableau.GetColumns(); c++) {
		if (fTableau.IsBasic(c)) {
			if (fTableau.IsSlack(c)) {
				rbasic[c] = true;
				urbasic[c] = false;
			} else {
				rbasic[c] = false;
				urbasic[c] = true;
			}
		} else {
			rbasic[c] = false;
			urbasic[c] = false;
		}
	}

	fEditVarsSetup = false;
	fTableau.Restore(); // Reset variable state information

	// Establish solved form and quasi-inverse from original constraints.
	// Step 1: Pivot using unrestricted variables.
	for (int c = 0; c < fTableau.GetColumns(); c++) {
		if (urbasic[c]) {
			int maxrow = -1;
			{
			  /* TODO: This code (or similar) appears very often.
			     Find the cousins and merge them into a single routine. */
			  numT max = QcUtility::qcMaxZeroVal;
			  QcTableauColIterator iC (fTableau, c);

			  while (!iC.AtEnd())
			    {
			      numT absVal = fabs (iC.getValue());
			      if (max < absVal)
				{
				  max = absVal;
				  maxrow = iC.getIndex();
				  assert (maxrow >= 0);
				}

			      iC.Increment();
			    }
			}

			// If there is at least one non-zero coefficient in the current
			// column.
			if (maxrow != -1)
				fTableau.Pivot(maxrow, c);
		}
	}

	// Step 2: Make sure that all solved form constraints with unrestricted
	// variables are assigned a basic variable.
	for (int r = 0; r < fTableau.GetRows(); r++) {
		if (fTableau.GetRowCondition(r) != QcLinEqRowState::fRegular) {
			QcTableauRowIterator iR(fTableau, r);

			if (iR.AtEnd())
			  {
			    /* Verify that the Reset operation is successful.
			       It fails if one of more of the solved form row is
			       marked as "invalid". */
			    if (QcUtility::IsZero(fTableau.GetRHS(r)))
			      fTableau.SetRowCondition(r, QcLinEqRowState::fRedundant);
			    else
			      {
				fTableau.SetRowCondition(r, QcLinEqRowState::fInvalid);
				result = false;
			      }
			  }
			else
			  {
			    int maxcol = -1;
			    {
			      numT max = QcUtility::qcMaxZeroVal;
			      assert (!iR.AtEnd());
			      do
				{
				  if (!fTableau.IsSlack (iR.getIndex()))
				    {
				      numT absVal = fabs (iR.getValue());
				      if (max < absVal)
					{
					  max = absVal;
					  maxcol = iR.getIndex();
					  assert (maxcol >= 0);
					}
				    }

				  iR.Increment();
				}
			      while (!iR.AtEnd());
			    }

			    /* If there is at least one non-zero coefficient in
			       the current row. */
			    if (maxcol != -1)
			      fTableau.Pivot(r, maxcol);
			  }
		}
	}

	// Step 3: Pivot using restricted variables.
	for (int c = 0; c < fTableau.GetColumns(); c++) {
		if (rbasic[c]) {
			int maxrow = -1;
			{
			  numT max = QcUtility::qcMaxZeroVal;
			  QcTableauColIterator iC (fTableau, c);

			  while (!iC.AtEnd())
			    {
			      numT absVal = fabs (iC.getValue());
			      if (max < absVal)
				{
				  max = absVal;
				  maxrow = iC.getIndex();
				  assert (maxrow >= 0);
				}

			      iC.Increment();
			    }
			}

			// If there is at least one non-zero coefficient in the current
			// column.
			if (maxrow != -1)
				fTableau.Pivot(maxrow, c);
		}
	}

	// Step 4: Pivot on any variables to make all the solved form rows with 
	// a basic variables.
	for (unsigned r = 0; r < fTableau.getNRows(); r++)
	  {
	    if (fTableau.GetRowCondition (r) == QcLinEqRowState::fRegular)
	      continue;

	    int maxcol = -1;
	    {
	      numT max = QcUtility::qcMaxZeroVal;
	      QcTableauRowIterator iR (fTableau, r);
	      while (!iR.AtEnd())
		{
		  numT absVal = fabs (iR.getValue());
		  if (max < absVal)
		    {
		      max = absVal;
		      maxcol = iR.getIndex();
		      assert (maxcol >= 0);
		    }
		  iR.Increment();
		}
	    }

	    // If there is at least one non-zero coefficient in the current
	    // row.
	    if (maxcol != -1)
	      fTableau.Pivot (r, maxcol);
	    else
	      {
		/* Verify that the Reset operation is successful.  It fails if
		   one of more of the solved form row is marked as "invalid". */
		if (QcUtility::IsZero (fTableau.GetRHS (r)))
		  fTableau.SetRowCondition (r, QcLinEqRowState::fRedundant);
		else
		  {
		    fTableau.SetRowCondition (r, QcLinEqRowState::fInvalid);
		    result = false;
		  }
	      }
	  }

	// Step 5: For each restricted rows with negative RHS, add an artifical
	// variable with -1 coefficient.  This is followed by doing a pivot on it.
	vector<unsigned int> artVars;
	bool moreProcess = false;

	for (unsigned r = 0; r < fTableau.getNRows(); r++)
	  {
	    if (fTableau.IsRestricted (r)
		&& QcUtility::IsNegative (fTableau.GetRHS (r)))
	      {
		int vi = fTableau.AddArtificial (-1.0);
		assert (vi >= 0);
		artVars.push_back (vi);
		fTableau.Pivot (r, vi);
				// May scale row by -1 to make the newly added artifical
				// variable to +1.  This method may require explicit
				// management of column and row state.
		moreProcess = true;
	      }
	  }

	if (!moreProcess)
	  return result;

	// Step 6: Set objective function based on the added artifical variables,
	// and perform Simplex phase II.

	// First, initialize the objective function.
	for (unsigned c = 0; c < fTableau.getNColumns(); c++)
	  fTableau.SetObjValue (c, 0.0);

	// Then, set the objective function base on the artifical variables.
	vector<unsigned int>::iterator aIt = artVars.begin();

	while (aIt != artVars.end()) {
		QcFloat &v = fVBiMap.Identifier(*aIt);
		fTableau.SetObjValue((*aIt), v.Weight());
		aIt++;
	}

	fTableau.EliminateObjective();
	SimplexII();

	// Step 7: Remove the added artifical variables from the tableau.
	aIt = artVars.begin();

	while (aIt != artVars.end()) {
		unsigned vi = (*aIt);

		if (!fTableau.IsBasic(vi))
			fTableau.RemoveVarix( vi);
		else {
			int maxcol = -1;
			unsigned r = fTableau.IsBasicIn(vi);
			{
			  numT max = QcUtility::qcMaxZeroVal;
			  QcTableauRowIterator iR (fTableau, r);

			  while (!iR.AtEnd())
			    {
			      numT absVal = fabs (iR.getValue());
			      unsigned ix = iR.getIndex();
			      if ((max < absVal)
				  && (ix != vi))
				{
				  max = absVal;
				  maxcol = ix;
				  assert (maxcol >= 0);
				}

			      iR.Increment();
			    }
			}

			// If there is at least one non-zero coefficient in the current
			// column.
			if (maxcol != -1) {
				fTableau.Pivot(r, maxcol);
				fTableau.RemoveVarix( vi);
			} else {
				if (QcUtility::IsZero(fTableau.GetRHS(r))) {
					fTableau.SetRowCondition(r, QcLinEqRowState::fRedundant);
					fTableau.RemoveVarix( vi);
				} else {
					fTableau.SetRowCondition(r, QcLinEqRowState::fInvalid);
					result = false;
				}
			}
		}
		aIt++;
	}
	
	delete [] rbasic;	// no longer needed.
	delete [] urbasic;	// no longer needed.

	return result;
}


void QcLinInEqSystem::Resolve()
	// This is the same as LinEqSystem::Resolve except where noted with the
	// comment ++++.
{
  if (!fEditVarsSetup)
    BeginEdit();

#ifdef qcSafetyChecks
  if (!fTableau.IsBasicFeasibleSolved())
    throw QcWarning("QcLinInEqSystem::Resolve: tableau is not in basic feasible solved form");
#endif

  // This paragraph not in LinEqSystem::Resolve ++++
  if (fTableau.getNRestrictedRows() != 0)
    {
      // Perform second phase of revised simplex algorithm on restricted
      // portion of the tableau.
      InitObjective();

      if (SimplexII())
	qcAssert( fTableau.IsBasicOptimal());
    }

  // Adjust the dependent basic variables.
  for (vector<unsigned int>::iterator vIt = fDepVars.begin();
       vIt != fDepVars.end();
       ++vIt)
    {
      QcFloatRep *dvr = fVBiMap.getIdentifierPtr( *vIt);
      qcAssert( fTableau.IsBasic( *vIt));
      vector<QcSparseCoeff> &Coeffs = fDepVarCoeffs[*vIt];
      numT dvValue = fDepVarBaseVals[*vIt];

      for (vector<QcSparseCoeff>::iterator pIt = Coeffs.begin();
	   pIt != Coeffs.end();
	   ++pIt)
	{
	  QcFloatRep const *pr = fVBiMap.getIdentifierPtr( pIt->getIndex());
	  dvValue -= (*pIt).getValue() * pr->DesireValue();
	}

      dvr->SetValue( QcUtility::Zeroise( dvValue));
    }

  // Adjust the parameters of interest.
  for (vector<unsigned int>::iterator vIt = fParsOfInterest.begin();
       vIt != fParsOfInterest.end();
       ++vIt)
    fVBiMap.getIdentifierPtr( *vIt)->SetToGoal();
}


void QcLinInEqSystem::RestSolver()
{
	QcStructVarIndexIterator vIt(fTableau);

	while (!vIt.AtEnd()) {
		fVBiMap.Identifier(vIt.getIndex()).RestVariable();
		vIt.Increment();
	}
}

int QcLinInEqSystem::SelectEntryVar()
	// Returns parameter for SimplexII to make basic.
	// Or returns qcInvalidVIndex if there are no candidates.
	// Any constrained parameter with negative objective coefficient will
	// do.  But to implement Bland's anti-cycling rule, we choose the
	// parameter with the lowest index.
{
	QcParamVarIndexIterator vIt(fTableau);

	while (!vIt.AtEnd()) {
		unsigned vi = vIt.getIndex();
		if (fTableau.IsConstrained(vi)) {
			numT objCoeff = fTableau.GetObjValue(vi);
			if (QcUtility::IsNegative(objCoeff))
				return vi;
		}
		vIt.Increment();
	}

	return QcTableau::fInvalidVariableIndex;
}

int QcLinInEqSystem::SelectExitVar (unsigned vi)
{
	qcAssertPre (vi < fTableau.getNColumns());

	int ei = QcTableau::fInvalidConstraintIndex;
	numT minval = DBL_MAX;
	int bi = fTableau.GetColumns();		// Bland Index

	#ifdef qcSafetyChecks
	qcAssert(!fTableau.IsBasic(vi) && fTableau.IsConstrained(vi));
	#endif

	QcTableauColIterator colCoeffs (fTableau, vi);
	while (!colCoeffs.AtEnd()) {
		qcAssert (fTableau.GetBasicVar (colCoeffs.getIndex()) != (int) vi);
		if (fTableau.IsRestricted (colCoeffs.getIndex())) {
			numT rhs = fTableau.GetRHS (colCoeffs.getIndex());
			if (CannotIncrease (colCoeffs.getValue(), rhs)) {
				numT coeff = rhs / colCoeffs.getValue();
				if ((coeff < minval)
				    || ((coeff == minval)
					&& (fTableau.GetBasicVar (colCoeffs.getIndex())
					    < bi)))
				  {
				    minval = coeff;
				    ei = colCoeffs.getIndex();
				    bi = fTableau.GetBasicVar (ei);
				    qcAssert (!QcUtility::IsNegative (minval));
				  }
			}
		}
		colCoeffs.Increment();
	}

	#ifdef qcSafetyChecks
	qcAssert(fTableau.IsBasicFeasibleSolved());
	if (ei < 0)
    		throw QcWarning("SelectExitVar: No suitable variable");
	#endif

	return ei;
}

int QcLinInEqSystem::SelectDualEntryVar(int ei)
{
	int vi = QcTableau::fInvalidVariableIndex;
	numT maxval = -DBL_MAX;

	qcAssert(fTableau.IsRestricted(ei));

	QcTableauRowIterator varCoeffs(fTableau,ei);

	while (!varCoeffs.AtEnd()) {
		if (fTableau.IsConstrained (varCoeffs.getIndex())) {
			if (QcUtility::IsNegative(varCoeffs.getValue())) {
				numT coeff = (fTableau.GetObjValue (varCoeffs.getIndex())
						/ varCoeffs.getValue());
				if (coeff > maxval) {
					maxval = coeff;
					vi = varCoeffs.getIndex();
				}
			}
		}
		varCoeffs.Increment();
	}

	#ifdef qcSafetyChecks
	if (vi == QcTableau::fInvalidVariableIndex)
		throw QcWarning("SelectDualEntryVar: No suitable variable");
	#endif

	return vi;
}

int QcLinInEqSystem::SelectDualExitVar()
	// Select the basic variable to leave the basis so that optimality
	// is maintained (in the narrow sense - feasibility is not assumed).
	// And feasibility is approached.
	// Returns qcInvalidCIndex if no such pivot is possible (i.e. optimal feasible).
{
	numT minval = 0.0;
	int ei = QcTableau::fInvalidConstraintIndex;	    // Result constraint index

	// Find the the constrainted basic with the most negative value.
	QcBasicVarIndexIterator bvIt(fTableau);
	while (!bvIt.AtEnd()) {
		unsigned bvi = bvIt.getIndex();
		if (fTableau.IsConstrained(bvi)) {
			int ci = fTableau.IsBasicIn(bvi);
			numT rhs = fTableau.GetRHS(ci);

			if (QcUtility::IsNegative(rhs) && (rhs < minval)) {
				minval = rhs;
				ei = ci;
			}
		}
		bvIt.Increment();
	}

	qcAssert(fTableau.IsBasicOptimalSolved());
	return ei;
}

bool QcLinInEqSystem::SimplexI (unsigned cs)
{
  qcAssertPre (cs < fTableau.getNRows());

  unsigned nIterationsDone = 0;
  unsigned ei;
  fTableau.Normalize (cs);

  do
    {
      numT rhs = fTableau.GetRHS (cs);

      // May be better than inf loop
      if (nIterationsDone++ == qcMaxLoopCycles)
	{
#ifdef qcSafetyChecks
	  throw QcWarning(
			  "QcLinInEqSystem::SimplexI appears to be cycling");
#endif
	  return false;
	}

      // Find parameter to make basic (any with positive coefficient)
      unsigned vi; // Index of variable to become basic.
      numT val;
      bool found = false;
      for (QcTableauRowIterator varCoeffs (fTableau, cs);
	   !varCoeffs.AtEnd();
	   varCoeffs.Increment())
	{
	  vi = varCoeffs.getIndex();
	  val = varCoeffs.getValue();

	  if (!fTableau.IsBasic (vi)
	      && fTableau.IsConstrained (vi)
	      && QcUtility::IsPositive (val))
	    {
	      found = true;
	      break;
	    }
	}

      // New constraint is inconsistant.
      if (!found)
	return false;

      // Choose basic variable to make parametric (ei)
      ei = cs;		// The prefered row to choose from.
      numT minval = rhs / val;
      QcTableauColIterator colCoeffs(fTableau, vi);
      while (!colCoeffs.AtEnd())
	{
	  numT rhs2 = fTableau.GetRHS (colCoeffs.getIndex());
	  numT minval2 = rhs2 / colCoeffs.getValue();
	  if (QcUtility::IsPositive (colCoeffs.getValue())
	      && fTableau.IsRestricted (colCoeffs.getIndex())
	      && ((minval2 < minval)
		  || ((minval2 == minval)
		      && (colCoeffs.getIndex() != cs)
		      && (fTableau.GetBasicVar (colCoeffs.getIndex())
			  < fTableau.GetBasicVar (ei)))))
	    {
	      minval = minval2;
	      ei = colCoeffs.getIndex();
	    }
	  colCoeffs.Increment();
	}

#ifdef qcSafetyChecks
      bool pivoted =
#endif
	fTableau.Pivot (ei, vi);
      qcAssert (pivoted);
    }
  while (ei != cs);

  qcAssert (fTableau.IsBasicFeasibleSolved());
  return true;
}

bool QcLinInEqSystem::SimplexII()
	// Perform simplex optimisation on the restricted rows of the tableau.
	// Should only be called if there are some restricted rows.
	// The objective function should be initialised before this call.
	// Bland's anti-cycling rule is implemented and depends upon tableau
	// column iterators providing coeffs in order of increasing
	// variable index.
{
	int n = 1;

	/* FIXME: These "safety checks" can fail (e.g. TestCassSolver.cc line
	   111, after AddConstraint), yet still produce the right answer (TODO:
	   actually, haven't checked this).

	   In the abovementioned example, IsBasicFeasible fails. */
	#if qcSafetyChecks
	qcAssert(fTableau.IsBasicFeasibleSolved());
	unsigned const nRestrictedRows = fTableau.getNRestrictedRows();
	qcAssert(nRestrictedRows != 0);
	#endif

	// This loop contains 2 exits, a "return true" and a "return false".
	while (true) {
		#if 0 && qcSafetyChecks
		qcAssert(nRestrictedRows == fTableau.getNRestrictedRows());

		for (int vi=0; vi < fTableau.GetColumns(); vi++)
			if (fTableau.GetObjValue(vi)!=0.0) {
				qcAssert(!fTableau.IsBasic(vi));
				qcAssert(fTableau.IsConstrained(vi));
			}

			qcAssert(fTableau.IsBasicFeasibleSolved());

			if (n == 1000)
				throw QcWarning(
                    	"QcLinInEqSystem::SimplexII appears to be cycling");
		#endif

		// may be better than inf loop
		if (++n == qcMaxLoopCycles)
			return false;

		// Find parameter to make basic.
		int vi = SelectEntryVar();

		// All objective coeffs are non-negative
		if (vi < 0)
			return true;

		// Choose row index of basic slack variable to make parametric (ei)
		int ei = SelectExitVar(vi);

		if (ei < 0)
			return false;

		#ifdef qcSafetyChecks
		qcAssert(fTableau.IsConstrained(fTableau.GetBasicVar(ei)));
		#endif

		// Do a full pivot for the now.  Later we may choose to implement a
		// slack pivot which only eliminates the new basic slack variable
		// from the restricted portion of the tableau.  This would violate
		// some of the integrity constraints for lineqtableau - so more
		// thought is needed first (maybe use two separate tableaus).
		fEditVarsSetup = false;	// for when called by Resolve

		#ifdef qcSafetyChecks
		qcAssert(fTableau.SimplexPivot(ei, vi));
		#else
		fTableau.SimplexPivot(ei, vi);
		#endif
	}
}

void QcLinInEqSystem::Solve()
{
	#ifdef qcSafetyChecks
	if (!fTableau.IsBasicFeasibleSolved())
		throw QcWarning(
          	"QcLinInEqSystem::Solve: tableau is not in basic feasible solved form");
	#endif

	if (fTableau.getNRestrictedRows() != 0) {
		// Perform second phase of revised simplex algorithm on restricted
		// portion of the tableau.
		InitObjective();

		if (SimplexII())
			qcAssert(fTableau.IsBasicOptimal());
	}

	qcAssert(fTableau.IsSolved());
	qcAssert(fTableau.IsBasicFeasible());

	// Calculate the value for each variable depending upon
	// whether it is basic or parametric.
	RawSolve();
}
