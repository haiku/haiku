// Generated automatically from QcVarStow.ch by /home/pmoulder/usr/local/bin/ch2xx.
#include "QcVarStow.hh"
#line 1 "QcVarStow.ch"

#ifdef QC_USING_NLPSOLVER


#line 10 "QcVarStow.ch"
#include <qoca/QcFloatRep.hh>







QcVarStow::QcVarStow()
  : fConstrCacheValid( false),
    fSolvedVals(),
    fVars(),
    fVar2Ix()
{
}


#ifndef NDEBUG

void
QcVarStow::assertInvar() const
{
  unsigned n = size();
  assert( fSolvedVals.size() == n);
  assert( fVars.size() == n);
  assert( fVar2Ix.size() == n);
  assert( fCounts.size() == n);
  /* TODO: assert that fVars is consistent with fVar2Ix. */
}
#endif

#line 56 "QcVarStow.ch"
void
QcVarStow::ensurePresent(QcFloatRep *v)
{
  qcAssertPre( v != 0);

  pair<QcFloatRep *, unsigned> ins( v, fVars.size());
  pair<Var2Ix_T::iterator, bool> res = fVar2Ix.insert( ins);
  if (!res.second)
    return; // already present

  fVars.push_back( v);
  fSolvedVals.push_back( dbl_val( v->Value()));
  fCounts.push_back( 0);

  dbg(assertInvar());
}

#line 82 "QcVarStow.ch"
void
QcVarStow::addReference(QcFloatRep *v)
{
  qcAssertPre( v != 0);

  pair<QcFloatRep *, unsigned> ins( v, fVars.size());
  pair<Var2Ix_T::iterator, bool> res = fVar2Ix.insert( ins);
  if (res.second)
    {
      fVars.push_back( v);
      fSolvedVals.push_back( dbl_val( v->Value()));
      fCounts.push_back( 1);
    }
  else
    fCounts[res.first->second]++;

  dbg(assertInvar());
  qcAssertPost( presentAndReferenced( v));
}


#line 107 "QcVarStow.ch"
bool
QcVarStow::tryRemove(QcFloatRep *v)
{
  qcAssertPre( v != 0);

  Var2Ix_T::iterator v_i = fVar2Ix.find( v);
  if (v_i == fVar2Ix.end())
    return false;
  unsigned v_ix = v_i->second;
  assert( v_ix < fVars.size());
  if( fCounts[v_ix] != 0)
    return false;

  unsigned e_ix = fVars.size() - 1;
  if (v_ix != e_ix)
    {
      fVars[v_ix] = fVars[e_ix];
      {
	Var2Ix_T::iterator e_i = fVar2Ix.find( fVars[e_ix]);
	assert( e_i != fVar2Ix.end());
	assert( e_i->second == e_ix);
	e_i->second = v_ix;
      }
      fSolvedVals[v_ix] = fSolvedVals[e_ix];
      fCounts[v_ix] = fCounts[e_ix];
      if (fCounts[e_ix] != 0)
	fConstrCacheValid = false;
    }
  fVars.pop_back();
  fSolvedVals.pop_back();
  fCounts.pop_back();
  fVar2Ix.erase( v_i);

  dbg(assertInvar());
  return true;
}


void
QcVarStow::incUsage( QcFloatRep *v)
{
  Var2Ix_T::const_iterator i = fVar2Ix.find( v);
  qcAssertPre( i != fVar2Ix.end());
  unsigned vi = i->second;
  assert( vi < fCounts.size());
  fCounts[vi]++;
  qcAssertPost( fCounts[vi] != 0);
}


void
QcVarStow::decUsage( QcFloatRep *v)
{
  Var2Ix_T::const_iterator i = fVar2Ix.find( v);
  qcAssertPre( i != fVar2Ix.end());
  unsigned vi = i->second;
  assert( vi < fCounts.size());
  qcAssertPre( fCounts[vi] != 0);
  fCounts[vi]--;
}

#line 222 "QcVarStow.ch"
bool
QcVarStow::isPresent( QcFloatRep *v) const
{
  Var2Ix_T::const_iterator i = fVar2Ix.find( v);
  return (i != fVar2Ix.end());
}



bool
QcVarStow::presentAndReferenced( QcFloatRep *v) const
{
  Var2Ix_T::const_iterator i = fVar2Ix.find( v);
  if (i == fVar2Ix.end())
    return false;
  unsigned ix = i->second;
  assert( ix < fCounts.size());
  return fCounts[ix] != 0;
}


#line 256 "QcVarStow.ch"
#else /* !QC_USING_NLPSOLVER */
// Make the linker happy.


void
QcVarStow_dummy()
{
}

#endif /* !QC_USING_NLPSOLVER */

/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/
