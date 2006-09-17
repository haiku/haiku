// $Id: QcOrigRowStateVector.ch,v 1.2 2000/12/06 05:32:56 pmoulder Exp $
// Originally written by Alan Finlay and Sitt Sen Chok

//%BeginLegal ================================================================
//									      
// The QOCA implementation is free software, but it is Copyright (C)          
// 1994-1999 Monash University.  It is distributed under the terms of the GNU 
// General Public License.  See the file COPYING for copying permission.      
//                                                                            
// The QOCA toolkit and runtime are distributed under the terms of the GNU    
// Library General Public License.  See the file COPYING.LIB for copying      
// permissions for those files.                                               
//                                                                            
// If those licencing arrangements are not satisfactory, please contact us!   
// We are willing to offer alternative arrangements, if the need should arise.
//                                                                            
// THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED OR  
// IMPLIED.  ANY USE IS AT YOUR OWN RISK.                                     
//                                                                            
// Permission is hereby granted to use or copy this program for any purpose,  
// provided the above notices are retained on all copies.  Permission to      
// modify the code and to distribute modified code is granted, provided the   
// above notices are retained, and a notice that the code was modified is     
// included with the above copyright notice.                                  
//
//%EndLegal ==================================================================

#include <qoca/QcDefines.hh>
#include "qoca/QcStateVector.hh"
#include "qoca/QcOrigRowState.hh"

//o[d] class QcOrigRowStateVector
//o[d]   : public QcStateVector
//o[d] {
//o[d] public:

//hf
inline
QcOrigRowStateVector()
  : QcStateVector()
{
}

//-----------------------------------------------------------------------//
// Query functions.                                                      //
//-----------------------------------------------------------------------//

inline bool
GetARowDeleted(unsigned i) const
{
  qcAssertPre (i < fSize);
  return getState( i)->fARowDeleted;
}

inline int
GetMRowIndex(unsigned i) const
{
  qcAssertPre (i < fSize);
  return getState( i)->fMRowIndex;
}


inline numT
GetRHS(unsigned i) const
{
  qcAssertPre (i < fSize);
  return getState( i)->fRHS;
}


//-----------------------------------------------------------------------//
// Set* functions.
//-----------------------------------------------------------------------//
inline void
SetARowDeleted(unsigned i, bool d)
{
  qcAssertPre (i < fSize);		
  getState( i)->fARowDeleted = d;
}

inline void
SetMRowIndex(unsigned i, int ri)
{
  qcAssertPre (i < fSize);		
  getState( i)->fMRowIndex = ri;
}

inline void
SetRHS(unsigned i, numT rhs)
{
  qcAssertPre (i < fSize);		
  getState( i)->fRHS = rhs;
}

//-----------------------------------------------------------------------//
// Manipulation functions.                                               //
//-----------------------------------------------------------------------//
//cf
virtual void
FixLinkage()
{
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
    *start = new QcOrigRowState( i);
}


//cf
virtual void
AddToList(QcState **start, QcState **finish)
{
  qcAssertPre( fStates <= start);
  qcAssertPre( start <= finish);
  qcAssertPre( finish <= getAllocEnd());

  UNUSED(start);
  UNUSED(finish);
}


//cf
virtual void
RemoveFromList(QcState **start, QcState **finish)
{
  qcAssertPre( fStates <= start);
  qcAssertPre( start <= finish);
  qcAssertPre( finish <= fStates + fSize);

  while(start != finish)
    {
      QcOrigRowState *state = CAST(QcOrigRowState *, *start);
      state->fRHS = 0.0;
      state->fMRowIndex = start - fStates;
      state->fARowDeleted = false;
      start++;
    }
}

#ifndef NDEBUG
//cf
virtual void
virtualAssertLinkageInvar() const
{
  for (QcState const * const *i = fStates + fSize; i < getAllocEnd(); i++)
    {
      QcOrigRowState const *state = CAST(QcOrigRowState const *, *i);
      assert( state->fRHS == 0.0);
      assert( state->fMRowIndex == (char *)i - (char *)fStates);
      assert( !state->fARowDeleted);
    }
}
#endif

//o[d] public:

//hf
virtual inline void
Print(ostream &os) const
{
  os << endl << "Original Row State Vector:";
  QcStateVector::Print( os);
}


//o[d] private:
inline QcOrigRowState *
getState( unsigned i) const
{
  qcAssertPre( i < fSize);
  QcOrigRowState *ret = CAST(QcOrigRowState *, fStates[i]);
  qcAssertPost( ret != 0);
  return ret;
}

//o[d] };


/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/
