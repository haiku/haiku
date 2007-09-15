// Generated automatically from QcOrigRowStateVector.ch by /home/pmoulder/usr/local/bin/ch2xx.
#include "QcOrigRowStateVector.H"
#ifndef QcOrigRowStateVectorIFN
#define QcOrigRowStateVectorIFN
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







inline
QcOrigRowStateVector::QcOrigRowStateVector()
  : QcStateVector()
{
}

//-----------------------------------------------------------------------//
// Query functions.                                                      //
//-----------------------------------------------------------------------//

inline bool
QcOrigRowStateVector::GetARowDeleted(unsigned i) const
{
  qcAssertPre (i < fSize);
  return getState( i)->fARowDeleted;
}

inline int
QcOrigRowStateVector::GetMRowIndex(unsigned i) const
{
  qcAssertPre (i < fSize);
  return getState( i)->fMRowIndex;
}


inline numT
QcOrigRowStateVector::GetRHS(unsigned i) const
{
  qcAssertPre (i < fSize);
  return getState( i)->fRHS;
}


//-----------------------------------------------------------------------//
// Set* functions.
//-----------------------------------------------------------------------//
inline void
QcOrigRowStateVector::SetARowDeleted(unsigned i, bool d)
{
  qcAssertPre (i < fSize);
  getState( i)->fARowDeleted = d;
}

inline void
QcOrigRowStateVector::SetMRowIndex(unsigned i, int ri)
{
  qcAssertPre (i < fSize);
  getState( i)->fMRowIndex = ri;
}

inline void
QcOrigRowStateVector::SetRHS(unsigned i, numT rhs)
{
  qcAssertPre (i < fSize);
  getState( i)->fRHS = rhs;
}

//-----------------------------------------------------------------------//
// Manipulation functions.                                               //
//-----------------------------------------------------------------------//

#line 152 "QcOrigRowStateVector.ch"
#ifndef NDEBUG

#line 165 "QcOrigRowStateVector.ch"
#endif




inline void
QcOrigRowStateVector::Print(ostream &os) const
{
  os << endl << "Original Row State Vector:";
  QcStateVector::Print( os);
}



inline QcOrigRowState *
QcOrigRowStateVector::getState( unsigned i) const
{
  qcAssertPre( i < fSize);
  QcOrigRowState *ret = CAST(QcOrigRowState *, fStates[i]);
  qcAssertPost( ret != 0);
  return ret;
}




/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/

#endif /* !QcOrigRowStateVectorIFN */
