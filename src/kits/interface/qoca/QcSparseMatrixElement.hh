// $Id: QcSparseMatrixElement.hh,v 1.7 2001/01/31 10:04:51 pmoulder Exp $

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

#ifndef __QcSparseMatrixElementH
#define __QcSparseMatrixElementH

#include "qoca/QcDefines.hh"

class QcSparseMatrixElement
{
public:
  friend class QcSparseMatrix;

  QcSparseMatrixElement( numT val, unsigned row, unsigned col)
    : fValue( val),
      fRow( row),
      fColumn( col)
    {
      qcAssertPre( int( row + 1) >= 0);
      qcAssertPre( int( col + 1) >= 0);
    }

private:
  // Dummy constructors so that usage is caught at compile-time.
  QcSparseMatrixElement();
  QcSparseMatrixElement(QcSparseMatrixElement const &);

public:

  inline numT
  getValue() const
  {
    return fValue;
  }

  inline unsigned
  getRowNr() const
  {
    qcAssertPost( int( fRow + 1) >= 0);
    return fRow;
  }

  inline bool
  isColHead() const
  {
    return fRow == ~0u;
  }

  inline bool
  isRowHead() const
  {
    return fColumn == ~0u;
  }

  inline unsigned
  getColNr() const
  {
    qcAssertPost( int( fColumn + 1) >= 0);
    return fColumn;
  }

  inline QcSparseMatrixElement const *
  getNextInCol() const
  {
    return fNextRow;
  }

  inline QcSparseMatrixElement const *
  getNextInRow() const
  {
    return fNextColumn;
  }

private:
	numT fValue;
	unsigned fRow;				// Row index of element.
	unsigned fColumn;			// Column index of element.
	QcSparseMatrixElement *fNextRow;	// Next elements in this row.
	QcSparseMatrixElement *fNextColumn;	// Next elements in this col.
	QcSparseMatrixElement *fPrevRow;	// Previous elements in this row.
	QcSparseMatrixElement *fPrevColumn;	// Previous elements in this col.
};

#endif
