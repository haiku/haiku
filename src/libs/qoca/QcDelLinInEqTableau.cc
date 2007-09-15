// $Id: QcDelLinInEqTableau.cc,v 1.9 2001/01/30 01:32:07 pmoulder Exp $

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
#include "qoca/QcDelLinInEqTableau.hh"
#include "qoca/QcSparseMatrixRowIterator.hh"
#include "qoca/QcDenseMatrixColIterator.hh"

int QcDelLinInEqTableau::AddGtEq(QcRowAdaptor &varCoeffs, numT rhs)
{
	int r = AddEq(varCoeffs, rhs);

	// Add new slack variable with coeff -1.0
	unsigned vi = NewVariable();
	SetColCondition(vi, QcLinInEqColState::fSlack |
    		QcLinInEqColState::fStructural);
	SetConstrained(vi, true);
	CAST(QcDelCoreTableau *, fCoreTableau)->fA.SetValue(r, vi, -1.0);
	fCoreTableau->fSF.SetValue(GetMRowIndex(r), vi, -1.0);

	return r;
}

int QcDelLinInEqTableau::AddLtEq(QcRowAdaptor &varCoeffs, numT rhs)
{
	int r = AddEq(varCoeffs, rhs);

	// Add new slack variable with coeff 1.0
	unsigned vi = NewVariable();
	SetColCondition(vi, QcLinInEqColState::fSlack |
    		QcLinInEqColState::fStructural);
	SetConstrained(vi, true);
	CAST(QcDelCoreTableau *, fCoreTableau)->fA.SetValue(r, vi, 1.0);
	fCoreTableau->fSF.SetValue(GetMRowIndex(r), vi, 1.0);

	return r;
}

int QcDelLinInEqTableau::RemoveEq(unsigned ci)
{
	numT min = INT_MAX;		// Maximal absolute value of coeff
	int minIndex = QcTableau::fInvalidConstraintIndex;
	bool foundRedund = false;
	int pivotRow;

	if ((int) ci < 0 || (int) ci >= GetRows() || fCoreTableau->GetARowDeleted(ci)) {
		throw QcWarning(qcPlace,
			"RemoveEq: invalid original constraint index", ci, GetRows() - 1);
		return QcTableau::fInvalidConstraintIndex;
	} else if (fCoreTableau->GetMRowIndex(ci) != QcTableau::fInvalidConstraintIndex) {
		qcAssert(IsUndetermined(fCoreTableau->GetMRowIndex(ci)));
		throw QcWarning(qcPlace, "RemoveEq for undetermined constraint");
		return QcTableau::fInvalidConstraintIndex;
	}
	#ifdef qcSafetyChecks
	else if (!IsSolved())
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
		qcAssert(!QcUtility::IsZero(quasiColCoeffs.GetValue()));

		if (IsRedundant(quasiColCoeffs.GetIndex())) {
			foundRedund = true;
			break;
		} else {
			qcAssert(GetBasicVar(quasiColCoeffs.GetIndex()) !=
              		QcTableau::fInvalidVariableIndex);
			#ifdef qcSafetyChecks
			if (GetRowCondition(quasiColCoeffs.GetIndex()) ==
              			QcLinEqRowState::fNormalised)
				throw QcWarning(qcPlace,
                   		"RemoveEq called with normalised constraint");
			else
              	qcAssert(GetRowCondition(quasiColCoeffs.GetIndex()) ==
                   		QcLinEqRowState::fRegular);
			#endif
			numT ratio = fabs(GetRHS(quasiColCoeffs.getIndex()) /
					    quasiColCoeffs.getValue());

			if (ratio < min) {
				min = ratio;
				minIndex = quasiColCoeffs.getIndex();
			}
			// Note that the ratio used in IPL 55 (1995) page 112 is
			// directly related to the inverse of max since the RHS
			// value is constant and we take the absolute value of max.
		}
		quasiColCoeffs.Increment();
	}

	#ifdef qcSafetyChecks
	if (foundRedund)
		qcAssert(IsRedundant(quasiColCoeffs.getIndex()));
	else
		qcAssert(minIndex != QcTableau::fInvalidConstraintIndex);
	#endif

	// Best to abort rather than use an invalid index
	if (!foundRedund && minIndex == QcTableau::fInvalidConstraintIndex)
		return QcTableau::fInvalidConstraintIndex;

	// delete selected row from fM, column ci from fM, row ci from fA
	if (foundRedund) {
		pivotRow = quasiColCoeffs.GetIndex();
		SetRowCondition(pivotRow, QcLinEqRowState::fNormalised);
	} else
		pivotRow = minIndex;

	CAST(QcDelCoreTableau *, fCoreTableau)->PivotQuasiInverse(pivotRow, ci);

	if (!foundRedund)
		Unsolve(pivotRow);	// removes pivotRow from the basis

   	// The situation now simulates that of a newly added row after
	// Eliminate has been called, but before a Pivot.
	SetMRowIndex(ci, pivotRow);
	SetARowIndex(pivotRow, ci);

	// Removing the original constraint from the constraint matrix.  First must
	// remove any associated slack variables.
	deque<unsigned int> slackList;
	QcSparseMatrixRowIterator var(CAST(QcDelCoreTableau *, fCoreTableau)->fA, ci);

	while (!var.AtEnd()) {
		// It is very important to put index to the front because larger index
		// should be deleted first so to avoid invaliding the index of the rest
		// of the slack variables.
		if (IsSlack(var.GetIndex()))
			slackList.push_front(var.GetIndex());
		var.Increment();
	}

	DeleteRow(ci);
	fNotifier.DropConstraint(ci);

	// Remove the slack variables if there are any.
	for (deque<unsigned int>::iterator slack = slackList.begin();
			slack != slackList.end(); ++slack)
		RemoveVarix(*slack);

	return pivotRow;
}
