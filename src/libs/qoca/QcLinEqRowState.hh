// $Id: QcLinEqRowState.hh,v 1.4 2000/12/13 01:44:58 pmoulder Exp $

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

#ifndef __QcLinEqRowStateH
#define __QcLinEqRowStateH

#include "qoca/QcQuasiRowState.hh"
#include "qoca/QcTableau.hh"

class QcLinEqRowState : public QcQuasiRowState
{
public:
	enum {
		fInvalid = 0,
		fNormalised = 1,
		fRegular = 2,
		fRedundant = 3
	};
	// If any row is normalised then the tableau is not in solved form
	// (undetermined means invalid or normalised).

	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
	QcLinEqRowState(int ci);

	//-----------------------------------------------------------------------//
	// Utility functions.                                                    //
	//-----------------------------------------------------------------------//
	virtual void Print(ostream &os);

  int getCondition() const
  {
    dbg(assertValidCondition( fCondition));
    return fCondition;
  }

  void setCondition(int c)
  {
    dbg(assertValidCondition( c));
    fCondition = c;
  }

  void swap(QcLinEqRowState &other)
  {
    // Swap fCondition
    {
      int tmp = fCondition;
    
      fCondition = other.fCondition;
      other.fCondition = tmp;
    }

    // Swap fBasicVar
    {
      int tmp = fBasicVar;

      fBasicVar = other.fBasicVar;
      other.fBasicVar = tmp;
    }
  }

private:
#ifndef NDEBUG
  void assertValidCondition(int c) const
  {
    assert( (unsigned) c <= 3);
  }
#endif

private:
	int fCondition;
public:
	int fBasicVar; 	// The basic variable for a regular row else
				// fInvalidVariableIndex.
};

inline QcLinEqRowState::QcLinEqRowState(int ci)
		: QcQuasiRowState(ci)
{
	fCondition = fInvalid;
	fBasicVar = QcTableau::fInvalidVariableIndex;
}

inline void QcLinEqRowState::Print(ostream &os)
{
	QcQuasiRowState::Print(os);

	os << ",Cond(";

	switch (fCondition) {
		case fInvalid:
            	os << "Invalid";
			break;
       	case fNormalised:
            	os << "Normalised";
			break;
       	case fRegular:
            	os << "Regular";
			break;
		case fRedundant:
			os << "Redundant";
			break;
	}

	os << "), BasicVar(" << fBasicVar << ")";
}

#endif
