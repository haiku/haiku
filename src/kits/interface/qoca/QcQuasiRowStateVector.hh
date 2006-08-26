// Generated automatically from QcQuasiRowStateVector.ch by /home/pmoulder/usr/local/bin/ch2xx.
#include "QcQuasiRowStateVector.H"
#ifndef QcQuasiRowStateVectorIFN
#define QcQuasiRowStateVectorIFN
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







#line 94 "QcQuasiRowStateVector.ch"
//-----------------------------------------------------------------------//
// Manipulation functions.                                               //
//-----------------------------------------------------------------------//



#line 120 "QcQuasiRowStateVector.ch"
#ifndef NDEBUG



#line 163 "QcQuasiRowStateVector.ch"
#endif




#line 191 "QcQuasiRowStateVector.ch"
inline void
QcQuasiRowStateVector::LinkRow(unsigned index)
{
  qcAssertPre (index < fSize);
  QcQuasiRowState *me = (QcQuasiRowState *)fStates[index];
  LinkRow( me);
}

inline void
QcQuasiRowStateVector::LinkRow(QcQuasiRowState *me)
{
  qcAssertPre( me->fPrevRow == 0);
  qcAssertPre( me->fNextRow == 0);
  qcAssertPre( me != fRowList);

  if (fRowList != 0)
    {
      me->fNextRow = fRowList;
      fRowList->fPrevRow = me;
    }

  fRowList = me;
}


inline void
QcQuasiRowStateVector::UnlinkRow(unsigned index)
{
  qcAssertPre (index < fSize);

  QcQuasiRowState *me = (QcQuasiRowState *)fStates[index];
  UnlinkRow( me);
}

inline void
QcQuasiRowStateVector::UnlinkRow(QcQuasiRowState *me)
{
  qcAssertPre( (me->fPrevRow == 0)
	       == (fRowList == me));

  if (me->fPrevRow == 0)
    fRowList = me->fNextRow;
  else
    me->fPrevRow->fNextRow = me->fNextRow;

  if (me->fNextRow != 0)
    me->fNextRow->fPrevRow = me->fPrevRow;

  me->fPrevRow = 0;
  me->fNextRow = 0;
}



inline void
QcQuasiRowStateVector::Restart(unsigned i)
{
  qcAssertPre (i < fSize);

  QcQuasiRowState *state = (QcQuasiRowState *)fStates[i];

  if (!state->fMRowDeleted)
    UnlinkRow(i);

  state->fARowIndex = (int) i;
  state->fMRowDeleted = false;
}


//-----------------------------------------------------------------------//
// Utility functions.                                                    //
//-----------------------------------------------------------------------//


#line 282 "QcQuasiRowStateVector.ch"
inline QcQuasiRowState *
QcQuasiRowStateVector::getState( unsigned i) const
{
  qcAssertPre( i < fSize);
  QcQuasiRowState *ret = CAST(QcQuasiRowState *, fStates[i]);
  qcAssertPost( ret != 0);
  return ret;
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

#endif /* !QcQuasiRowStateVectorIFN */
