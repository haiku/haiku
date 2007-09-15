// Generated automatically from QcVariableBiMap.ch by /home/pmoulder/usr/local/bin/ch2xx.
#ifndef QcVariableBiMapDCL
#define QcVariableBiMapDCL
#line 1 "QcVariableBiMap.ch"

#include <qoca/QcDefines.hh>
#include <qoca/QcAliasedBiMap.H>
#include <qoca/QcNullableFloat.hh>


#line 14 "QcVariableBiMap.ch"
#define super QcAliasedBiMap<QcNullableFloat, QcFloat, QcFloatRep>

class QcVariableBiMap
  : public super
{


public:
  QcVariableBiMap()
    : super()
  {
  }


#ifndef NDEBUG

void
assertInvar() const;
#line 40 "QcVariableBiMap.ch"
void
assertDeepInvar() const;
#line 48 "QcVariableBiMap.ch"
virtual void
vAssertDeepInvar() const;
#line 53 "QcVariableBiMap.ch"
#endif


void
Update(QcFloat const &ident, int index);
#line 64 "QcVariableBiMap.ch"
void
Update(QcFloatRep *ident, int index);
#line 72 "QcVariableBiMap.ch"
//-----------------------------------------------------------------------//
// Query functions                                                       //
//-----------------------------------------------------------------------//

void
GetVariableSet(vector<QcFloat> &vars);
#line 100 "QcVariableBiMap.ch"
};


/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/

#endif /* !QcVariableBiMapDCL */
