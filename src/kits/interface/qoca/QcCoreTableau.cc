// $Id: QcCoreTableau.cc,v 1.12 2001/01/30 01:32:07 pmoulder Exp $

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
#include "qoca/QcCoreTableau.hh"
#include "qoca/QcRowAdaptor.hh"

unsigned QcCoreTableau::AddRow(QcRowAdaptor &varCoeffs, numT rhs)
{
	unsigned r = 0;     // Row of fA, col of fM, int for original constraint
	int quasiRow;	// Row of fM, int for solved form constraint

	// Look for a deleted row or create a new row if one not found
	while (r < fRows && !GetARowDeleted(r))
		r++;

	if (r == fRows) {
		dbg(unsigned r1 =)
		  IncreaseRows();
		assert( r == r1);
		quasiRow = GetMRowIndex(r);
		qcAssert( quasiRow == (int) r);	// Currently IncreaseRows ensures this
	} else {
    		quasiRow = GetMRowIndex(r);
		SetARowDeleted(r, false);
		SetMRowDeleted(quasiRow, false);
	}

	qcAssert(GetRawRHS(r) == 0.0);

	// Load the new values into the fA matrix and RHS
	SetRawRHS(r, rhs);
	fSF.SetRHS(quasiRow, rhs);

	while (!varCoeffs.AtEnd()) {
		qcAssert(varCoeffs.getIndex() < fColumns);
		#ifdef qcSafetyChecks
		if (QcUtility::IsZero( varCoeffs.getValue()))
			throw QcWarning(qcPlace, "AddRow: Zero coeff supplied");
		#endif
		fSF.SetValue(quasiRow, varCoeffs.getIndex(), varCoeffs.getValue());
		varCoeffs.Increment();
	}

	return r;
}

#ifndef NDEBUG
void
QcCoreTableau::assertInvar() const
{
  // A few important basic checks
  assert( fAllocRows >= fRows &&
	  fAllocColumns >= fColumns);

  // Check matrices and vectors are the right size
  assert( (fRowColState.fRowState->GetSize() == fRows)
	  &&        (fOrigRowState.GetSize() == fRows));

  // Now check a few integrity constraints for row state data
  for (unsigned r = 0; r < fRows; r++)
    {
      // First consider r as indexing rows of fM
      {
	int rARowIx = GetARowIndex( r);
	assert( (rARowIx == QcTableau::fInvalidConstraintIndex)
		|| ((unsigned) rARowIx < fRows));
	if (GetMRowDeleted( r))
	  {
	    assert( (r == (unsigned) GetMRowIndex( rARowIx))
		    && GetARowDeleted( rARowIx));
	  }
      }

      // Now consider r as indexing rows of fA (and hence columns of fM)
      {
	int rMRowIx = GetMRowIndex( r);
	assert( (rMRowIx == QcTableau::fInvalidConstraintIndex)
		|| ((unsigned) rMRowIx < fRows));
	if (GetARowDeleted( r))
	  {
	    assert( (r == (unsigned) GetARowIndex( rMRowIx))
		    && GetMRowDeleted( rMRowIx));
	  }
      }
    }
}
#endif

int QcCoreTableau::DeleteRow(unsigned ci)
{
	int quasiRow = GetMRowIndex(ci);
	qcAssert(GetARowIndex(quasiRow) == (int) ci);
	SetMRowDeleted(quasiRow, true);
	SetARowDeleted(ci, true);
	fSF.ZeroRow(quasiRow);
	SetRawRHS(ci, 0.0);
	return quasiRow;
}

unsigned QcCoreTableau::IncreaseColumns()
{
	qcAssert(fColumns <= fAllocColumns);

	if (fColumns == fAllocColumns) {
		fAllocColumns += (fAllocColumns == 0) ? 1 : fAllocColumns;
		fSF.Reserve(fAllocRows, fAllocColumns);
		fRowColState.fColState->Reserve(fAllocColumns);
	}

	fColumns++;
	fSF.Resize(fRows, fColumns);
	fRowColState.fColState->Resize(fColumns);

	// Set the default data values for the newly added column.
	fSF.ZeroColumn(fColumns - 1);

	return fColumns - 1;
}

unsigned QcCoreTableau::IncreaseRows()  // Add an extra row
{
	assert( fRows <= fAllocRows);

	if (fRows == fAllocRows) {
		fAllocRows += (fAllocRows == 0) ? 1 : fAllocRows;
		fSF.Reserve(fAllocRows, fAllocColumns);
		fRowColState.fRowState->Reserve(fAllocRows);
		fOrigRowState.Reserve(fAllocRows);
	}

	fRows++;
	fSF.Resize(fRows, fColumns);
	fRowColState.fRowState->Resize(fRows);
	fOrigRowState.Resize(fRows);

	// Set the default data values for the newly added row.
	fSF.ZeroRow(fRows - 1);
	return fRows - 1;
}

void QcCoreTableau::Initialize(unsigned hintRows, unsigned hintCols)
{
	qcAssertPre ((int) hintRows >= 0);
	qcAssertPre ((int) hintCols >= 0);

	fAllocRows = hintRows;
	fAllocColumns = hintCols;
	fSF.Reserve(hintRows, hintCols);
	fRowColState.Reserve(hintRows, hintCols);
	fOrigRowState.Reserve(hintRows);
	fRows = 0;
	fColumns = 0;
}

void QcCoreTableau::Print(ostream &os) const
{
	os << "QcCoreTableau with " << fRows << " rows and "
		<< fColumns << " cols." << endl;
	os << "==========================================================" << endl;
	os << "Raw RHS: ";

	for (unsigned i = 0; i < fRows; i++)
		os << "\t" << GetRawRHS(i);

	os << endl;
	os << "Deleted rows of fM are: ";

	for (unsigned i = 0; i < fRows; i++)
		if (GetMRowDeleted(i))
         		os << "\t" << i;

	os << endl;
	os << "Deleted rows of fA are: ";

	for (unsigned i = 0; i < fRows; i++)
		if (GetARowDeleted(i))
         		os << "\t" << i;

	os << endl << "Quasi RowState: ";
     fRowColState.fRowState->Print(os);
	fOrigRowState.Print(os);
}

void QcCoreTableau::Restart()
{
	fRows = 0;
	fColumns = 0;
	fSF.Restart();
	fRowColState.Restart();
	fOrigRowState.Resize(0);
}

void QcCoreTableau::Restore()
{
	unsigned co;		// original constraint index (row of fA)
	unsigned cs;		// solved form constraint index (row of fM)

	// Reset constraint state information
	for (cs = 0, co = 0; cs < fRows; cs++, co++) {
		// Find next undeleted constraint
		while (cs < fRows && GetMRowDeleted(cs))
			++cs;

		while (co < fRows && GetARowDeleted(co))
			++co;

		if (co < fRows) {
			qcAssert(cs < fRows);
			qcAssert(!GetARowDeleted(co));
			qcAssert(!GetMRowDeleted(cs));
			fSF.ZeroRow(cs);
			fSF.SetRHS(cs, fOrigRowState.GetRHS(co));
			SetMRowIndex(co, cs);
			SetARowIndex(cs, co);
			// N.B. We permit Reset to change the co<->cs relationship for
			// a previously invalid constraint.  As long as they
			// are empty it doesn't matter which are used.
		} else
			break;
	}

	// Fix row and column linked lists.
	fRowColState.fRowState->FixLinkage();
	fRowColState.fColState->FixLinkage();

	dbg(assertInvar());
}
