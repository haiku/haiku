// $Id: QcCompPivotSystem.hh,v 1.7 2000/12/12 01:29:00 pmoulder Exp $

//============================================================================//
// Written by Sitt Sen Chok                                                   //
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

#ifndef __QcCompPivotSystemH
#define __QcCompPivotSystemH

#include "qoca/QcLinInEqSystem.hh"
#include "qoca/QcCompPivotTableau.hh"

class QcCompPivotSystem : public QcLinInEqSystem
{
protected:
	QcCompPivotTableau &fTableau;

public:
	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
	QcCompPivotSystem();
	QcCompPivotSystem(int hintRows, int hintCols);
	QcCompPivotSystem(QcCompPivotTableau &tab);

	//-----------------------------------------------------------------------//
	// Variable management methods                                           //
	//-----------------------------------------------------------------------//
	void AddSlack(QcFloat &v)
		{ fVBiMap.Update(v, fTableau.AddSlack()); }

	//-----------------------------------------------------------------------//
	// Constraint management methods                                         //
	//-----------------------------------------------------------------------//
	virtual bool ApplyHints(int cs);
		// This is a convenience function to pivot an independent new
		// solved form constraint according to the PivotHints.
	virtual bool CopyInEq(QcConstraint &c);
	virtual bool CopyConstraint(QcConstraint &c, int bvi);
		// Extracts the various components and enters them in the sub-tableau
		// as well as updating the bimapnotifier for the sub-tableau
		// if successful.
	virtual void EqRawSolve();
	virtual void EqRawSolveComplex();
	numT EvalBasicVar(unsigned vi);
	numT EvalBasicVarComplex(unsigned vi);
 	virtual void InEqRawSolve();
	virtual void InEqRawSolveComplex();

	virtual int SelectExitVar (unsigned vi);
		// see superclass for doc

	virtual int SelectExitVarComplex(int vi);
		// Select the pivot row for pivot variable vi (slack parameter)
		// maintaining Basic Feasible Solved Form.
		// Returns fInvalidConstraintIndex if no such pivot is possible.
		// This version implement's Bland's anticycling rule.
};

inline QcCompPivotSystem::QcCompPivotSystem()
	: QcLinInEqSystem(*new QcCompPivotTableau(0, 0, fNotifier)),
	  fTableau((QcCompPivotTableau &)QcLinInEqSystem::fTableau)
{
}

inline QcCompPivotSystem::QcCompPivotSystem(int hintRows, int hintCols)
	: QcLinInEqSystem(*new QcCompPivotTableau(hintRows, hintCols, fNotifier)),
	  fTableau((QcCompPivotTableau &)QcLinInEqSystem::fTableau)
{
}

inline QcCompPivotSystem::QcCompPivotSystem(QcCompPivotTableau &tab)
	: QcLinInEqSystem(tab),
	  fTableau(tab)
{
}

#endif
