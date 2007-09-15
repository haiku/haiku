// Generated automatically from QcFloatRep.ch by /home/pmoulder/usr/local/bin/ch2xx.
#include "QcFloatRep.H"
#ifndef QcFloatRepIFN
#define QcFloatRepIFN
#line 1 "QcFloatRep.ch"
// $Id: QcFloatRep.ch,v 1.13 2001/01/10 05:01:51 pmoulder Exp $

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

#include <string>
#include "qoca/QcUtility.hh"


#line 48 "QcFloatRep.ch"
	//-----------------------------------------------------------------------//
	// Constructors                                                          //
	//-----------------------------------------------------------------------//

#line 93 "QcFloatRep.ch"
//-----------------------------------------------------------------------//
// Set and Get member functions                                          //
//-----------------------------------------------------------------------//

#line 166 "QcFloatRep.ch"
inline char const *QcFloatRep::Name() const
{
  qcAssertPre( isQcFloatRep());
  if (fName == 0)
    return 0;
  else
    return (fName + qcMagic2Len);
}


#line 248 "QcFloatRep.ch"
inline void QcFloatRep::SetToGoal()
{
  qcAssertPre( isQcFloatRep());
  SetValue (fDesireValue);
}


#line 273 "QcFloatRep.ch"
#ifndef qcCheckInternalPre
inline void QcFloatRep::QcFloatRep::assertInvar() const
{ }
#endif


#line 322 "QcFloatRep.ch"
inline void QcFloatRep::Reset()
{
  fCounter = 1;
  fId = fNextValidId++;
  qcAssertPost (fNextValidId > 0);
}


#line 352 "QcFloatRep.ch"
//-----------------------------------------------------------------------//
// Utility functions.                                                    //
//-----------------------------------------------------------------------//


#line 416 "QcFloatRep.ch"
/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/

#endif /* !QcFloatRepIFN */
