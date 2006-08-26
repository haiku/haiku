// Generated automatically from QcQuasiRowColStateVector.ch by /home/pmoulder/usr/local/bin/ch2xx.
#include "QcQuasiRowColStateVector.H"
#ifndef QcQuasiRowColStateVectorIFN
#define QcQuasiRowColStateVectorIFN
#line 1 "QcQuasiRowColStateVector.ch"
// $Id: QcQuasiRowColStateVector.ch,v 1.4 2001/01/30 01:32:08 pmoulder Exp $

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



#include <qoca/QcQuasiRowStateVector.hh>






//-----------------------------------------------------------------------//
// Constructor.                                                          //
//-----------------------------------------------------------------------//
inline
QcQuasiRowColStateVector::QcQuasiRowColStateVector(QcQuasiRowStateVector *rs,
			 QcQuasiColStateVector *cs)
  : fRowState(rs), fColState(cs)
{
}


#line 63 "QcQuasiRowColStateVector.ch"
//-----------------------------------------------------------------------//
// Manipulation functions.                                               //
//-----------------------------------------------------------------------//
inline void
QcQuasiRowColStateVector::Restart()
{
  fRowState->Resize(0);
  fColState->Resize(0);
}

inline void
QcQuasiRowColStateVector::Reserve(unsigned rows, unsigned cols)
{
  fRowState->Reserve(rows);
  fColState->Reserve(cols);
}

#if 0 /* unused */

#line 135 "QcQuasiRowColStateVector.ch"
#endif /* unused */

//-----------------------------------------------------------------------//
// Utility functions.                                                    //
//-----------------------------------------------------------------------//
inline void
QcQuasiRowColStateVector::Print(ostream &os)
{
  fRowState->Print(os);
  fColState->Print(os);
}


#line 155 "QcQuasiRowColStateVector.ch"
/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/


#endif /* !QcQuasiRowColStateVectorIFN */
