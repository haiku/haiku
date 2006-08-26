#ifndef __QcNullableElementH
#define __QcNullableElementH

#include "qoca/QcDefines.hh"


#if qcReportAlloc
# define reportAction(_action) \
	do { \
		cerr << "0: " << &fRep << _action << fRep << '\n'; \
	} while(0)
# define reportConstruction() reportAction(" constructed from QcFloatRep ")
# define reportIncrease() reportAction(" increasing ")
# define reportDecrease() reportAction(" decreasing ")
# undef reportAction
#else /* !qcReportAlloc */
# define reportConstruction() do { } while(0)
# define reportIncrease() do { } while(0)
# define reportDecrease() do { } while(0)
#endif /* !qcReportAlloc */


/** Reference-counting handle.  Its data is a single pointer, and the
    constructor and deconstructor do reference counting.

    <p>The semantics of constrant handles are sharing on assignment
    with automatic destruction when the last handle is lost.

    <p>Pass-by-reference versus pass-by-value: Passing by reference
    avoids having the constructor and destructor called, at the cost
    of an extra level of indirection per method call.  So
    pass-by-reference is best for short routines, while pass-by-value
    is best for routines that make many method calls on this handle.
**/
template<class ElementRep, class ElementRef>
class QcNullableElement
{
public:
	friend class QcFloat;
	friend class QcConstraint;

	QcNullableElement()
		: fRep(0)
	{
		reportConstruction();
	}

	explicit QcNullableElement(ElementRep *rep)
		: fRep(rep)
	{
		reportConstruction();
		Increase();
	}

	QcNullableElement(QcNullableElement const &other)
		: fRep(other.fRep)
	{
		reportConstruction();
		Increase();
		assertInvar();
	}

	explicit QcNullableElement(ElementRef const &other)
		: fRep(other.fRep)
	{
		reportConstruction();
		Increase();
		assertInvar();
	}

	~QcNullableElement()
		{ Decrease(); }

	void assertInvar() const
	{
#if qcCheckInternalInvar
		if (fRep != 0)
			fRep->assertInvar();
#endif
	}

	ElementRep *pointer() const
		{ return fRep; }

	int getId() const
		{ return fRep->Id(); }

	QcNullableElement &operator=(const QcNullableElement<ElementRep, ElementRef> &other);

	QcNullableElement &operator=(const ElementRef &other);

	bool operator <(const QcNullableElement<ElementRep, ElementRef> &other) const
		{ return *fRep < *other.fRep; }

	/* [pjm]: I'm not too sure why `<' compares fId's whereas `==' and `!='
	   compare pointers.  See the comment in QcFloat.hh. */
	bool operator ==(const QcNullableElement<ElementRep, ElementRef> &other) const
		{ return fRep == other.fRep; }

	bool operator !=(const QcNullableElement<ElementRep, ElementRef> &other) const
		{ return fRep != other.fRep; }

	bool isDead() const
		{ return (fRep == 0); }

	bool notDead() const
		{ return (fRep != 0); }

	char const *Name() const
		{ return fRep->Name(); }

	/** Called when number of references decreases. */
	void Decrease();

private:
	/** Call when number of references increases. */
	void Increase()
	{
		reportIncrease();
		if (fRep != 0)
			fRep->Increase();
	}

	ElementRep *fRep;
};

template <class ElementRep, class ElementRef>
inline void QcNullableElement<ElementRep, ElementRef>::Decrease()
{
	reportDecrease();
	if (fRep == 0)
		return;
	fRep->Decrease();
	if (fRep->Counter() == 0) {
		fRep->assertInvar(); // Check that the pointer is valid.
		delete fRep;
	}
	fRep = 0;
}

template <class ElementRep, class ElementRef>
QcNullableElement<ElementRep, ElementRef> &QcNullableElement<ElementRep, ElementRef>::operator=(const QcNullableElement<ElementRep, ElementRef> &other)
{
	other.fRep->Increase();
	Decrease();
	fRep = other.fRep;
	return *this;
}

#if 0
template <class ElementRep, class ElementRef>
QcNullableElement<ElementRep, ElementRef> &QcNullableElement<ElementRep, ElementRef>::operator=(const ElementRef &other)
{
	other.Rep()->Increase();
	Decrease();
	fRep = other.Rep();
	return *this;
}
#endif

#ifndef qcNoStream
template <class ElementRep, class ElementRef>
inline ostream &operator<<(ostream &os, QcNullableElement<ElementRep, ElementRef> const &cf)
{
	reinterpret_cast<ElementRef const &>(cf).Print(os);
	return os;
}
#endif

#endif /* !__QcNullableElementH */
