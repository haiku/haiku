// Generated automatically from QcLinInEqRowColStateVector.ch by /home/pmoulder/usr/local/bin/ch2xx.
#ifndef QcLinInEqRowColStateVectorDCL
#define QcLinInEqRowColStateVectorDCL
#line 1 "QcLinInEqRowColStateVector.ch"
// $Id: QcLinInEqRowColStateVector.ch,v 1.1 2000/11/29 04:31:16 pmoulder Exp $

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

#include "qoca/QcLinEqRowColStateVector.hh"
#include "qoca/QcLinInEqColStateVector.H"


class QcLinInEqRowColStateVector
  : public QcLinEqRowColStateVector
{
public:

//-----------------------------------------------------------------------//
// Constructor.                                                          //
//-----------------------------------------------------------------------//
QcLinInEqRowColStateVector()
  : QcLinEqRowColStateVector(new QcLinInEqColStateVector()),
    fColState((QcLinInEqColStateVector *)QcLinEqRowColStateVector::fColState)
{
}


//-----------------------------------------------------------------------//
// Manipulation functions.                                               //
//-----------------------------------------------------------------------//

virtual void
SwapColumns(unsigned v1, unsigned v2);
#line 142 "QcLinInEqRowColStateVector.ch"
public:
	QcLinInEqColStateVector *fColState;

};

/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/

#endif /* !QcLinInEqRowColStateVectorDCL */
