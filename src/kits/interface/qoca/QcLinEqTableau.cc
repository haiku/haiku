// $Id: QcLinEqTableau.cc,v 1.12 2001/01/31 07:37:13 pmoulder Exp $

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
#include "qoca/QcDefines.hh"
#include "qoca/QcLinEqTableau.hh"
#include "qoca/QcTableauRowIterator.hh"
#include "qoca/QcTableauColIterator.hh"
#include "qoca/QcConstraintIndexIterator.hh"
#include "qoca/QcBasicVarIndexIterator.hh"
#include "qoca/QcDenseTableauRowIterator.hh"
#include "qoca/QcDenseTableauColIterator.hh"

#ifndef NDEBUG
void
QcLinEqTableau::assertDeepInvar() const
{
  assertInvar();
  QcTableau::assertDeepInvar();
  fCoreTableau->vAssertDeepInvar();
}

void
QcLinEqTableau::vAssertDeepInvar() const
{
  assertDeepInvar();
}
#endif


int
QcLinEqTableau::Eliminate(unsigned ci)
{
  qcAssertPre( ci < getNRows());
  qcAssertPre( !fCoreTableau->GetARowDeleted( ci));

	int quasiRow = fCoreTableau->GetMRowIndex(ci);
     	// The corresponding row of fM

	qcAssert(quasiRow >= 0 && quasiRow < GetRows() &&
    		!fCoreTableau->GetMRowDeleted(quasiRow));

	#ifdef qcSafetyChecks
	if (GetRowCondition(quasiRow) != QcLinEqRowState::fInvalid)
		throw QcWarning(qcPlace, "Eliminate called for a valid constraint");
	qcAssert(GetBasicVar(quasiRow) == QcTableau::fInvalidVariableIndex);
	#endif

	// Erase the list of pivot variable index hints.
	fPivotHints.resize(0);

	// Eliminate the basic variables from quasiRow.
#ifdef qcUseSafeIterator
	QcDenseTableauRowIterator rowIt( *this, quasiRow);
     
	numT maxCoeff( 0);
	for(; !rowIt.AtEnd(); rowIt.Increment())
	  {
	    unsigned col = rowIt.getIndex();

	    if(IsBasic( col))
	      fCoreTableau->AddScaledRow( quasiRow, IsBasicIn( col),
					  -rowIt.getValue());
	    else
	      {
		// Arrange to put the biggest hint coeff first.
		numT absVal = fabs( rowIt.getValue());
		if(absVal > maxCoeff)
		  {
		    maxCoeff = absVal;
		    fPivotHints.push_front( col);
		  }
		else
		  fPivotHints.push_back( col);
	      }
	}
#else
	// N.B. Eliminate cannot use QcLeqTabRowIterator as the tableau
	// quasi matrix fM is altered (and this is essential to Eliminate).
	QcTableauRowIterator rowIt(*this, quasiRow);
	{
	  vector<QcSparseCoeff> coeffCache;

	  while (!rowIt.AtEnd())
	    {
	      coeffCache.push_back( QcSparseCoeff( rowIt.getValue(),
						   rowIt.getIndex()));
	      rowIt.Increment();
	    }

	  numT maxCoeff( 0);
	  for(vector<QcSparseCoeff>::iterator cIt = coeffCache.begin();
	      cIt != coeffCache.end();
	      cIt++)
	    {
	      QcSparseCoeff &coeff = (*cIt);
	      unsigned col = coeff.getIndex();

	      if(IsBasic( col))
		{
		  fCoreTableau->AddScaledRow( quasiRow, IsBasicIn( col),
					      -coeff.getValue());
		}
	      else
		{
		  /* Arrange to put the biggest (in magnitude) hint coeff first.
		     A full sort shouldn't be necessary, as usually only the
		     first element is consulted. */
		  numT absVal = fabs( coeff.getValue());
		  if(absVal > maxCoeff)
		    {
		      maxCoeff = absVal;
		      fPivotHints.push_front( col);
		    }
		  else
		    fPivotHints.push_back( col);
		}
	    }
	}
#endif

	// Check for linear independance
	rowIt.SetToBeginOf(quasiRow);

	if (rowIt.AtEnd()) {				// all coeffs are zero
		if (QcUtility::IsZero(fCoreTableau->GetRHS(quasiRow))) {
			SetRowCondition(quasiRow, QcLinEqRowState::fRedundant);
			SetMRowIndex(ci, QcTableau::fInvalidConstraintIndex);
			SetARowIndex(quasiRow, QcTableau::fInvalidConstraintIndex);
		} else {
			#ifdef qcSafetyChecks
			throw QcWarning(qcPlace, "Inconsistant Constraint");
			#endif
			qcAssert(GetRowCondition(quasiRow) == QcLinEqRowState::fInvalid);
			DeleteRow(ci);
			return QcTableau::fInvalidConstraintIndex;
		}
	} else
		SetRowCondition(quasiRow, QcLinEqRowState::fNormalised);

	return quasiRow;		// Note alternative return qcInvalidCIndex above.
}

numT QcLinEqTableau::EvalBasicVar(unsigned vi) const
{
  qcAssertPre( IsBasic( vi));
  int ci = IsBasicIn( vi);
  assert( ci != QcTableau::fInvalidConstraintIndex);

  numT value = GetRHS( ci);

  for (QcTableauRowIterator varCoeffs( *this, ci);
       !varCoeffs.AtEnd();
       varCoeffs.Increment())
    {
      unsigned coeff_vi = varCoeffs.getIndex();
      if (coeff_vi != vi)
	value -= varCoeffs.getValue() * GetDesireValue( coeff_vi);
    }

  return value;
}

#if 0 /* unused */
//numT QcLinEqTableau::GetSparsenessExBasic() const
//{
//  unsigned nZeros = 0;
//  unsigned total = 0;
//
//  for (unsigned i = 0; i < getNRows(); i++)
//    if (GetRowCondition( i) == QcLinEqRowState::fRegular)
//      for (unsigned j = 0; j < getNColumns(); j++)
//	if (!IsBasic( j))
//	  {
//	    total++;
//	    if (QcUtility::IsZero(GetValue( i, j)))
//	      nZeros++;
//	  }
//
//  if (total == 0)
//    return 0;
//#if NUMT_IS_DOUBLE
//  return (total - nZeros) / (numT) total;
//#else
//  return numT( total - nZeros, total);
//#endif
//}
//
//numT QcLinEqTableau::GetSparsenessInBasic() const
//{
//  unsigned nZeros = 0;
//  unsigned total = 0;
//
//  for (unsigned i = 0; i < getNRows(); i++)
//    if (GetRowCondition( i) == QcLinEqRowState::fRegular)
//      for (unsigned j = 0; j < getNColumns(); j++)
//	{
//	  total++;
//	  if (QcUtility::IsZero( GetValue( i, j)))
//	    nZeros++;
//	}
//
//  if (total == 0)
//    return 0;
//#if NUMT_IS_DOUBLE
//  return (total - nZeros) / (numT) total;
//#else
//  return numT( total - nZeros, total);
//#endif
//}
#endif /* unused */

numT QcLinEqTableau::GetValue(unsigned ci, unsigned vi) const
{
	qcAssertPre (ci < getNRows());
	qcAssertPre (vi < getNColumns());

	// Short cut evaluations
	switch (GetRowCondition(ci)) {
		case QcLinEqRowState::fRedundant:
			return 0.0;
		case QcLinEqRowState::fNormalised:
			if (IsBasic(vi))
				return 0.0;
			break;
		case QcLinEqRowState::fRegular:
			if (IsBasic(vi)) {
				if ((int) vi == GetBasicVar(ci))
					return numT( 1);
				else
					return numT( 0);
			}
			break;
		case QcLinEqRowState::fInvalid: break;
		default:
			throw QcWarning(qcPlace, "Invalid LinEqRowCondition");
	}

	return fCoreTableau->GetValue(ci, vi);
}

bool QcLinEqTableau::IsSolved() const
{
  for (QcConstraintIndexIterator cIt( *this);
       !cIt.AtEnd();
       cIt.Increment())
    if (IsUndetermined( cIt.getIndex()))
      return false;

  return true;
}

bool QcLinEqTableau::Pivot(unsigned ci, unsigned vi)
{
	qcAssertPre( ci < getNRows());
	qcAssertPre( !IsRedundant( ci));
	qcAssertPre( GetRowCondition(ci) != QcLinEqRowState::fInvalid);

	numT p = GetValue(ci, vi);	// GetValue returns the raw coefficient

	if (QcUtility::IsZero(p)) 		// so this test is required.
		return false;

	#ifdef qcSafetyChecks
	if (IsBasic( vi)
	    && !((GetRowCondition( ci) == QcLinEqRowState::fRegular)
		 && (GetBasicVar( ci) == (int) vi)))
	  {
	    throw QcWarning(qcPlace,
			    "Pivot attempted for variable already basic in another constraint");
	    return false;
	  }
	#endif

	// If ci is regular then remove it from the basis.
	if (GetRowCondition(ci) == QcLinEqRowState::fRegular)
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
	// Restore, or AddRow.  And of course if IsSolved is
	// called with checks enabled it may report an assert violation.

	// From this point the pivot must succeed.
	// So can now wipe fARowIndex and fMRowIndex
	if (GetARowIndex(ci) != QcTableau::fInvalidConstraintIndex) {
		SetMRowIndex(GetARowIndex(ci), QcTableau::fInvalidConstraintIndex);
		SetARowIndex(ci, QcTableau::fInvalidConstraintIndex);
	}

	#ifdef qcSafetyChecks
	qcAssert(GetARowIndex(ci) == QcTableau::fInvalidConstraintIndex);
	qcAssert(GetRowCondition(ci) == QcLinEqRowState::fNormalised);
	qcAssert(GetBasicVar(ci) == QcTableau::fInvalidVariableIndex);
	qcAssert(IsBasicIn(vi) == QcTableau::fInvalidConstraintIndex);
	#endif

	// Scale the pivot row so pivot coeff is 1.0
	fCoreTableau->ScaleRow(ci, recip( p));
	// FractionRow may be used here rather than ScaleRow(ci,recip(p))
	// so that we don't introduce rounding errors at the cost of
	// efficiency (FractionRow uses division so is less efficient).

#ifdef qcUseSafeIterator
	// Eliminate the new basic variable from all the other rows.
	QcDenseTableauColIterator varCoeffs(*this, vi);

	while (!varCoeffs.AtEnd())
	  {
	    // Only process regular rows.  Redundant and deleted rows would
	    // not be affected in any case.  Undetermined rows are not
	    // in solved form so should not be included.  By checking here
	    // we can allow rows to be added without requiring an
	    // immediate pivot and also enable a full resolve by
	    // setting all the rows to normalised and fM to identity.
	    if ((GetRowCondition( varCoeffs.getIndex()) == QcLinEqRowState::fRegular)
		|| ((GetRowCondition( varCoeffs.getIndex()) == QcLinEqRowState::fNormalised)
		    && (varCoeffs.getIndex() != ci)))
	      {
		// Excludes (varCoeffs.Index() == ci)
		// since (GetRowCondition(ci) != regular).
		fCoreTableau->AddScaledRow( varCoeffs.getIndex(), ci, -varCoeffs.getValue());
	      }
	    varCoeffs.Increment();
	  }
#else
	// Eliminate the new basic variable from all the other rows.
	{
	  vector<QcSparseCoeff> coeffCache;

	  for(QcTableauColIterator varCoeffs( *this, vi);
	      !varCoeffs.AtEnd();
	      varCoeffs.Increment())
	    {
	      unsigned row = varCoeffs.getIndex();

	      /* Process only regular rows.  Redundant and deleted rows would
		 not be affected in any case.  Undetermined rows are not in
		 solved form so should not be included.  By checking here we can
		 allow rows to be added without requiring an immediate pivot and
		 also enable a full resolve by setting all the rows to
		 normalised and fM to identity. */
	      // Excludes (varCoeffs.Index() == ci)
	      // since (GetRowCondition(ci) != regular).
	      if ((GetRowCondition( row) == QcLinEqRowState::fRegular)
		  || ((GetRowCondition( row) == QcLinEqRowState::fNormalised)
		      && (row != ci)))
		coeffCache.push_back( QcSparseCoeff( varCoeffs.getValue(), row));
	    }

	  for(vector<QcSparseCoeff>::iterator cIt = coeffCache.begin();
	      cIt != coeffCache.end();
	      cIt++)
	    fCoreTableau->AddScaledRow( cIt->getIndex(), ci, -cIt->getValue());
	}
#endif

	SetBasic(vi, true);
	SetBasicIn(vi, ci);
	SetBasicVar(ci, vi);
	SetRowCondition(ci, QcLinEqRowState::fRegular);

	return true;
}

void QcLinEqTableau::Print(ostream &os) const
{
	unsigned rows = getNRows();
	unsigned cols = getNColumns();

	fCoreTableau->Print(os);
	os << endl;
	os << (IsSolved() ? "Solved " : "Unsolved ") << endl;

	os << "Tableau with " << rows << " rows and " << cols << " columns." << endl;
	os << "==================================================" << endl;
	os << "Basic vars are: " << endl;

	QcBasicVarIndexIterator it(*this);
	while (!it.AtEnd()) {
		os << it.getIndex() << ", ";
		it.Increment();
	}

	os << endl << "Solved Form:" << endl;

	for (unsigned i = 0; i < rows; i++) {
		os << i << ":";

		switch (GetRowCondition(i)) {
			case QcLinEqRowState::fInvalid:
				if (fCoreTableau->GetMRowDeleted(i))
              			os << "deleted\t";
				else
					os << "invalid\t";
				break;
			case QcLinEqRowState::fNormalised:
         			os << "normalised\t";
				break;
			case QcLinEqRowState::fRegular:
				os << "regular\t";
				break;
			case QcLinEqRowState::fRedundant:
				os << "redundant\t";
				break;
			default:
				throw QcWarning("RowCondition is invalid");
		}

		os << "Basic(" << GetBasicVar(i) << ")\t";

		for (unsigned j = 0; j < cols; j++)
   			os << GetValue(i, j) << "\t";

		os << endl;
	}

	os << endl;

	for (unsigned i = 0; i < rows; i++)
		os << "[" << i << "] " << GetRHS(i) << endl;
}

void QcLinEqTableau::Restore()
{
	// Reset state information.
	for (int c = 0; c < GetColumns(); c++) {   
		SetBasic(c, false);
		SetBasicIn(c, QcTableau::fInvalidConstraintIndex);
	}

	for (int r = 0; r < GetRows(); r++) {
		// Row condition is set to normalised so that a full pivot can be
		// performed on Reset().
		SetBasicVar(r, QcTableau::fInvalidVariableIndex);
		SetRowCondition(r, GetMRowDeleted(r) ?
				QcLinEqRowState::fInvalid : QcLinEqRowState::fNormalised);
	}

	fCoreTableau->Restore();	// Do this last as it calls Row/Col FixLinkage

	dbg(assertInvar());
}

void QcLinEqTableau::Unsolve(unsigned ci)
	// Remove constraint from the basis given solved form constraint index
{
  qcAssertPre( ci < getNRows());

	qcAssert(GetRowCondition(ci) == QcLinEqRowState::fRegular);
	int b = GetBasicVar(ci);
	qcAssert(b != QcTableau::fInvalidVariableIndex);
	SetBasic(b, false);
	SetBasicIn(b, QcTableau::fInvalidConstraintIndex);
	SetBasicVar(ci, QcTableau::fInvalidVariableIndex);
	SetRowCondition(ci, QcLinEqRowState::fNormalised);
}
