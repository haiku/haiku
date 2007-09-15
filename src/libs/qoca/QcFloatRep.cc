// Generated automatically from QcFloatRep.ch by /home/pmoulder/usr/local/bin/ch2xx.
#include "QcFloatRep.hh"
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


#line 48 "QcFloatRep.ch"
	//-----------------------------------------------------------------------//
	// Constructors                                                          //
	//-----------------------------------------------------------------------//

QcFloatRep::QcFloatRep (const char *name, numT desval, numT sw, numT ew,
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


#line 93 "QcFloatRep.ch"
//-----------------------------------------------------------------------//
// Set and Get member functions                                          //
//-----------------------------------------------------------------------//

#line 176 "QcFloatRep.ch"
void QcFloatRep::SetWeight (numT w)
{
  qcAssertPre( isQcFloatRep());
  fWeight = w;
}


void QcFloatRep::SetValue (numT v)
{
  qcAssertPre( isQcFloatRep());
#ifdef qcSafetyChecks
  if (fRestricted && QcUtility::IsNegative(v))
    throw QcWarning("Restricted float made negative");
#endif

  fValue = v;
}


#line 199 "QcFloatRep.ch"
void
QcFloatRep::SuggestValue (numT dv)
{
  qcAssertPre( isQcFloatRep());
  fDesireValue = dv;
}


#line 213 "QcFloatRep.ch"
void QcFloatRep::SetName (char const *n)
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

#line 279 "QcFloatRep.ch"
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




#line 299 "QcFloatRep.ch"
bool QcFloatRep::FreeName()
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


#line 352 "QcFloatRep.ch"
//-----------------------------------------------------------------------//
// Utility functions.                                                    //
//-----------------------------------------------------------------------//


void QcFloatRep::Print (ostream &os) const
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


#line 414 "QcFloatRep.ch"
QcFloatRep::TId QcFloatRep::fNextValidId = 1;

/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/
