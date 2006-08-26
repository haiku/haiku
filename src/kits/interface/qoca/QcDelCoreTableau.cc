// $Id: QcDelCoreTableau.cc,v 1.16 2001/01/31 10:03:35 pmoulder Exp $

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
#include "qoca/QcSparseMatrixRowIterator.hh"
#include "qoca/QcDenseMatrixColIterator.hh"
#include "qoca/QcDelCoreTableau.hh"
#include "qoca/QcRowAdaptor.hh"

unsigned QcDelCoreTableau::AddRow(QcRowAdaptor &varCoeffs, numT rhs)
{
	unsigned r = 0;     // Row of fA, col of fM, int for original constraint
	int quasiRow;	// Row of fM, int for solved form constraint

	// Look for a deleted row or create a new row if one not found
	while (r < fRows && !GetARowDeleted(r))
		r++;

	if (r == fRows) {
		r = IncreaseRows();
		quasiRow = GetMRowIndex(r);
		qcAssert(quasiRow == (int) r);	// Currently IncreaseRows ensures this
	} else {
		quasiRow = GetMRowIndex(r);
		fM.SetValue(quasiRow, r, 1.0);
		SetARowDeleted(r, false);
		SetMRowDeleted(quasiRow, false);
	}

	#ifdef qcSafetyChecks
	for (unsigned j = 0; j < fColumns; j++)
    		qcAssert(fA.GetValue(r, j) == 0);
	qcAssert(GetRawRHS(r) == 0.0);

	for (unsigned i = 0; i < fRows; i++) {
		if (i != r)
			qcAssert(fM.GetValue(quasiRow, i) == 0.0); // quasiRow of fM
		if ((int) i != quasiRow)
			qcAssert(fM.GetValue(i, r) == 0.0); // column r of fM
	}
	qcAssert(fM.GetValue(quasiRow, r) == 1);
	#endif

	// Load the new values into the fA matrix and RHS
	SetRawRHS(r, rhs);
	fSF.SetRHS(quasiRow, rhs);

	while (!varCoeffs.AtEnd()) {
		assert( varCoeffs.getIndex() < fColumns);
		#ifdef qcSafetyChecks
		if (QcUtility::IsZero( varCoeffs.getValue()))
			throw QcWarning(qcPlace, "AddRow: Zero coeff supplied");
		#endif
		fA.SetValue(r, varCoeffs.getIndex(), varCoeffs.getValue());
		fSF.SetValue(quasiRow, varCoeffs.getIndex(), varCoeffs.getValue());
		varCoeffs.Increment();
	}

	return r;
}

void QcDelCoreTableau::ChangeRHS(unsigned ci, numT rhs)
{
#ifdef qcRealTableauRHS
  numT diff = rhs - GetRawRHS( ci);

  if(!QcUtility::IsZero( diff))
    {
# ifdef qcDenseQuasiInverse
      QcDenseMatrixColIterator colIt( fM, ci);
# else
      QcSparseMatrixColIterator colIt( fM, ci);
# endif

      assert( GetRows() == fM.getNRows());
      for(; !colIt.AtEnd(); colIt.Increment())
	{
	  unsigned i = colIt.getIndex();

	  if(!GetMRowDeleted( i))
	    fSF.IncreaseRHS( i, colIt.getValue() * diff);
	}
    }
#endif

  QcCoreTableau::ChangeRHS( ci, rhs);
}

void QcDelCoreTableau::DecreaseColumns(QcBiMapNotifier &note)
	// Get rid of last column provided the variable is not used.
{
	qcAssert(fColumns > 0);
	qcAssert(IsFree(fColumns - 1));
	fColumns--;
	fA.Resize(fRows, fColumns);
	fRowColState.fColState->Resize(fColumns);
	note.DropVariable(fColumns);
	fSF.Resize(fRows, fColumns);
}

void QcDelCoreTableau::DecreaseRows(QcBiMapNotifier &note)
	// Get rid of the last row - provided it is a deleted row for
	// both original and solved form constraint.
	// N.B. The deleted solved form and original constaint need
	// not correspond to each other.
{
	qcAssert(fRows > 0);
	qcAssert(GetARowDeleted(fRows - 1));
	qcAssert(GetMRowDeleted(fRows - 1));
	int AIndex = GetARowIndex(fRows - 1);
	int MIndex = GetMRowIndex(fRows - 1);

	// If the last (deleted) original and solved form constraints
	// don't correspond then swap the indices around so there will
	// be no invalid indices after this routine finishes.
	if (AIndex != (int) fRows - 1) {
		qcAssert(MIndex != (int) fRows - 1);
		SetMRowIndex(AIndex, MIndex);
		SetARowIndex(MIndex, AIndex);
	}

	fRows--;
	fM.Resize(fRows, fRows);
	fA.Resize(fRows, fColumns);
	fSF.Resize(fRows, fColumns);
	fRowColState.fRowState->Resize(fRows);
	fOrigRowState.Resize(fRows);
	note.DropConstraint(fRows);
}

numT QcDelCoreTableau::GetRHSVirtual(unsigned ci) const
	// See note for GetValue member
{
	qcAssertPre (ci < fRows);

	if (GetMRowDeleted(ci))
		return 0.0;

	numT sum = 0.0;

	for (unsigned i = 0; i < fRows; i++)
		sum += fM.GetValue(ci, i) * GetRawRHS(i);

	return QcUtility::Zeroise(sum);
}

numT QcDelCoreTableau::GetValueVirtual(unsigned ci, unsigned vi) const
{
  qcAssertPre( ci < fRows);
  qcAssertPre( vi < fColumns);

  numT sum( 0);

  if (GetMRowDeleted( ci))
    return sum;

  for (QcSparseMatrixColIterator varcol( fA, vi);
       !varcol.AtEnd();
       varcol.Increment())
    {
      sum += (fM.GetValue( ci, varcol.getIndex())
	      * varcol.getValue());
    }
  return QcUtility::Zeroise(sum);
}

unsigned QcDelCoreTableau::IncreaseColumns()
	// Add and extra column
{
	qcAssert(fColumns <= fAllocColumns);

	if (fColumns == fAllocColumns) {
		fAllocColumns += (fAllocColumns == 0) ? 8 : fAllocColumns;
		fA.Reserve(fAllocRows, fAllocColumns);
		fSF.Reserve(fAllocRows, fAllocColumns);
		fRowColState.fColState->Reserve(fAllocColumns);
	}

	fColumns++;
	fA.Resize(fRows, fColumns);
	fRowColState.fColState->Resize(fColumns);
	fSF.Resize(fRows, fColumns);

	// Set the default data values for the newly added column
	fA.ZeroColumn(fColumns - 1);
	fSF.ZeroColumn(fColumns - 1);

	return fColumns - 1;
}

unsigned QcDelCoreTableau::IncreaseRows()
	// Add an extra row
{
	qcAssert(fRows <= fAllocRows);

	if (fRows == fAllocRows) {
		fAllocRows += (fAllocRows == 0) ? 8 : fAllocRows;
		fA.Reserve(fAllocRows, fAllocColumns);
		fM.Reserve(fAllocRows, fAllocRows);
		fSF.Reserve(fAllocRows, fAllocColumns);
		fRowColState.fRowState->Reserve(fAllocRows);
		fOrigRowState.Reserve(fAllocRows);
	}

	fRows++;			     	// logically, these 3 lines are where the
	fA.Resize(fRows, fColumns);	// number of rows increases, the optimiser
	fM.Resize(fRows, fRows);		// should cache the (fRows-1) that follow.
	fSF.Resize(fRows, fColumns);
	fRowColState.fRowState->Resize(fRows);
	fOrigRowState.Resize(fRows);

	// Set the default data values for the newly added row
	fA.ZeroRow(fRows - 1);
	fM.ZeroRow(fRows - 1);
	fM.ZeroColumn(fRows - 1);
	fM.SetValue(fRows - 1, fRows - 1, 1.0);
	fSF.ZeroRow(fRows - 1);
	return fRows - 1;
}

bool QcDelCoreTableau::IsValidSolvedForm() const
{
  /* effic: Use iterators. */
  for (unsigned i = 0; i < fRows; i++)
    {
      for (unsigned j = 0; j < fColumns; j++)
	{
	  // TODO: Should be numRefT.
	  numT val_ij( GetValue( i, j));
	  numT virt_val_ij( GetValueVirtual( i, j));
	  if (QcUtility::IsZero( val_ij)
	      ? !QcUtility::IsZero( virt_val_ij)
	      : !QcUtility::IsVaguelyZero( virt_val_ij - val_ij))
	    return false;
	}

      numT rhs_i( GetRHS( i));
      numT virt_rhs_i( GetRHSVirtual( i));
      if (QcUtility::IsZero( rhs_i)
	  ? !QcUtility::IsZero( virt_rhs_i)
	  : !QcUtility::IsVaguelyZero( virt_rhs_i - rhs_i))
	return false;
    }

  return true;
}

void QcDelCoreTableau::Print(ostream &os) const
{
	os << "QcDelCoreTableau with " << fRows << " rows and ";
	os << fColumns << " cols." << endl;
	os << "==========================================================" << endl;
	os << "Quasi Inverse Matrix:" << endl << fM << endl;
	os << "Constraint Matrix:" << endl << fA << endl;
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
	os << "Quasi OrigRowState: ";
	fOrigRowState.Print(os);
}

void QcDelCoreTableau::PivotQuasiInverse(unsigned row, unsigned col)
{
#ifdef qcRealTableauCoeff
  numT p = fM.GetValue(row, col);
  qcAssertPre( !QcUtility::IsZero(p));

  ScaleRow( row, recip( p));

# ifdef qcDenseQuasiInverse
  for (QcDenseMatrixColIterator colCoeffs( fM, col);
       !colCoeffs.AtEnd();
       colCoeffs.Increment())
    {
      if (colCoeffs.getIndex() != row)
	AddScaledRow( colCoeffs.getIndex(), row, -colCoeffs.getValue());
    }
# elif defined(qcUseSafeIterator)
  for (unsigned i = 0; i < fM.GetRows(); i++)
    {
      if (i != (unsigned) row)
	AddScaledRow( i, row, -fM.GetValue( i, col));
    }
# else
  /* Sparse column iterators are not guaranteed to be sane if the underlying
     matrix gets modified (here by AddScaledRow), so we cache the column
     coefficients in advance. */
  {
    vector<QcSparseCoeff> coeffCache;

    for (QcSparseMatrixColIterator colIt( fM, col);
	 !colIt.AtEnd();
	 colIt.Increment())
      {
	unsigned i = colIt.getIndex();
	if(i != row)
	  coeffCache.push_back( QcSparseCoeff( colIt.getValue(), i));
      }

    for (vector<QcSparseCoeff>::iterator colIt = coeffCache.begin();
	 colIt != coeffCache.end();
	 colIt++)
      AddScaledRow( colIt->getIndex(), row, -colIt->getValue());
  }
# endif
#else /* !qcRealTableauCoeff */
  fM.Pivot( row, col);
#endif
}

void QcDelCoreTableau::Restart()
{
	fRows = 0;
	fColumns = 0;
	fM.Resize(0, 0);
	fA.Resize(0, 0);
	fSF.Restart();
	fRowColState.Restart();
	fOrigRowState.Resize(0);
}

void QcDelCoreTableau::Restore()
{
  unsigned co;		// original constraint index (row of fA)
  unsigned cs;		// solved form constraint index (row of fM)

  // Reset constraint state information
  for (cs = 0, co = 0; cs < fRows; cs++, co++)
    {
      // Find next undeleted constraint
      while (cs < fRows && GetMRowDeleted(cs))
	++cs;

      while (co < fRows && GetARowDeleted(co))
	++co;

      if (co < fRows)
	{
	  qcAssert( cs < fRows);
	  qcAssert( !GetARowDeleted( co));
	  qcAssert( !GetMRowDeleted( cs));
	  fM.ZeroRow( cs);
	  fM.ZeroColumn( co);
	  fM.SetValue( cs, co, 1.0);
	  fSF.CopyCoeffRow( cs, fA, co);
	  fSF.SetRHS( cs, fOrigRowState.GetRHS( co));
	  SetMRowIndex( co, cs);
	  SetARowIndex( cs, co);
	  // N.B. We permit Reset to change the co<->cs relationship for
	  // a previously invalid constraint.  As long as they
	  // are empty it doesn't matter which are used.
	}
      else
	break;
    }

  // Fix row and column linked lists.
  fRowColState.fRowState->FixLinkage();
  fRowColState.fColState->FixLinkage();

  dbg(assertInvar());
}

void QcDelCoreTableau::TransitiveClosure(vector<unsigned int> &vars) const
{
  /* effic: Should use a todo set.  Using a todo set is probably O(n), whereas
     the current method is probably O(n**2). */
  bool checkAgain = true;

  if (fColumns == 0 || fRows == 0)
    return;

  QcSparseMatrixColIterator varCol(fA, 0);
  QcSparseMatrixRowIterator varRow(fA, 0);
  static vector<bool> fScatchFlags;
  fScatchFlags.resize(fColumns);

  for (unsigned int i = 0; i < fScatchFlags.size(); i++)
    fScatchFlags[i] = false;

  for (unsigned int i = 0; i < vars.size(); i++)
    fScatchFlags[vars[i]] = true;

  while (checkAgain)
    {
      checkAgain = false;

      for (unsigned int i = 0; i < fScatchFlags.size(); i++)
	{
	  if (fScatchFlags[i])
	    {
	      for (varCol.SetToBeginOf( i);
		   !varCol.AtEnd();
		   varCol.Increment())
		{
		  for (varRow.SetToBeginOf( varCol.GetIndex());
		       !varRow.AtEnd();
		       varRow.Increment())
		    {
		      if (!fScatchFlags[varRow.GetIndex()])
			{
			  fScatchFlags[varRow.GetIndex()] = true;
			  vars.push_back( varRow.GetIndex());
			  checkAgain = true;
			}
		    }
		}
	    }
	}
    }
}
