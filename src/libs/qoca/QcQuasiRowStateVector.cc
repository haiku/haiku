// Generated automatically from QcQuasiRowStateVector.ch by /home/pmoulder/usr/local/bin/ch2xx.
#include "QcQuasiRowStateVector.hh"
#line 1 "QcQuasiRowStateVector.ch"
// $Id: QcQuasiRowStateVector.ch,v 1.1 2000/11/29 01:49:41 pmoulder Exp $

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

#include "qoca/QcStateVector.hh"
#include "qoca/QcQuasiRowState.hh"







#line 78 "QcQuasiRowStateVector.ch"
void
QcQuasiRowStateVector::SetMRowDeleted(unsigned i, bool d)
{
  qcAssertPre( i < fSize);
  QcQuasiRowState *state = getState( i);
  if (state->fMRowDeleted != d)
    {
      if (d)
	UnlinkRow( state);
      else
	LinkRow( state);
    }
  state->fMRowDeleted = d;
}


//-----------------------------------------------------------------------//
// Manipulation functions.                                               //
//-----------------------------------------------------------------------//



void
QcQuasiRowStateVector::AddToList(QcState **start, QcState **finish)
{
  qcAssertPre( fStates <= start);
  qcAssertPre( start < finish);
  qcAssertPre( finish <= getAllocEnd());

  for(; start != finish; start++)
    {
      QcQuasiRowState *state = CAST(QcQuasiRowState *, *start);

      assert( (state->fNextRow == 0)
	      && (state->fPrevRow == 0)
	      && (state->fARowIndex == start - fStates)
	      && (!state->fMRowDeleted));

      LinkRow( state);
    }
}

#ifndef NDEBUG



void
QcQuasiRowStateVector::assertLinkageInvar() const
{
  unsigned nLive = 0;
  for (QcState const * const *i = fStates + fSize; i-- != fStates;)
    {
      QcQuasiRowState const *state = CAST(QcQuasiRowState const *, *i);
      if (state->fMRowDeleted)
	{
	  assert( state->fPrevRow == 0);
	  assert( state->fNextRow == 0);
	}
      else
	nLive++;
    }

  assert( (fRowList == 0)
	  || (fRowList->fPrevRow == 0));
  for (QcQuasiRowState const *curr = fRowList;
       curr != 0;
       curr = curr->fNextRow)
    {
      assert( !curr->fMRowDeleted);
      assert( (curr->fNextRow == 0)
	      || (curr->fNextRow->fPrevRow == curr));
      assert( nLive != 0);
      nLive--;
    }
  assert( nLive == 0);
}




void
QcQuasiRowStateVector::virtualAssertLinkageInvar() const
{
  assertLinkageInvar();
}
#endif




void
QcQuasiRowStateVector::FixLinkage()
{
  dbg(assertLinkageInvar());

#if 0 /* should not be necessary */
  fRowList = 0;

  for (unsigned i = 0; i < fSize; i++)
    {
      QcQuasiRowState *state = (QcQuasiRowState *)fStates[i];
      state->fNextRow = 0;
      state->fPrevRow = 0;

      if (!state->fMRowDeleted)
	LinkRow( state);
    }
#endif
}




#line 260 "QcQuasiRowStateVector.ch"
//-----------------------------------------------------------------------//
// Utility functions.                                                    //
//-----------------------------------------------------------------------//


void
QcQuasiRowStateVector::Print(ostream &os) const
{
  os << endl << "Row State Vector:";
  QcStateVector::Print(os);
  os << endl << "Row List:";
  QcQuasiRowState *curr = fRowList;

  while (curr != 0)
    {
      curr->Print(os);
      os << endl;
      curr = curr->fNextRow;
    }
}


#line 299 "QcQuasiRowStateVector.ch"
/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/
