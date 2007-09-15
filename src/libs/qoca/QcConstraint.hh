// $Id: QcConstraint.hh,v 1.11 2001/01/10 05:01:33 pmoulder Exp $

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
//============================================================================//

#ifndef __QcConstriantH
#define __QcConstriantH

#include "qoca/QcDefines.hh"
#include "qoca/QcConstraintRep.hh"
#include "qoca/QcLinPoly.hh"

/** This class provides the only mechanism to create constraints and the
    constructors and the constructors return a handle.  See
    <tt>QcNullableElement</tt> for comments on assignment semantics etc.
**/
class QcConstraint
{
public:
	friend class QcNullableElement<QcConstraintRep,QcConstraint>;

	//-----------------------------------------------------------------------//
	// Constructors                                                          //
	//-----------------------------------------------------------------------//
	QcConstraint(QcConstraintRep &r);
	QcConstraint(QcConstraintRep *p);
	QcConstraint();
	QcConstraint(const char *name);
	QcConstraint(const char *name, const QcLinPoly &p);
	QcConstraint(const char *name, const QcLinPoly &p, QcConstraintRep::TOp op);
	QcConstraint(const char *name, const QcLinPoly &p, QcConstraintRep::TOp op,
    		numT rhs);
	QcConstraint(QcConstraint const &other);
	QcConstraint(QcNullableElement<QcConstraintRep,QcConstraint> const &other);

	~QcConstraint()
    		{ Decrease(); }

	void assertInvar() const
	{
		qcAssertInvar(fRep != NULL);
		fRep->assertInvar();
	}

	//-----------------------------------------------------------------------//
	// Data structure access functions                                       //
	//-----------------------------------------------------------------------//
	QcConstraintRep const *pointer() const
    		{ return fRep; }

	QcConstraintRep *pointer()
    		{ return fRep; }

#if 0
	const QcConstraintRep &Rep() const
    		{ return *fRep; }
#endif

	//-----------------------------------------------------------------------//
	// Query functions                                                       //
	//-----------------------------------------------------------------------//
	long Counter() const
    		{ return fRep->Counter(); }

	const char *Name() const
		{ return fRep->Name(); }

	QcConstraintRep::TId Id() const
		{ return fRep->Id(); }

	const QcLinPoly& LinPoly() const
		{ return fRep->LinPoly(); }

	QcConstraintRep::TOp Operator() const
		{ return fRep->Operator(); }

	QcConstraintRep::TOp getRelation() const
		{ return fRep->Operator(); }

	numT RHS() const
		{ return fRep->RHS(); }

	numT getRHS() const
		{ return fRep->RHS(); }

	bool hasVar (QcFloatRep const *v) const
		{ return fRep->LinPoly().hasVar (v); }

	unsigned getNVars() const
		{ return fRep->LinPoly().getNTerms(); }

	QcFloat const &getVar (unsigned i) const
		{ return fRep->LinPoly().GetTerm (i).GetVariable(); }

	int getVarId (unsigned i) const
		{ return getVar(i).Id(); }

	bool operator <(const QcConstraint &other) const
		{ return *fRep < *other.fRep; }

	bool operator ==(const QcConstraint &other) const
		{ return (fRep == other.fRep); }

	bool operator !=(const QcConstraint &other) const
		{ return (fRep != other.fRep); }

	//-----------------------------------------------------------------------//
	// Set and Get member functions                                          //
	//-----------------------------------------------------------------------//
	void SetLinPoly(QcLinPoly &p)
		{ fRep->SetLinPoly(p); }

	void SetRHS(numT rhs)
		{ fRep->SetRHS(rhs); }

	void SetOperator(QcConstraintRep::TOp op)
		{ fRep->SetOperator(op); }

	void SetName(char const * const n)
		{ fRep->SetName(n); }

	void makeEq(QcLinPoly *p, numT rhs)
	{
		fRep->SetLinPoly(*p);
		fRep->SetOperator(QcConstraintRep::coEQ);
		fRep->SetRHS(rhs);
	}

	void makeLe(QcLinPoly *p, numT rhs)
	{
		fRep->SetLinPoly(*p);
		fRep->SetOperator(QcConstraintRep::coLE);
		fRep->SetRHS(rhs);
	}

	void makeGe(QcLinPoly *p, numT rhs)
	{
		fRep->SetLinPoly(*p);
		fRep->SetOperator(QcConstraintRep::coGE);
		fRep->SetRHS(rhs);
	}

	void Decrease();
		// Called when number of references decreases.

	bool FreeName()
 		// Deallocates old name string.
		{ return fRep->FreeName(); }

	void Increase() const
		// Call when number of references increases.
		{ fRep->Increase(); }

	QcConstraint &operator=(const QcConstraint &other);
		// N.B. checked self assignment, x = x ok

	/** Returns true iff this constraint is satisfied or
	    <tt>QcUtility.isZero(LHS - RHS)</tt>.  The current implementation
	    ignores the possibility of NaN or infinity problems. */
	bool isSatisfied() const
		{ return fRep->isSatisfied(); }

	//------------------------------------------------------------------------//
	// Utility functions                                                      //
	//------------------------------------------------------------------------//
	void Print(ostream &os) const
		{ fRep->Print(os); }

	bool isDead() const
		{ return fRep == 0; }

protected:
	QcConstraintRep *fRep;
};

inline QcConstraint::QcConstraint(QcConstraintRep &r)
		: fRep(&r)
{
}

inline QcConstraint::QcConstraint (QcConstraintRep *p)
		: fRep (p)
{
}

inline QcConstraint::QcConstraint()
		: fRep(new QcConstraintRep())
{
}

inline QcConstraint::QcConstraint(const char *name)
		: fRep(new QcConstraintRep(name))
{
}

inline QcConstraint::QcConstraint(const char *name, const QcLinPoly &p)
		: fRep(new QcConstraintRep(name, p))
{
}
inline QcConstraint::QcConstraint(const char *name, const QcLinPoly &p,
		QcConstraintRep::TOp op)
		: fRep(new QcConstraintRep(name, p, op))
{
}

inline QcConstraint::QcConstraint(const char *name, const QcLinPoly &p,
		QcConstraintRep::TOp op, numT rhs)
		: fRep(new QcConstraintRep(name, p, op, rhs))

{
}

inline QcConstraint::QcConstraint(const QcConstraint &other)
		: fRep(other.fRep)
{
	Increase();
}

inline QcConstraint::QcConstraint(QcNullableElement<QcConstraintRep,QcConstraint> const &other)
		: fRep(other.fRep)
{
	Increase();
}

#ifndef qcSafetyChecks
inline void QcConstraint::Decrease()
{
	fRep->Decrease();

	if (fRep->Counter() == 0)
		delete fRep;
	fRep = 0;
}
#else
inline void QcConstraint::Decrease()
{
	fRep->Decrease();

	if (fRep->Counter() < 0)
		throw QcWarning(
			"QcConstraint::Decrease: invalid count (already free?)");
	else if (fRep->Counter() == 0) {
		if (fRep->Magic() == qcConstraintMagic1 && FreeName()) {
			delete fRep;
		} else
         		throw QcWarning(
				"QcConstraint::Decrease detected constraint corruption");
	}
	fRep = 0;
}
#endif

inline QcConstraint &QcConstraint::operator=(const QcConstraint &other)
{
	other.fRep->Increase();
	Decrease();
	fRep = other.fRep;
	return *this;
}

#ifndef QOCA_NO_FANCY_OPERATORS
inline QcConstraint operator ==(const QcLinPoly &lhs, numT rhs)
{
	return QcConstraint("", lhs, QcConstraintRep::coEQ, rhs);
}

inline QcConstraint operator <=(const QcLinPoly &lhs, numT rhs)
{
	return QcConstraint("", lhs, QcConstraintRep::coLE, rhs);
}

inline QcConstraint operator >=(const QcLinPoly &lhs, numT rhs)
{
	return QcConstraint("", lhs, QcConstraintRep::coGE, rhs);
}

inline QcConstraint operator ==(const QcLinPolyTerm &lhs, numT rhs)
{
	return QcConstraint("", QcLinPoly(lhs), QcConstraintRep::coEQ, rhs);
}

inline QcConstraint operator <=(const QcLinPolyTerm &lhs, numT rhs)
{
	return QcConstraint("", QcLinPoly(lhs), QcConstraintRep::coLE, rhs);
}

inline QcConstraint operator >=(const QcLinPolyTerm &lhs, numT rhs)
{
	return QcConstraint("", QcLinPoly(lhs), QcConstraintRep::coGE, rhs);
}
#endif /* QOCA_NO_FANCY_OPERATORS */

#ifndef qcNoStream
inline ostream &operator<<(ostream &os, const QcConstraint &c)
{
	c.Print(os);
	return os;
}
#endif

#endif /* !__QcConstriantH */
