// $Id: QcDelLinEqSystem.hh,v 1.6 2001/01/30 01:32:07 pmoulder Exp $

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

#ifndef __QcDelLinEqSystemH
#define __QcDelLinEqSystemH

#include "qoca/QcLinEqSystem.hh"
#include "qoca/QcDelLinEqTableau.hh"

/** Version of QcLinEqSystem that allows <tt>removeConstraint</tt>.

    <p>Note that this interface needs to be read in conjuction with that of
    <tt>QcSolver</tt>.
**/
class QcDelLinEqSystem : public QcLinEqSystem
{
public:
	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
	QcDelLinEqSystem();
	QcDelLinEqSystem(unsigned hintRows, unsigned hintCols);
	QcDelLinEqSystem(QcLinEqTableau &tab);

	//-----------------------------------------------------------------------//
	// Variable management methods (see QcSolver.java for descriptions)      //
	//-----------------------------------------------------------------------//
	virtual bool RemoveVar(QcFloat &v);

	//-----------------------------------------------------------------------//
	// Enquiry functions for variables                                       //
	//-----------------------------------------------------------------------//
	virtual bool IsFree(QcFloat const &v) const
		{ return fTableau.IsFree(fVBiMap.Index(v)); }

	//-----------------------------------------------------------------------//
	// High Level Edit Variable Interface for use by solver clients.         //
	// The methods addEditVar and endEdit are more efficient than the ones   //
	// inherited from Solver since variable weights have no effect on        //
	// QcLinEqSystem::Resolve.                                               //
	//-----------------------------------------------------------------------//
	virtual void TransitiveClosure(vector<unsigned int> &vars)
		{ fTableau.TransitiveClosure(vars); }

	//-----------------------------------------------------------------------//
	// Constraint management methods                                         //
	//-----------------------------------------------------------------------//
	virtual bool RemoveConstraint(QcConstraint &c);
};

inline QcDelLinEqSystem::QcDelLinEqSystem()
	: QcLinEqSystem(*new QcDelLinEqTableau(0, 0, fNotifier))
{
}

inline QcDelLinEqSystem::QcDelLinEqSystem(unsigned hintRows, unsigned hintCols)
	: QcLinEqSystem(*new QcDelLinEqTableau(hintRows, hintCols, fNotifier))
{
}

inline QcDelLinEqSystem::QcDelLinEqSystem(QcLinEqTableau &tab)
	:  QcLinEqSystem(tab)
{
}

inline bool QcDelLinEqSystem::RemoveVar(QcFloat &v)
{
	int ix = fVBiMap.safeIndex(v);
	if (ix >= 0)
		return fTableau.RemoveVarix( ix);
	else
		return false;
}

inline bool QcDelLinEqSystem::RemoveConstraint(QcConstraint &c)
{
	int ix = fOCBiMap.safeIndex(c);
	if (ix < 0)
		return false;

	fTableau.RemoveEq(ix);
	fEditVarsSetup = false;
	return true;
}

#endif
