// Generated automatically from QcLinEqColStateVector.ch by /home/pmoulder/usr/local/bin/ch2xx.
#include "QcLinEqColStateVector.H"
#ifndef QcLinEqColStateVectorIFN
#define QcLinEqColStateVectorIFN
#line 1 "QcLinEqColStateVector.ch"
// $Id: QcLinEqColStateVector.ch,v 1.2 2000/12/06 05:32:56 pmoulder Exp $

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

#include "qoca/QcQuasiColStateVector.hh"
#include "qoca/QcLinEqColState.hh"







#line 49 "QcLinEqColStateVector.ch"
#ifndef NDEBUG



#line 121 "QcLinEqColStateVector.ch"
#endif /* !NDEBUG */


//-----------------------------------------------------------------------//
// Query functions.                                                      //
//-----------------------------------------------------------------------//


inline numT
QcLinEqColStateVector::GetDesireValue(unsigned i) const
{
  qcAssertPre (i < fSize);
  return ((QcLinEqColState *)fStates[i])->fDesValue;
}

inline bool
QcLinEqColStateVector::IsBasic(unsigned i) const
{
  qcAssertPre (i < fSize);
  return ((QcLinEqColState *)fStates[i])->fIsBasic;
}

inline int
QcLinEqColStateVector::IsBasicIn(unsigned i) const
{
  qcAssertPre (i < fSize);
  return ((QcLinEqColState *)fStates[i])->fIsBasicIn;
}


//-----------------------------------------------------------------------//
// Set functions.                                                        //
//-----------------------------------------------------------------------//
inline void
QcLinEqColStateVector::SetDesireValue(unsigned i, numT dv) const
{
  qcAssertPre (i < fSize);
  ((QcLinEqColState *)fStates[i])->fDesValue = dv;
}

inline QcLinEqColState *
QcLinEqColStateVector::head( bool isbasic)
{
  QcLinEqColState *heads = &fParamHead;
  assert( &heads[1] == &fBasicHead);
  unsigned ix = isbasic;
  qcAssertPre( ix <= 1);
  return &heads[ix];
}


#line 187 "QcLinEqColStateVector.ch"
inline void
QcLinEqColStateVector::SetBasicIn(unsigned i, int bi) const
{
  qcAssertPre (i < fSize);
  ((QcLinEqColState *)fStates[i])->fIsBasicIn = bi;
}


//-----------------------------------------------------------------------//
// Manipulation functions.                                               //
//-----------------------------------------------------------------------//

#line 223 "QcLinEqColStateVector.ch"
inline void
QcLinEqColStateVector::Link(QcLinEqColState *state, QcLinEqColState *head)
{
  qcAssertPre( (head == &fParamHead)
	       || (head == &fBasicHead));
  qcAssertPre( (state->fNextCol == 0)
	       && (state->fPrevCol == 0));

  state->fPrevCol = head;
  head->fNextCol->fPrevCol = state;
  state->fNextCol = head->fNextCol;
  head->fNextCol = state;
}


inline void
QcLinEqColStateVector::Unlink(QcLinEqColState *state)
{
  qcAssertPre( (state->fNextCol != 0)
	       && (state->fPrevCol != 0));

  state->fNextCol->fPrevCol = state->fPrevCol;
  state->fPrevCol->fNextCol = state->fNextCol;

  dbg(state->fPrevCol = state->fNextCol = 0);
}





#line 340 "QcLinEqColStateVector.ch"
inline QcLinEqColState *
QcLinEqColStateVector::getState(unsigned i) const
{
  qcAssertPre( i < fSize);
  QcLinEqColState *ret = CAST(QcLinEqColState *, fStates[i]);
  qcAssertPost( ret != 0);
  return ret;
}



#line 358 "QcLinEqColStateVector.ch"
/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/

#endif /* !QcLinEqColStateVectorIFN */
