// $Id: QcStructVarIndexIterator.hh,v 1.5 2000/11/29 01:58:42 pmoulder Exp $

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

#ifndef __QcStructVarIndexIteratorH
#define __QcStructVarIndexIteratorH

class QcStructVarIndexIterator
{
public:
  QcStructVarIndexIterator(const QcLinInEqTableau &tab);

#if 0
  QcStructVarIndexIterator(const QcLinInEqColStateVector &csv)
    : fColState( csv)
  {
    Reset();
  }
#endif

  bool AtEnd() const
    { return (fCurrent == 0); }

  unsigned getIndex() const
  {
    qcAssertPre (!AtEnd());
    unsigned ret = fCurrent->fIndex;
    qcAssertPost ((int) ret >= 0);
    return ret;
  }

  void Increment();
  void Reset();

private:
  QcLinInEqColStateVector const &fColState;
  QcLinInEqColState const *fCurrent;
};

inline QcStructVarIndexIterator::QcStructVarIndexIterator(
		const QcLinInEqTableau &tab)
  : fColState((QcLinInEqColStateVector const &)tab.GetColState())
{
	Reset();
}

inline void QcStructVarIndexIterator::Increment()
{
	qcAssertPre (!AtEnd());
	fCurrent = fCurrent->getNextStructuralCol();
	qcAssertPost( AtEnd()
		      || (fCurrent->getCondition()
			  & QcLinInEqColState::fStructural));
}

inline void QcStructVarIndexIterator::Reset()
{
	fCurrent = fColState.fStructVarList;
	qcAssertPost( AtEnd()
		      || (fCurrent->getCondition()
			  & QcLinInEqColState::fStructural));
}

#endif /* !__QcStructVarIndexIteratorH */
