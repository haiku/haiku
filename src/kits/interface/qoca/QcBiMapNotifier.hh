// $Id: QcBiMapNotifier.hh,v 1.6 2001/01/30 01:32:07 pmoulder Exp $

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
// THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY	EXPRESSED OR //
// IMPLIED.  ANY USE IS AT YOUR OWN RISK.                                     //
//                                                                            //
// Permission is hereby granted to use or copy this program for any purpose,  //
// provided the above notices are retained on all copies.  Permission to      //
// modify the code and to distribute modified code is granted, provided the   //
// above notices are retained, and a notice that the code was modified is     //
// included with the above copyright notice.                                  //
//============================================================================//

#ifndef __QcBiMapNotifierH
#define __QcBiMapNotifierH

#include "qoca/QcConstraintBiMap.hh"
#include "qoca/QcVariableBiMap.hh"

class QcBiMapNotifier
{
private:
	QcConstraintBiMap fOCMap;
	QcVariableBiMap fVMap;

public:
	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
	QcBiMapNotifier() {}
	virtual ~QcBiMapNotifier() {}

#ifndef NDEBUG
  void assertDeepInvar() const;

  virtual void vAssertDeepInvar() const
  {
    assertDeepInvar();
  }
#endif

	//-----------------------------------------------------------------------//
	// Query functions.                                                      //
	//-----------------------------------------------------------------------//
	QcConstraintBiMap &GetOCMap()
  		{ return fOCMap; }

	QcVariableBiMap &GetVMap()
  		{ return fVMap; }

	//-----------------------------------------------------------------------//
	// Manipulation functions.                                               //
	//-----------------------------------------------------------------------//
	virtual void DropConstraint(int ci);

	virtual void DropVariable(int vi)
  		{ fVMap.EraseByIndex(vi); }

	virtual void SwapVariables(int vi1, int vi2)
  		{ fVMap.SwapByIndex(vi1, vi2); }

	virtual void Restart();
		// Restart erases everything ready to start afresh.

	//-----------------------------------------------------------------------//
	// Utility functions.                                                    //
	//-----------------------------------------------------------------------//
	virtual void Print(ostream &os) const;
};

#ifndef NDEBUG
inline void QcBiMapNotifier::assertDeepInvar() const
{
	fOCMap.assertDeepInvar();
	fVMap.assertDeepInvar();
}
#endif

inline void QcBiMapNotifier::DropConstraint(int ci)
{
	fOCMap.EraseByIndex(ci);
}

inline void QcBiMapNotifier::Print(ostream &os) const
{
	os << "Variables:" << endl;
	fVMap.Print(os);
	os << endl << "Constraints:" << endl;
	fOCMap.Print(os);
	os << endl;
}

inline void QcBiMapNotifier::Restart()
{
	fOCMap.Restart();
	fVMap.Restart();
}

#ifndef qcNoStream
inline ostream& operator<<(ostream& os, const QcBiMapNotifier &n)
{
	n.Print(os);
	return os;
}
#endif

#endif
