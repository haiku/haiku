//o[d] #include <qoca/QcEnables.H>
#ifdef QC_USING_NLPSOLVER

//begin o[d]
#include <vector>
#include <hash_map>
#include <qoca/QcFloatRep.H>
//end o[d]
//begin o[c]
#include <qoca/QcFloatRep.hh>
//end o[c]

//o[d] class QcVarStow
//o[d] {
//o[d] public:

//cf
QcVarStow()
  : fConstrCacheValid( false),
    fSolvedVals(),
    fVars(),
    fVar2Ix()
{
}


#ifndef NDEBUG
//cf
void
assertInvar() const
{
  unsigned n = size();
  assert( fSolvedVals.size() == n);
  assert( fVars.size() == n);
  assert( fVar2Ix.size() == n);
  assert( fCounts.size() == n);
  /* TODO: assert that fVars is consistent with fVar2Ix. */
}
#endif

inline unsigned
size() const
{
  return fVars.size();
}

inline unsigned
getVarIx( QcFloatRep *v) const
{
  Var2Ix_T::const_iterator i = fVar2Ix.find( v);
  qcAssertPre( i != fVar2Ix.end());
  return i->second;
}

//cf
void
ensurePresent(QcFloatRep *v)
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

inline bool
constrCacheValid() const
{
  return fConstrCacheValid;
}

/** Equivalent to <tt>ensurePresent(v); incUsage(v);</tt>.
    @precondition <tt>v != 0</tt>
**/ //cf
void
addReference(QcFloatRep *v)
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
  
/** If <tt>v</tt> is present and has usage count 0, then remove <tt>v</tt> and
    return <tt>true</tt>; otherwise return <tt>false</tt>.

    @precondition <tt>v != 0</tt>
**/ //cf
bool
tryRemove(QcFloatRep *v)
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

//cf
void
incUsage( QcFloatRep *v)
{
  Var2Ix_T::const_iterator i = fVar2Ix.find( v);
  qcAssertPre( i != fVar2Ix.end());
  unsigned vi = i->second;
  assert( vi < fCounts.size());
  fCounts[vi]++;
  qcAssertPost( fCounts[vi] != 0);
}

//cf
void
decUsage( QcFloatRep *v)
{
  Var2Ix_T::const_iterator i = fVar2Ix.find( v);
  qcAssertPre( i != fVar2Ix.end());
  unsigned vi = i->second;
  assert( vi < fCounts.size());
  qcAssertPre( fCounts[vi] != 0);
  fCounts[vi]--;
}

inline unsigned
getUsage(QcFloatRep *v) const
{
  Var2Ix_T::const_iterator i = fVar2Ix.find( v);
  qcAssertPre( i != fVar2Ix.end());
  unsigned vi = i->second;
  assert( vi < fCounts.size());
  return fCounts[vi];
}

inline double *
getSolvedValArray()
{
  qcAssertPost( fSolvedVals.size() == size());

  return fSolvedVals.begin();
}

inline double const *
getSolvedValArray() const
{
  qcAssertPost( fSolvedVals.size() == size());

  return fSolvedVals.begin();
}

typedef vector<QcFloatRep *>::const_iterator var_const_iterator;
typedef vector<QcFloatRep *>::iterator var_iterator;

inline QcVarStow::var_const_iterator
vars_begin() const
{
  return fVars.begin();
}

inline QcVarStow::var_const_iterator
vars_end() const
{
  return fVars.end();
}

inline QcVarStow::var_iterator
vars_begin()
{
  return fVars.begin();
}

inline QcVarStow::var_iterator
vars_end()
{
  return fVars.end();
}

//cf
bool
isPresent( QcFloatRep *v) const
{
  Var2Ix_T::const_iterator i = fVar2Ix.find( v);
  return (i != fVar2Ix.end());
}

//ho private:
//cf
bool
presentAndReferenced( QcFloatRep *v) const
{
  Var2Ix_T::const_iterator i = fVar2Ix.find( v);
  if (i == fVar2Ix.end())
    return false;
  unsigned ix = i->second;
  assert( ix < fCounts.size());
  return fCounts[ix] != 0;
}

//begin o[d]
public:
bool fConstrCacheValid;

private:
vector<double> fSolvedVals;
vector<QcFloatRep *> fVars;
vector<unsigned> fCounts;
typedef hash_map<QcFloatRep *, unsigned,
  QcFloatRep_hash, QcFloatRep_equal> Var2Ix_T;
Var2Ix_T fVar2Ix;
//end o[d]
//o[d] };

#else /* !QC_USING_NLPSOLVER */
// Make the linker happy.
//$class_prefix=
//cf
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
