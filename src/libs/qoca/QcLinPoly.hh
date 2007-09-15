// $Id: QcLinPoly.hh,v 1.10 2000/12/14 02:28:54 pmoulder Exp $

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
// THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY	EXPRESSED OR  //
// IMPLIED.  ANY USE IS AT YOUR OWN RISK.                                     //
//                                                                            //
// Permission is hereby granted to use or copy this program for any purpose,  //
// provided the above notices are retained on all copies.  Permission to      //
// modify the code and to distribute modified code is granted, provided the   //
// above notices are retained, and a notice that the code was modified is     //
// included with the above copyright notice.                                  //
//----------------------------------------------------------------------------//
// Note that although LinPoly uses a VectorOfLinPolyTermPtr and allocates the //
// LinPolyTerm values on the heap, the semantics of LinPoly costructors,      //
// copy, assignment, and destructor all create, copy or destroy private       //
// copies of the LinPolyTerm values.                                          //
//============================================================================//

#ifndef __QcLinPolyH
#define __QcLinPolyH

#include "qoca/QcUnsortedListSet.hh"
#include "qoca/QcLinPolyTerm.hh"

class QcLinPoly
{
public:
	friend QcLinPoly operator+(const QcLinPolyTerm &lhs, const QcLinPolyTerm &rhs);
	friend QcLinPoly operator-(const QcLinPolyTerm &lhs, const QcLinPolyTerm &rhs);
	friend QcLinPoly operator+(const QcLinPolyTerm &lhs, QcLinPoly &rhs);
	friend QcLinPoly operator-(const QcLinPolyTerm &lhs, QcLinPoly &rhs);

	//typedef vector<QcLinPolyTerm *> TTermsVector;
	typedef QcUnsortedListSet<QcLinPolyTerm *> TTermsVector;

public:
	//-----------------------------------------------------------------------//
	// Constructors                                                          //
	//-----------------------------------------------------------------------//
	QcLinPoly();
 	QcLinPoly(const QcLinPoly &p);
	QcLinPoly(const QcLinPolyTerm &lpt);
	virtual ~QcLinPoly();

	//-----------------------------------------------------------------------//
	// Query functions                                                       //
	//-----------------------------------------------------------------------//

#ifndef NDEBUG
	void assertInvar() const;
#endif

	const TTermsVector &GetTerms() const
		{ return fTerms; }

	const QcLinPolyTerm &GetTerm(int i) const
		{ return *fTerms[i]; }

	bool hasVar (QcFloatRep const *v) const;

	unsigned GetSize() const
		{ return fTerms.size(); }

	unsigned getNTerms() const
		{ return fTerms.size(); }

	numT getValue() const;

	//-----------------------------------------------------------------------//
	// Manipulation functions                                                //
	//-----------------------------------------------------------------------//
	void Negate();

	/** Push does not check the LinPoly to consolidate coeffs for the same
	    variable.  Hence it should only be used when this can be guaranteed.
	    It checks and does nothing if the coeff is zero.
	    [Roughly equivalent to <tt>addUniqueTerm</tt> in Java version.]
	**/
	void Push(numT coeff, QcFloat &var);

	void push (numT coeff, QcFloatRep *var);

  /** Like operator+=: coeff is allowed to be zero, and var is allowed to be already present. */
  void add(numT coeff, QcFloatRep *var);
  void add(numT coeff, QcFloat const &var);

	QcLinPoly &operator=(const QcLinPoly &rhs);
	QcLinPoly &operator+=(const QcLinPolyTerm &rhs);
	QcLinPoly &operator-=(const QcLinPolyTerm &rhs);
	QcLinPoly &operator+(const QcLinPolyTerm &rhs);
	QcLinPoly &operator-(const QcLinPolyTerm &rhs);

	QcLinPoly &operator-()
		{ Negate(); return (*this); }

	//-----------------------------------------------------------------------//
	// Utility functions                                                     //
	//-----------------------------------------------------------------------//
	virtual void Print(ostream &os) const;

private:
	void doPlus(numT coeff, QcFloatRep *var);

	TTermsVector fTerms;
};

inline QcLinPoly::QcLinPoly()
{
}

inline QcLinPoly::QcLinPoly(const QcLinPolyTerm &term)
{
	if (term.GetCoeff() != 0.0)
		fTerms.push_back(new QcLinPolyTerm(term));
}

inline void QcLinPoly::Push(numT coeff, QcFloat &var)
{
#if qcCheckPre
	for (unsigned int i = fTerms.size(); i-- != 0;)
		qcAssertExternalPre(var != fTerms[i]->GetVariable());
#endif
	if (!QcUtility::IsZero(coeff))
		fTerms.push_back(new QcLinPolyTerm(coeff, var));
	dbg(assertInvar());
}

inline void QcLinPoly::push (numT coeff, QcFloatRep *var)
{
#if qcCheckPre
	for (unsigned int i = fTerms.size(); i-- != 0;)
		qcAssertExternalPre(var != fTerms[i]->getVariable());
#endif
	if (!QcUtility::IsZero(coeff))
		fTerms.push_back (new QcLinPolyTerm (coeff, var));
	dbg(assertInvar());
}

inline QcLinPoly &QcLinPoly::operator+(const QcLinPolyTerm& rhs)
{
	if (rhs.GetCoeff() != 0.0)
		*this += rhs;

	return *this;
}


inline QcLinPoly& QcLinPoly::operator-(const QcLinPolyTerm& rhs)
{
    if (rhs.GetCoeff() != 0.0)
		*this -= rhs;

    return *this;
}

inline QcLinPoly operator+(const QcLinPolyTerm &lhs, QcLinPoly &rhs)
{
	if (lhs.GetCoeff() != 0.0)
   		rhs += lhs;

	return rhs;
}

inline QcLinPoly operator-(const QcLinPolyTerm &lhs, QcLinPoly &rhs)
{
	rhs.Negate();

	if (lhs.GetCoeff() != 0.0)
		rhs += lhs;

	return rhs;
}

inline QcLinPoly operator+(const QcLinPolyTerm &lhs, const QcLinPolyTerm &rhs)
{
	QcLinPoly new_lin_poly(lhs);
	new_lin_poly += rhs;
	return new_lin_poly;
}

inline QcLinPoly operator-(const QcLinPolyTerm &lhs, const QcLinPolyTerm &rhs)
{
	QcLinPoly new_lin_poly(lhs);
	new_lin_poly -= rhs;
	return new_lin_poly;
}

#ifndef qcNoStream
inline ostream& operator<<(ostream &os, const QcLinPoly &p)
{
	p.Print(os);
	return os;
}
#endif

#endif
