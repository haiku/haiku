// $Id: QcOrigRowState.hh,v 1.4 2000/12/06 05:32:56 pmoulder Exp $

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
//----------------------------------------------------------------------------//
// The class QcBiMap provides a bi-directional mapping from                   //
// identifier class to index class.                                           //
//============================================================================//

#ifndef __QcOrigRowStateH
#define __QcOrigRowStateH

#include "qoca/QcState.hh"

class QcOrigRowState : public QcState
{
public:
	numT fRHS;		// The rhs coeff corres to row of fA.
	int fMRowIndex;
	bool fARowDeleted;

	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
	QcOrigRowState(int ci);

	//-----------------------------------------------------------------------//
	// Utility functions.                                                    //
	//-----------------------------------------------------------------------//
  	virtual void Print(ostream &os);
};

inline QcOrigRowState::QcOrigRowState(int ci)
  : QcState(),
    fRHS( 0),
    fMRowIndex( ci),
    fARowDeleted( false)
{
}

inline void QcOrigRowState::Print(ostream &os)
{
	os << "[RHS(" << fRHS << "),"
		<< "[MRowIndex(" << fMRowIndex << "),"
		<< "[ARowDeleted(" << fARowDeleted << ")]";
}

#endif /* !__QcOrigRowStateH */
