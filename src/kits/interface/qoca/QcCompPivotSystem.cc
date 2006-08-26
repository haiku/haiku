// $Id: QcCompPivotSystem.cc,v 1.11 2001/01/31 04:53:11 pmoulder Exp $

//============================================================================//
// Written by Sitt Sen Chok                                                   //
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
#include "qoca/QcCompPivotSystem.hh"
#include "qoca/QcStructVarIndexIterator.hh"
#include "qoca/QcTableauColIterator.hh"
#include "qoca/QcTableauRowIterator.hh"

bool QcCompPivotSystem::ApplyHints(int cs)
{
	bool pivoted = false;

	if (cs < 0 || cs >= fTableau.GetRows() || fTableau.IsRedundant(cs))
		throw QcWarning("ApplyHints called for invalid constraint",
			cs, fTableau.GetRows() - 1);
	else {
		for (unsigned int i = 0; i < fTableau.fPivotHints.size() && !pivoted;
          		i++) {
			int hint = fTableau.fPivotHints[i];
			if (!fTableau.IsConstrained(hint))
				pivoted = fTableau.Pivot(cs, hint);
         }
	}

	return pivoted;
}

bool QcCompPivotSystem::CopyConstraint(QcConstraint &c, int bvi)
{
	int ci = AddToTableau(c);

	if (ci == QcTableau::fInvalidConstraintIndex)
		return false;
	else {
		fOCBiMap.Update(c, ci);
	    	fTableau.SetRowState(bvi, ci, QcLinEqRowState::fRegular);
	}

	return true;
}

bool QcCompPivotSystem::CopyInEq(QcConstraint &c)
{
	int ci = AddToTableau(c);

	if (ci == QcTableau::fInvalidConstraintIndex)
    		return false;
	else {
    		fOCBiMap.Update(c, ci);
	    	fTableau.SetRowState(fTableau.GetColumns() - 1, ci,
         		QcLinEqRowState::fRegular);
	}

	fEditVarsSetup = false;
	return true;
}

void QcCompPivotSystem::EqRawSolve()
{
#ifdef qcSafetyChecks
  if (!fTableau.IsBasicFeasibleSolved())
    throw QcWarning(qcPlace,
		    "Tableau is not in basic feasible solved form");
#endif

  QcStructVarIndexIterator vIt(fTableau);
  while (!vIt.AtEnd())
    {
      unsigned vi = vIt.getIndex();
      int ci = fTableau.IsBasicIn (vi);
      if ((ci >= fTableau.GetBeginEq())
	  && (ci != QcTableau::fInvalidConstraintIndex))
	{
	  numT value = EvalBasicVar (vi);
	  QcFloat &v = fVBiMap.Identifier (vi);
	  v.SetValue (QcUtility::Zeroise (value));
	}
      vIt.Increment();
    }
}

void QcCompPivotSystem::EqRawSolveComplex()
{
#ifdef qcSafetyChecks
  if (!fTableau.IsBasicFeasibleSolved())
    throw QcWarning(qcPlace,
		    "Tableau is not in basic feasible solved form");
#endif

  QcStructVarIndexIterator vIt (fTableau);
  while (!vIt.AtEnd())
    {
      unsigned vi = vIt.getIndex();
      int ci = fTableau.IsBasicIn (vi);
      if ((ci >= fTableau.GetBeginEq())
	  && (ci != QcTableau::fInvalidConstraintIndex))
	{
	  numT value = EvalBasicVarComplex (vi);
	  QcFloat v = fVBiMap.Identifier (vi);
	  v.SetValue (QcUtility::Zeroise (value));
	}
      vIt.Increment();
    }
}

numT
QcCompPivotSystem::EvalBasicVar(unsigned vi)
{
  qcAssertPre( IsBasic( fVBiMap.Identifier( vi)));

  int ci = fTableau.IsBasicIn( vi);
  assert( ci != QcTableau::fInvalidConstraintIndex);
  numT value = fTableau.GetRHS(ci); // rhs sign is not critical here

  for (QcTableauRowIterator varCoeffs( fTableau, ci);
       !varCoeffs.AtEnd();
       varCoeffs.Increment())
    {
      unsigned coeff_vi = varCoeffs.getIndex();
      if ((coeff_vi != vi)
	  && (fTableau.IsStructural( coeff_vi)))
	{
	  QcFloatRep *vr = fVBiMap.getIdentifierPtr( coeff_vi);
	  value -= varCoeffs.getValue() * vr->Value();
	}
    }

  return value;
}

numT QcCompPivotSystem::EvalBasicVarComplex(unsigned vi)
{
  qcAssertPre( IsBasic( fVBiMap.Identifier( vi)));

  int ci = fTableau.IsBasicIn( vi);
  assert( ci != QcTableau::fInvalidConstraintIndex);

  numT value = fTableau.GetComplexRHS( ci); // rhs sign is not critical here

  for (QcTableauRowIterator varCoeffs( fTableau, ci);
       !varCoeffs.AtEnd();
       varCoeffs.Increment())
    {
      unsigned coeff_vi = varCoeffs.getIndex();
      if ((coeff_vi != vi)
	  && (fTableau.IsStructural( coeff_vi)))
	{
	  QcFloatRep *vr = fVBiMap.getIdentifierPtr( coeff_vi);
	  value -= varCoeffs.GetValue() * vr->Value();
	}
    }

  return value;
}

void QcCompPivotSystem::InEqRawSolve()
{
#ifdef qcSafetyChecks
  if (!fTableau.IsBasicFeasibleSolved())
    throw QcWarning(qcPlace,
		    "Tableau is not in basic feasible solved form");
#endif

  QcStructVarIndexIterator vIt (fTableau);
  while (!vIt.AtEnd())
    {
      unsigned vi = vIt.getIndex();
      int ci = fTableau.IsBasicIn (vi);

      /* TODO: What happens if GetBeginEq < 0?
	 Should `!= fInvalidConstraintIndex' be tested first? */
      if (ci < fTableau.GetBeginEq())
	{
	  QcFloat &v = fVBiMap.Identifier (vi);

	  if (ci != QcTableau::fInvalidConstraintIndex)
	    {
	      numT value = fTableau.GetRHS (ci);
	      v.SetValue (QcUtility::Zeroise (value));
	    }
	  else
	    v.SetVariable();
	}
      vIt.Increment();
    }
}

void QcCompPivotSystem::InEqRawSolveComplex()
{
#ifdef qcSafetyChecks
  if (!fTableau.IsBasicFeasibleSolved())
    throw QcWarning(qcPlace,
		    "Tableau is not in basic feasible solved form");
#endif

  QcStructVarIndexIterator vIt (fTableau);
  while (!vIt.AtEnd())
    {
      unsigned vi = vIt.getIndex();
      int ci = fTableau.IsBasicIn (vi);

      if (ci < fTableau.GetBeginEq())
	{
	  QcFloat &v = fVBiMap.Identifier (vi);

	  if (ci != QcTableau::fInvalidConstraintIndex)
	    {
	      numT value = fTableau.GetComplexRHS (ci);
	      v.SetValue (QcUtility::Zeroise (value));
	    }
	  else
	    v.SetVariable();
	}
      vIt.Increment();
    }
}

int QcCompPivotSystem::SelectExitVar (unsigned vi)
{
  qcAssertPre( vi < fTableau.getNColumns());

  int ei = QcTableau::fInvalidConstraintIndex;
  numT minval = DBL_MAX;
  int bi = fTableau.GetColumns();		// Bland Index

#ifdef qcSafetyChecks
  qcAssert(!fTableau.IsBasic(vi) && fTableau.IsConstrained(vi));
#endif

  for (QcTableauColIterator colCoeffs( fTableau, vi);
       !colCoeffs.AtEnd();
       colCoeffs.Increment())
   if(colCoeffs.GetIndex() < fTableau.GetBeginEq())
    {
      qcAssert( fTableau.GetBasicVar( colCoeffs.GetIndex()) != (int) vi);
      if (fTableau.IsRestricted( colCoeffs.GetIndex()))
	{
	  numT rhs = fTableau.GetRHS( colCoeffs.GetIndex());
	  if (CannotIncrease( colCoeffs.GetValue(), rhs))
	    {
	      numT coeff = rhs / colCoeffs.GetValue();
	      if ((coeff < minval)
		  || ((coeff == minval)
		      && (fTableau.GetBasicVar(colCoeffs.GetIndex()) < bi)))
		{
		  minval = coeff;
		  ei = colCoeffs.GetIndex();
		  bi = fTableau.GetBasicVar( ei);
		  qcAssert( !QcUtility::IsNegative( minval));
		}
	    }
	}
    }

#ifdef qcSafetyChecks
  qcAssert( fTableau.IsBasicFeasibleSolved());
  if (ei < 0)
    throw QcWarning("SelectExitVar: No suitable variable");
#endif

  return ei;
}


int QcCompPivotSystem::SelectExitVarComplex (int vi)
{
  int ei = QcTableau::fInvalidConstraintIndex;
  numT minval = DBL_MAX;
  int bi = fTableau.GetColumns();		// Bland Index

#ifdef qcSafetyChecks
  qcAssert(!fTableau.IsBasic(vi) && fTableau.IsConstrained(vi));
#endif

  for (QcTableauColIterator colCoeffs( fTableau, vi);
       !colCoeffs.AtEnd();
       colCoeffs.Increment())
   if(colCoeffs.GetIndex() < fTableau.GetBeginEq())
    {
      unsigned const coeff_ix = colCoeffs.getIndex();
      qcAssert (fTableau.GetBasicVar (coeff_ix) != vi);
      if (fTableau.IsRestricted (coeff_ix))
	{
	  numT rhs = fTableau.GetComplexRHS (coeff_ix);
	  if (CannotIncrease (colCoeffs.getValue(), rhs))
	    {
	      numT coeff = rhs / colCoeffs.getValue();
	      if ((coeff < minval)
		  || ((coeff == minval)
		      && (fTableau.GetBasicVar (coeff_ix)
			  < bi)))
		{
		  minval = coeff;
		  ei = coeff_ix;
		  bi = fTableau.GetBasicVar (ei);
		  qcAssert (!QcUtility::IsNegative (minval));
		}
	    }
	}
    }

#ifdef qcSafetyChecks
  /* fixme: the following assertion can fail (in the test suite), with fTableau
     solved but not basic feasible. */
  qcAssert (fTableau.IsBasicFeasibleSolved());
  if (ei < 0)
    throw QcWarning("SelectExitVar: No suitable variable");
#endif

  return ei;
}
