// $Id: QcDelLinEqTableau.cc,v 1.9 2001/01/31 07:37:13 pmoulder Exp $

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

#include <cmath>
#include "qoca/QcDefines.hh"
#include "qoca/QcDelLinEqTableau.hh"
#ifdef qcDenseQuasiInverse
# include <qoca/QcDenseMatrixColIterator.hh>
#else
# include <qoca/QcSparseMatrixColIterator.hh>
#endif

numT QcDelLinEqTableau::GetValueVirtual(int ci, int vi) const
{
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
				if (vi == GetBasicVar(ci))
					return numT( 1u);
				else
					return 0.0;
			}
			break;
		case QcLinEqRowState::fInvalid:
			break;
	}

	return CAST(QcDelCoreTableau *, fCoreTableau)->GetValueVirtual(ci,vi);
}

int QcDelLinEqTableau::RemoveEq(unsigned ci)
{
	numT max = 0.0;		// Maximal absolute value of coeff
	int maxIndex = QcTableau::fInvalidConstraintIndex;
	bool foundRedund = false;
	int pivotRow;

#ifndef NDEBUG
	if ((int) ci < 0 || (int) ci >= GetRows() || fCoreTableau->GetARowDeleted(ci)) {
		throw QcWarning(qcPlace,
          		"RemoveEq: invalid original constraint index", ci, GetRows() - 1);
		return QcTableau::fInvalidConstraintIndex;
	} else if (fCoreTableau->GetMRowIndex(ci) != QcTableau::fInvalidConstraintIndex) {
		qcAssert(IsUndetermined(fCoreTableau->GetMRowIndex(ci)));
		throw QcWarning(qcPlace, "RemoveEq for undetermined constraint");
		return QcTableau::fInvalidConstraintIndex;
	} else if (!IsSolved())
		throw QcWarning(qcPlace,
          	"RemoveEq called when tableau not in solved form");
#endif

	// First look for a suitable redundant constraint, meanwhile
	// keep track of the maximal size quasiColCoeff.
	#ifdef qcDenseQuasiInverse
 	QcDenseMatrixColIterator quasiColCoeffs(
	   		CAST(QcDelCoreTableau *, fCoreTableau)->fM, ci);
	#else
 	QcSparseMatrixColIterator quasiColCoeffs(
	   		CAST(QcDelCoreTableau *, fCoreTableau)->fM, ci);
	#endif

	while (!quasiColCoeffs.AtEnd()) { // N.B. This loop contains a break
		qcAssert(!QcUtility::IsZero(quasiColCoeffs.getValue()));

		if (IsRedundant(quasiColCoeffs.getIndex())) {
			foundRedund = true;
			break;
		} else {
			#ifdef qcSafetyChecks
			qcAssert(GetBasicVar(quasiColCoeffs.GetIndex())
				 != QcTableau::fInvalidVariableIndex);
			if (GetRowCondition(quasiColCoeffs.GetIndex())
			    == QcLinEqRowState::fNormalised)
				throw QcWarning(qcPlace,
                   		"RemoveEq called with normalised constraint");
			else
				qcAssert(GetRowCondition(quasiColCoeffs.GetIndex())
					 == QcLinEqRowState::fRegular);
			#endif
			numT absValue = fabs (quasiColCoeffs.getValue());
			if (absValue > max)
			  {
			    max = absValue;
			    maxIndex = quasiColCoeffs.getIndex();
			  }
			// Note that the ratio used in IPL 55 (1995) page 112 is
			// directly related to the inverse of max since the RHS
			// value is constant and we take the absolute value of max.
		}
		quasiColCoeffs.Increment();
	}

	#ifdef qcSafetyChecks
	if (foundRedund)
		qcAssert(IsRedundant(quasiColCoeffs.GetIndex()));
	else
		qcAssert(max != 0.0 && maxIndex != QcTableau::fInvalidConstraintIndex);
	#endif

	// Best to abort rather than use an invalid index
	if (!foundRedund && maxIndex == QcTableau::fInvalidConstraintIndex)
		return QcTableau::fInvalidConstraintIndex;

	// delete selected row from fM, column ci from fM, row ci from fA
	if (foundRedund) {
		pivotRow = quasiColCoeffs.GetIndex();
		SetRowCondition(pivotRow, QcLinEqRowState::fNormalised);
	} else
		pivotRow = maxIndex;

	CAST(QcDelCoreTableau *, fCoreTableau)->PivotQuasiInverse(pivotRow, ci);

	if (!foundRedund)
		Unsolve(pivotRow);	// removes pivotRow from the basis

	// The situation now simulates that of a newly added row after
	// Eliminate has been called, but before a Pivot.
	SetMRowIndex(ci, pivotRow);
	SetARowIndex(pivotRow, ci);
	DeleteRow(ci);
	fNotifier.DropConstraint(ci);
	return pivotRow;
}

bool QcDelLinEqTableau::RemoveVarix(unsigned vi)
{
	if (!IsFree(vi))
		return false;

	qcAssert(!IsBasic(vi));
	qcAssert(IsBasicIn(vi) == QcTableau::fInvalidConstraintIndex);

	unsigned lastColumn = getNColumns() - 1;
	if (vi != lastColumn) {	// Swap column vi with last column
		fCoreTableau->CopyColumn( vi, lastColumn);
		fRowColState->SwapColumns( vi, lastColumn);
		fNotifier.SwapVariables( vi, lastColumn);
	}

	qcAssert(!IsBasic( lastColumn));
	qcAssert(IsBasicIn( lastColumn) == QcTableau::fInvalidConstraintIndex);
	DecreaseColumns(fNotifier);
	return true;
}
