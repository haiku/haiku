// Generated automatically from QcQuasiRowStateVector.ch by /home/pmoulder/usr/local/bin/ch2xx.
#ifndef QcQuasiRowStateVectorDCL
#define QcQuasiRowStateVectorDCL
#line 1 "QcQuasiRowStateVector.ch"
// $Id: QcQuasiRowStateVector.ch,v 1.1 2000/11/29 01:49:41 pmoulder Exp $

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

#include "qoca/QcStateVector.hh"
#include "qoca/QcQuasiRowState.hh"

class QcQuasiRowStateVector
  : public QcStateVector
{
public:



	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
	QcQuasiRowStateVector()
	  : QcStateVector(),
	    fRowList( 0)
	{
	}

	//-----------------------------------------------------------------------//
	// Query functions.                                                      //
	//-----------------------------------------------------------------------//

	unsigned getNRows() const
	{
	  return fSize;
	}

	int GetARowIndex(unsigned i) const
	{
		qcAssertPre (i < fSize);
		return ((QcQuasiRowState *)fStates[i])->fARowIndex;
	}

	bool GetMRowDeleted(unsigned i) const
	{
		qcAssertPre (i < fSize);
		return ((QcQuasiRowState *)fStates[i])->fMRowDeleted;
	}

	//-----------------------------------------------------------------------//
	// Set functions.                                                        //
	//-----------------------------------------------------------------------//
	void SetARowIndex(unsigned i, int ri)
	{
		qcAssertPre (i < fSize);
		((QcQuasiRowState *)fStates[i])->fARowIndex = ri;
	}



void
SetMRowDeleted(unsigned i, bool d);
#line 94 "QcQuasiRowStateVector.ch"
//-----------------------------------------------------------------------//
// Manipulation functions.                                               //
//-----------------------------------------------------------------------//

protected:

virtual void
AddToList(QcState **start, QcState **finish);
#line 120 "QcQuasiRowStateVector.ch"
#ifndef NDEBUG
public:


void
assertLinkageInvar() const;
#line 155 "QcQuasiRowStateVector.ch"
protected:


virtual void
virtualAssertLinkageInvar() const;
#line 163 "QcQuasiRowStateVector.ch"
#endif

public:


virtual void
FixLinkage();
#line 189 "QcQuasiRowStateVector.ch"
private:

inline void
LinkRow(unsigned index);
#line 199 "QcQuasiRowStateVector.ch"
inline void
LinkRow(QcQuasiRowState *me);
#line 216 "QcQuasiRowStateVector.ch"
inline void
UnlinkRow(unsigned index);
#line 225 "QcQuasiRowStateVector.ch"
inline void
UnlinkRow(QcQuasiRowState *me);
#line 244 "QcQuasiRowStateVector.ch"
protected:
inline void
Restart(unsigned i);
#line 260 "QcQuasiRowStateVector.ch"
//-----------------------------------------------------------------------//
// Utility functions.                                                    //
//-----------------------------------------------------------------------//
public:

virtual void
Print(ostream &os) const;
#line 281 "QcQuasiRowStateVector.ch"
private:
inline QcQuasiRowState *
getState( unsigned i) const;
#line 293 "QcQuasiRowStateVector.ch"
public:
	QcQuasiRowState *fRowList;	// head of undeleted fM rows linked list

};


/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/

#endif /* !QcQuasiRowStateVectorDCL */
