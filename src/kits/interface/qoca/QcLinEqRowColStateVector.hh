// $Id: QcLinEqRowColStateVector.hh,v 1.8 2001/01/30 01:32:08 pmoulder Exp $

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

#ifndef __QcLinEqRowColStateVectorH
#define __QcLinEqRowColStateVectorH

#include "qoca/QcQuasiRowColStateVector.hh"
#include "qoca/QcLinEqRowStateVector.hh"
#include "qoca/QcLinEqColStateVector.hh"

class QcLinEqRowColStateVector : public QcQuasiRowColStateVector
{
public:
	QcLinEqRowStateVector *fRowState;
	QcLinEqColStateVector *fColState;

	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
	QcLinEqRowColStateVector();
	QcLinEqRowColStateVector(QcLinEqColStateVector *csv);

	//-----------------------------------------------------------------------//
	// Query functions.                                                      //
	//-----------------------------------------------------------------------//
	QcLinEqRowStateVector &GetRowState() const
		{ return *fRowState; }

	QcLinEqColStateVector &GetColState() const
		{ return *fColState; }

	//-----------------------------------------------------------------------//
	// Manipulation functions.                                               //
	//-----------------------------------------------------------------------//
	virtual void SwapColumns(unsigned v1, unsigned v2);
#if 0 /* unused */
	virtual void SwapRows(unsigned c1, unsigned c2);
#endif
};

inline QcLinEqRowColStateVector::QcLinEqRowColStateVector()
	: QcQuasiRowColStateVector(new QcLinEqRowStateVector(),
				   new QcLinEqColStateVector())
{
	fRowState = (QcLinEqRowStateVector *)QcQuasiRowColStateVector::fRowState;
  	fColState = (QcLinEqColStateVector *)QcQuasiRowColStateVector::fColState;
}

inline QcLinEqRowColStateVector::QcLinEqRowColStateVector(
		QcLinEqColStateVector *csv)
	: QcQuasiRowColStateVector(new QcLinEqRowStateVector(), csv) 
{
	fRowState = (QcLinEqRowStateVector *)QcQuasiRowColStateVector::fRowState;
  	fColState = (QcLinEqColStateVector *)QcQuasiRowColStateVector::fColState;
}

inline void QcLinEqRowColStateVector::SwapColumns(unsigned v1, unsigned v2)
{
  qcAssertPre( v1 < fColState->getNColumns());
  qcAssertPre( v2 < fColState->getNColumns());

  if (v1 == v2)
    return;

  QcLinEqColState *r1 = (QcLinEqColState *)fColState->GetState(v1);
  QcLinEqColState *r2 = (QcLinEqColState *)fColState->GetState(v2);

  // Swap col references in fRowState
  {
    int c1 = r1->getConstraintBasicIn();
    int c2 = r2->getConstraintBasicIn();

    if (c1 != QcTableau::fInvalidConstraintIndex)
      fRowState->SetBasicVar( c1, v2);

    if (c2 != QcTableau::fInvalidConstraintIndex)
      fRowState->SetBasicVar( c2, v1);
  }

  // Exchange the data contents (excluding the Index field)
  {
    int isbin = r1->getConstraintBasicIn();
    r1->fIsBasicIn = r2->getConstraintBasicIn();
    r2->fIsBasicIn = isbin;
  }
  {
    numT dv = r1->fDesValue;
    r1->fDesValue = r2->fDesValue;
    r2->fDesValue = dv;
  }

  // Now fix up the linkage
  if (r1->fIsBasic != r2->fIsBasic)
    {	// N.B. its an unordered list!
      if (r2->fIsBasic)
	{				// Make r1-> original basic
	  QcLinEqColState *temp = r2;	//  and r2-> original parameter
	  r2 = r1;
	  r1 = temp;
	}
      assert( r1->fIsBasic && !r2->fIsBasic);

      // First, the Basic vars linked list
      fColState->Unlink( r1);
      fColState->Unlink( r2);

      // Finally, swap over the descriminator.
      r1->fIsBasic = false;
      r2->fIsBasic = true;

      fColState->Link( r1, &fColState->fParamHead);
      fColState->Link( r2, &fColState->fBasicHead);
    }
}

#if 0 /* unused */
inline void QcLinEqRowColStateVector::SwapRows(unsigned c1, unsigned c2)
{
  qcAssertPre( c1 < fRowState->getNRows());
  qcAssertPre( c2 < fRowState->getNRows());

  if (c1 == c2)
    return;

  QcLinEqRowState *r1 = (QcLinEqRowState *)fRowState->GetState(c1);
  QcLinEqRowState *r2 = (QcLinEqRowState *)fRowState->GetState(c2);

  // Swap row references in ColState
  int v1 = r1->fBasicVar;
  int v2 = r2->fBasicVar;

  if (v1 != QcTableau::fInvalidVariableIndex)
    fColState->SetBasicIn(v1, c2);

  if (v2 != QcTableau::fInvalidVariableIndex)
    fColState->SetBasicIn(v2, c1);

  // Swap fCondition, fBasicVar
  r1->swap( *r2);

  // Swap lower level properties
  QcQuasiRowColStateVector::SwapRows(c1, c2);
}
#endif /* unused */

#endif /* !__QcLinEqRowColStateVectorH */
