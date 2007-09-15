// Generated automatically from QcLinInEqColStateVector.ch by /home/pmoulder/usr/local/bin/ch2xx.
#include "QcLinInEqColStateVector.hh"
#line 1 "QcLinInEqColStateVector.ch"
// $Id: QcLinInEqColStateVector.ch,v 1.5 2000/12/13 01:44:58 pmoulder Exp $

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



#include <qoca/QcLinInEqColState.hh>







#line 89 "QcLinInEqColStateVector.ch"
//-----------------------------------------------------------------------//
// Setter functions
//-----------------------------------------------------------------------//


void
QcLinInEqColStateVector::SetCondition(unsigned i, int c)
{
  qcAssertPre( i < fSize);
  QcLinInEqColState *state = getState( i);
  if ((c ^ state->getCondition())
      & QcLinInEqColState::fStructural)
    {
      if (c & QcLinInEqColState::fStructural)
	LinkStruct( state);
      else
	UnlinkStruct( state);
    }
  state->setCondition( c);
}

#line 125 "QcLinInEqColStateVector.ch"
#ifndef NDEBUG

void
QcLinInEqColStateVector::assertLinkageInvar() const
{
  unsigned nStructural = 0;
  for (QcState **i = fStates + fSize; i-- != fStates;)
    {
      QcLinInEqColState const *state = CAST(QcLinInEqColState const *, *i);
      if (state->getCondition() & QcLinInEqColState::fStructural)
	nStructural++;
      else
	{
	  assert( state->fPrevStruct == 0);
	  assert( state->fNextStruct == 0);
	}
    }

  assert( (fStructVarList == 0)
	  || (fStructVarList->fPrevStruct == 0));
  for (QcLinInEqColState *curr = fStructVarList;
       curr != 0;
       curr = curr->fNextStruct)
    {
      assert( curr->getCondition() & QcLinInEqColState::fStructural);
      assert( (curr->fNextStruct == 0)
	      || (curr->fNextStruct->fPrevStruct == curr));
      assert( nStructural != 0);
      nStructural--;
    }
  assert( nStructural == 0);
}


void
QcLinInEqColStateVector::virtualAssertLinkageInvar() const
{
  assertLinkageInvar();
  QcLinEqColStateVector::virtualAssertLinkageInvar();
}
#endif



void
QcLinInEqColStateVector::FixLinkage()
{
  dbg(assertLinkageInvar());

#if 0 /* shouldn't be necessary any more. */
  fStructVarList = 0;

  for (unsigned i = 0; i < fSize; i++)
    {
      QcLinInEqColState *state = (QcLinInEqColState *)fStates[i];
      state->fNextStruct = 0;
      state->fPrevStruct = 0;

      if (state->getCondition() & QcLinInEqColState::fStructural)
	LinkStruct( state);
    }
#endif

  QcLinEqColStateVector::FixLinkage();
}



#line 242 "QcLinInEqColStateVector.ch"
void
QcLinInEqColStateVector::Alloc(QcState **start, QcState **finish)
{
  qcAssertPre( fStates <= start);
  qcAssertPre( start <= finish);
  qcAssertPre( finish <= fStates + fCapacity);

  for (unsigned i = start - fStates;
       start != finish;
       start++, i++)
    *start = new QcLinInEqColState( i);
}



void
QcLinInEqColStateVector::AddToList(QcState **start, QcState **finish)
{
  qcAssertPre( fStates <= start);
  qcAssertPre( start < finish);
  qcAssertPre( finish <= getAllocEnd());

  QcLinEqColStateVector::AddToList( start, finish);

  for(; start != finish; start++)
    {
      QcLinInEqColState *state = CAST(QcLinInEqColState *, *start);

      assert( (state->fNextStruct == 0)
	      && (state->fPrevStruct == 0)
	      && (state->getCondition() == QcLinInEqColState::fStructural)
	      && (!state->fIsConstrained)
	      && (state->fObj == 0.0));

      LinkStruct( state);
    }
}



void
QcLinInEqColStateVector::RemoveFromList(QcState **start, QcState **finish)
{
  qcAssertPre( fStates <= start);
  qcAssertPre( start <= finish);
  qcAssertPre( finish <= fStates + fSize);

  for (QcState **i = start; i != finish; i++)
    {
      QcLinInEqColState *state = CAST(QcLinInEqColState *, *i);

      if (state->getCondition() & QcLinInEqColState::fStructural)
	UnlinkStruct( i - fStates);
      state->setCondition( QcLinInEqColState::fStructural);
      state->fIsConstrained = false;
      state->fObj = 0.0;
    }

  QcLinEqColStateVector::RemoveFromList( start, finish);
}



#line 323 "QcLinInEqColStateVector.ch"
/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/
