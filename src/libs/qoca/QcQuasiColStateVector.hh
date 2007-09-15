// $Id: QcQuasiColStateVector.hh,v 1.4 2000/11/29 01:58:42 pmoulder Exp $

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

#ifndef __QcQuasiColStateVectorH
#define __QcQuasiColStateVectorH

#include "qoca/QcStateVector.hh"

class QcQuasiColStateVector : public QcStateVector
{
public:
  //-----------------------------------------------------------------------//
  // Constructor.                                                          //
  //-----------------------------------------------------------------------//
  QcQuasiColStateVector();

  unsigned getNColumns() const
  {
    return fSize;
  }

  //-----------------------------------------------------------------------//
  // Utility functions.                                                    //
  //-----------------------------------------------------------------------//
  virtual void Print(ostream &os) const;
};

inline QcQuasiColStateVector::QcQuasiColStateVector()
		: QcStateVector()
{
}

inline void QcQuasiColStateVector::Print(ostream &os) const
{
	os << endl << "Column State Vector:";
	QcStateVector::Print(os);
}

#endif
