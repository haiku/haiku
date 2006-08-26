// Generated automatically from QcFloatRep.ch by /home/pmoulder/usr/local/bin/ch2xx.
#ifndef QcFloatRepDCL
#define QcFloatRepDCL
#line 1 "QcFloatRep.ch"
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


#define qcFloatMagic1	18732
#define qcFloatMagic2	"Qoca var:"
#define qcMagic2Len	9
	// The length of magic2 without the terminal '\0'



class QcFloatRep
{
public:

/** It is intended that <tt>TId</tt> be signed.  Negative values are not
    allocated and are reserved for future (unknown) usage.  The value 0
    is reserved as an invalid identifier. */
typedef long TId;

public:
	//-----------------------------------------------------------------------//
	// Constructors                                                          //
	//-----------------------------------------------------------------------//

QcFloatRep (const char *name, numT desval, numT sw, numT ew,
	    bool restricted);
#line 74 "QcFloatRep.ch"
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


//-----------------------------------------------------------------------//
// Set and Get member functions                                          //
//-----------------------------------------------------------------------//

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




inline char const *Name() const;
#line 176 "QcFloatRep.ch"
virtual void SetWeight (numT w);
#line 183 "QcFloatRep.ch"
virtual void SetValue (numT v);
#line 194 "QcFloatRep.ch"
/** Set the goal value of this variable to <tt>dv</tt>.

    <p>Note that many solvers ignore SuggestValue on variables that are not edit
    variables.
**/
void
SuggestValue (numT dv);
#line 206 "QcFloatRep.ch"
/** Sets field <tt>fName</tt> to point to a heap-allocated string with a hidden
    magic number field at the front.  (The magic number is qcFloatMagic2 without
    the '\0' terminator.)

    <p>(The Java version doesn't bother with a magic number, presumably because
    of lack of pointers.  The Java version consists of `fName = n'.)
**/
void SetName (char const *n);
#line 248 "QcFloatRep.ch"
inline void SetToGoal();
#line 255 "QcFloatRep.ch"
virtual void SetToEditWeight() = 0;

virtual void SetToStayWeight() = 0;


/** Do RestDesVal, then return true iff the goal value changed. */
virtual bool RestDesVal_changed() = 0;

/** For nomadic variables, set the goal value to equal the actual
    (solved) value; does nothing for "fixed" (non-nomadic) variables.
**/
virtual void RestDesVal() = 0;



/** Called just before delete, to detect pointer corruption. */
virtual void assertInvar() const;

#line 296 "QcFloatRep.ch"
/** Deallocates old name string checking magic2.
    Returns success indicator.
**/
bool FreeName();
#line 321 "QcFloatRep.ch"
/** Set variables to defaults (including getting a new fId). */
inline void Reset();
#line 330 "QcFloatRep.ch"
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



//-----------------------------------------------------------------------//
// Utility functions.                                                    //
//-----------------------------------------------------------------------//


virtual void Print (ostream &os) const;
#line 372 "QcFloatRep.ch"
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

};



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




/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/

#endif /* !QcFloatRepDCL */
