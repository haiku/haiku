// $Id: QcDelLinInEqSystem.hh,v 1.4 2000/11/23 06:44:20 pmoulder Exp $

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

#ifndef __QcDelLinInEqSystemH
#define __QcDelLinInEqSystemH

#include "qoca/QcLinInEqSystem.hh"
#include "qoca/QcDelLinInEqTableau.hh"

class QcDelLinInEqSystem : public QcLinInEqSystem
{
public:
	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
	QcDelLinInEqSystem();
	QcDelLinInEqSystem(unsigned hintRows, unsigned hintCols);

	//-----------------------------------------------------------------------//
	// Variable management methods (see solver.h for descriptions)           //
	//-----------------------------------------------------------------------//
	virtual bool RemoveVar(QcFloat &v)
		{ return QcDelLinEqSystem::RemoveVar(v); }

	//-----------------------------------------------------------------------//
	// Constraint management methods                                         //
	//-----------------------------------------------------------------------//
	virtual bool RemoveConstraint(QcConstraint &c);
};

inline QcDelLinInEqSystem::QcDelLinInEqSystem()
	: QcLinInEqSystem(*new QcDelLinInEqTableau(0, 0, fNotifier))
{
}

inline QcDelLinInEqSystem::QcDelLinInEqSystem(unsigned hintRows, unsigned hintCols)
	: QcLinInEqSystem(*new QcDelLinInEqTableau(hintRows, hintCols, fNotifier))
{
}

inline bool QcDelLinInEqSystem::RemoveConstraint(QcConstraint &c)
{
	bool present = fOCBiMap.IdentifierPresent(c);

	if (present)
		fTableau.RemoveEq(fOCBiMap.Index(c));
	#ifdef qcSafetyChecks
	else
		throw QcWarning(
         		"QcDelLinEqSystem::RemoveConstraint: invalid QcConstraint");
	#endif

	fEditVarsSetup = false;
	return present;
}

#endif
