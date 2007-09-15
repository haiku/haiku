// $Id: QcConstraintIndexIterator.hh,v 1.5 2000/11/29 01:58:42 pmoulder Exp $

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

#ifndef __QcConstraintIndexIteratorH
#define __QcConstraintIndexIteratorH

class QcConstraintIndexIterator
{
public:
  //-----------------------------------------------------------------------//
  // Constructor.                                                          //
  //-----------------------------------------------------------------------//
  QcConstraintIndexIterator (const QcLinEqTableau &tab);


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
  QcQuasiRowStateVector const &fRowState;
  QcQuasiRowState const *fCurrent;
};

inline
QcConstraintIndexIterator::QcConstraintIndexIterator (const QcLinEqTableau &tab)
  : fRowState(tab.GetRowState())
{
  Reset();
}

inline void QcConstraintIndexIterator::Increment()
{
  qcAssertPre (!AtEnd());
  fCurrent = fCurrent->getNextUndeletedRow();
}

inline void QcConstraintIndexIterator::Reset()
{
  fCurrent = fRowState.fRowList;
}

#endif
