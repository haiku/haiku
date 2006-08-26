// $Id: QcLinPoly.cc,v 1.11 2000/12/14 02:28:54 pmoulder Exp $

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

#include "qoca/QcDefines.hh"
#include "qoca/QcLinPoly.hh"

QcLinPoly::QcLinPoly(const QcLinPoly& rhs)
{
	size_t n = rhs.fTerms.size();
	fTerms.resize(n);
	for (unsigned int i = n; i-- != 0;)
		fTerms[i] = new QcLinPolyTerm(*rhs.fTerms[i]);
}

QcLinPoly::~QcLinPoly()
{
	unsigned n = fTerms.size();
	for (unsigned int i = 0; i < n; i++)
		delete fTerms[i];

	fTerms.resize(0);
}

#ifndef NDEBUG
void QcLinPoly::assertInvar() const
{
	unsigned n = getNTerms();
	for(unsigned i = 0; i < n; i++) {
		QcLinPolyTerm *ti = fTerms[i];
		qcAssertInvar(ti != NULL);
		ti->assertInvar();
		qcAssertInvar(ti->GetCoeff() != 0.0);
		/* effic: If we expect largish (n >= 6) termsets, then we could
		   use a marking technique (using a flag in each QcFloatRep). */
		for(unsigned j = i; j--;)
			qcAssertInvar(fTerms[j]->GetVariable() != ti->GetVariable());
	}
}
#endif

numT QcLinPoly::getValue() const
{
	numT v = 0.0;
	for (unsigned i = fTerms.size(); i-- != 0;)
		v += fTerms[i]->GetValue();
	return v;
}

bool QcLinPoly::hasVar (QcFloatRep const *v) const
{
	for (unsigned i = fTerms.size(); i-- != 0;)
		if (fTerms[i]->hasVar (v))
			return true;
	return false;
}

void QcLinPoly::Negate()
{
	for (unsigned int i = fTerms.size(); i-- != 0;)
		fTerms[i]->Negate();
}

void QcLinPoly::Print(ostream &os) const
{
	os << "(";
    
	if (fTerms.size() > 0) {
		os << *fTerms[0];

		for (unsigned int i = 1; i < fTerms.size(); i++)
			os << ", " << *fTerms[i];
 	}

	os << ")";
}

QcLinPoly &QcLinPoly::operator=(const QcLinPoly &rhs)
{
	if (this != &rhs) {
		for (unsigned int i = 0; i < fTerms.size(); i++)
			delete fTerms[i];

		fTerms.resize(0);

		for (unsigned int i = 0; i < rhs.fTerms.size(); i++)
			fTerms.push_back(new QcLinPolyTerm(*rhs.fTerms[i]));
	}

	dbg(assertInvar());
	return *this;
}

void QcLinPoly::add(numT coeff, QcFloatRep *var)
{
  if (!QcUtility::IsZero( coeff))
    doPlus( coeff, var);
}

void QcLinPoly::add(numT coeff, QcFloat const &var)
{
  if (!QcUtility::IsZero( coeff))
    doPlus( coeff, var.pointer());
}

QcLinPoly &QcLinPoly::operator+=(const QcLinPolyTerm& rhs)
	// If rhs is not zero then; if rhs does not exists in fTerms, add it,
	// otherwise add the existing lin_poly_term together with rhs
{
	numT coeff = rhs.GetCoeff();
	if (coeff != 0.0)
		doPlus( coeff, rhs.getVariable());
	return *this;
}

QcLinPoly& QcLinPoly::operator-=(const QcLinPolyTerm& rhs)
	//If rhs is not zero then; if rhs does not exists in fTerms, add it,
	// otherwise subtract rhs from the existing lin_poly_term.
{
	numT coeff = rhs.GetCoeff();
	if (coeff != 0.0)
		doPlus( -coeff, rhs.getVariable());
	return *this;
}

void QcLinPoly::doPlus(numT coeff, QcFloatRep *var)
{
  // First try to find existing term of same variable.
  for (TTermsVector::iterator i = fTerms.begin();
       i != fTerms.end();
       i++)
    {
      if ((*i)->getVariable() != var)
	continue;
      numT result = (*i)->addToCoeff( coeff);
      assert( QcUtility::IsZeroised( result));
      if (result == 0.0)
	{
	  delete( *i);
	  fTerms.erase( i);
	}
      goto out;
    }

  // No existing term with variable var, so append new term to list.
  fTerms.push_back( new QcLinPolyTerm( coeff, var));

 out:
  dbg(assertInvar());
}
