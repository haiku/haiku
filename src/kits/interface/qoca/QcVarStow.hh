// Generated automatically from QcVarStow.ch by /home/pmoulder/usr/local/bin/ch2xx.
#include "QcVarStow.H"
#ifndef QcVarStowIFN
#define QcVarStowIFN
#line 1 "QcVarStow.ch"

#ifdef QC_USING_NLPSOLVER


#line 27 "QcVarStow.ch"
#ifndef NDEBUG

#line 39 "QcVarStow.ch"
#endif

inline unsigned
QcVarStow::size() const
{
  return fVars.size();
}

inline unsigned
QcVarStow::getVarIx( QcFloatRep *v) const
{
  Var2Ix_T::const_iterator i = fVar2Ix.find( v);
  qcAssertPre( i != fVar2Ix.end());
  return i->second;
}


#line 73 "QcVarStow.ch"
inline bool
QcVarStow::constrCacheValid() const
{
  return fConstrCacheValid;
}


#line 168 "QcVarStow.ch"
inline unsigned
QcVarStow::getUsage(QcFloatRep *v) const
{
  Var2Ix_T::const_iterator i = fVar2Ix.find( v);
  qcAssertPre( i != fVar2Ix.end());
  unsigned vi = i->second;
  assert( vi < fCounts.size());
  return fCounts[vi];
}

inline double *
QcVarStow::getSolvedValArray()
{
  qcAssertPost( fSolvedVals.size() == size());

  return fSolvedVals.begin();
}

inline double const *
QcVarStow::getSolvedValArray() const
{
  qcAssertPost( fSolvedVals.size() == size());

  return fSolvedVals.begin();
}




inline QcVarStow::var_const_iterator
QcVarStow::vars_begin() const
{
  return fVars.begin();
}

inline QcVarStow::var_const_iterator
QcVarStow::vars_end() const
{
  return fVars.end();
}

inline QcVarStow::var_iterator
QcVarStow::vars_begin()
{
  return fVars.begin();
}

inline QcVarStow::var_iterator
QcVarStow::vars_end()
{
  return fVars.end();
}


#line 256 "QcVarStow.ch"
#else /* !QC_USING_NLPSOLVER */
// Make the linker happy.


#line 265 "QcVarStow.ch"
#endif /* !QC_USING_NLPSOLVER */

/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/

#endif /* !QcVarStowIFN */
