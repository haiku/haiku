// Generated automatically from QcNomadicFloatRep.ch by /home/pmoulder/usr/local/bin/ch2xx.
#include "QcNomadicFloatRep.hh"
#line 1 "QcNomadicFloatRep.ch"
// $Id: QcNomadicFloatRep.ch,v 1.7 2000/12/12 09:59:27 pmoulder Exp $

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
// THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY	EXPRESSED OR  //
// IMPLIED.  ANY USE IS AT YOUR OWN RISK.                                     //
//                                                                            //
// Permission is hereby granted to use or copy this program for any purpose,  //
// provided the above notices are retained on all copies.  Permission to      //
// modify the code and to distribute modified code is granted, provided the   //
// above notices are retained, and a notice that the code was modified is     //
// included with the above copyright notice.                                  //
//============================================================================//

#include <qoca/QcDefines.hh>



#define qcDefaultStayWeight 	1.0
#define qcDefaultEditWeight 	10000.0







#line 53 "QcNomadicFloatRep.ch"
//-----------------------------------------------------------------------//
// Manipulatiom functions.                                               //
//-----------------------------------------------------------------------//

bool QcNomadicFloatRep::RestDesVal_changed()
{
  bool changed = fDesireValue != fValue;
  fDesireValue = fValue;
  return changed;
}


void QcNomadicFloatRep::RestDesVal()
{
  fDesireValue = fValue;
}


void QcNomadicFloatRep::SetToEditWeight()
{
  fWeight = EditWeight();
}


void QcNomadicFloatRep::SetToStayWeight()
{
  fWeight = StayWeight();
}



/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/
