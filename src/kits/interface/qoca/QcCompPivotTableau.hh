// $Id: QcCompPivotTableau.hh,v 1.7 2000/12/15 01:20:05 pmoulder Exp $

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

#ifndef __QcCompPivotTableauH
#define __QcCompPivotTableauH

#include "qoca/QcLinInEqTableau.hh"

class QcCompPivotTableau : public QcLinInEqTableau
{
protected:
	int fAI;
	int fBeginEq;

	//------------------------------------------------------------------------//
	// Constructor.                                                           //
	//------------------------------------------------------------------------//
public:
	QcCompPivotTableau(int hintRows, int hintCols, QcBiMapNotifier &n);

	//------------------------------------------------------------------------//
	// Query functions.                                                       //
	//------------------------------------------------------------------------//

	/** @postcondition <tt>(ret == fInvalidConstraintIndex)
	    || ((unsigned) ret</tt> &le; <tt>getNRows()</tt>)
	**/
	int GetBeginEq() const
  	{
	  qcAssertPost ((fBeginEq == fInvalidConstraintIndex)
			|| ((unsigned) fBeginEq <= getNRows()));
	  return fBeginEq;
	}

	numT GetComplexRHS(int ci) const;

	//------------------------------------------------------------------------//
	// Manipulation functions.                                                //
	//------------------------------------------------------------------------//
	virtual int AddArtificial(numT coeff);
	virtual int AddDesValVar();
	virtual void Restart();
  		// Erase everything ready to start afresh.

  /** The parameters indicate a solved form coeff.  Pivot can only be applied to
      a normalised or regular constraint.

      <p><tt>getValue(ci, vi)</tt> is allowed to be zero, in which case does
      nothing and returns false.

      <p>This pivot function only work on restricted rows which ends on
      fBeginEq.

      @precondition <tt>ci &lt; getNRows()</tt>
      @precondition <tt>vi &lt; getNColumns()</tt>
      @precondition <tt>!IsRedundant(ci)</tt>
  **/
  bool RestrictedPivot(unsigned ci, unsigned vi);

  /** In each constraint, set the coefficient of the artificial variable to
      <tt>coeff</tt>. */
  void SetArtificial(numT coeff);

	void SetBeginEq()
		{ fBeginEq = GetRows(); }
         
  	void SetRowState(int bvi, int ci, int rs);

	//------------------------------------------------------------------------//
	// Utility functions.                                                     //
	//------------------------------------------------------------------------//
	virtual void Print(ostream &os) const;
};

inline QcCompPivotTableau::QcCompPivotTableau(int hintRows, int hintCols,
					      QcBiMapNotifier &n)
	: QcLinInEqTableau(*new QcCoreTableau(hintRows, hintCols,
					      *new QcLinInEqRowColStateVector()),
			   n),
	  fAI(fInvalidVariableIndex),
	  fBeginEq(fInvalidConstraintIndex)
{
}

#endif
