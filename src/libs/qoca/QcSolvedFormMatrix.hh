// $Id: QcSolvedFormMatrix.hh,v 1.8 2001/01/30 01:32:08 pmoulder Exp $
// Originally written by Alan Finlay and Sitt Sen Chok

//%BeginLegal ===============================================================
//
// The QOCA implementation is free software, but it is Copyright (C)          
// 1994-1999 Monash University.  It is distributed under the terms of the GNU 
// General Public License.  See the file COPYING for copying permission.      
//                                                                            
// The QOCA toolkit and runtime are distributed under the terms of the GNU    
// Library General Public License.  See the file COPYING.LIB for copying      
// permissions for those files.                                               
//                                                                            
// If those licencing arrangements are not satisfactory, please contact us!   
// We are willing to offer alternative arrangements, if the need should arise.
//                                                                            
// THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED OR  
// IMPLIED.  ANY USE IS AT YOUR OWN RISK.                                     
//                                                                            
// Permission is hereby granted to use or copy this program for any purpose,  
// provided the above notices are retained on all copies.  Permission to      
// modify the code and to distribute modified code is granted, provided the   
// above notices are retained, and a notice that the code was modified is     
// included with the above copyright notice.                                  
//
//%EndLegal =================================================================


#ifndef __QcSolvedFormMatrixH
#define __QcSolvedFormMatrixH

#include <vector>
#include "qoca/QcUtility.hh"
#include "qoca/QcSparseMatrix.hh"

/** A matrix of coefficients, and a RHS vector.  Except that if
    qcRealTableauCoeff is not defined, then the matrix is considered
    to be all zeroes; similarly, if qcRealTableauRHS is undefined then
    the RHS vector is considered to be all zeroes.
**/
class QcSolvedFormMatrix
{
public:
  //-----------------------------------------------------------------------//
  // Constructor.                                                          //
  //-----------------------------------------------------------------------//
  QcSolvedFormMatrix();

#ifndef NDEBUG
  void assertInvar() const
  {
# if defined(qcRealTableauCoeff) && defined(RealTableauRHS)
    assert( fCoeffs.getNRows() == fRHS.size());
# endif
  }

  void assertDeepInvar() const
  {
    assertInvar();
# ifdef qcRealTableauCoeff
    fCoeffs.assertDeepInvar();
# endif
  }  
#endif

  //-----------------------------------------------------------------------//
  // Query functions.                                                      //
  //-----------------------------------------------------------------------//

  numT GetValue( unsigned row, unsigned col) const;

  numT GetRHS( unsigned row) const;

#ifdef qcRealTableauCoeff
  QcSparseMatrix const &getCoeffs() const
  { return fCoeffs; }
#endif

  //-----------------------------------------------------------------------//
  // Manipulation functions.                                               //
  //-----------------------------------------------------------------------//
  bool AddScaledRow( unsigned destRow, unsigned srcRow, numT factor);

  /** @precondition <tt>destRow &lt; getNRows()</tt>
      @precondition <tt>srcRow &lt; srcMatrix.getNRows()</tt>
  **/
  void CopyCoeffRow( unsigned destRow, QcSparseMatrix const &srcMatrix, unsigned srcRow);

  void Reserve( unsigned r, unsigned c);
  void Resize( unsigned rows, unsigned cols);
  void Restart();

  /** Equivalent to ScaleRow(row, -1), but faster (especially if numT is a compound type). */
  void NegateRow( unsigned row);

  bool ScaleRow( unsigned row, numT factor);
  void SetValue( unsigned row, unsigned col, numT val);
  void CopyColumn( unsigned destCol, unsigned srcCol);
  void ZeroRow( unsigned row);
  void ZeroColumn( unsigned row);
  void IncreaseRHS( unsigned row, numT inc);
  void SetRHS( unsigned row, numT rhs);

#if 0 /* unused */
  void Print(ostream &os) const;
#endif

#if qcCheckInternalPre
private:
  unsigned getNRows() const;
  unsigned getNColumns() const;
#endif

private:
#ifdef qcRealTableauCoeff
  QcSparseMatrix fCoeffs;
#elif qcCheckInternalPre
  unsigned fColumns;
#endif
#ifdef qcRealTableauRHS
  vector<numT> fRHS;
#elif !defined(qcRealTableauCoeff) && qcCheckInternalPre
  unsigned fRows;
#endif
};


inline QcSolvedFormMatrix::QcSolvedFormMatrix()
#define SEPARATOR :
#ifdef qcRealTableauCoeff
  SEPARATOR	fCoeffs()
# undef SEPARATOR
# define SEPARATOR ,
#elif qcCheckInternalPre
  SEPARATOR	fColumns(0)
# undef SEPARATOR
# define SEPARATOR ,
#endif
#ifdef qcRealTableauRHS
  SEPARATOR	fRHS()
#elif !defined(qcRealTableauCoeff) && qcCheckInternalPre
  SEPARATOR	fRows(0)
#endif
#undef SEPARATOR
{
  dbg(assertInvar());
}


#if qcCheckInternalPre
inline unsigned
QcSolvedFormMatrix::getNRows() const
{
# ifdef qcRealTableauRHS
  return fRHS.size();
# elif defined(qcRealTableauCoeff)
  return fCoeffs.getNRows();
# else
  return fRows;
# endif
}

inline unsigned
QcSolvedFormMatrix::getNColumns() const
{
# ifdef qcRealTableauCoeff
  return fCoeffs.getNColumns();
# else
  return fColumns;
# endif
}
#endif


inline bool QcSolvedFormMatrix::AddScaledRow( unsigned destRow, unsigned srcRow,
					      numT factor)
{
  qcAssertPre( srcRow < getNRows());
  qcAssertPre( destRow < getNRows());

#ifdef qcRealTableauCoeff
  bool ret = fCoeffs.AddScaledRow( destRow, srcRow, factor);
#else
  bool ret = !qcIsZero( factor);
#endif

#ifdef qcRealTableauRHS
  if (ret)
    fRHS[destRow] += fRHS[srcRow] * factor;
#endif

  return ret;
}


inline numT QcSolvedFormMatrix::GetRHS( unsigned row) const
{
  qcAssertPre( row < getNRows());

#ifdef qcRealTableauRHS
  return fRHS[row];
#else
  return 0.0;
#endif
}


inline numT
QcSolvedFormMatrix::GetValue( unsigned row, unsigned col) const
{
  qcAssertPre( row < getNRows());
  qcAssertPre( col < getNColumns());

#ifdef qcRealTableauCoeff
  return fCoeffs.GetValue( row, col);
#else
  return 0.0;
#endif
}


inline void
QcSolvedFormMatrix::CopyCoeffRow( unsigned destRow, QcSparseMatrix const &srcMatrix, unsigned srcRow)
{
  qcAssertPre( destRow < getNRows());
  qcAssertPre( srcRow < srcMatrix.getNRows());

#ifdef qcRealTableauCoeff
  fCoeffs.CopyRow( destRow, srcMatrix, srcRow);
#else
  UNUSED(destRow);
  UNUSED(srcMatrix);
  UNUSED(srcRow);
#endif
}


inline void QcSolvedFormMatrix::IncreaseRHS( unsigned row, numT inc)
{
  qcAssertPre( row < getNRows());

#ifdef qcRealTableauRHS
  fRHS[row] += inc;
#endif
}


#if 0 /* unused */
inline void QcSolvedFormMatrix::Print(ostream &os) const
{
# ifdef qcRealTableauCoeff
  os << "Solved Form Matrix:" << endl;
  os << "===================" << endl;
  QcMatrix::Print(os);
  os << endl << endl;
# endif
# ifdef qcRealTableauRHS
  os << "rhs:" << endl;
  os << "====" << endl;
  for (unsigned i = 0; i < fRows; i++)
    os << "[" << i << "] " << GetRHS(i) << endl;
# endif
}
#endif


inline void QcSolvedFormMatrix::Resize( unsigned rows, unsigned cols)
{
#ifdef qcRealTableauRHS
  fRHS.resize( rows);
#endif
#ifdef qcRealTableauCoeff
  fCoeffs.Resize( rows, cols);
  dbg(assertInvar());
#endif
}


inline void QcSolvedFormMatrix::Restart()
{
#ifdef qcRealTableauRHS
  fRHS.resize(0);
#endif
#ifdef qcRealTableauCoeff
  fCoeffs.Resize(0, 0);
#endif
}


inline void QcSolvedFormMatrix::Reserve(unsigned row, unsigned col)
{
#ifdef qcRealTableauRHS
  fRHS.reserve( row);
#endif
#ifdef qcRealTableauCoeff
  fCoeffs.Reserve( row, col);
#endif
}


inline bool QcSolvedFormMatrix::ScaleRow(unsigned row, numT factor)
{
  qcAssertPre( row < getNRows());
  qcAssertPre( QcUtility::isFinite( factor));

#ifdef qcRealTableauCoeff
  bool ret = fCoeffs.ScaleRow( row, factor);
#else
  bool ret = !qcIsZero( factor - 1.0);
#endif

#ifdef qcRealTableauRHS
  if (ret)
    fRHS[row] *= factor;
#endif

  return ret;
}


inline void QcSolvedFormMatrix::NegateRow(unsigned row)
{
  qcAssertPre( row < getNRows());

#ifdef qcRealTableauCoeff
  fCoeffs.NegateRow( row);
#endif

#ifdef qcRealTableauRHS
  make_neg( fRHS[row]);
#endif
}


inline void QcSolvedFormMatrix::SetRHS(unsigned row, numT rhs)
{
  qcAssertPre( row < getNRows());

#ifdef qcRealTableauRHS
  fRHS[row] = rhs;
#endif
}

inline void QcSolvedFormMatrix::SetValue(unsigned row, unsigned col, numT val)
{
  qcAssertPre( row < getNRows());
  qcAssertPre( col < getNColumns());

#ifdef qcRealTableauCoeff
  fCoeffs.SetValue(row, col, val);
#endif
}

inline void QcSolvedFormMatrix::CopyColumn(unsigned destCol, unsigned srcCol)
{
  qcAssertPre( destCol < getNColumns());
  qcAssertPre( srcCol < getNColumns());

#ifdef qcRealTableauCoeff
  fCoeffs.CopyColumn( destCol, srcCol);
#endif
}

inline void QcSolvedFormMatrix::ZeroColumn( unsigned col)
{
  qcAssertPre( col < getNColumns());

#ifdef qcRealTableauCoeff
  fCoeffs.ZeroColumn( col);
#endif
}

inline void QcSolvedFormMatrix::ZeroRow(unsigned row)
{
  qcAssertPre( row < getNRows());

#ifdef qcRealTableauRHS
  fRHS[row] = 0.0;
#endif
#ifdef qcRealTableauCoeff
  fCoeffs.ZeroRow( row);
#endif
}


#endif /* !__QcSolvedFormMatrixH */
