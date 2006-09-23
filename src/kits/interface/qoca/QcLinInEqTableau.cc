// $Id: QcLinInEqTableau.cc,v 1.10 2001/01/04 05:20:11 pmoulder Exp $

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

#include <math.h>
#include <vector>
#include "qoca/QcDefines.hh"
#include "qoca/QcLinInEqTableau.hh"
#include "qoca/QcConstraintIndexIterator.hh"
#include "qoca/QcBasicVarIndexIterator.hh"
#include "qoca/QcParamVarIndexIterator.hh"
#include "qoca/QcVariableIndexIterator.hh"
#include "qoca/QcTableauRowIterator.hh"

int QcLinInEqTableau::AddArtificial(numT coeff)
{
	unsigned vi = IncreaseColumns();
	SetColCondition (vi, QcLinInEqColState::fArtificial);
	SetConstrained (vi, true);
	QcConstraintIndexIterator cIt (*this);

	while (!cIt.AtEnd()) {
		unsigned ci = cIt.getIndex();
		fCoreTableau->fSF.SetValue (ci, vi, coeff);
		cIt.Increment();
	}
	return vi;
}

int QcLinInEqTableau::AddGtEq(QcRowAdaptor &varCoeffs, numT rhs)
{
	unsigned r = AddEq(varCoeffs, rhs);

	// Add new slack variable with coeff -1.0
	unsigned vi = AddSlack();
#ifdef qcRealTableauCoeff
	fCoreTableau->fSF.SetValue(r, vi, -1.0);
#endif

	qcAssertPost( (int) vi >= 0);
	qcAssertPost( vi == getNColumns() - 1);
	qcAssertPost( r < getNRows());
	return r;
}

int QcLinInEqTableau::AddLtEq(QcRowAdaptor &varCoeffs, numT rhs)
{
	int r = AddEq( varCoeffs, rhs);

	// Add new slack variable with coeff 1.0.
	unsigned vi = AddSlack();
#ifdef qcRealTableauCoeff
	fCoreTableau->fSF.SetValue(r, vi, 1.0);
#endif

	qcAssertPost( int( vi) >= 0);
	qcAssertPost( vi == getNColumns() - 1);
	qcAssertPost( unsigned( r) < getNRows());
	return r;
}

unsigned QcLinInEqTableau::AddSlack()
{
	unsigned vi = NewVariable();
	SetColCondition( vi, (QcLinInEqColState::fSlack
			      | QcLinInEqColState::fStructural));
	SetConstrained( vi, true);

	qcAssertPost( (int) vi >= 0);
	qcAssertPost( vi == getNColumns() - 1);
	return vi;
}

void QcLinInEqTableau::EliminateObjective()
{
  QcBasicVarIndexIterator bvIt (*this);

  while (!bvIt.AtEnd())
    {
      unsigned bvi = bvIt.getIndex();
      numT objcoeff = GetObjValue (bvi);

      if (objcoeff != 0.0)
	{
	  SetObjValue (bvi, 0.0);
	  int ci = IsBasicIn (bvi);
	  assert (ci >= 0);
	  QcTableauRowIterator varCoeffs (*this, ci);
	  while (!varCoeffs.AtEnd())
	    {	// deduct constrained parameter coeffs
	      if (IsConstrained (varCoeffs.getIndex())
		  && (varCoeffs.getIndex() != bvi))
		SetObjValue (varCoeffs.getIndex(),
			     (GetObjValue (varCoeffs.getIndex())
			      - (objcoeff * varCoeffs.getValue())));
	      varCoeffs.Increment();
	    }
	}
      bvIt.Increment();
    }
}


numT
QcLinInEqTableau::EvalBasicVar(unsigned vi) const
{
  qcAssertPre( IsBasic( vi));

  int ci = IsBasicIn( vi);
  assert( ci != QcTableau::fInvalidConstraintIndex);

  numT value = GetRHS( ci); // rhs sign is not critical here

  for (QcTableauRowIterator varCoeffs( *this, ci);
       !varCoeffs.AtEnd();
       varCoeffs.Increment())
    {
      unsigned coeff_vi = varCoeffs.getIndex();
      if ((coeff_vi != vi)
	  && IsStructural( coeff_vi))
	value -= (varCoeffs.getValue() * GetDesireValue( coeff_vi));
    }

  return value;
}

unsigned QcLinInEqTableau::getNRestrictedRows() const
{
	unsigned count = 0;

	for (unsigned i = 0; i < getNRows(); i++)
		if (IsRestricted( i))
         		count++;

	qcAssertPost( count <= getNRows());
	return count;
}

bool QcLinInEqTableau::IsBasicFeasible() const
{
  for (QcConstraintIndexIterator cIt( *this);
       !cIt.AtEnd();
       cIt.Increment())
    {
      unsigned const ci = cIt.getIndex();
      if (IsRestricted( ci)
	  && (GetRHS( ci) < 0.0))    // N.B. We rely on GetRHS to do zeroisation here.
	return false;
    }
  return true;
}

bool QcLinInEqTableau::IsBasicOptimal() const
{
	QcParamVarIndexIterator pIt(*this);

	while (!pIt.AtEnd()) {
		unsigned pi = pIt.getIndex();
		if (IsConstrained(pi)) {		// Check this with Kim
			numT obj = GetObjValue(pi);
			if (QcUtility::IsNegative(obj))
				return false;
		}
		pIt.Increment();
	}

	return true;
}

#if PROVIDE_LoadArtificial
int QcLinInEqTableau::LoadArtificial(const QcLinInEqTableau &other)
	// Used to add artificial variable in complementary pivot algorithm
{
	int ocs;		// Solved form constraint index for other tableau.
	int *cassoc;	// Associate each ocs with new ci

	cassoc = new int[other.GetRows()];
	Restart();	// Start with a fresh tableau

	// Preallocate the variable indices and initialise column properties
	while (GetColumns() < other.GetColumns())
		NewVariable();

	for (ocs = 0; ocs < other.GetRows(); ocs++)
		if (other.IsRestricted(ocs)) {
			QcTableauRowIterator varCoeffs(other, ocs);
			cassoc[ocs] = AddEq(varCoeffs, other.GetRHS(ocs));
		} else
			cassoc[ocs] = QcTableau::fInvalidConstraintIndex;

	for (int vi = 0; vi < other.GetColumns(); vi++)
		if (other.IsSlack(vi))
			SetConstrained(vi, true);

	int ai = AddArtificial(-1.0);

	for (ocs = 0; ocs < other.GetRows(); ocs++) {
		int co = cassoc[ocs];

		if (co != QcTableau::fInvalidConstraintIndex) {
			int cs = Eliminate(co);
			#ifdef qcSafetyChecks
			qcAssert(Pivot(cs, other.GetBasicVar(ocs)));
			#else
			Pivot(cs, other.GetBasicVar(ocs));
			#endif
		}
	}

	delete[] cassoc;
	return ai;
}
#endif

void QcLinInEqTableau::SetObjective(QcRowAdaptor &objCoeffs)
{
	QcVariableIndexIterator vIt(*this);

	while (!vIt.AtEnd()) {
		SetObjValue(vIt.getIndex(), 0.0);
		vIt.Increment();
	}

	while (!objCoeffs.AtEnd()) {
		SetObjValue(objCoeffs.getIndex(), objCoeffs.getValue());
		objCoeffs.Increment();
	}
}

bool QcLinInEqTableau::SimplexPivot(unsigned ci, unsigned vi)
{
  bool pivoted = Pivot( ci, vi);

  if (pivoted)
    {
      // express new basic var coeff in objective function using parameters
      numT const objCoeff = GetObjValue( vi);

      for (QcTableauRowIterator varCoeffs( *this, ci);
	   !varCoeffs.AtEnd();
	   varCoeffs.Increment())
	{
	  unsigned const coeff_ix = varCoeffs.getIndex();

	  qcAssert( IsConstrained( coeff_ix));

	  // Checking varCoeffs.Index()!=vi is not worth the time.
	  SetObjValue( coeff_ix,
		       (GetObjValue( coeff_ix)
			- (objCoeff * varCoeffs.getValue())));
	}
      assert( GetObjValue( vi) == 0.0);
    }

  return pivoted;
}
