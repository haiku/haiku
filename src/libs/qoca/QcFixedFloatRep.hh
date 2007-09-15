// Generated automatically from QcFixedFloatRep.ch by /home/pmoulder/usr/local/bin/ch2xx.
#include "QcFixedFloatRep.H"
#ifndef QcFixedFloatRepIFN
#define QcFixedFloatRepIFN
#line 1 "QcFixedFloatRep.ch"
// $Id: QcFixedFloatRep.ch,v 1.7 2000/12/12 09:59:27 pmoulder Exp $

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

#include <qoca/QcFloatRep.hh>

#define qcDefaultFixedWeight  5000.0







#line 48 "QcFixedFloatRep.ch"
//-----------------------------------------------------------------------//
// Manipulatiom functions.                                               //
//-----------------------------------------------------------------------//

#line 75 "QcFixedFloatRep.ch"
inline QcFixedFloatRep::QcFixedFloatRep::QcFixedFloatRep(bool restricted)
		: QcFloatRep("", 0.0, qcDefaultFixedWeight, qcDefaultFixedWeight,
         		restricted)
{
#if qcReportAlloc
	cerr << "inited fixed " << this << endl;
#endif
}

inline QcFixedFloatRep::QcFixedFloatRep::QcFixedFloatRep(const char *name, bool restricted)
		: QcFloatRep(name, 0.0, qcDefaultFixedWeight, qcDefaultFixedWeight,
			restricted)
{
#if qcReportAlloc
	cerr << "inited fixed " << this << endl;
#endif
}

inline QcFixedFloatRep::QcFixedFloatRep::QcFixedFloatRep(const char *name, numT desval,
		bool restricted)
		: QcFloatRep(name, desval, qcDefaultFixedWeight,
			qcDefaultFixedWeight, restricted)
{
#if qcReportAlloc
	cerr << "inited fixed " << this << endl;
#endif
}

inline QcFixedFloatRep::QcFixedFloatRep::QcFixedFloatRep(const char *name, numT desval,
		numT w, bool restricted)
		: QcFloatRep(name, desval, w, w, restricted)
{
#if qcReportAlloc
	cerr << "inited fixed " << this << endl;
#endif
}


/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/

#endif /* !QcFixedFloatRepIFN */
