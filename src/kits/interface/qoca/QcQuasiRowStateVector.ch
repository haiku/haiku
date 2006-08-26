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

//o[d] class QcQuasiRowStateVector
//o[d]   : public QcStateVector
//o[d] {
//o[d] public:

//begin o[d]

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
//end o[d]

//cf
void
SetMRowDeleted(unsigned i, bool d)
{
  qcAssertPre( i < fSize);
  QcQuasiRowState *state = getState( i);
  if (state->fMRowDeleted != d)
    {
      if (d)
	UnlinkRow( state);
      else
	LinkRow( state);
    }
  state->fMRowDeleted = d;
}


//-----------------------------------------------------------------------//
// Manipulation functions.                                               //
//-----------------------------------------------------------------------//

//o[d] protected:
//cf
virtual void
AddToList(QcState **start, QcState **finish)
{
  qcAssertPre( fStates <= start);
  qcAssertPre( start < finish);
  qcAssertPre( finish <= getAllocEnd());

  for(; start != finish; start++)
    {
      QcQuasiRowState *state = CAST(QcQuasiRowState *, *start);

      assert( (state->fNextRow == 0)
	      && (state->fPrevRow == 0)
	      && (state->fARowIndex == start - fStates)
	      && (!state->fMRowDeleted));

      LinkRow( state);
    }
}

#ifndef NDEBUG
//o[d] public:

//cf
void
assertLinkageInvar() const
{
  unsigned nLive = 0;
  for (QcState const * const *i = fStates + fSize; i-- != fStates;)
    {
      QcQuasiRowState const *state = CAST(QcQuasiRowState const *, *i);
      if (state->fMRowDeleted)
	{
	  assert( state->fPrevRow == 0);
	  assert( state->fNextRow == 0);
	}
      else
	nLive++;
    }

  assert( (fRowList == 0)
	  || (fRowList->fPrevRow == 0));
  for (QcQuasiRowState const *curr = fRowList;
       curr != 0;
       curr = curr->fNextRow)
    {
      assert( !curr->fMRowDeleted);
      assert( (curr->fNextRow == 0)
	      || (curr->fNextRow->fPrevRow == curr));
      assert( nLive != 0);
      nLive--;
    }
  assert( nLive == 0);
}

//o[d] protected:

//cf
virtual void
virtualAssertLinkageInvar() const
{
  assertLinkageInvar();
}
#endif

//o[d] public:

//cf
virtual void
FixLinkage()
{
  dbg(assertLinkageInvar());

#if 0 /* should not be necessary */
  fRowList = 0;

  for (unsigned i = 0; i < fSize; i++)
    {
      QcQuasiRowState *state = (QcQuasiRowState *)fStates[i];
      state->fNextRow = 0;
      state->fPrevRow = 0;

      if (!state->fMRowDeleted)
	LinkRow( state);
    }
#endif
}


//o[d] private:

inline void
LinkRow(unsigned index)
{
  qcAssertPre (index < fSize);
  QcQuasiRowState *me = (QcQuasiRowState *)fStates[index];
  LinkRow( me);
}

inline void
LinkRow(QcQuasiRowState *me)
{
  qcAssertPre( me->fPrevRow == 0);
  qcAssertPre( me->fNextRow == 0);
  qcAssertPre( me != fRowList);

  if (fRowList != 0)
    {
      me->fNextRow = fRowList;
      fRowList->fPrevRow = me;
    }

  fRowList = me;
}


inline void
UnlinkRow(unsigned index)
{
  qcAssertPre (index < fSize);

  QcQuasiRowState *me = (QcQuasiRowState *)fStates[index];
  UnlinkRow( me);
}

inline void
UnlinkRow(QcQuasiRowState *me)
{
  qcAssertPre( (me->fPrevRow == 0)
	       == (fRowList == me));

  if (me->fPrevRow == 0)
    fRowList = me->fNextRow;
  else
    me->fPrevRow->fNextRow = me->fNextRow;

  if (me->fNextRow != 0)
    me->fNextRow->fPrevRow = me->fPrevRow;

  me->fPrevRow = 0;
  me->fNextRow = 0;
}


//o[d] protected:
inline void
Restart(unsigned i)
{
  qcAssertPre (i < fSize);

  QcQuasiRowState *state = (QcQuasiRowState *)fStates[i];

  if (!state->fMRowDeleted)
    UnlinkRow(i);

  state->fARowIndex = (int) i;
  state->fMRowDeleted = false;
}


//-----------------------------------------------------------------------//
// Utility functions.                                                    //
//-----------------------------------------------------------------------//
//o[d] public:
//cf
virtual void
Print(ostream &os) const
{
  os << endl << "Row State Vector:";
  QcStateVector::Print(os);
  os << endl << "Row List:";
  QcQuasiRowState *curr = fRowList;

  while (curr != 0)
    {
      curr->Print(os);
      os << endl;
      curr = curr->fNextRow;
    }
}

//o[d] private:
inline QcQuasiRowState *
getState( unsigned i) const
{
  qcAssertPre( i < fSize);
  QcQuasiRowState *ret = CAST(QcQuasiRowState *, fStates[i]);
  qcAssertPost( ret != 0);
  return ret;
}


//begin o[d]
public:
	QcQuasiRowState *fRowList;	// head of undeleted fM rows linked list
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
