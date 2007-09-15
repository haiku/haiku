// $Id: QcRowAdaptor.hh,v 1.10 2001/01/04 05:20:11 pmoulder Exp $

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

#ifndef __QcRowAdaptorH
#define __QcRowAdaptorH

#include "qoca/QcDefines.hh"
#include "qoca/QcFloat.hh"
#include "qoca/QcBiMapNotifier.hh"
#include "qoca/QcTableau.hh"
#include "qoca/QcConstraint.hh"
#include "qoca/QcLinPoly.hh"

/** Extracts the coefficients from a <tt>QcLinPoly</tt> (typically in a
    <tt>QcLinConstraint</tt>).  It is assumed the <tt>QcVariableBiMap</tt>
    supplied has all the variables registered and will continue to so have while
    the iterator is incremented.
**/
class QcRowAdaptor
{
public:
	//-----------------------------------------------------------------------//
	// Constructor                                                           //
	//-----------------------------------------------------------------------//
	QcRowAdaptor(const QcLinPoly &lp, QcVariableBiMap &vb, QcTableau &tab);

	//-----------------------------------------------------------------------//
	// Query functions                                                       //
	//-----------------------------------------------------------------------//
	bool AtEnd() const
		{ return (fCurrent == fLinPoly.GetSize()); }

	int GetIndex() const
	{
		qcAssertPre (!AtEnd());
		return (int) fIndex;
	}

	unsigned getIndex() const
	{
		qcAssertPre (!AtEnd());
		return fIndex;
	}

	numT GetValue() const
	{
		qcAssertPre (!AtEnd());
		return fValue;
	}

	numT getValue() const
	{
		qcAssertPre (!AtEnd());
		return fValue;
	}

	//-----------------------------------------------------------------------//
	// Iteration functions.                                                  //
	//-----------------------------------------------------------------------//
	void Increment();
	void Reset();

private:
	//-----------------------------------------------------------------------//
	// Utility functions.                                                    //
	//-----------------------------------------------------------------------//
	void Advance();

	void Refresh();
		// Refresh fIndex according to the current value of fIt.
		// As a side effect, get the variable for this polyterm and
		// register it in the variable bimap if not already present.

private:
	QcVariableBiMap &fVariableBiMap;
	QcTableau &fTableau;
	const QcLinPoly &fLinPoly;
	unsigned fCurrent;       // Current position on the linpoly.

	/** fVariableBiMap.index(fLinPoly.getVariable(fCurrent)). */
	unsigned fIndex;

	/** fLinPoly.getCoeff(fCurrent). */
	numT fValue;
};

inline QcRowAdaptor::QcRowAdaptor(const QcLinPoly &lp, QcVariableBiMap &vb,
				  QcTableau &tab)
	: fVariableBiMap(vb),
	  fTableau(tab),
	  fLinPoly(lp),
	  fValue()
{
	if (fLinPoly.GetSize() > 0) {
		fCurrent = 0;
		fValue = fLinPoly.GetTerm(0).GetCoeff();
		Refresh();
	}
}

inline void QcRowAdaptor::Advance()
{
	qcAssertPre( !AtEnd());

	fCurrent++;
	fValue = fLinPoly.GetTerm(fCurrent).GetCoeff();
	Refresh();
}

inline void QcRowAdaptor::Increment()
{
	qcAssertPre (!AtEnd());

   	fCurrent++;

	while (fCurrent < fLinPoly.GetSize()) {
		fValue = fLinPoly.GetTerm(fCurrent).GetCoeff();

		if (!QcUtility::IsZero(fValue)) {
			Refresh();
			break;
		}
	}
}

inline void QcRowAdaptor::Refresh()
	// Refresh fIndex according to the current value of fIt.
	// As a side effect, get the variable for this polyterm and
	// register it in the variable bimap if not already present.
{
	qcAssertPre( !AtEnd());

	QcFloatRep *ident = fLinPoly.GetTerm( fCurrent).getVariable();

	if (!fVariableBiMap.IdentifierPtrPresent( ident)) {
		fIndex = fTableau.NewVariable();
		fVariableBiMap.Update(ident, fIndex);
	} else
		fIndex = fVariableBiMap.Index(ident);
}

inline void QcRowAdaptor::Reset()
{
	fCurrent = 0;
	fValue = fLinPoly.GetTerm(0).GetCoeff();
	Refresh();
}

#endif
