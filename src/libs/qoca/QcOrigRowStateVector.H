// Generated automatically from QcOrigRowStateVector.ch by /home/pmoulder/usr/local/bin/ch2xx.
#ifndef QcOrigRowStateVectorDCL
#define QcOrigRowStateVectorDCL
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

class QcOrigRowStateVector
  : public QcStateVector
{
public:


inline
QcOrigRowStateVector();
#line 44 "QcOrigRowStateVector.ch"
//-----------------------------------------------------------------------//
// Query functions.                                                      //
//-----------------------------------------------------------------------//

inline bool
GetARowDeleted(unsigned i) const;
#line 55 "QcOrigRowStateVector.ch"
inline int
GetMRowIndex(unsigned i) const;
#line 63 "QcOrigRowStateVector.ch"
inline numT
GetRHS(unsigned i) const;
#line 71 "QcOrigRowStateVector.ch"
//-----------------------------------------------------------------------//
// Set* functions.
//-----------------------------------------------------------------------//
inline void
SetARowDeleted(unsigned i, bool d);
#line 81 "QcOrigRowStateVector.ch"
inline void
SetMRowIndex(unsigned i, int ri);
#line 88 "QcOrigRowStateVector.ch"
inline void
SetRHS(unsigned i, numT rhs);
#line 95 "QcOrigRowStateVector.ch"
//-----------------------------------------------------------------------//
// Manipulation functions.                                               //
//-----------------------------------------------------------------------//

virtual void
FixLinkage();
#line 104 "QcOrigRowStateVector.ch"
protected:


virtual void
Alloc(QcState **start, QcState **finish);
#line 122 "QcOrigRowStateVector.ch"
virtual void
AddToList(QcState **start, QcState **finish);
#line 135 "QcOrigRowStateVector.ch"
virtual void
RemoveFromList(QcState **start, QcState **finish);
#line 152 "QcOrigRowStateVector.ch"
#ifndef NDEBUG

virtual void
virtualAssertLinkageInvar() const;
#line 165 "QcOrigRowStateVector.ch"
#endif

public:


virtual inline void
Print(ostream &os) const;
#line 178 "QcOrigRowStateVector.ch"
private:
inline QcOrigRowState *
getState( unsigned i) const;
#line 188 "QcOrigRowStateVector.ch"
};


/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/

#endif /* !QcOrigRowStateVectorDCL */
