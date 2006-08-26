// Generated automatically from QcVarStow.ch by /home/pmoulder/usr/local/bin/ch2xx.
#ifndef QcVarStowDCL
#define QcVarStowDCL
#line 1 "QcVarStow.ch"
#include <qoca/QcEnables.H>
#ifdef QC_USING_NLPSOLVER


#include <vector>
#include <hash_map>
#include <qoca/QcFloatRep.H>


#line 13 "QcVarStow.ch"
class QcVarStow
{
public:


QcVarStow();
#line 27 "QcVarStow.ch"
#ifndef NDEBUG

void
assertInvar() const;
#line 39 "QcVarStow.ch"
#endif

inline unsigned
size() const;
#line 47 "QcVarStow.ch"
inline unsigned
getVarIx( QcFloatRep *v) const;
#line 56 "QcVarStow.ch"
void
ensurePresent(QcFloatRep *v);
#line 73 "QcVarStow.ch"
inline bool
constrCacheValid() const;
#line 79 "QcVarStow.ch"
/** Equivalent to <tt>ensurePresent(v); incUsage(v);</tt>.
    @precondition <tt>v != 0</tt>
**/
void
addReference(QcFloatRep *v);
#line 102 "QcVarStow.ch"
/** If <tt>v</tt> is present and has usage count 0, then remove <tt>v</tt> and
    return <tt>true</tt>; otherwise return <tt>false</tt>.

    @precondition <tt>v != 0</tt>
**/
bool
tryRemove(QcFloatRep *v);
#line 145 "QcVarStow.ch"
void
incUsage( QcFloatRep *v);
#line 157 "QcVarStow.ch"
void
decUsage( QcFloatRep *v);
#line 168 "QcVarStow.ch"
inline unsigned
getUsage(QcFloatRep *v) const;
#line 178 "QcVarStow.ch"
inline double *
getSolvedValArray();
#line 186 "QcVarStow.ch"
inline double const *
getSolvedValArray() const;
#line 194 "QcVarStow.ch"
typedef vector<QcFloatRep *>::const_iterator var_const_iterator;
typedef vector<QcFloatRep *>::iterator var_iterator;

inline QcVarStow::var_const_iterator
vars_begin() const;
#line 203 "QcVarStow.ch"
inline QcVarStow::var_const_iterator
vars_end() const;
#line 209 "QcVarStow.ch"
inline QcVarStow::var_iterator
vars_begin();
#line 215 "QcVarStow.ch"
inline QcVarStow::var_iterator
vars_end();
#line 222 "QcVarStow.ch"
bool
isPresent( QcFloatRep *v) const;
#line 229 "QcVarStow.ch"
private:

bool
presentAndReferenced( QcFloatRep *v) const;
#line 243 "QcVarStow.ch"
public:
bool fConstrCacheValid;

private:
vector<double> fSolvedVals;
vector<QcFloatRep *> fVars;
vector<unsigned> fCounts;
typedef hash_map<QcFloatRep *, unsigned,
  QcFloatRep_hash, QcFloatRep_equal> Var2Ix_T;
Var2Ix_T fVar2Ix;

};

#else /* !QC_USING_NLPSOLVER */
// Make the linker happy.


void
QcVarStow_dummy();
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

#endif /* !QcVarStowDCL */
