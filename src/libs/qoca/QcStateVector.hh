// $Id: QcStateVector.hh,v 1.6 2000/11/29 01:58:42 pmoulder Exp $

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

#ifndef __QcStateVectorH
#define __QcStateVectorH

#include <iostream>
using namespace std;
#include "qoca/QcDefines.hh"
#include "qoca/QcState.hh"

/** Much like an STL vector, but with infrastructure for proper
    initialization &amp; cleanup of elements. */
class QcStateVector 
{
public:
	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
	QcStateVector();
	virtual ~QcStateVector();

#ifndef NDEBUG
	void assertInvar()
	{
		assert (fSize <= fAllocCount);
		assert (fAllocCount <= fCapacity);
	}

  virtual void virtualAssertLinkageInvar() const = 0;
#endif

	//-----------------------------------------------------------------------//
	// Query functions.                                                      //
	//-----------------------------------------------------------------------//
 	unsigned GetSize() const
		{ return fSize; }

	/** @precondition i &lt; GetSize().
	    @postcondition ret != NULL
	**/
  	QcState *GetState(unsigned i)
  	{
		qcAssertPre (i < fSize);
		QcState *ret = fStates[i];
		qcAssertPost (ret != 0);
		return ret;
	}

	//-----------------------------------------------------------------------//
	// Manipulation functions.                                               //
	//-----------------------------------------------------------------------//
	virtual void FixLinkage() = 0;

	void Reserve(unsigned size);

	void Resize(unsigned size);

	virtual void Alloc(QcState **start, QcState **finish) = 0;
	virtual void AddToList(QcState **start, QcState **finish) = 0;
	virtual void RemoveFromList(QcState **start, QcState **finish) = 0;

	virtual void Print(ostream &os) const;

protected:
  QcState * const * const getAllocEnd() const
  {
    return fStates + fAllocCount;
  }

protected:
	unsigned fSize;
	unsigned fCapacity;
	QcState **fStates;

private:
	unsigned fAllocCount;
};

inline QcStateVector::QcStateVector()
		: fSize(0), fCapacity(0), fStates(0), fAllocCount(0)
{
}

inline QcStateVector::~QcStateVector()
{
	for (unsigned i = 0; i < fAllocCount; i++)
		delete fStates[i];
	delete [] fStates;
}

inline void QcStateVector::Print(ostream &os) const
{
	for (unsigned i = 0; i < fSize; i++) {
		fStates[i]->Print(os);
		os << endl;
	}
}

inline void QcStateVector::Reserve(unsigned cap)
{
	if (cap <= fCapacity)
		return;

	QcState **oldStates = fStates;
	fStates = new QcState*[cap];

	memcpy(fStates, oldStates, fAllocCount * sizeof(QcState *));

	fCapacity = cap;

	dbg(assertInvar());
}

#ifndef MAX
# define MAX(_a, _b) ((_a) > (_b) ? (_a) : (_b))
#endif

inline void QcStateVector::Resize(unsigned s)
{
  qcAssertPre( s <= fCapacity);
  /* TODO: Remove either the above or the handling of
     the fCapacity < s case below. */

  if (s == fSize)
    return;

  if (s < fSize)
    {
      RemoveFromList( fStates + s,
		      fStates + fSize);
    }
  else
    {
      if (fAllocCount < s)
	{
	  if (fCapacity < s)
	    Reserve( MAX( s, fCapacity * 2));
	  Alloc( fStates + fAllocCount,
		 fStates + s);
	  fAllocCount = s;
	}
      AddToList( fStates + fSize,
		 fStates + s);
    }

  fSize = s;

  dbg(assertInvar());
  dbg(virtualAssertLinkageInvar());
}

#endif
