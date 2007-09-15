// Generated automatically from QcLinInEqRowColStateVector.ch by /home/pmoulder/usr/local/bin/ch2xx.
#include "QcLinInEqRowColStateVector.hh"
#line 1 "QcLinInEqRowColStateVector.ch"
// $Id: QcLinInEqRowColStateVector.ch,v 1.1 2000/11/29 04:31:16 pmoulder Exp $

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

#include "qoca/QcLinEqRowColStateVector.hh"








#line 46 "QcLinInEqRowColStateVector.ch"
//-----------------------------------------------------------------------//
// Manipulation functions.                                               //
//-----------------------------------------------------------------------//

void
QcLinInEqRowColStateVector::SwapColumns(unsigned v1, unsigned v2)
{
  qcAssertPre( v1 < fColState->getNColumns());
  qcAssertPre( v2 < fColState->getNColumns());

  if (v1 == v2)
    return;

  QcLinInEqColState *r1 = (QcLinInEqColState *)fColState->GetState(v1);
  QcLinInEqColState *r2 = (QcLinInEqColState *)fColState->GetState(v2);

  /* TODO: find out why we swap fIsConstrained and fCondition only if
     at least one of v1,v2 is structural. */

  // Exchange the data contents. First fix up the linkage
  if ((r1->getCondition() ^ r2->getCondition())
      & QcLinInEqColState::fStructural)
    {
      if (r2->getCondition() & QcLinInEqColState::fStructural) {
	// Make r1 original structural
	QcLinInEqColState *temp = r2;
	r2 = r1;
	r1 = temp;
      }

      if (r1->fPrevStruct == 0)
	{
	  assert( fColState->fStructVarList == r1);
	  fColState->fStructVarList = r2;
	}
      else
	r1->fPrevStruct->fNextStruct = r2;

      r2->fNextStruct = r1->fNextStruct;
      r2->fPrevStruct = r1->fPrevStruct;

      if (r1->fNextStruct != 0)
	r1->fNextStruct->fPrevStruct = r2;

      r1->fNextStruct = 0;
      r1->fPrevStruct = 0;

      {
	int r1Cond = r1->fCondition;
	r1->fCondition = r2->fCondition;
	r2->fCondition = r1Cond;
      }

      {
	bool r1IsConst = r1->fIsConstrained;
	r1->fIsConstrained = r2->fIsConstrained;
	r2->fIsConstrained = r1IsConst;
      }
    }
  else if ((r1->fCondition & r2->fCondition)
	   & QcLinInEqColState::fStructural)
    {
      {
	int r1Cond = r1->fCondition;
	r1->fCondition = r2->fCondition;
	r2->fCondition = r1Cond;
      }
      {
	bool r1IsConst = r1->fIsConstrained;
	r1->fIsConstrained = r2->fIsConstrained;
	r2->fIsConstrained = r1IsConst;
      }
    }
  else
    {
      static bool foundSame = false;
      if (!foundSame
	  && (r1->fCondition == r2->fCondition)
	  && (r1->fIsConstrained == r2->fIsConstrained))
	{
	  cerr << __FILE__ ":" << __LINE__ << ": found same.\n";
	  foundSame = true;
	}
      else
	{
	  cerr << __FILE__ ":" << __LINE__ << ": Note: Found difference.\n";
	}
    }

  //effic: Don't do this each time.
  dbg(fColState->assertLinkageInvar());

  QcLinEqRowColStateVector::SwapColumns(v1, v2);
}


#line 147 "QcLinInEqRowColStateVector.ch"
/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/
