// $Id: QcConstraintRep.hh,v 1.9 2001/01/04 05:20:11 pmoulder Exp $

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

#ifndef __QcConstriantRepH
#define __QcConstriantRepH

#include "qoca/QcDefines.hh"
#include "qoca/QcLinPoly.hh"

#define qcConstraintMagic1 45919
#define qcConstraintMagic2 "Qoca constraint:"
#define qcConstraintMagic2Len 16
        // The length of qcConstraintMagic2 without the terminal '\0'

class QcConstraintRep
{
public:
	typedef long TId;
		// It is intended that this be signed.  Negative values are not
		// allocated and are reserved for future (unknown) usage. The value 0
		// is reserved as an invalid identifier.

	enum TOp {
		coEQ,	// ==
		coLE,	// <=
		coGE	// >=
	};
	enum deprecated_TOp_vals {
		coLT = coLE,	// <  (implemented as <=)
		coGT = coGE	// >  (implemented as >=)
	};

	/** The highest valid operator number. */
	static const int coLastOp = coGE;

public:
	//-----------------------------------------------------------------------//
	// Constructors                                                          //
	//-----------------------------------------------------------------------//
	QcConstraintRep();
	QcConstraintRep(const QcLinPoly &lp);
	QcConstraintRep(const QcLinPoly &lp, const TOp op);
	QcConstraintRep(const QcLinPoly &lp, const TOp op, numT rhs);
	QcConstraintRep(const char *name);
	QcConstraintRep(const char *name, const QcLinPoly &lp);
	QcConstraintRep(const char *name, const QcLinPoly &lp, TOp op);
	QcConstraintRep(const char *name, const QcLinPoly &lp, TOp op, numT rhs);

	virtual ~QcConstraintRep()
    		{ delete [] fName; }

	//-----------------------------------------------------------------------//
	// Data Structure access functions.                                      //
	//-----------------------------------------------------------------------//
	const QcLinPoly &LinPoly() const
		{ return fLinPoly; }

	//-----------------------------------------------------------------------//
	// Query functions.                                                      //
	//-----------------------------------------------------------------------//
	long Counter() const
		{ return fCounter; }

	TId Id() const
		{ return fId; }

	TOp Operator() const
		{ return fOperator; }

	TOp getRelation() const
		{ return fOperator; }

	bool hasVar (QcFloatRep const *v) const
		{ return LinPoly().hasVar (v); }

	unsigned getNVars() const
		{ return LinPoly().getNTerms(); }

	QcFloat const &getVar (unsigned i) const
		{ return LinPoly().GetTerm (i).GetVariable(); }

  QcFloatRep *getVarPtr( unsigned i)
  {
    return LinPoly().GetTerm( i).getVariable();
  }

#if 0
	QcFloat &getVar (unsigned i)
		{ return LinPoly().GetTerm (i).GetVariable(); }
#endif

	int getVarId (unsigned i) const
		{ return getVar(i).Id(); }

	numT RHS() const
		{ return fRHS; }

	long Magic() const
		{ return fMagic; }

	const char *Name() const;

	bool operator<(const QcConstraintRep &other) const
		// The comparisons are for when QcConstraintRep is used as an
		// identifier and compares instances not structure.
		// General pointer inequality comparisons are not reliable
		// (see the C++ ARM section 5.9), fId is provided for this.

	  /* [pjm:] Another reason for comparing ids rather than pointers is for
	     weighted constraints, where we generate a new constraint object
	     with the same id.  The ids need to be the same so that
	     changeRHS,removeConstraint work.  See
	     QcSolver::AddConstraint(QcConstraint c, numT weight) for where
	     these half-copies are created. */
		{ return (fId < other.fId); }

#if 0
	bool operator==(const QcConstraintRep &other) const
		{ return (fId == other.fId); }

	bool operator!=(const QcConstraintRep &other) const
		{ return (fId != other.fId); }
#endif

	void assertInvar() const;

	//-----------------------------------------------------------------------//
	// Manipulation functions                                                //
	//-----------------------------------------------------------------------//
	virtual void SetLinPoly(QcLinPoly &p)
		{ fLinPoly = p; }

	virtual void SetRHS(numT rhs)
		{ fRHS = rhs; }

	virtual void SetOperator(TOp op)
		{ fOperator = op; }

	virtual void SetName(const char *n);

	void Decrease()
		// Called when number of references decreases
	{
		fCounter--;
		qcAssertPre(fCounter >= 0);
	}

	void Drop()
	{
		Decrease();
		if (fCounter != 0)
			return;
		dbg(assertInvar());
		delete this;
	}

	bool FreeName();
		// Deallocates old name string checking magic4.
		// Returns success indicator.

	void Increase()
		// Call when number of references increases
		{ fCounter++; }

	void Reset(); 	// Set variables to defaults

	/** Returns true iff this constraint is satisfied or
	    <tt>QcUtility.isZero(LHS - RHS)</tt>.  The current implementation
	    ignores the possibility of NaN or infinity problems. */
	bool isSatisfied() const;

	//-----------------------------------------------------------------------//
	// Utility functions                                                     //
	//-----------------------------------------------------------------------//
	virtual void Print(ostream &os) const;

protected:
	static TId fNextValidId;
	static const TId fInvalidId;
		// InvalidId is currently set to 0.
		// All negative values are reserved for future use.

	void Initialize(const char *n, const QcLinPoly& lp, const TOp op);
		// Allocate the representation record, set indicated values, set
		// other representation variables to default values and call Reset.

private:
	long fMagic;				// To detect corruption.
	long fCounter;				// Reference count.
	TId fId;					// Used as index key and for operator <.
	QcLinPoly fLinPoly;			// The constraint variable coeffs.
	TOp fOperator;				// The relation.
	numT fRHS;				// The RHS value.
	char *fName;				// 0 or a heap allocated string with hidden
    							// prefix.
};

#ifdef QOCA_INTERNALS
# if defined(qcCheckPre) || defined(qcCheckPost)
#  define VALID_OP(_op) ((_op) >= 0 && (_op) <= coLastOp)
# endif
#endif

inline QcConstraintRep::QcConstraintRep()
  : fRHS( 0)
{
	Initialize("", QcLinPoly(), coEQ);
}

inline QcConstraintRep::QcConstraintRep(const QcLinPoly &lp)
  : fRHS( 0)
{
	Initialize("", lp, coEQ);
}

inline QcConstraintRep::QcConstraintRep(const QcLinPoly &lp, TOp op)
  : fRHS( 0)
{
	Initialize("", lp, op);
}

inline QcConstraintRep::QcConstraintRep(const QcLinPoly &lp, TOp op, numT rhs)
  : fRHS( rhs)
{
	Initialize("", lp, op);
}

inline QcConstraintRep::QcConstraintRep(char const *name)
  : fRHS( 0)
{
	Initialize(name, QcLinPoly(), coEQ);
}

inline QcConstraintRep::QcConstraintRep(const char *name, const QcLinPoly &lp)
  : fRHS( 0)
{
	Initialize(name, lp, coEQ);
}

inline QcConstraintRep::QcConstraintRep(char const *name, const QcLinPoly &lp,
					TOp op)
  : fRHS( 0)
{
	Initialize(name, lp, op);
}

inline QcConstraintRep::QcConstraintRep(const char *name, const QcLinPoly &lp,
					TOp op, numT rhs)
  : fRHS( rhs)
{
	Initialize(name, lp, op);
}


inline const char *QcConstraintRep::Name() const
{
	if (fName == 0)
   		return 0;

	return (fName + qcConstraintMagic2Len);
}

inline void QcConstraintRep::Reset()
{
	fCounter = 1;
	fId = fNextValidId++;
	qcAssertInvar(fNextValidId > 0);
}

/** Returns true iff this constraint is satisfied or
    <tt>QcUtility.isZero(LHS - RHS)</tt>.  The current implementation
    ignores the possibility of NaN or infinity problems. */
inline bool QcConstraintRep::isSatisfied() const
{
	numT x = fLinPoly.getValue() - fRHS;

	if (QcUtility::IsZero (x))
		return true;

	switch (fOperator) {
	case coEQ:
		return false;

	case coLE:
		return x < 0;

	case coGE:
		return x > 0;
	}
	qcDurchfall("fOp " + fOperator);
	abort();
}

#if !qcCheckInternalInvar
inline void QcConstraintRep::assertInvar() const
{ }
#endif

#ifndef qcNoStream
inline ostream &operator<<(ostream &os, const QcConstraintRep &r)
{
	r.Print(os);
	return os;
}
#endif

struct QcConstraintRep_hash
{
  size_t operator()(QcConstraintRep const *c) const
  {
    return (size_t) c->Id();
  }
};

struct QcConstraintRep_equal
{
  size_t operator()(QcConstraintRep const *c1,
		    QcConstraintRep const *c2) const
  {
    return c1->Id() == c2->Id();
  }
};

#endif /* !__QcConstriantRepH */
