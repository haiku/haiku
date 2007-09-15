// $Id: QcLinEqColState.hh,v 1.5 2000/12/06 05:32:56 pmoulder Exp $

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

#ifndef __QcLinEqColStateH
#define __QcLinEqColStateH

#include <qoca/QcTableau.hh>
#include "qoca/QcQuasiColState.hh"

class QcLinEqColState : public QcQuasiColState
{
  friend class QcLinEqColStateVector;
  friend class QcLinEqRowColStateVector;

public:

  //-----------------------------------------------------------------------//
  // Constructor.                                                          //
  //-----------------------------------------------------------------------//
  QcLinEqColState(int vi);
  QcLinEqColState(bool isbasic);

  //-----------------------------------------------------------------------//
  // Query functions
  //-----------------------------------------------------------------------//

  QcLinEqColState const *getNextBasicnessCol() const
  {
    return fNextCol;
  }

  /** Returns true iff the variable is a basic (i.e.&nbsp;dependent)
      variable. */
  bool isBasic() const
  { return fIsBasic; }

  /** The constraint that this variable is basic in, or InvalidCIndex if
      none. */
  int getConstraintBasicIn() const
  { return fIsBasicIn; }

  /** Returns cached value of variable's desired value, for during solve. */
  numT getGoalVal() const
  {
    return fDesValue;
  }

  //-----------------------------------------------------------------------
  // Setter functions
  //-----------------------------------------------------------------------
    /** The constraint that this variable is basic in, or InvalidCIndex if
      none. */
  void setConstraintBasicIn(int c)
  { fIsBasicIn = c; }

  //-----------------------------------------------------------------------//
  // Utility functions.                                                    //
  //-----------------------------------------------------------------------//
  virtual void Print(ostream &os);

private:
  numT fDesValue;	// cache desired value of variable during solve
  bool fIsBasic;		// means variable is basic, otherwise it is parametric.
  int fIsBasicIn;

  /** fNextCol and fPrevCol are used by QcLinEqColStateVector for some
      doubly-linked lists.  See that class for details. */
  QcLinEqColState *fNextCol;
  QcLinEqColState *fPrevCol;

protected:
  /** Arbitrary constant used as fIndex for the heads of linked lists.  Used
      only in assertion checking. */
  static unsigned const fHeadIndex = 0x4eadad4e;
};

inline QcLinEqColState::QcLinEqColState(int vi)
	: QcQuasiColState(vi),
	  fDesValue( 0),
	  fIsBasic( false),
	  fIsBasicIn(QcTableau::fInvalidConstraintIndex),
	  fNextCol( 0),
	  fPrevCol( 0)
{
}

inline QcLinEqColState::QcLinEqColState(bool isbasic)
	: QcQuasiColState( fHeadIndex),
	  fDesValue(),
	  fIsBasic( isbasic),
	  fNextCol( this),
	  fPrevCol( this)
{
}

inline void QcLinEqColState::Print(ostream &os)
{
	QcQuasiColState::Print(os);
  	os << ",DV(" << fDesValue << "),"
		<< "IsBasic(" << fIsBasic << "),"
		<< "BasicIn(" << fIsBasicIn << "),"
		<< "PrevCol(" << fPrevCol << "),"
		<< "NextCol(" << fNextCol << ")";
}

#endif
