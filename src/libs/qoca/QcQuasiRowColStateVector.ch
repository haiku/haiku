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

//o[d] #include <iostream.h>
//o[d] #include <qoca/QcQuasiRowStateVector.H>
//o[i] #include <qoca/QcQuasiRowStateVector.hh>
//o[d] #include <qoca/QcQuasiColStateVector.hh>

//o[d] class QcQuasiRowColStateVector
//o[d] {
public:

//-----------------------------------------------------------------------//
// Constructor.                                                          //
//-----------------------------------------------------------------------//
inline
QcQuasiRowColStateVector(QcQuasiRowStateVector *rs,
			 QcQuasiColStateVector *cs)
  : fRowState(rs), fColState(cs)
{
}

//begin o[d]
protected:
/** This constructor should only be used by derived classes.  The derived
    class should initialize all the member data in this class. */
QcQuasiRowColStateVector()
{
}

public:
virtual ~QcQuasiRowColStateVector()
{
  delete fRowState;
  delete fColState;
}
//end o[d]


//-----------------------------------------------------------------------//
// Manipulation functions.                                               //
//-----------------------------------------------------------------------//
inline void
Restart()
{
  fRowState->Resize(0);
  fColState->Resize(0);
}

inline void
Reserve(unsigned rows, unsigned cols)
{
  fRowState->Reserve(rows);
  fColState->Reserve(cols);
}

#if 0 /* unused */
//cf
virtual void
SwapRows(unsigned c1, unsigned c2)
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
inline void
Print(ostream &os)
{
  fRowState->Print(os);
  fColState->Print(os);
}

//begin o[d]
public:
  QcQuasiRowStateVector *fRowState;
  QcQuasiColStateVector *fColState;
//end o[d]

//o[d] };

/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/

