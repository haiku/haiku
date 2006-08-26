// $Id: QcDelLinInEqTableau.hh,v 1.8 2001/01/30 01:32:07 pmoulder Exp $

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

#ifndef __QcDelLinInEqTableauH
#define __QcDelLinInEqTableauH

#include "qoca/QcLinInEqTableau.hh"
#include "qoca/QcLinInEqRowColStateVector.hh"

class QcDelLinInEqTableau : public QcLinInEqTableau
{
public:
	//-----------------------------------------------------------------------//
	// Constructors.                                                         //
	//-----------------------------------------------------------------------//
	QcDelLinInEqTableau(unsigned hintRows, unsigned hintCols, QcBiMapNotifier &n);
	QcDelLinInEqTableau(QcCoreTableau &tab, QcBiMapNotifier &n);

	//-----------------------------------------------------------------------//
	// Manipulation functions.                                               //
	//-----------------------------------------------------------------------//
	virtual int AddGtEq(QcRowAdaptor &varCoeffs, numT rhs);
		// Similar to addLtEq, only uses a negative slack variable coeff.

	virtual int AddLtEq(QcRowAdaptor &varCoeffs, numT rhs);
		// AddLessEq does the same as AddEq but also introduces a new
		// slack variable so the varCoeffs will represent a less than or
		// equal to constraint.  The slack variable is added after
		// any other new regular variables.

	virtual int RemoveEq(unsigned ci);
		// Eliminate the original constraint ci from the solved form and
		// delete it.  Returns the associated solved form constraint removed.
		// If the operation cannot be performed for some reason InvalidCIndex
		// is returned.

	virtual bool RemoveVarix(unsigned vi)
		// Removes a free variable from the tableau, returns false if not free.
  		{ return QcDelLinEqTableau::RemoveVarix( vi); }
};

inline QcDelLinInEqTableau::QcDelLinInEqTableau(unsigned hintRows, unsigned hintCols,
						QcBiMapNotifier &n)
	: QcLinInEqTableau(*new QcDelCoreTableau(hintRows, hintCols,
						 *new QcLinInEqRowColStateVector()),
			   n)
{
}

inline QcDelLinInEqTableau::QcDelLinInEqTableau(QcCoreTableau &tab, QcBiMapNotifier &n)
		: QcLinInEqTableau(tab, n)
{
}

#endif /* !__QcDelLinInEqTableauH */
