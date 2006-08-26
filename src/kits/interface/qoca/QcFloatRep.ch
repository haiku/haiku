// $Id: QcFloatRep.ch,v 1.13 2001/01/10 05:01:51 pmoulder Exp $

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

#include <string>
#include "qoca/QcUtility.hh"

//begin o[d]
#define qcFloatMagic1	18732
#define qcFloatMagic2	"Qoca var:"
#define qcMagic2Len	9
	// The length of magic2 without the terminal '\0'
//end o[d]


//ho class QcFloatRep
//ho {
//ho public:

/** It is intended that <tt>TId</tt> be signed.  Negative values are not
    allocated and are reserved for future (unknown) usage.  The value 0
    is reserved as an invalid identifier. */
typedef long TId;

//ho public:
	//-----------------------------------------------------------------------//
	// Constructors                                                          //
	//-----------------------------------------------------------------------//
//cf
QcFloatRep (const char *name, numT desval, numT sw, numT ew,
	    bool restricted)
  : fStayWeight( sw),
    fEditWeight( ew),
    fValue( 0), /* perhaps should be desval; but perhaps not too important so
		   long as it is understood that Solve needs to be called. */
    fWeight( sw),
    fDesireValue( desval)
{
  qcAssertExternalPre( sw >= 0);
  qcAssertExternalPre( ew >= 0);
#if qcReportAlloc
  cerr << "initing " << this << endl;
#endif
  fMagic = qcFloatMagic1;
  Reset();
  fRestricted = restricted;
  fName = 0;
  SetName( name);
}

//begin ho
#ifndef NDEBUG
bool isQcFloatRep() const
{ return fMagic == qcFloatMagic1; }
#endif

virtual ~QcFloatRep()
{
#if qcReportAlloc
  cout << "destroying " << this << endl;
#endif
  qcAssertPre( isQcFloatRep());
  qcAssertPre ((fName == 0)
	       || memcmp (fName, qcFloatMagic2,
			  qcMagic2Len) == 0);
  delete [] fName;
  fMagic = 0;
}
//end ho

//-----------------------------------------------------------------------//
// Set and Get member functions                                          //
//-----------------------------------------------------------------------//
//begin o[d]
long Counter() const
{
  qcAssertPre( isQcFloatRep());
  return fCounter;
}

numT EditWeight() const
{
  qcAssertPre( isQcFloatRep());
  return fEditWeight;
}

long Magic() const
{ return fMagic; }

numT StayWeight() const
{
  qcAssertPre( isQcFloatRep());
  return fStayWeight;
}

numT Value() const
{
  qcAssertPre( isQcFloatRep());
  return fValue;
}

numT Weight() const
{
  qcAssertPre( isQcFloatRep());
  return fWeight;
}

numT DesireValue() const
{
  qcAssertPre( isQcFloatRep());
  return fDesireValue;
}

bool IsRestricted() const
{
  qcAssertPre( isQcFloatRep());
  return fRestricted;
}

TId Id() const
{
  qcAssertPre( isQcFloatRep());
  return fId;
}

/** Called when number of references decreases. */
void Decrease()
{
  qcAssertPre( isQcFloatRep());
  fCounter--;
  qcAssertPre (fCounter >= 0);
}

/** Call when number of references increases. */
void Increase()
{
  qcAssertPre( isQcFloatRep());
  fCounter++;
}

//end o[d]

//hf
inline char const *Name() const
{
  qcAssertPre( isQcFloatRep());
  if (fName == 0)
    return 0;
  else
    return (fName + qcMagic2Len);
}

//cf
virtual void SetWeight (numT w)
{
  qcAssertPre( isQcFloatRep());
  fWeight = w;
}

//cf
virtual void SetValue (numT v)
{
  qcAssertPre( isQcFloatRep());
#ifdef qcSafetyChecks
  if (fRestricted && QcUtility::IsNegative(v))
    throw QcWarning("Restricted float made negative");
#endif

  fValue = v;
}

/** Set the goal value of this variable to <tt>dv</tt>.

    <p>Note that many solvers ignore SuggestValue on variables that are not edit
    variables.
**/ //cf
void
SuggestValue (numT dv)
{
  qcAssertPre( isQcFloatRep());
  fDesireValue = dv;
}

/** Sets field <tt>fName</tt> to point to a heap-allocated string with a hidden
    magic number field at the front.  (The magic number is qcFloatMagic2 without
    the '\0' terminator.)

    <p>(The Java version doesn't bother with a magic number, presumably because
    of lack of pointers.  The Java version consists of `fName = n'.)
**/ //cf
void SetName (char const *n)
{
  qcAssertPre( isQcFloatRep());
  /* Note: Although <strings.h> is pretty standard, for portability we try
     to avoid using it here as we have no other need for it. */
  int len = 0;
  char const *magic2 = qcFloatMagic2;

  // find length of source string
  while (n[len] != '\0')
    len++;

  qcAssertPost (n[len] == '\0');
#ifdef qcSafetyChecks
  if (len > 1000)
    throw QcWarning("QcFloatRep::SetName name over 1000 chars long");
#endif

  /* Deallocate existing name string.  If FreeName fails, leave old string
     and allocate a new one. */
  if (!FreeName())
    throw QcWarning("QcFloatRep::SetName: failed to free old name string");

  fName = new char[len + qcMagic2Len + 1]; // allocate for magic+string

  for (int i = 0; i < qcMagic2Len; i++)
    fName[i] = magic2[i];

  for (int i=0; i <= len; i++)
    fName[i + qcMagic2Len] = n[i];

  qcAssertPost (fName[len + qcMagic2Len] == '\0');
}

//hf
inline void SetToGoal()
{
  qcAssertPre( isQcFloatRep());
  SetValue (fDesireValue);
}

//begin o[d]
virtual void SetToEditWeight() = 0;

virtual void SetToStayWeight() = 0;


/** Do RestDesVal, then return true iff the goal value changed. */
virtual bool RestDesVal_changed() = 0;

/** For nomadic variables, set the goal value to equal the actual
    (solved) value; does nothing for "fixed" (non-nomadic) variables.
**/
virtual void RestDesVal() = 0;
//end o[d]


/** Called just before delete, to detect pointer corruption. */
//o[d] virtual void assertInvar() const;
//begin o[i]
#ifndef qcCheckInternalPre
inline void QcFloatRep::assertInvar() const
{ }
#endif
//end o[i]
//begin o[c]
#if qcCheckInternalPre
void QcFloatRep::assertInvar() const
{
  qcAssertInvar (fId > 0);
  qcAssertInvar (Magic() == qcFloatMagic1);
  qcAssertInvar (fCounter >= 0);
  qcAssertInvar (fStayWeight >= 0);
  qcAssertInvar (fEditWeight >= 0);
  qcAssertInvar (fWeight >= 0);
  qcAssertInvar ((fName == 0)
		 || (memcmp (fName, qcFloatMagic2, qcMagic2Len)
		     == 0));
}
#endif
//end o[c]


/** Deallocates old name string checking magic2.
    Returns success indicator.
**/ //cf
bool FreeName()
{
  qcAssertPre( isQcFloatRep());
  bool ok = true;

  if (fName != 0)
    {	// check for magic2
      char const *magic2 = qcFloatMagic2;

      for (int i = 0; i < qcMagic2Len; i++) 
	ok &= (fName[i] == magic2[i]);

      if (ok)
	{
	  delete[] fName;
	  fName = 0;
	}
    }
    
  return ok;
}

/** Set variables to defaults (including getting a new fId). */
inline void Reset()
{
  fCounter = 1;
  fId = fNextValidId++;
  qcAssertPost (fNextValidId > 0);
}

//begin o[d]
bool operator<(const QcFloatRep &other) const
  // The comparisons are for when QcFloatRep is used
  // as an identifier and compares instances not structure.
  // General pointer inequality comparisons are not reliable
  // (see the C++ ARM section 5.9), fId is provided for this.
{
  qcAssertPre( isQcFloatRep());
  qcAssertPre( other.isQcFloatRep());

  return fId < other.fId;
}

#if 0
bool operator==(const QcFloatRep &other) const
{ return fId == other.fId; }

bool operator!=(const QcFloatRep &other) const
{ return fId != other.fId; }
#endif
//end o[d]


//-----------------------------------------------------------------------//
// Utility functions.                                                    //
//-----------------------------------------------------------------------//

//cf
virtual void Print (ostream &os) const
{
  qcAssertPre( isQcFloatRep());

  if (fName != 0)
    os << Name();

  os << "#" << fId
     << "(" << fValue << ", "
     << fDesireValue << ", "
     << fWeight
     << ")";
}

//begin ho
protected:
	static TId fNextValidId;
	static const TId fInvalidId = 0;
		// fInvalidId is currently set to 0. All negative values are reserved
		// for future use.

private:
	TId fId;			// Used as index key and for operator<.
	long fMagic;		// To detect corruption.
	long fCounter;		// Reference count.
	numT fStayWeight;
	numT fEditWeight;
protected:
	numT fValue;
	numT fWeight;
	numT fDesireValue;
private:
	char *fName;		// 0 or a heap allocated string with hidden prefix.
	bool fRestricted;
//end ho
//ho };


//begin ho
struct QcFloatRep_hash
{
  size_t operator()(QcFloatRep const *v) const
  {
    return (size_t) v->Id();
  }
};

struct QcFloatRep_equal
{
  size_t operator()(QcFloatRep const *v1,
		    QcFloatRep const *v2) const
  {
    return v1->Id() == v2->Id();
  }
};
//end ho

//o[c] QcFloatRep::TId QcFloatRep::fNextValidId = 1;

/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/
