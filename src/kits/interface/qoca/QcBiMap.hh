// Generated automatically from QcBiMap.ch by /home/pmoulder/usr/local/bin/ch2xx.
#include "QcBiMap.H"
#ifndef QcBiMapIFN
#define QcBiMapIFN
#line 1 "QcBiMap.ch"

#line 21 "QcBiMap.ch"
#define TMPL template<class AKey>

#if __GNUC__ >= 4
	#define TYPENAMEDEF typename
#else
	#define TYPENAMEDEF
#endif




#line 39 "QcBiMap.ch"
#ifndef NDEBUG
TMPL
inline void
QcBiMap<AKey>::assertInvar() const
{
  for(int i = fIndexMap.size(); --i >= 0;)
    {
      fIndexMap[ i].assertInvar();
      if(fIndexMap[ i].isDead())
	continue;
      TYPENAMEDEF TIdentifierMap::const_iterator f = fIdentifierMap.find(fIndexMap[i]);
      qcAssertInvar( f != fIdentifierMap.end());
      qcAssertInvar( f->second == i);
    }

  for(TYPENAMEDEF TIdentifierMap::const_iterator ii = fIdentifierMap.begin(); ii != fIdentifierMap.end(); ii++)
    {
      unsigned ix = ii->second;
      qcAssertInvar( ix < fIndexMap.size());
      qcAssertInvar( fIndexMap[ ix] == ii->first);
    }
}

TMPL
inline void
QcBiMap<AKey>::assertDeepInvar() const
{
  assertInvar();
}

TMPL
inline void
QcBiMap<AKey>::vAssertDeepInvar() const
{
  assertDeepInvar();
}
#endif


#line 86 "QcBiMap.ch"
	//-----------------------------------------------------------------------//
	// BiMap manipulation functions.                                         //
	//-----------------------------------------------------------------------//


#line 93 "QcBiMap.ch"
TMPL
inline void
QcBiMap<AKey>::EraseByIndex(int index)
{
  qcAssertPre( unsigned( index) < fIndexMap.size());

  AKey &ident = fIndexMap[ index];
  qcAssertPre( ident.notDead());
  fIdentifierMap.erase( ident);
  qcAssertPost( ident.notDead());
  ident.Decrease();
}



TMPL
inline void
QcBiMap<AKey>::Restart()
{
  fIdentifierMap.clear();
  fIndexMap.clear();
}



#line 123 "QcBiMap.ch"
TMPL
inline void
QcBiMap<AKey>::SwapByIndex(int i1, int i2)
{
  qcAssertPre( unsigned( i1) < fIndexMap.size());
  qcAssertPre( unsigned( i2) < fIndexMap.size());

  AKey ident1 = fIndexMap[ i1];
  AKey ident2 = fIndexMap[ i2];
  qcAssertPre( ident1.notDead());
  qcAssertPre( ident2.notDead());
  fIndexMap[ i1] = ident2;
  fIndexMap[ i2] = ident1;

  TYPENAMEDEF QcBiMap<AKey>::TIdentifierMap::iterator it1 = fIdentifierMap.find( ident1);
  qcAssertPost( it1 != fIdentifierMap.end());
  it1->second = i2;

  TYPENAMEDEF QcBiMap<AKey>::TIdentifierMap::iterator it2 = fIdentifierMap.find( ident2);
  qcAssertPost( it2 != fIdentifierMap.end());
  it2->second = i1;
}



#line 156 "QcBiMap.ch"
TMPL
inline void
QcBiMap<AKey>::Update(const AKey &ident, int index)
{
  if(index >= (int) fIndexMap.size())
    fIndexMap.resize( (index * 2) + 1);
  else
    {
      qcAssertPre( index >= 0);
      qcAssertPre( fIndexMap[ index].isDead()); // index not present
    }

  // Insert index->ident mapping.
  fIndexMap[ index] = ident;

  // Insert ident->index mapping.
  TYPENAMEDEF TIdentifierMap::value_type ins (ident, index);
  typedef pair<TYPENAMEDEF TIdentifierMap::iterator, bool> insResultT;
  dbgPre(insResultT insResult =)
    fIdentifierMap.insert (ins);

  qcAssertPre (insResult.second); // assert that ident wasn't present

  qcAssertPost (Identifier (index) == ident);
  qcAssertPost (safeIndex (ident) == index);
}


//-----------------------------------------------------------------------//
// Query functions.                                                      //
//-----------------------------------------------------------------------//


#line 209 "QcBiMap.ch"
TMPL
inline AKey &
QcBiMap<AKey>::Identifier(int index)
{
  qcAssertPre( unsigned( index) < fIndexMap.size());
  qcAssertPre( fIndexMap[ index].notDead());
  return fIndexMap[ index];
}


TMPL
inline AKey &
QcBiMap<AKey>::Identifier(char const *n)
{
  TYPENAMEDEF QcBiMap<AKey>::TIdentifierMap::iterator iIt = fIdentifierMap.begin();

  while(iIt != fIdentifierMap.end())
    {
      AKey const &id = iIt->first;
      if(strcmp( id.Name(), n) == 0)
	return const_cast<AKey &>( id);
      iIt++;
    }

  throw QcWarning("Identifier cannot be found");
}



#line 242 "QcBiMap.ch"
TMPL
inline int
QcBiMap<AKey>::Index(AKey const &ident) const
{
  TYPENAMEDEF QcBiMap<AKey>::TIdentifierMap::const_iterator iIt = fIdentifierMap.find( ident);
  qcAssertPre( iIt != fIdentifierMap.end());
  unsigned ix = iIt->second;
  qcAssertPost( (ix < fIndexMap.size())
		&& (fIndexMap[ix] == ident));
  return ix;
}



#line 260 "QcBiMap.ch"
TMPL
inline int
QcBiMap<AKey>::safeIndex(AKey const &ident) const
{
  TYPENAMEDEF QcBiMap<AKey>::TIdentifierMap::const_iterator iIt = fIdentifierMap.find( ident);
  if(iIt == fIdentifierMap.end())
    return -1;
  int ix = iIt->second;
  qcAssertPost( ix >= 0);
  return ix;
}


//-----------------------------------------------------------------------//
// Utility functions.                                                    //
//-----------------------------------------------------------------------//
#ifndef qcNoStream
TMPL
inline void
QcBiMap<AKey>::Print(ostream &os) const
{
  os << "Map start:" << endl;

  for (unsigned int i = 0; i < fIndexMap.size(); i++)
    os << i << " -> " << fIndexMap[i] << endl;

  os << "Reverse Map:" << endl;
  TYPENAMEDEF QcBiMap<AKey>::TIdentifierMap::const_iterator itF;

  for (itF = fIdentifierMap.begin(); itF != fIdentifierMap.end(); ++itF)
    os << (*itF).first << " -> " << (*itF).second << endl;

  os << "Map end." << endl;
}
#endif /* !qcNoStream */


#line 315 "QcBiMap.ch"
#undef TMPL

/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/

#endif /* !QcBiMapIFN */
