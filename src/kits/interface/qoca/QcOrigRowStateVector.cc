// Generated automatically from QcOrigRowStateVector.ch by /home/pmoulder/usr/local/bin/ch2xx.
#include "QcOrigRowStateVector.hh"
#line 1 "QcOrigRowStateVector.ch"
// $Id: QcOrigRowStateVector.ch,v 1.2 2000/12/06 05:32:56 pmoulder Exp $
// Originally written by Alan Finlay and Sitt Sen Chok

//%BeginLegal ================================================================
//
// The QOCA implementation is free software, but it is Copyright (C)
// 1994-1999 Monash University.  It is distributed under the terms of the GNU
// General Public License.  See the file COPYING for copying permission.
//
// The QOCA toolkit and runtime are distributed under the terms of the GNU
// Library General Public License.  See the file COPYING.LIB for copying
// permissions for those files.
//
// If those licencing arrangements are not satisfactory, please contact us!
// We are willing to offer alternative arrangements, if the need should arise.
//
// THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED OR
// IMPLIED.  ANY USE IS AT YOUR OWN RISK.
//
// Permission is hereby granted to use or copy this program for any purpose,
// provided the above notices are retained on all copies.  Permission to
// modify the code and to distribute modified code is granted, provided the
// above notices are retained, and a notice that the code was modified is
// included with the above copyright notice.
//
//%EndLegal ==================================================================

#include <qoca/QcDefines.hh>
#include "qoca/QcStateVector.hh"
#include "qoca/QcOrigRowState.hh"






#line 44 "QcOrigRowStateVector.ch"
//-----------------------------------------------------------------------//
// Query functions.                                                      //
//-----------------------------------------------------------------------//

#line 71 "QcOrigRowStateVector.ch"
//-----------------------------------------------------------------------//
// Set* functions.
//-----------------------------------------------------------------------//
#line 95 "QcOrigRowStateVector.ch"
//-----------------------------------------------------------------------//
// Manipulation functions.                                               //
//-----------------------------------------------------------------------//

void
QcOrigRowStateVector::FixLinkage()
{
}




void
QcOrigRowStateVector::Alloc(QcState **start, QcState **finish)
{
  qcAssertPre( fStates <= start);
  qcAssertPre( start <= finish);
  qcAssertPre( finish <= fStates + fCapacity);

  for (unsigned i = start - fStates;
       start != finish;
       start++, i++)
    *start = new QcOrigRowState( i);
}



void
QcOrigRowStateVector::AddToList(QcState **start, QcState **finish)
{
  qcAssertPre( fStates <= start);
  qcAssertPre( start <= finish);
  qcAssertPre( finish <= getAllocEnd());

  UNUSED(start);
  UNUSED(finish);
}



void
QcOrigRowStateVector::RemoveFromList(QcState **start, QcState **finish)
{
  qcAssertPre( fStates <= start);
  qcAssertPre( start <= finish);
  qcAssertPre( finish <= fStates + fSize);

  while(start != finish)
    {
      QcOrigRowState *state = CAST(QcOrigRowState *, *start);
      state->fRHS = 0.0;
      state->fMRowIndex = start - fStates;
      state->fARowDeleted = false;
      start++;
    }
}

#ifndef NDEBUG

void
QcOrigRowStateVector::virtualAssertLinkageInvar() const
{
  for (QcState const * const *i = fStates + fSize; i < getAllocEnd(); i++)
    {
      QcOrigRowState const *state = CAST(QcOrigRowState const *, *i);
      assert( state->fRHS == 0.0);
      assert( state->fMRowIndex == (i - (QcState const * const *)fStates));
      assert( !state->fARowDeleted);
    }
}
#endif



#line 191 "QcOrigRowStateVector.ch"
/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/
