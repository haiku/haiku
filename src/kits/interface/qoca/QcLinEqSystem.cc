// $Id: QcLinEqSystem.cc,v 1.14 2001/01/31 07:37:13 pmoulder Exp $

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

#include <math.h>
#include "qoca/QcDefines.hh"
#include "qoca/QcLinEqSystem.hh"
#include "qoca/QcRowAdaptor.hh"
#include "qoca/QcTableauRowIterator.hh"
#include "qoca/QcTableauColIterator.hh"
#include "qoca/QcBasicVarIndexIterator.hh"
#include "qoca/QcVariableIndexIterator.hh"
#include "qoca/QcConstraintIndexIterator.hh"
#include "qoca/QcDenseTableauRowIterator.hh"

#if qcCheckInternalInvar
void QcLinEqSystem::assertDeepInvar() const
{
	QcSolver::assertDeepInvar();
	fVBiMap.vAssertDeepInvar();
	fNotifier.vAssertDeepInvar();
	fOCBiMap.vAssertDeepInvar();
	fTableau.vAssertDeepInvar();
}

void QcLinEqSystem::vAssertDeepInvar() const
{
  assertDeepInvar();
}
#endif


bool QcLinEqSystem::AddConstraint(QcConstraint &c)
{
	return doAddConstraint(c, -1);
}

bool QcLinEqSystem::AddConstraint(QcConstraint &c, QcFloat &hint)
{
#if qcCheckPre
	int hintIx = fVBiMap.safeIndex(hint);
	qcAssertExternalPre(hintIx >= 0);
#else
	int hintIx = fVBiMap.Index(hint);
#endif

	return doAddConstraint(c, hintIx);
}


bool QcLinEqSystem::doAddConstraint(QcConstraint &c, int hintIx)
{
	qcAssertExternalPre(c.getRelation() == QcConstraintRep::coEQ);

	if (fBatchAddConst && fBatchAddConstFail)
		return false;

	QcRowAdaptor ra(c.LinPoly(), fVBiMap, fTableau);
	bool success = true;

	int co = fTableau.AddEq(ra, c.RHS());    // co is original constraint index
	int cs = fTableau.Eliminate(co);         // cs is solved form constraint index

	if (hintIx >= 0) {
		fTableau.fPivotHints.clear();
		fTableau.fPivotHints.push_back(hintIx);
	}

	if (cs == QcTableau::fInvalidConstraintIndex) {
		fInconsistant.push_back(c);
		success = false;
	} else if (!fTableau.IsRedundant(cs))
		ApplyHints(cs);

	if (success) {
		fOCBiMap.Update(c, co);
		fBatchConstraints.push_back(c);
	} else if (fBatchAddConst)
		fBatchAddConstFail = true;

	fEditVarsSetup = false;

	dbg(assertInvar());
	return success;
}

bool QcLinEqSystem::ApplyHints(int cs)
{
	bool pivoted = false;

	if (cs < 0 || cs >= fTableau.GetRows() || fTableau.IsRedundant(cs))
		throw QcWarning("ApplyHints called for invalid constraint",
			cs, fTableau.GetRows() - 1);
	else {
		for (unsigned int i = 0; i < fTableau.fPivotHints.size() && !pivoted; i++)
			pivoted = fTableau.Pivot(cs,fTableau.fPivotHints[i]);

		// If PivotHints failed, this means all the pivot hints variables
		// have zero coeffs.  Now try all the rest of the parametric
		// variables with non-zero coeffs in the solved form constraint.
		if (!pivoted)
		  {
#ifdef qcUseSafeIterator
		    {
		      QcDenseTableauRowIterator varCoeffs(fTableau, cs);

		      // Note this iterator will not return zero coeffs, so
		      // there is no need to exclude the hints.
		      while (!varCoeffs.AtEnd()) {
			if (!fTableau.IsBasic(varCoeffs.getIndex()))
			  pivoted = fTableau.Pivot(cs, varCoeffs.getIndex());
			varCoeffs.Increment();
		      }
		    }
#else
		    // Cache tableau column coefficients since it is possible that
		    // the iterator may be invalidated (by Pivot) during iteration.
		    {
		      vector<QcSparseCoeff> coeffCache;

		      for(QcTableauRowIterator rowIt( fTableau, cs);
			  !rowIt.AtEnd();
			  rowIt.Increment())
			coeffCache.push_back( QcSparseCoeff( rowIt.getValue(),
							     rowIt.getIndex()));

		      /* Note that the iterator above will not return zero
			 coeffs, so there is no need to exclude the hints. */
		      for (vector<QcSparseCoeff>::iterator cIt = coeffCache.begin();
			   (cIt != coeffCache.end()) && !pivoted;
			   cIt++)
			{
			  unsigned cix = cIt->getIndex();

			  if (!fTableau.IsBasic( cix))
			    pivoted = fTableau.Pivot( cs, cix);
			}
		    }
#endif
		  }
		qcAssert(pivoted);
	}

	return true;
}

void QcLinEqSystem::BeginEdit()
{
  qcAssert(fTableau.IsSolved());

  // build a version of fEditVars by index.
  vector<QcFloat>::iterator voiIt;
  vector<unsigned int>::iterator viIt;
  fVarsByIndex.resize(0);

  for (voiIt = fEditVars.begin(); voiIt != fEditVars.end(); ++voiIt)
    if (IsRegistered( *voiIt))
      fVarsByIndex.push_back( fVBiMap.Index( *voiIt));

  // try to make all variables of interest into parameters.
  for (viIt = fVarsByIndex.begin();
       viIt != fVarsByIndex.end();
       ++viIt)
    {
      int const vi = *viIt;
      if (fTableau.IsBasic( vi)) {
	int const ci = fTableau.IsBasicIn(vi);

#ifdef qcUseSafeIterator

	for (QcDenseTableauRowIterator varCoeffs( fTableau, ci);
	     !varCoeffs.AtEnd();
	     varCoeffs.Increment())
	  {
	    unsigned const coeff_ix = varCoeffs.getIndex();

	    bool reject = false;
	    for (vector<unsigned int>::const_iterator viIt2 = fVarsByIndex.begin();
		 !reject && (viIt2 != fVarsByIndex.end());
		 viIt2++)
	      if (coeff_ix == *viIt2)
		reject = true;

	    if (!reject)
	      {
		qcAssert( fTableau.IsBasic( vi));
		qcAssert( fTableau.GetBasicVar( ci) == vi);
		qcAssert( !fTableau.IsBasic( coeff_ix));
		qcAssert( fVBiMap.Identifier( coeff_ix)
			  != *voiIt); // TODO: looks wrong; perhaps meant `coeff_ix != *viIt'?
		dbg(bool pivoted =)
		  fTableau.Pivot( ci, coeff_ix);
		assert( pivoted);
		break;
	      }
	  }
#else
	// Cache tableau column coefficients since it is possible that
	// the iterator may be invalidated during iteration.
	{
	  vector<QcSparseCoeff> coeffCache;

	  for (QcTableauRowIterator varCoeffs( fTableau, ci);
	       !varCoeffs.AtEnd();
	       varCoeffs.Increment())
	    coeffCache.push_back( QcSparseCoeff( varCoeffs.getValue(),
						 varCoeffs.getIndex()));

	  for (vector<QcSparseCoeff>::iterator cIt = coeffCache.begin();
	       cIt != coeffCache.end();
	       cIt++)
	    {
	      QcSparseCoeff &coeff = (*cIt);
	      unsigned const coeff_ix = coeff.getIndex();

	      bool reject = false;
	      for (vector<unsigned int>::const_iterator viIt2 = fVarsByIndex.begin();
		   !reject && (viIt2 != fVarsByIndex.end());
		   viIt2++)
		if (coeff_ix == *viIt2)
		  reject = true;

	      if (!reject)
		{
		  qcAssert( fTableau.IsBasic( vi));
		  qcAssert( fTableau.GetBasicVar( ci) == vi);
		  qcAssert( !fTableau.IsBasic( coeff_ix));
		  qcAssert( fVBiMap.Identifier( coeff_ix) != *voiIt); // TODO: looks wrong; perhaps meant `coeff_ix != *viIt'?

		  dbg(bool pivoted =)
		    fTableau.Pivot(ci, coeff_ix);
		  assert( pivoted);
		  break;
		}
	    }
	}
#endif
	qcAssert( !fTableau.IsBasic( vi));
      }
    }

  Solve();					// Compute new solution here.
  fParsOfInterest.resize(0); 	// setup fParsOfInterest

  for (viIt = fVarsByIndex.begin(); viIt != fVarsByIndex.end(); ++viIt)
    if (!fTableau.IsBasic(*viIt) && !IsParOfInterest(*viIt))
      fParsOfInterest.push_back(*viIt);

  fDepVars.resize(0);		// setup fDepVars

  // setup fDepVarCoeffs
  // extend fDepVarCoeffs so 0..fTableau.GetColumns()-1 are legal indices.
  fDepVarCoeffs.resize(fTableau.GetColumns());
  fDepVarBaseVals.resize(fTableau.GetColumns());

  for (unsigned int i = 0; i < fDepVarCoeffs.size(); i++)
    fDepVarCoeffs[i].resize(0);

  QcBasicVarIndexIterator bvIt(fTableau);
  while (!bvIt.AtEnd())
    {
      unsigned bvi = bvIt.getIndex();
      assert (fTableau.IsBasic (bvi));
      unsigned ci = bvIt.getConstraintBasicIn();
      fDepVarBaseVals[bvi] = fTableau.GetRHS (ci);;
      QcTableauRowIterator varCoeffs (fTableau, ci);
      while (!varCoeffs.AtEnd())
	{
	  if (IsParOfInterest (varCoeffs.getIndex()))
	    {
	      /* TODO: Check this logic here.  The previous formatting
		 gave the impression that the author intended something
		 else. */
	      if (!IsDepVar (bvi))
		fDepVars.push_back (bvi);
	      fDepVarCoeffs[bvi].push_back (QcSparseCoeff (varCoeffs.getValue(),
							   varCoeffs.getIndex()));
	    }
	  else if (bvi != varCoeffs.getIndex())
	    {
	      numT val = fVBiMap.getIdentifierPtr( varCoeffs.getIndex())->Value();
	      fDepVarBaseVals[bvi] -= val * varCoeffs.getValue();
	    }
	  varCoeffs.Increment();
	}

      bvIt.Increment();
    }

  fEditVarsSetup = true;
}


void QcLinEqSystem::EndEdit()
{
  for (vector<unsigned int>::const_iterator vIt = fDepVars.begin(); vIt != fDepVars.end(); ++vIt)
    fVBiMap.getIdentifierPtr( *vIt)->RestDesVal();

  for (vector<unsigned int>::const_iterator vIt = fParsOfInterest.begin(); vIt != fParsOfInterest.end(); ++vIt)
    fVBiMap.getIdentifierPtr( *vIt)->RestDesVal();

  QcSolver::EndEdit();
}

bool QcLinEqSystem::IsDepVar(int vi) const
{
	if (vi < 0) /* TODO: We can probably assertPre (vi > 0).  Similarly for other IsDepPar implementations. */
		return false;

	vector<unsigned int>::const_iterator loc = fDepVars.begin();
	while (loc != fDepVars.end()) {
		if (*loc == (unsigned) vi)
         		return true;
		++loc;
	}

	return false;
}

bool QcLinEqSystem::IsParOfInterest(int vi) const
{
	if (vi < 0) /* TODO: We can probably assertPre (vi > 0).  Similarly for other IsParOfInterest implementations. */
		return false;

	vector<unsigned int>::const_iterator loc = fParsOfInterest.begin();
	while (loc != fParsOfInterest.end()) {
		if (*loc == (unsigned) vi)
			return true;
		++loc;
	}

	return false;
}


void QcLinEqSystem::LoadDesValCache()
{
  for (QcVariableIndexIterator vIt( fTableau);
       !vIt.AtEnd();
       vIt.Increment())
    {
      unsigned const vi = vIt.getIndex();
      fTableau.SetDesireValue( vi, fVBiMap.getIdentifierPtr( vi)->DesireValue());
    }
}


bool QcLinEqSystem::Reset()
{
  bool result = true;

  /* Record the columns of the previous basic variables so to make them
     basic after this Reset() method. */
  bool *basic = new bool[fTableau.getNColumns()];

  for (unsigned c = 0; c < fTableau.getNColumns(); c++) 
    basic[c] = fTableau.IsBasic(c);

  fEditVarsSetup = false;
  fTableau.Restore(); // Reset variable state information

  // Establish solved form and quasi-inverse from original constraints.
  for (unsigned c = 0; c < fTableau.getNColumns(); c++)
    {
      if (!basic[c])
	continue;

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
	      }

	    iC.Increment();
	  }
	assert( (maxrow < 0) == (QcUtility::IsZero( max)));
      }

      // If there is at least one non-zero coefficient in the current
      // column.
      if (maxrow != -1)
	fTableau.Pivot (maxrow, c);

    }

  delete [] basic;	// no longer need.

  // Make sure that all solved form constraints are assigned a basic variable.
  for (unsigned r = 0; r < fTableau.getNRows(); r++)
    {
      if (fTableau.GetRowCondition (r) != QcLinEqRowState::fNormalised)
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
      // column.
      if (maxcol != -1)
	fTableau.Pivot (r, maxcol);
      else
	{
	  // Verify that the Reset operation is successful.  It fails
	  // if one of more of the solved form row is marked as
	  // "invalid".
	  if (QcUtility::IsZero (fTableau.GetRHS (r)))
	    fTableau.SetRowCondition (r, QcLinEqRowState::fRedundant);
	  else
	    {
	      fTableau.SetRowCondition (r, QcLinEqRowState::fInvalid);
	      result = false;
	    }
	}
    }

  dbg(assertInvar());
  return result;
}

void QcLinEqSystem::Resolve()
{
  if (!fEditVarsSetup)
    BeginEdit();

  // adjust the dependent basic variables
  vector<unsigned int>::const_iterator vIt;
  vector<QcSparseCoeff>::iterator pIt;

  for (vIt = fDepVars.begin(); vIt != fDepVars.end(); ++vIt)
    {
      QcFloatRep *dvr = fVBiMap.getIdentifierPtr( *vIt);
      qcAssert(fTableau.IsBasic(*vIt));
      vector<QcSparseCoeff> &Coeffs = fDepVarCoeffs[*vIt];
      numT dvValue = fDepVarBaseVals[*vIt];

      for (pIt = Coeffs.begin(); pIt != Coeffs.end(); ++pIt)
	{
	  QcFloatRep const *pr = fVBiMap.getIdentifierPtr( (*pIt).getIndex());
	  dvValue -= (*pIt).getValue() * pr->DesireValue();
	}

      dvr->SetValue( QcUtility::Zeroise( dvValue));
    }

  // Adjust the parameters of interest
  for (vIt = fParsOfInterest.begin(); vIt != fParsOfInterest.end(); ++vIt)
    fVBiMap.getIdentifierPtr( *vIt)->SetToGoal();
}


void QcLinEqSystem::Restart()
{
  QcSolver::Restart();
  fTableau.Restart();
  fNotifier.Restart();
  fDepVars.resize(0);
  fParsOfInterest.resize(0);
  fDepVarCoeffs.resize(0);
}


void QcLinEqSystem::RestSolver()
{
  for (QcVariableIndexIterator vIt( fTableau);
       !vIt.AtEnd();
       vIt.Increment())
    fVBiMap.getIdentifierPtr( vIt.getIndex())->RestDesVal();
}


void QcLinEqSystem::Solve()
{
  LoadDesValCache();

  for (QcVariableIndexIterator vIt( fTableau);
       !vIt.AtEnd();
       vIt.Increment())
    {
      unsigned const vi = vIt.getIndex();
      QcFloatRep *vr = fVBiMap.getIdentifierPtr( vi);

      if (fTableau.IsBasic( vi))
	{
	  numT value = fTableau.EvalBasicVar( vi);
	  vr->SetValue( QcUtility::Zeroise( value));
	}
      else
	vr->SetToGoal();
    }
}
