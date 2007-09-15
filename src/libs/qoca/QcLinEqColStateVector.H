// Generated automatically from QcLinEqColStateVector.ch by /home/pmoulder/usr/local/bin/ch2xx.
#ifndef QcLinEqColStateVectorDCL
#define QcLinEqColStateVectorDCL
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

class QcLinEqColStateVector
  : public QcQuasiColStateVector
{
public:



//-----------------------------------------------------------------------//
// Constructor.                                                          //
//-----------------------------------------------------------------------//

QcLinEqColStateVector()
  : QcQuasiColStateVector(),
    fParamHead( false),
    fBasicHead( true)
{
}


#ifndef NDEBUG
private:


void
assertLinkageInvar() const;
#line 113 "QcLinEqColStateVector.ch"
protected:


virtual void
virtualAssertLinkageInvar() const;
#line 121 "QcLinEqColStateVector.ch"
#endif /* !NDEBUG */


//-----------------------------------------------------------------------//
// Query functions.                                                      //
//-----------------------------------------------------------------------//
public:

inline numT
GetDesireValue(unsigned i) const;
#line 136 "QcLinEqColStateVector.ch"
inline bool
IsBasic(unsigned i) const;
#line 143 "QcLinEqColStateVector.ch"
inline int
IsBasicIn(unsigned i) const;
#line 151 "QcLinEqColStateVector.ch"
//-----------------------------------------------------------------------//
// Set functions.                                                        //
//-----------------------------------------------------------------------//
inline void
SetDesireValue(unsigned i, numT dv) const;
#line 161 "QcLinEqColStateVector.ch"
inline QcLinEqColState *
head( bool isbasic);
#line 172 "QcLinEqColStateVector.ch"
void
SetBasic(unsigned i, bool b);
#line 187 "QcLinEqColStateVector.ch"
inline void
SetBasicIn(unsigned i, int bi) const;
#line 195 "QcLinEqColStateVector.ch"
//-----------------------------------------------------------------------//
// Manipulation functions.                                               //
//-----------------------------------------------------------------------//

virtual void
FixLinkage();
#line 223 "QcLinEqColStateVector.ch"
inline void
Link(QcLinEqColState *state, QcLinEqColState *head);
#line 238 "QcLinEqColStateVector.ch"
inline void
Unlink(QcLinEqColState *state);
#line 251 "QcLinEqColStateVector.ch"
protected:


virtual void
Alloc(QcState **start, QcState **finish);
#line 269 "QcLinEqColStateVector.ch"
virtual void
AddToList(QcState **start, QcState **finish);
#line 292 "QcLinEqColStateVector.ch"
virtual void
RemoveFromList(QcState **start, QcState **finish);
#line 310 "QcLinEqColStateVector.ch"
public:


virtual void
Print(ostream &os) const;
#line 339 "QcLinEqColStateVector.ch"
private:
inline QcLinEqColState *
getState(unsigned i) const;
#line 351 "QcLinEqColStateVector.ch"
public:
  QcLinEqColState fParamHead; // head of linked list of non-basic variables.
  QcLinEqColState fBasicHead; // head of linked list of basic variables.

};


/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/

#endif /* !QcLinEqColStateVectorDCL */
