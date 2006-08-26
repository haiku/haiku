// $Id: QcDesireValueStore.hh,v 1.4 2000/12/11 07:48:23 pmoulder Exp $

//============================================================================//
// Written by Sitt Sen Chok                                                   //
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

#ifndef __QcDesireValueStoreH
#define __QcDesireValueStoreH

#include <qoca/QcFloatRep.hh>

class QcDesireValueStore
{
public:
  //-----------------------------------------------------------------------//
  // Constructors
  //-----------------------------------------------------------------------//

  /* Used by vector as default initializer. */
  QcDesireValueStore()
    : fVariablePtr( 0),
      fDesValue()
    { }

  QcDesireValueStore( QcFloatRep *vr)
    : fVariablePtr( vr),
      fDesValue( vr->DesireValue())
    { }

  //-----------------------------------------------------------------------//
  // Manipulation function.                                                //
  //-----------------------------------------------------------------------//
  void Restore()
    { fVariablePtr->SuggestValue( fDesValue); }

  //-----------------------------------------------------------------------//
  // Utility functions.                                                    //
  //-----------------------------------------------------------------------//
  void Print(ostream &os) const;

private:
  QcFloatRep *fVariablePtr;
  numT fDesValue;
};

inline void QcDesireValueStore::Print(ostream &os) const
{
  fVariablePtr->Print( os);
  os << ":" << fDesValue;
}

#endif /* !__QcDesireValueStoreH */
