// $Id: QcDelLinEqTableau.hh,v 1.9 2001/01/30 01:32:07 pmoulder Exp $

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

#ifndef __QcDelLinEqTableauH
#define __QcDelLinEqTableauH

#include "qoca/QcLinEqTableau.hh"

class QcDelLinEqTableau : public QcLinEqTableau
{
public:
	//-----------------------------------------------------------------------//
	// Constructors.                                                         //
	//-----------------------------------------------------------------------//
	QcDelLinEqTableau(unsigned hintRows, unsigned hintCols, QcBiMapNotifier &n);
	QcDelLinEqTableau(QcCoreTableau &tab, QcBiMapNotifier &n);

	//-----------------------------------------------------------------------//
	// Query functions.                                                      //
	//-----------------------------------------------------------------------//
	virtual bool IsFree(unsigned vi) const
		// Indicates if a particular variable is in use (i.e. has a
		// non-zero coefficient in any solved form (or original) constraint.
  		{ return CAST(QcDelCoreTableau *, fCoreTableau)->IsFree(vi); }

	numT GetRHSVirtual(int ci) const
  		{ return CAST(QcDelCoreTableau *, fCoreTableau)->GetRHSVirtual(ci); }

	numT GetValueVirtual(int ci, int vi) const;
		// This is the similar to getValue except that it will always obtain
		// value from computation of the quasi-inverse and orignial constraint
		// coefficients.  Whereas getValue obtain value directly from the
		// solved form if the real tableau is used.

	bool IsValidSolvedForm() const
		// For debugging purposes to ensure that the real solved form is
		// consistant with the virtual solved form.
		{ return CAST(QcDelCoreTableau *, fCoreTableau)->IsValidSolvedForm(); }

	//-----------------------------------------------------------------------//
	// Manipulation functions.                                               //
	//-----------------------------------------------------------------------//
	virtual void DecreaseRows(QcBiMapNotifier &note)
		// Get rid of the last (deleted) row
		{ CAST(QcDelCoreTableau *, fCoreTableau)->DecreaseRows(note); }

	virtual void DecreaseColumns(QcBiMapNotifier &note)
		// Get rid of the last (empty) column
  		{ CAST(QcDelCoreTableau *, fCoreTableau)->DecreaseColumns(note); }

	virtual int RemoveEq(unsigned ci);
		// RemoveEq removes an original constraint with index ci from
		// the solved form without disturbing the solution.  This requires
		// removal of one of the solved form constraints.  The index of
		// the removed solved form constraint is returned or InvalidCIndex
		// if there is a serious error (e.g. ci invalid).  For the current
		// implementation: after a row is removed the CIndex values for
		// particular rows are unchanged.
		// The notifier is told to drop the constraint index.

	virtual bool RemoveVarix(unsigned vi);
		// Removes a free variable from the tableau, returns false if not free.

	//-----------------------------------------------------------------------//
	// Utility function.                                                     //
	//-----------------------------------------------------------------------//
	virtual void Print(ostream &os) const;

	virtual void TransitiveClosure(vector<unsigned int> &vars)
  		{ CAST(QcDelCoreTableau *, fCoreTableau)->TransitiveClosure(vars); }

protected:
	//-----------------------------------------------------------------------//
	// Constructors.                                                         //
	//-----------------------------------------------------------------------//
	QcDelLinEqTableau(QcBiMapNotifier &n);
};

inline QcDelLinEqTableau::QcDelLinEqTableau(unsigned hintRows, unsigned hintCols,
					    QcBiMapNotifier &n)
	: QcLinEqTableau(*new QcDelCoreTableau(hintRows, hintCols,
					       *new QcLinEqRowColStateVector()),
			 n)
{
}

inline QcDelLinEqTableau::QcDelLinEqTableau(QcCoreTableau &tab, QcBiMapNotifier &n)
	: QcLinEqTableau(tab, n)
{
}

inline QcDelLinEqTableau::QcDelLinEqTableau(QcBiMapNotifier &n)
		: QcLinEqTableau(n)
{
}

inline void QcDelLinEqTableau::Print(ostream &os) const
{
	QcLinEqTableau::Print(os);
	fCoreTableau->Print(os);
	os << endl << "Consistant: "
		<< (IsValidSolvedForm() ? "yes" : "no") << endl;
}

#endif /* !__QcDelLinEqTableauH */
