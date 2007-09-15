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

//o[d] #include <qoca/QcLinEqColStateVector.H>
//o[i] #include <qoca/QcLinEqColStateVector.hh>
#include <qoca/QcLinInEqColState.hh>

//o[d] class QcLinInEqColStateVector
//o[d]  : public QcLinEqColStateVector
//o[d] {
//o[d] public:

//begin o[d]
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

//end o[d]

//-----------------------------------------------------------------------//
// Setter functions
//-----------------------------------------------------------------------//

//cf
void
SetCondition(unsigned i, int c)
{
  qcAssertPre( i < fSize);
  QcLinInEqColState *state = getState( i);
  if ((c ^ state->getCondition())
      & QcLinInEqColState::fStructural)
    {
      if (c & QcLinInEqColState::fStructural)
	LinkStruct( state);
      else
	UnlinkStruct( state);
    }
  state->setCondition( c);
}

inline void
SetObjValue(unsigned i, numT v)
{
  qcAssertPre (i < fSize);
  ((QcLinInEqColState *)fStates[i])->fObj = v;
}

inline void
SetConstrained(unsigned i, bool c)
{
  qcAssertPre (i < fSize);
  ((QcLinInEqColState *)fStates[i])->fIsConstrained = c;
}


#ifndef NDEBUG
//cf
void
assertLinkageInvar() const
{
  unsigned nStructural = 0;
  for (QcState **i = fStates + fSize; i-- != fStates;)
    {
      QcLinInEqColState const *state = CAST(QcLinInEqColState const *, *i);
      if (state->getCondition() & QcLinInEqColState::fStructural)
	nStructural++;
      else
	{
	  assert( state->fPrevStruct == 0);
	  assert( state->fNextStruct == 0);
	}
    }

  assert( (fStructVarList == 0)
	  || (fStructVarList->fPrevStruct == 0));
  for (QcLinInEqColState *curr = fStructVarList;
       curr != 0;
       curr = curr->fNextStruct)
    {
      assert( curr->getCondition() & QcLinInEqColState::fStructural);
      assert( (curr->fNextStruct == 0)
	      || (curr->fNextStruct->fPrevStruct == curr));
      assert( nStructural != 0);
      nStructural--;
    }
  assert( nStructural == 0);
}

//cf
virtual void
virtualAssertLinkageInvar() const
{
  assertLinkageInvar();
  QcLinEqColStateVector::virtualAssertLinkageInvar();
}
#endif


//cf
virtual void
FixLinkage()
{
  dbg(assertLinkageInvar());

#if 0 /* shouldn't be necessary any more. */
  fStructVarList = 0;

  for (unsigned i = 0; i < fSize; i++)
    {
      QcLinInEqColState *state = (QcLinInEqColState *)fStates[i];
      state->fNextStruct = 0;
      state->fPrevStruct = 0;

      if (state->getCondition() & QcLinInEqColState::fStructural)
	LinkStruct( state);
    }
#endif

  QcLinEqColStateVector::FixLinkage();
}

//o[d] private:

inline void
LinkStruct(QcLinInEqColState *state)
	// Only accesses the current item and those linked to StructVarList.
	// Other items may have invalid NextStruct and PrevStruct pointers.
{
  qcAssertPre( state->fPrevStruct == 0);
  qcAssertPre( state->fNextStruct == 0);
  qcAssertPre( state != fStructVarList);

  if (fStructVarList != 0)
    {
      state->fNextStruct = fStructVarList;
      fStructVarList->fPrevStruct = state;
    }

  fStructVarList = state;
}

inline void
UnlinkStruct(QcLinInEqColState *me)
{
  qcAssertPre( (me->fPrevStruct == 0)
	       == (fStructVarList == me));

  if (me->fPrevStruct == 0)
    fStructVarList = me->fNextStruct;
  else
    me->fPrevStruct->fNextStruct = me->fNextStruct;

  if (me->fNextStruct != 0)
    me->fNextStruct->fPrevStruct = me->fPrevStruct;

  me->fPrevStruct = 0;
  me->fNextStruct = 0;
}

inline void
UnlinkStruct(unsigned i)
{
  qcAssertPre (i < fSize);

  QcLinInEqColState *me = (QcLinInEqColState *)fStates[i];
  UnlinkStruct( me);
}


//o[d] protected:

//cf
virtual void
Alloc(QcState **start, QcState **finish)
{
  qcAssertPre( fStates <= start);
  qcAssertPre( start <= finish);
  qcAssertPre( finish <= fStates + fCapacity);

  for (unsigned i = start - fStates;
       start != finish;
       start++, i++)
    *start = new QcLinInEqColState( i);
}


//cf
virtual void
AddToList(QcState **start, QcState **finish)
{
  qcAssertPre( fStates <= start);
  qcAssertPre( start < finish);
  qcAssertPre( finish <= getAllocEnd());

  QcLinEqColStateVector::AddToList( start, finish);

  for(; start != finish; start++)
    {
      QcLinInEqColState *state = CAST(QcLinInEqColState *, *start);

      assert( (state->fNextStruct == 0)
	      && (state->fPrevStruct == 0)
	      && (state->getCondition() == QcLinInEqColState::fStructural)
	      && (!state->fIsConstrained)
	      && (state->fObj == 0.0));

      LinkStruct( state);
    }
}


//cf
virtual void
RemoveFromList(QcState **start, QcState **finish)
{
  qcAssertPre( fStates <= start);
  qcAssertPre( start <= finish);
  qcAssertPre( finish <= fStates + fSize);

  for (QcState **i = start; i != finish; i++)
    {
      QcLinInEqColState *state = CAST(QcLinInEqColState *, *i);

      if (state->getCondition() & QcLinInEqColState::fStructural)
	UnlinkStruct( i - fStates);
      state->setCondition( QcLinInEqColState::fStructural);
      state->fIsConstrained = false;
      state->fObj = 0.0;
    }

  QcLinEqColStateVector::RemoveFromList( start, finish);
}


private:
inline QcLinInEqColState *
getState(unsigned i) const
{
  qcAssertPre( i < fSize);
  QcLinInEqColState *ret = CAST(QcLinInEqColState *, fStates[i]);
  qcAssertPost( ret != 0);
  return ret;
}


//begin o[d]
public:
  /** head of linked list of structural variables. */
  QcLinInEqColState *fStructVarList;
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
