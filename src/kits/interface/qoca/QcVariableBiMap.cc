// Generated automatically from QcVariableBiMap.ch by /home/pmoulder/usr/local/bin/ch2xx.
#include "QcVariableBiMap.hh"
#line 1 "QcVariableBiMap.ch"

#line 10 "QcVariableBiMap.ch"
#include <stdio.h>



#define super QcAliasedBiMap<QcNullableFloat, QcFloat, QcFloatRep>






#line 28 "QcVariableBiMap.ch"
#ifndef NDEBUG

void
QcVariableBiMap::assertInvar() const
{
  for(TIdentifierMap::const_iterator
	i = fIdentifierMap.begin(), iEnd = fIdentifierMap.end();
      i != iEnd; i++)
    assert( i->first.pointer()->isQcFloatRep());
}


void
QcVariableBiMap::assertDeepInvar() const
{
  super::assertDeepInvar();
  assertInvar();
}


void
QcVariableBiMap::vAssertDeepInvar() const
{
  assertDeepInvar();
}
#endif


void
QcVariableBiMap::Update(QcFloat const &ident, int index)
{
  super::Update( ident, index);
  dbg( assertInvar());
}


void
QcVariableBiMap::Update(QcFloatRep *ident, int index)
{
  super::Update( ident, index);
  dbg( assertInvar());
}


//-----------------------------------------------------------------------//
// Query functions                                                       //
//-----------------------------------------------------------------------//

void
QcVariableBiMap::GetVariableSet(vector<QcFloat> &vars)
{
  unsigned size = GetSize();

  for(unsigned i = 0; i < size; i++)
    {
      QcNullableFloat &nf = fIndexMap[i];
      if(nf.isDead())
	continue;

      QcFloat &f = reinterpret_cast<QcFloat &>(nf);

      if (f.Name()[0] == '\0')
	{
	  char name[20];
	  sprintf(name, "$%ld", f.Id());
	  f.SetName(name);
	}

      vars.push_back(f);
    }
}




/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/
