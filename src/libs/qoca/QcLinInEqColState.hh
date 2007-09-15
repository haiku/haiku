// $Id: QcLinInEqColState.hh,v 1.8 2000/12/13 05:59:49 pmoulder Exp $

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

#ifndef __QcLinInEqColStateH
#define __QcLinInEqColStateH

#include "qoca/QcQuasiColState.hh"

class QcLinInEqColState : public QcLinEqColState
{
  friend class QcLinInEqColStateVector;
  friend class QcLinInEqRowColStateVector;

public:
	enum {
		fStructural = 0x01,
		fSlack = 0x02,
		fDual = 0x04,
		fArtificial = 0x08,
		fDesire = 0x10,
		fError = 0x20
	};

	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
	QcLinInEqColState(int vi);

private:
#ifndef NDEBUG
  bool isValidColCondition(int c) const
  {
    switch( c)
      {
      case fDual:
      case fArtificial:
      case fDesire:
      case fStructural:
      case (fStructural | fSlack):
      case (fStructural | fSlack | fError):
	return true;
      }
    return false;
  }
#endif

  //-----------------------------------------------------------------------
  // Query functions
  //-----------------------------------------------------------------------
public:
  int getCondition() const
  {
    qcAssertPost( isValidColCondition( fCondition));
    return fCondition;
  }

  void setCondition(int c)
  {
    qcAssertPre( isValidColCondition( c));
    fCondition = c;
  }

  QcLinInEqColState const *getNextStructuralCol() const
  {
    return fNextStruct;
  }

  //-----------------------------------------------------------------------//
  // Utility functions.                                                    //
  //-----------------------------------------------------------------------//
  virtual void Print(ostream &os);

public:
  /** A constrained variable may only take non-negative values and hence if it
      is basic, its coefficient cannot be allowed to have a different sign from
      the rhs value if the tableau is in solved form.  A solved form tableau row
      with a constrained basic variable is called restricted.
  **/
  bool fIsConstrained;

private:
	int fCondition;
		// Currently only slack and dual variables are constrained.
public:
	numT fObj;			// Objective function coeff (for Solver's use)
private:
	QcLinInEqColState *fNextStruct;	// Circular doubly linked list of
	QcLinInEqColState *fPrevStruct;	// slack variables.
};

inline QcLinInEqColState::QcLinInEqColState(int vi)
		: QcLinEqColState(vi)
{
	fIsConstrained = false;
	fCondition = fStructural;
	fObj = 0.0;
	fNextStruct = 0;
	fPrevStruct = 0;
}

inline void QcLinInEqColState::Print(ostream &os)
{
  	QcLinEqColState::Print(os);
	os << ",IsConst(" << fIsConstrained << "),"
		<< "Cond(" << fCondition << "),";

	if (fCondition & fStructural)
		os << "[structural]";

	if (fCondition & fSlack)
		os << "[slack]";

	if (fCondition & fDual)
		os << "[dual]";

	if (fCondition & fArtificial)
		os << "[artificial]";

	if (fCondition & fDesire)
		os << "[desire]";

  	os << "),Obj(" << fObj << "),"
		<< "PrevStruct(" << fPrevStruct << "),"
		<< "NextStruct(" << fNextStruct << ")";
}

#endif
