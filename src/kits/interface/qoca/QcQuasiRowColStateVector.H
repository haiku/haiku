// Generated automatically from QcQuasiRowColStateVector.ch by /home/pmoulder/usr/local/bin/ch2xx.
#ifndef QcQuasiRowColStateVectorDCL
#define QcQuasiRowColStateVectorDCL
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

#include <iostream.h>
#include <qoca/QcQuasiRowStateVector.H>

#include <qoca/QcQuasiColStateVector.hh>

class QcQuasiRowColStateVector
{
public:

//-----------------------------------------------------------------------//
// Constructor.                                                          //
//-----------------------------------------------------------------------//
inline
QcQuasiRowColStateVector(QcQuasiRowStateVector *rs,
			 QcQuasiColStateVector *cs);
#line 47 "QcQuasiRowColStateVector.ch"
protected:
/** This constructor should only be used by derived classes.  The derived
    class should initialize all the member data in this class. */
QcQuasiRowColStateVector()
{
}

public:
virtual ~QcQuasiRowColStateVector()
{
  delete fRowState;
  delete fColState;
}



//-----------------------------------------------------------------------//
// Manipulation functions.                                               //
//-----------------------------------------------------------------------//
inline void
Restart();
#line 73 "QcQuasiRowColStateVector.ch"
inline void
Reserve(unsigned rows, unsigned cols);
#line 80 "QcQuasiRowColStateVector.ch"
#if 0 /* unused */

virtual void
SwapRows(unsigned c1, unsigned c2);
#line 135 "QcQuasiRowColStateVector.ch"
#endif /* unused */

//-----------------------------------------------------------------------//
// Utility functions.                                                    //
//-----------------------------------------------------------------------//
inline void
Print(ostream &os);
#line 148 "QcQuasiRowColStateVector.ch"
public:
  QcQuasiRowStateVector *fRowState;
  QcQuasiColStateVector *fColState;


};

/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/


#endif /* !QcQuasiRowColStateVectorDCL */
