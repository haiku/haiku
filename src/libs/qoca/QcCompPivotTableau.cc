// $Id: QcCompPivotTableau.cc,v 1.12 2001/01/31 07:37:13 pmoulder Exp $

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
#include "qoca/QcCompPivotTableau.hh"
#include "qoca/QcConstraintIndexIterator.hh"
#include "qoca/QcTableauColIterator.hh"
#include "qoca/QcDenseTableauColIterator.hh"

int QcCompPivotTableau::AddArtificial(numT coeff)
{
	int vi = IncreaseColumns();
	SetColCondition(vi, QcLinInEqColState::fArtificial);
	SetConstrained(vi, true);

	QcConstraintIndexIterator cIt(*this);

	while (!cIt.AtEnd()) {
		fCoreTableau->fSF.SetValue (cIt.getIndex(), vi, coeff);
		cIt.Increment();
	}

	fAI = vi;
	return vi;
}

int QcCompPivotTableau::AddDesValVar(void)
{
	int vi = IncreaseColumns();
	SetColCondition(vi, QcLinInEqColState::fDesire);
	SetConstrained(vi, false);
	return vi;
}

numT QcCompPivotTableau::GetComplexRHS(int ci) const
{
	numT crhs = QcLinInEqTableau::GetRHS(ci);

	for (unsigned vi = 0; (int) vi < GetColumns(); vi++) {
		if (IsDesire(vi))
			crhs -= GetValue(ci, vi) * GetDesireValue(vi);
	}

	return QcUtility::Zeroise(crhs);
}

void QcCompPivotTableau::Print(ostream &os) const
{
	QcLinInEqTableau::Print(os);
	os << "Complex RHS:" << endl;
	int rows = GetRows();

	for (int i = 0; i < rows; i++)
		os << "CRHS[" << i << "] = " << GetComplexRHS(i) << endl;

	os << "Begin Eq: " << fBeginEq << endl;
}

void QcCompPivotTableau::Restart(void)
{
	QcLinInEqTableau::Restart();
	fAI = QcTableau::fInvalidVariableIndex;
	fBeginEq = QcTableau::fInvalidConstraintIndex;
}

bool QcCompPivotTableau::RestrictedPivot(unsigned ci, unsigned vi)
{
  qcAssertPre( ci < getNRows());
  qcAssertPre( vi < getNColumns());
  qcAssertPre( !fCoreTableau->GetMRowDeleted( ci));
  qcAssertPre( !IsRedundant( ci));
  qcAssertPre( GetRowCondition( ci) != QcLinEqRowState::fInvalid);

  numT p = GetValue( ci, vi);	// GetValue returns the raw coefficient

  if (QcUtility::IsZero( p))		// so this test is required.
    return false;

  // If ci is regular then remove it from the basis.
  if (GetRowCondition( ci) == QcLinEqRowState::fRegular)
    Unsolve(ci);
  // Note that fARowIndex[ci] can be left as qcInvalidCIndex,
  // hence check below.

	// From this point, until the pivot is completed,
	// the conditions MRowIndex(fARowIndex(ci))==ci and
	// ARowIndex(MRowIndex(ci2))==ci2 may not hold.  Since it is
	// Pivot that causes a row to become regular, and these conditions
	// do not hold for regular rows, this is a consequence of the
	// change of state.  We could introduce another row state for this
	// phase, but that seems a bit over the top.  Just check that all
	// tableau methods that Pivot calls from here on do not assume the
	// conditions.  I.E. don't call: DecreaseRows, Eliminate, RemoveEq,
	// CompactRows, Restore, or AddRow.  And of course if IsSolved is
	// called with checks enabled it may report an assert violation.

	// From this point the pivot must succeed.
	// So can now wipe fARowIndex and fMRowIndex
  if (GetARowIndex(ci) != QcTableau::fInvalidConstraintIndex)
    {
      SetMRowIndex( GetARowIndex( ci), QcTableau::fInvalidConstraintIndex);
      SetARowIndex( ci, QcTableau::fInvalidConstraintIndex);
    }

	#ifdef qcSafetyChecks
	qcAssert(GetARowIndex(ci) == QcTableau::fInvalidConstraintIndex);
	qcAssert(GetRowCondition(ci) == QcLinEqRowState::fNormalised);
	qcAssert(GetBasicVar(ci) == QcTableau::fInvalidVariableIndex);
	qcAssert(IsBasicIn(vi) == QcTableau::fInvalidConstraintIndex);
	#endif

	// Scale the pivot row so pivot coeff is 1.0
	fCoreTableau->fSF.ScaleRow( ci, recip( p));
	// FractionRow may be used here rather than ScaleRow(ci,recip(p))
	// so that we don't introduce rounding errors at the cost of
	// efficiency (FractionRow uses division so is less efficient).

#ifdef qcUseSafeIterator
	// Eliminate the new basic variable from all the other rows.
	for (QcDenseTableauColIterator varCoeffs( *this, vi);
	     !varCoeffs.AtEnd();
	     varCoeffs.Increment())
	  {
	    /* Process only regular rows.  Redundant and deleted rows would not
	       be affected in any case.  Undetermined rows are not in solved
	       form so should not be included.  By checking here we can allow
	       rows to be added without requiring an immediate pivot and also
	       enable a full resolve by setting all the rows to normalised and
	       fM to identity. */
	    if (varCoeffs.GetIndex() >= fBeginEq)
	      break;

	    if ((GetRowCondition( varCoeffs.getIndex()) == QcLinEqRowState::fRegular)
		|| ((GetRowCondition( varCoeffs.getIndex()) == QcLinEqRowState::fNormalised)
		    && (varCoeffs.getIndex() != ci)))
	      {
		// Excludes (varCoeffs.fIndex == ci) since
		// (GetRowCondition(ci) != QcLinEqRowState::fRegular).
		fCoreTableau->fSF.AddScaledRow( varCoeffs.getIndex(), ci,
						-varCoeffs.getValue());
	      }

	}
#else /* !qcUseSafeIterator */
	// Eliminate the new basic variable from all the other rows.
	{
	  vector<QcSparseCoeff> coeffCache;

	  for(QcTableauColIterator varCoeffs( *this, vi);
	      !varCoeffs.AtEnd();
	      varCoeffs.Increment())
	    {
	      unsigned coeff_ix = varCoeffs.getIndex();

	      /* Process only regular rows.  Redundant and deleted rows would
	         not be affected in any case.  Undetermined rows are not
	         in solved form so should not be included.  By checking here
	         we can allow rows to be added without requiring an
	         immediate pivot and also enable a full resolve by
	         setting all the rows to normalised and fM to identity. */
	      if((int( coeff_ix) < fBeginEq)
		 && (GetRowCondition( coeff_ix) == QcLinEqRowState::fRegular
		     || (GetRowCondition( coeff_ix) == QcLinEqRowState::fNormalised
			 && (coeff_ix != ci))))
		/* Excludes (coeff_ix == ci) since
		 * (GetRowCondition(ci) != QcLinEqRowState::fRegular).
		 */
		coeffCache.push_back( QcSparseCoeff( varCoeffs.getValue(),
						     coeff_ix));
	    }

	  for (vector<QcSparseCoeff>::iterator cIt = coeffCache.begin();
	       cIt != coeffCache.end();
	       cIt++)
	    {
	      QcSparseCoeff &coeff = (*cIt);
	      unsigned const coeff_ix = coeff.getIndex();

	      fCoreTableau->fSF.AddScaledRow( coeff_ix, ci,
					      -coeff.getValue());
	    }
	}
#endif

	SetBasic(vi, true);
	SetBasicIn(vi, ci);
	SetBasicVar(ci, vi);
	SetRowCondition(ci, QcLinEqRowState::fRegular);

	return true;
}

void QcCompPivotTableau::SetArtificial(numT coeff)
{
	QcConstraintIndexIterator cIt(*this);

	while (!cIt.AtEnd()) {
		fCoreTableau->fSF.SetValue (cIt.getIndex(), fAI, coeff);
		cIt.Increment();
	}
}

void QcCompPivotTableau::SetRowState(int bvi, int ci, int rs)
{
	// Need to reverse the sign of all the row coefficients if the coefficients
	// for the variable to make basic is -1.0.
	if (QcUtility::IsNegative(GetValue(ci, bvi)))
		fCoreTableau->fSF.NegateRow( ci);

	SetBasic(bvi, true);
	SetBasicIn(bvi, ci);
	SetBasicVar(ci, bvi);
	SetRowCondition(ci, rs);
}


