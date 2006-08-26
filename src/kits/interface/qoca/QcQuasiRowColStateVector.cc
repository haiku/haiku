// Generated automatically from QcQuasiRowColStateVector.ch by /home/pmoulder/usr/local/bin/ch2xx.
#include "QcQuasiRowColStateVector.hh"
#line 1 "QcQuasiRowColStateVector.ch"
// $Id: QcQuasiRowColStateVector.ch,v 1.4 2001/01/30 01:32:08 pmoulder Exp $

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










//-----------------------------------------------------------------------//
// Constructor.                                                          //
//-----------------------------------------------------------------------//
#line 63 "QcQuasiRowColStateVector.ch"
//-----------------------------------------------------------------------//
// Manipulation functions.                                               //
//-----------------------------------------------------------------------//
#line 80 "QcQuasiRowColStateVector.ch"
#if 0 /* unused */

void
QcQuasiRowColStateVector::SwapRows(unsigned c1, unsigned c2)
{
  qcAssertPre( c1 < fRowState->getNRows());
  qcAssertPre( c2 < fRowState->getNRows());

  if (c1 == c2)
    return;

  QcQuasiRowState *r1 = (QcQuasiRowState *)fRowState->GetState(c1);
  QcQuasiRowState *r2 = (QcQuasiRowState *)fRowState->GetState(c2);

  // Exchange the data contents (excluding the fIndex field).
  {
    int tmp = r1->fARowIndex;
    r1->fARowIndex = r2->fARowIndex;
    r2->fARowIndex = tmp;
  }

  // Now fix up the linkage
  if (r1->fMRowDeleted != r2->fMRowDeleted)
    {
      if (r2->fMRowDeleted)
	{				// Make r1-> original deleted
	  QcQuasiRowState *tmp = r2;
	  r2 = r1;
	  r1 = tmp;
	}
      assert( !r2->fMRowDeleted);

      if (r2->fPrevRow == 0)
	{
	  assert( fRowState->fRowList == r2);
	  fRowState->fRowList = r1;
	}
      else
	r2->fPrevRow->fNextRow = r1;

      r1->fNextRow = r2->fNextRow;
      r1->fPrevRow = r2->fPrevRow;

      if (r2->fNextRow != 0)
	r2->fNextRow->fPrevRow = r1;

      r2->fNextRow = 0;
      r2->fPrevRow = 0;
      r1->fMRowDeleted = false;
      r2->fMRowDeleted = true;
    }

  // effic: don't do this all the time
  dbg(fRowState->assertLinkageInvar());
}
#endif /* unused */

//-----------------------------------------------------------------------//
// Utility functions.                                                    //
//-----------------------------------------------------------------------//
#line 155 "QcQuasiRowColStateVector.ch"
/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/

