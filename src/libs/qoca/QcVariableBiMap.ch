//begin o[d]
#include <qoca/QcDefines.hh>
#include <qoca/QcAliasedBiMap.H>
#include <qoca/QcNullableFloat.hh>
//end o[d]
//begin o[i]
#include <qoca/QcAliasedBiMap.hh>
//end o[i]
//begin o[c]
#include <stdio.h>
//end o[c]


#define super QcAliasedBiMap<QcNullableFloat, QcFloat, QcFloatRep>

//o[d] class QcVariableBiMap
//o[d]   : public super
//o[d] {

//begin o[d]
public:
  QcVariableBiMap()
    : super()
  {
  }
//end o[d]

#ifndef NDEBUG
//cf
void
assertInvar() const
{
  for(TIdentifierMap::const_iterator
	i = fIdentifierMap.begin(), iEnd = fIdentifierMap.end();
      i != iEnd; i++)
    assert( i->first.pointer()->isQcFloatRep());
}

//cf
void
assertDeepInvar() const
{
  super::assertDeepInvar();
  assertInvar();
}

//cf
virtual void
vAssertDeepInvar() const
{
  assertDeepInvar();
}
#endif

//cf
void
Update(QcFloat const &ident, int index)
{
  super::Update( ident, index);
  dbg( assertInvar());
}  

//cf
void
Update(QcFloatRep *ident, int index)
{
  super::Update( ident, index);
  dbg( assertInvar());
}


//-----------------------------------------------------------------------//
// Query functions                                                       //
//-----------------------------------------------------------------------//
//cf
void
GetVariableSet(vector<QcFloat> &vars)
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

//o[d] };


/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/
