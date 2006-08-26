// Generated automatically from QcLinInEqColStateVector.ch by /home/pmoulder/usr/local/bin/ch2xx.
#include "QcLinInEqColStateVector.H"
#ifndef QcLinInEqColStateVectorIFN
#define QcLinInEqColStateVectorIFN
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


#include <qoca/QcLinEqColStateVector.hh>
#include <qoca/QcLinInEqColState.hh>







#line 89 "QcLinInEqColStateVector.ch"
//-----------------------------------------------------------------------//
// Setter functions
//-----------------------------------------------------------------------//


#line 110 "QcLinInEqColStateVector.ch"
inline void
QcLinInEqColStateVector::SetObjValue(unsigned i, numT v)
{
  qcAssertPre (i < fSize);
  ((QcLinInEqColState *)fStates[i])->fObj = v;
}

inline void
QcLinInEqColStateVector::SetConstrained(unsigned i, bool c)
{
  qcAssertPre (i < fSize);
  ((QcLinInEqColState *)fStates[i])->fIsConstrained = c;
}


#ifndef NDEBUG

#line 165 "QcLinInEqColStateVector.ch"
#endif



#line 193 "QcLinInEqColStateVector.ch"
inline void
QcLinInEqColStateVector::LinkStruct(QcLinInEqColState *state)
	// Only accesses the current item and those linked to StructVarList.
	// Other items may have invalid NextStruct and PrevStruct pointers.
{
  qcAssertPre( state->fPrevStruct == 0);
  qcAssertPre( state->fNextStruct == 0);
  qcAssertPre( state != fStructVarList);

  if (fStructVarList != 0)
    {
      state->fNextStruct = fStructVarList;
      fStructVarList->fPrevStruct = state;
    }

  fStructVarList = state;
}

inline void
QcLinInEqColStateVector::UnlinkStruct(QcLinInEqColState *me)
{
  qcAssertPre( (me->fPrevStruct == 0)
	       == (fStructVarList == me));

  if (me->fPrevStruct == 0)
    fStructVarList = me->fNextStruct;
  else
    me->fPrevStruct->fNextStruct = me->fNextStruct;

  if (me->fNextStruct != 0)
    me->fNextStruct->fPrevStruct = me->fPrevStruct;

  me->fPrevStruct = 0;
  me->fNextStruct = 0;
}

inline void
QcLinInEqColStateVector::UnlinkStruct(unsigned i)
{
  qcAssertPre (i < fSize);

  QcLinInEqColState *me = (QcLinInEqColState *)fStates[i];
  UnlinkStruct( me);
}





#line 305 "QcLinInEqColStateVector.ch"
inline QcLinInEqColState *
QcLinInEqColStateVector::getState(unsigned i) const
{
  qcAssertPre( i < fSize);
  QcLinInEqColState *ret = CAST(QcLinInEqColState *, fStates[i]);
  qcAssertPost( ret != 0);
  return ret;
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

#endif /* !QcLinInEqColStateVectorIFN */
