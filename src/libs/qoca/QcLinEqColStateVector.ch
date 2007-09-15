// $Id: QcLinEqColStateVector.ch,v 1.2 2000/12/06 05:32:56 pmoulder Exp $

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

#include "qoca/QcQuasiColStateVector.hh"
#include "qoca/QcLinEqColState.hh"

//o[d] class QcLinEqColStateVector
//o[d]   : public QcQuasiColStateVector
//o[d] {
//o[d] public:

//begin o[d]

//-----------------------------------------------------------------------//
// Constructor.                                                          //
//-----------------------------------------------------------------------//

QcLinEqColStateVector()
  : QcQuasiColStateVector(),
    fParamHead( false),
    fBasicHead( true)
{
}
//end o[d]

#ifndef NDEBUG
//o[d] private:

//cf
void
assertLinkageInvar() const
{
  unsigned nParams = 0, nBasic = 0;

  assert( fParamHead.getIndex() == QcLinEqColState::fHeadIndex);
  assert( fBasicHead.getIndex() == QcLinEqColState::fHeadIndex);

  {
    QcState const * const *i, * const *end = fStates + fSize;

    for (i = getAllocEnd();
	 i-- != end;)
      {
	QcLinEqColState const *curr = CAST(QcLinEqColState const *, *i);
	assert( curr->fNextCol == 0);
	assert( curr->fPrevCol == 0);
      }
    assert( i + 1 == end);

    for (;i >= fStates; i--)
      {
	QcLinEqColState const *curr = CAST(QcLinEqColState const *, *i);
	if (curr->isBasic())
	  nBasic++;
	else
	  nParams++;
	assert( curr->fNextCol != 0);
	assert( curr->fNextCol->fPrevCol == curr);
      }
  }
  assert( nBasic + nParams == fSize);

  assert( fParamHead.fNextCol != 0);
  assert( fParamHead.fNextCol->fPrevCol == &fParamHead);
  for (QcLinEqColState *curr = fParamHead.fNextCol;
       curr != &fParamHead;
       curr = curr->fNextCol)
    {
      assert( !curr->isBasic());
      assert( curr->fIndex < (int) fSize);
      assert( nParams != 0);
      nParams--;
    }
  assert( nParams == 0);

  assert( fBasicHead.fNextCol != 0);
  assert( fBasicHead.fNextCol->fPrevCol == &fBasicHead);
  for (QcLinEqColState *curr = fBasicHead.fNextCol;
       curr != &fBasicHead;
       curr = curr->fNextCol)
    {
      assert( curr->isBasic());
      assert( curr->getIndex() < fSize);
      assert( nBasic != 0);
      nBasic--;
    }
  assert( nBasic == 0);
}

//o[d] protected:

//cf
virtual void
virtualAssertLinkageInvar() const
{
  assertLinkageInvar();
}
#endif /* !NDEBUG */


//-----------------------------------------------------------------------//
// Query functions.                                                      //
//-----------------------------------------------------------------------//
//o[d] public:

inline numT
GetDesireValue(unsigned i) const
{
  qcAssertPre (i < fSize);
  return ((QcLinEqColState *)fStates[i])->fDesValue;
}

inline bool
IsBasic(unsigned i) const
{
  qcAssertPre (i < fSize);
  return ((QcLinEqColState *)fStates[i])->fIsBasic;
}

inline int
IsBasicIn(unsigned i) const
{
  qcAssertPre (i < fSize);
  return ((QcLinEqColState *)fStates[i])->fIsBasicIn;
}


//-----------------------------------------------------------------------//
// Set functions.                                                        //
//-----------------------------------------------------------------------//
inline void
SetDesireValue(unsigned i, numT dv) const
{
  qcAssertPre (i < fSize);
  ((QcLinEqColState *)fStates[i])->fDesValue = dv;
}

inline QcLinEqColState *
head( bool isbasic)
{
  QcLinEqColState *heads = &fParamHead;
  assert( &heads[1] == &fBasicHead);
  unsigned ix = isbasic;
  qcAssertPre( ix <= 1);
  return &heads[ix];
}

//cf
void
SetBasic(unsigned i, bool b)
{
  qcAssertPre( i < fSize);
  qcAssertPre( (unsigned) b <= 1);

  QcLinEqColState *state = getState( i);
  if (state->fIsBasic != b)
    {
      state->fIsBasic = b;
      Unlink( state);
      Link( state, head( b));
    }
}

inline void
SetBasicIn(unsigned i, int bi) const
{
  qcAssertPre (i < fSize);
  ((QcLinEqColState *)fStates[i])->fIsBasicIn = bi;
}


//-----------------------------------------------------------------------//
// Manipulation functions.                                               //
//-----------------------------------------------------------------------//
//cf
virtual void
FixLinkage()
{
  dbg(assertLinkageInvar());

#if 0 /* shouldn't be necessary any more. */
  fBasicHead->fPrevCol = fBasicHead->fNextCol = &fBasicHead;
  fParamHead->fPrevCol = fParamHead->fNextCol = &fParamHead;
  QcLinEqColState heads[] = {fParamHead, fBasicHead};

  for (unsigned i = 0; i < fSize; i++)
    {
      QcLinEqColState *state = getState( i);

      unsigned ix = state->isBasic();
      assert (ix <= 1);
      Link( state, heads[ix]);
    }

  dbg(assertLinkageInvar());
#endif
}


inline void
Link(QcLinEqColState *state, QcLinEqColState *head)
{
  qcAssertPre( (head == &fParamHead)
	       || (head == &fBasicHead));
  qcAssertPre( (state->fNextCol == 0)
	       && (state->fPrevCol == 0));

  state->fPrevCol = head;
  head->fNextCol->fPrevCol = state;
  state->fNextCol = head->fNextCol;
  head->fNextCol = state;
}


inline void
Unlink(QcLinEqColState *state)
{
  qcAssertPre( (state->fNextCol != 0)
	       && (state->fPrevCol != 0));

  state->fNextCol->fPrevCol = state->fPrevCol;
  state->fPrevCol->fNextCol = state->fNextCol;

  dbg(state->fPrevCol = state->fNextCol = 0);
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
    *start = new QcLinEqColState( (int) i);
}


//cf
virtual void
AddToList(QcState **start, QcState **finish)
{
  qcAssertPre( fStates <= start);
  qcAssertPre( start < finish);
  qcAssertPre( finish <= getAllocEnd());

  for(; start != finish; start++)
    {
      QcLinEqColState *state = CAST(QcLinEqColState *, *start);

      assert( (state->fNextCol == 0)
	      && (state->fPrevCol == 0)
	      && (state->fDesValue == 0.0)
	      && (state->fIsBasic == false)
	      && (state->fIsBasicIn == QcTableau::fInvalidConstraintIndex));

      Link( state, &fParamHead);
    }
}


//cf
virtual void
RemoveFromList(QcState **start, QcState **finish)
{
  qcAssertPre( fStates <= start);
  qcAssertPre( start <= finish);
  qcAssertPre( finish <= fStates + fSize);

  for(; start != finish; start++)
    {
      QcLinEqColState *state = CAST(QcLinEqColState *, *start);

      Unlink( state);
      state->fDesValue = 0.0f;
      state->fIsBasic = false;
      state->fIsBasicIn = QcTableau::fInvalidConstraintIndex;
    }
}

//o[d] public:

//cf
virtual void
Print(ostream &os) const
{
  QcQuasiColStateVector::Print(os);
  os << endl << "Basic variable list:" << endl;
  QcLinEqColState *curr = fBasicHead.fNextCol;

  while (curr != &fBasicHead)
    {
      curr->Print(os);
      os << endl;
      curr = curr->fNextCol;
    }

  os << endl << "Param variable list:" << endl;
  curr = fParamHead.fNextCol;

  while (curr != &fBasicHead)
    {
      curr->Print(os);
      os << endl;
      curr = curr->fNextCol;
    }
}


private:
inline QcLinEqColState *
getState(unsigned i) const
{
  qcAssertPre( i < fSize);
  QcLinEqColState *ret = CAST(QcLinEqColState *, fStates[i]);
  qcAssertPost( ret != 0);
  return ret;
}


//begin o[d]
public:
  QcLinEqColState fParamHead; // head of linked list of non-basic variables.
  QcLinEqColState fBasicHead; // head of linked list of basic variables.
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
