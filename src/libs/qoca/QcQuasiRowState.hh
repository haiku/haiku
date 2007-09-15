// $Id: QcQuasiRowState.hh,v 1.6 2000/12/12 09:59:27 pmoulder Exp $

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

#ifndef __QcQuasiRowStateH
#define __QcQuasiRowStateH

#include "qoca/QcState.hh"

class QcQuasiRowState : public QcState
{
  friend class QcQuasiRowStateVector;
  friend class QcQuasiRowColStateVector;

public:
  //-----------------------------------------------------------------------//
  // Constructor.                                                          //
  //-----------------------------------------------------------------------//
  QcQuasiRowState(int ci);

  //-----------------------------------------------------------------------
  // Query functions
  //-----------------------------------------------------------------------
  bool isMRowDeleted() const
  {
    return fMRowDeleted;
  }

  QcQuasiRowState const *getNextUndeletedRow() const
  {
    return fNextRow;
  }

  //-----------------------------------------------------------------------//
  // Utililty functions.                                                   //
  //-----------------------------------------------------------------------//
  virtual void Print(ostream &os);

public:
  int fIndex;	// The property vector index of this record.
  int fARowIndex;

private:
  bool fMRowDeleted;
  /** fNextRow and fPrevRow are used for an unsorted, doubly-linked list of
      undeleted (i.e.&nbsp;!fMRowDeleted) rows kept by QcQuasiRowStateVector. */
  QcQuasiRowState *fNextRow;
  QcQuasiRowState *fPrevRow;

};

inline QcQuasiRowState::QcQuasiRowState(int ci)
{
	fIndex = ci;
	fARowIndex = ci;
	fMRowDeleted = false;
	fNextRow = 0;
	fPrevRow = 0;
}

inline void QcQuasiRowState::Print(ostream &os)
{
  os << "Index(" << fIndex << "),"
     << "ARowIndex(" << fARowIndex << "),"
     << "MRowDeleted(" << fMRowDeleted << "),"
     << "NextRow(" << fNextRow << "),"
     << "PrevRow(" << fPrevRow << ")";
}

#endif
