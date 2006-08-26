// Generated automatically from QcLinInEqColStateVector.ch by /home/pmoulder/usr/local/bin/ch2xx.
#ifndef QcLinInEqColStateVectorDCL
#define QcLinInEqColStateVectorDCL
#line 1 "QcLinInEqColStateVector.ch"
// $Id: QcLinInEqColStateVector.ch,v 1.5 2000/12/13 01:44:58 pmoulder Exp $

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

#include <qoca/QcLinEqColStateVector.H>

#include <qoca/QcLinInEqColState.hh>

class QcLinInEqColStateVector
 : public QcLinEqColStateVector
{
public:


	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
	QcLinInEqColStateVector()
	  : QcLinEqColStateVector(), fStructVarList(0)
	{
	}

	//-----------------------------------------------------------------------//
	// Query functions.                                                      //
	//-----------------------------------------------------------------------//
	int GetCondition(unsigned i) const
	{
		qcAssertPre (i < fSize);
		return ((QcLinInEqColState *)fStates[i])->getCondition();
	}

	numT GetObjValue(unsigned i) const
	{
		qcAssertPre (i < fSize);
		return ((QcLinInEqColState *)fStates[i])->fObj;
	}

	bool IsArtificial(unsigned vi) const
	{
		qcAssertPre (vi < fSize);
		return GetCondition(vi) & QcLinInEqColState::fArtificial;
	}

	bool IsConstrained(unsigned i) const
	{
		qcAssertPre (i < fSize);
		return ((QcLinInEqColState *)fStates[i])->fIsConstrained;
	}

	bool IsDual(unsigned vi) const
		{ return GetCondition(vi) & QcLinInEqColState::fDual; }

	bool IsDesire(unsigned vi) const
		{ return GetCondition(vi) & QcLinInEqColState::fDesire; }

	bool IsError(unsigned vi) const
		{ return GetCondition(vi) & QcLinInEqColState::fError; }

	bool IsSlack(unsigned vi) const
		{ return GetCondition(vi) & QcLinInEqColState::fSlack; }

	bool IsStructural(unsigned vi) const
		{ return GetCondition(vi) & QcLinInEqColState::fStructural; }



//-----------------------------------------------------------------------//
// Setter functions
//-----------------------------------------------------------------------//


void
SetCondition(unsigned i, int c);
#line 110 "QcLinInEqColStateVector.ch"
inline void
SetObjValue(unsigned i, numT v);
#line 117 "QcLinInEqColStateVector.ch"
inline void
SetConstrained(unsigned i, bool c);
#line 125 "QcLinInEqColStateVector.ch"
#ifndef NDEBUG

void
assertLinkageInvar() const;
#line 159 "QcLinInEqColStateVector.ch"
virtual void
virtualAssertLinkageInvar() const;
#line 165 "QcLinInEqColStateVector.ch"
#endif



virtual void
FixLinkage();
#line 191 "QcLinInEqColStateVector.ch"
private:

inline void
LinkStruct(QcLinInEqColState *state);
#line 211 "QcLinInEqColStateVector.ch"
inline void
UnlinkStruct(QcLinInEqColState *me);
#line 229 "QcLinInEqColStateVector.ch"
inline void
UnlinkStruct(unsigned i);
#line 239 "QcLinInEqColStateVector.ch"
protected:


virtual void
Alloc(QcState **start, QcState **finish);
#line 257 "QcLinInEqColStateVector.ch"
virtual void
AddToList(QcState **start, QcState **finish);
#line 282 "QcLinInEqColStateVector.ch"
virtual void
RemoveFromList(QcState **start, QcState **finish);
#line 304 "QcLinInEqColStateVector.ch"
private:
inline QcLinInEqColState *
getState(unsigned i) const;
#line 316 "QcLinInEqColStateVector.ch"
public:
  /** head of linked list of structural variables. */
  QcLinInEqColState *fStructVarList;

};


/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/

#endif /* !QcLinInEqColStateVectorDCL */
