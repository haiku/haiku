// $Id: QcFloat.hh,v 1.15 2000/12/14 02:28:54 pmoulder Exp $

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
// This class provides the only mechanism to create cfloats and the           //
// constructors return a handle.  The semantics of cfloat handles are sharing //
// on assignment with automatic destruction when the last handle is lost.     //
// The handles are implemented to use a representation that fits in a single  //
// word.  Handles are therefore quite efficient to pass by value. Pass        //
// handles by reference where efficiency is critical.  It is intended that    //
// clients be able to derive classes from QcFloat and be able to              //
// override most public methods.                                              //
//============================================================================//

#ifndef __QcFloatH
#define __QcFloatH

#if HAVE_LROUND || HAVE_LRINT || HAVE_ROUND
# define _ISOC9X_SOURCE 1
#endif
#if HAVE_LROUND || HAVE_LRINT || HAVE_ROUND || HAVE_RINT
# include <math.h>
#endif
#include "qoca/QcDefines.hh"
#include "qoca/QcFloatRep.hh"
#include "qoca/QcNullableElement.hh"
#include "qoca/QcNomadicFloatRep.hh"

#define subclassOfNullable 0
class QcFloat
#if subclassOfNullable
	: QcNullableElement<QcFloatRep,QcFloat>
#endif
{
public:
	friend class QcNullableElement<QcFloatRep,QcFloat>;

	//-----------------------------------------------------------------------//
	// Constructors                                                          //
	//-----------------------------------------------------------------------//
	explicit QcFloat(QcFloatRep *ptr);
	explicit QcFloat(QcFloatRep &rep);
	explicit QcFloat(bool restricted = false);
	explicit QcFloat(const char *name, bool restricted = false);
	QcFloat(const char *name, numT desval, bool restricted = false);
	QcFloat(const char *name, numT desval, numT sw, numT ew,
    		bool restricted = false);
	QcFloat(const QcFloat &other);
	//QcFloat(QcNullableElement<QcFloatRep,QcFloat> const &other);

	~QcFloat()
    		{ Decrease(); }

	void assertInvar() const
	{
		qcAssertInvar(fRep != 0);
		fRep->assertInvar();
	}

	//-----------------------------------------------------------------------//
	// Data structure access functions                                       //
	//-----------------------------------------------------------------------//
	QcFloatRep *pointer() const
		{ return fRep; }

#if 0
	const QcFloatRep &Rep() const
		{ return *fRep; }

	const QcFloatRep &getRep() const
		{ return *fRep; }
#endif


#if NUMT_IS_DOUBLE
# ifdef HAVE_LROUND
#  define Round(_x) lround (_x)
# elif HAVE_LRINT
#  define Round(_x) lrint (_x)
# elif HAVE_ROUND
#  define Round(_x) ((long) round (_x))
# elif HAVE_RINT
#  define Round(_x) ((long) rint (_x))
# else
private:
	static inline long Round (double x)
	{
		return long (x > 0.0
			     ? x + 0.5
			     : x - 0.5);
	}
public:
# endif
#elif NUMT_IS_PJM_MPQ
# define Round(_x) ((_x).get_lround())
#else
# error "Unknown numT"
#endif


	//-----------------------------------------------------------------------//
	// Query functions                                                       //
	//-----------------------------------------------------------------------//
	long Counter() const
		{ return (long)fRep->Counter(); }

	numT DesireValue() const
		{ return fRep->DesireValue(); }

	numT EditWeight() const
		{ return fRep->EditWeight(); }

	QcFloatRep::TId Id() const
		{ return fRep->Id(); }

	QcFloatRep::TId getId() const
		{ return fRep->Id(); }

	long IntDesireValue() const
		{ return Round(fRep->DesireValue()); }

	long IntValue() const
		{ return Round(fRep->Value()); }

	bool IsRestricted() const
		{ return fRep->IsRestricted(); }

	const char *Name() const
		{ return fRep->Name(); }

	numT StayWeight() const
		{ return fRep->StayWeight(); }

	numT Weight() const
		{ return fRep->Weight(); }

	numT Value() const
		{ return GetValue(); }
	numT GetValue() const
		{ return fRep->Value(); }

	bool operator <(const QcFloat &other) const
		{ return *fRep < *other.fRep; }

	/* FIXME: Is it a problem that these compare pointers whereas `<' compares ids?
	   E.g. is it possible that (a != b) && !(a < b) && !(b < a)? */
	bool operator ==(const QcFloat &other) const
		{ return (fRep == other.fRep); }

	bool operator !=(const QcFloat &other) const
		{ return (fRep != other.fRep); }

#if 0
	operator int() const
		{ return (int) Round( fRep->Value()); }
#endif

	bool isDead() const
		{ return fRep == 0; }

	//-----------------------------------------------------------------------//
	// Manipulation functions                                                //
	//-----------------------------------------------------------------------//
private:
	/** Deallocates old name string. */
	bool FreeName()
		{ return fRep->FreeName(); }

	/** Called when number of references decreases. */
	void Decrease();

	/** Call when number of references increases. */
	void Increase()
	{
#if qcReportAlloc
		cerr << "f: " << &fRep << " increasing " << fRep << endl;
#endif
		fRep->Increase();
	}

public:
	void SetWeight(numT w)
	{ fRep->SetWeight(w); }
	
	void SetValue(numT v)
	{ fRep->SetValue(v); }

	void SuggestValue(numT dv)
	{ fRep->SuggestValue(dv); }
	
	void SetName(const char *n)
	{ fRep->SetName(n); }
	
	void SetToEditWeight()
	{ fRep->SetToEditWeight(); }
	
	void SetToStayWeight()
	{ fRep->SetToStayWeight(); }
	
	void SetVariable()
	{ fRep->SetToGoal(); }

	/** Old name for <tt>RestDesVal</tt>. */
	void RestVariable()
	{ fRep->RestDesVal(); }

	void RestDesVal()
	{ fRep->RestDesVal(); }

	bool RestDesVal_changed()
	{ return fRep->RestDesVal_changed(); }

	QcFloat &operator=(const QcFloat &other);
		// N.B. checked self assignment, x = x ok

	//-----------------------------------------------------------------------//
	// Utility functions                                                     //
	//-----------------------------------------------------------------------//
	void Print(ostream &os) const
	{ fRep->Print(os); }

protected:
	QcFloatRep *fRep;
};

inline QcFloat::QcFloat(QcFloatRep *ptr)
	: fRep (ptr)
{
#if qcReportAlloc
	cerr << "f: " << &fRep << " constructing from rep " << fRep << endl;
#endif
	Increase();
}

inline QcFloat::QcFloat(QcFloatRep &rep)
		: fRep(&rep)
{
#if qcReportAlloc
	cerr << "f: " << &fRep << " constructing from rep " << fRep << endl;
#endif
	Increase();
}

inline QcFloat::QcFloat(bool restricted)
		: fRep(new QcNomadicFloatRep(restricted))
{
#if qcReportAlloc
	cerr << "f: " << &fRep << " created QcFloatRep " << fRep << endl;
#endif
}

inline QcFloat::QcFloat(const char *name, bool restricted)
		: fRep(new QcNomadicFloatRep(name, restricted))
{
#if qcReportAlloc
	cerr << "f: " << &fRep << " created QcFloatRep " << fRep << endl;
#endif
}

inline QcFloat::QcFloat(const char *name, numT desval, bool restricted)
		: fRep(new QcNomadicFloatRep(name, desval, restricted))
{
#if qcReportAlloc
	cerr << "f: " << &fRep << " created QcFloatRep " << fRep << endl;
#endif
}

inline QcFloat::QcFloat(const char *name, numT desval, numT sw, numT ew,
		bool restricted)
		: fRep(new QcNomadicFloatRep(name, desval, sw, ew, restricted))
{
#if qcReportAlloc
	cerr << "f: " << &fRep << " created QcFloatRep " << fRep << endl;
#endif
}

inline QcFloat::QcFloat(const QcFloat &other)
		: fRep(other.fRep)
{
#if qcReportAlloc
	cerr << "f: " << &fRep << " constructed from QcFloatRep " << fRep << endl;
#endif
	Increase();
}

#if 0
inline QcFloat::QcFloat(QcNullableElement<QcFloatRep,QcFloat> const &other)
		: fRep(other.fRep)
{
# if qcReportAlloc
	cerr << "f: " << &fRep << " constructed from QcFloatRep " << fRep << endl;
# endif
	Increase();
}
#endif

#ifndef qcSafetyChecks
inline void QcFloat::Decrease()
{
# if qcReportAlloc
	cerr << "f: " << &fRep << " decreasing " << fRep << endl;
# endif
# if 0
	if (fRep == 0)
		return;
# endif
	fRep->Decrease();

	if (fRep->Counter() == 0)
		delete fRep;
	fRep = 0;
}
#else /* qcSafetyChecks */
inline void QcFloat::Decrease()
{
# if qcReportAlloc
	cout << "Decreasing " << fRep << endl;
# endif
# if 0
	if (fRep == 0)
		return;
# endif
	fRep->Decrease();

	if (fRep->Counter() == 0) {
		fRep->assertInvar();
		if (fRep->Magic() == qcFloatMagic1 && fRep->FreeName()) {
			delete fRep;
		} else
         		throw QcWarning("QcFloat::Decrease detected cfloat corruption");
	}
	fRep = 0;
}
#endif /* qcSafetyChecks */

inline QcFloat &QcFloat::operator=(const QcFloat &other)
{
	other.fRep->Increase();
	Decrease();
	fRep = other.fRep;
	return *this;
}

#ifndef qcNoStream
inline ostream &operator<<(ostream &os, const QcFloat &cf)
{
	cf.Print(os);
	return os;
}
#endif

#endif /* !__QcFloatH */
