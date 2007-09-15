// $Id: QcLinPolyTerm.hh,v 1.10 2001/01/05 02:43:48 pmoulder Exp $

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
// Constants are no longer accepted as valid LinPolyTerms.  It will be easy   //
// to extend linpolyterm (and call it polyterm) to allow quadratic, cubic,    //
// etc terms.                                                                 //
//============================================================================//

#ifndef __QcLinPolyTermH
#define __QcLinPolyTermH

#include "qoca/QcDefines.hh"
#include "qoca/QcFloat.hh"
#include "qoca/QcUtility.hh"

class QcLinPolyTerm
{
public:
	//-----------------------------------------------------------------------//
	// Constructor                                                           //
	//-----------------------------------------------------------------------//
	QcLinPolyTerm(numT coeff, const QcFloat &var);
	QcLinPolyTerm(numT coeff, QcFloatRep *var);
	QcLinPolyTerm(const QcFloat &var);
	QcLinPolyTerm(const QcLinPolyTerm &term);
	virtual ~QcLinPolyTerm() {}

	//-----------------------------------------------------------------------//
	// Data Structure access functions.                                      //
	//-----------------------------------------------------------------------//
	QcFloat &GetVariable()
		{ return fVariable; }

	QcFloatRep *getVariable() const
		{ return fVariable.pointer(); }

	//-----------------------------------------------------------------------//
	// Query functions.                                                      //
	//-----------------------------------------------------------------------//

  void assertInvar() const
  {
    assert( QcUtility::IsZeroised( fCoeff));
  }

  /** @postcondition IsZeroised( ret) */
  numT GetCoeff() const
  {
    qcAssertPost( QcUtility::IsZeroised( fCoeff));
    return fCoeff;
  }

	QcFloat const &GetVariable() const
		{ return fVariable; }

#if 0
	QcFloat &GetVariable()
		{ return fVariable; }
#endif

	bool hasVar (QcFloatRep const *v) const
		{ return fVariable.pointer() == v; }

	numT GetValue() const
		{ return QcUtility::Zeroise( fCoeff * fVariable.Value()); }

	bool operator==(const QcLinPolyTerm &rhs) const;

	int operator!=(const QcLinPolyTerm &rhs) const
		{ return !(*this == rhs); }

	//-----------------------------------------------------------------------//
	// Manipulation functions.                                               //
	//-----------------------------------------------------------------------//
	void Negate()
		{ make_neg( fCoeff); }

  /** Used only by QcLinPoly.
      @postcondition IsZeroised(ret)
  **/
  numT addToCoeff( numT delta)
    {
      fCoeff = QcUtility::Zeroise( fCoeff + delta);
      dbg(assertInvar());
      return fCoeff;
    }

  void SetCoeff(numT coeff)
    {
      fCoeff = QcUtility::Zeroise( coeff);
      dbg(assertInvar());
    }

private:
	QcLinPolyTerm &operator+=(const QcLinPolyTerm &rhs);
	QcLinPolyTerm &operator-=(const QcLinPolyTerm &rhs);

	//-----------------------------------------------------------------------//
	// Utility functions.                                                    //
	//-----------------------------------------------------------------------//
public:
	virtual void Print(ostream &os) const;

private:
	numT fCoeff;
	QcFloat fVariable;
};

inline QcLinPolyTerm::QcLinPolyTerm(numT coeff, const QcFloat &var)
		: fCoeff( QcUtility::Zeroise( coeff)), fVariable( var)
{
  dbg(assertInvar());
}

inline QcLinPolyTerm::QcLinPolyTerm(numT coeff, QcFloatRep *var)
		: fCoeff( QcUtility::Zeroise( coeff)), fVariable( *var)
{
  dbg(assertInvar());
}

inline QcLinPolyTerm::QcLinPolyTerm(const QcFloat &var)
		: fCoeff( 1u), fVariable( var)
{
  dbg(assertInvar());
}

inline QcLinPolyTerm::QcLinPolyTerm(const QcLinPolyTerm &term)
		: fCoeff(term.fCoeff), fVariable(term.fVariable)
{
  dbg(assertInvar());
}

#if 0 /* unused */
inline void QcLinPolyTerm::assertDeepInvar() const
{
  fVariable.assertDeepInvar();
}
#endif

inline void QcLinPolyTerm::Print(ostream &os) const
{
  const char *name = fVariable.Name();

  os << fCoeff << " * "
     << ((name) ? name : "")
     << "#" << fVariable.Id();
}


#if 0
inline QcLinPolyTerm &QcLinPolyTerm::operator+=(const QcLinPolyTerm &other)
{
  qcAssertPre( fVariable == other.fVariable);

  fCoeff = QcUtility::Zeroise( fCoeff + other.fCoeff);
  dbg(assertInvar());
  return( *this);
}

inline QcLinPolyTerm &QcLinPolyTerm::operator-=(const QcLinPolyTerm& other)
{
  qcAssertPre( fVariable == other.fVariable);

  fCoeff = QcUtility::Zeroise( fCoeff - other.fCoeff);
  dbg(assertInvar());
  return( *this);
}
#endif


inline bool QcLinPolyTerm::operator==(const QcLinPolyTerm &rhs) const
{
  return (QcUtility::IsZero(fCoeff - rhs.fCoeff)
	  && ((fCoeff == 0)
	      || (fVariable == rhs.fVariable)));
}

inline QcLinPolyTerm operator-(QcLinPolyTerm const &pos)
{
  QcLinPolyTerm ret( pos);
  ret.Negate();
  return ret;
}

inline QcLinPolyTerm operator*(numT coeff, QcFloat &var)
{
  return QcLinPolyTerm( coeff, var);
}

inline ostream& operator<<(ostream &os, const QcLinPolyTerm &term)
{
  term.Print( os);
  return os;
}

#endif
