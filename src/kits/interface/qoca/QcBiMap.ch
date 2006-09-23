//begin o[d]
#include <iostream>
#include <map>
#include <vector>
#include "qoca/QcDefines.hh"
#include "qoca/KeyIterator.hh"
//end o[d]

/** Bi-directional one-to-one mapping between identifier class and non-negative
    integers.

    @invariant &forall;[ix &isin; indexes] ix &ge; 0
    @invariant &forall;[id &isin; idents] index(id) &isin; indexes
    @invariant &forall;[ix &isin; indexes] identifier(ix) &isin; idents
    @invariant &neg;&exist;[i &ne; j] {i, j} &subeq; indexes &and; identifier(i) = identifier(j)
    @invariant &neg;&exist;[i &ne; j] {i, j} &subeq; idents &and; index(i) = index(j)
    @invariant &forall;[ix &isin; indexes] index(identifier(ix)) = ix
    @invariant &forall;[id &isin; idents] identifier(index(id)) = id
**/

//o[ci] #define TMPL template<class AKey>

//o[d] template<class AKey>
//o[d] class QcBiMap
//o[d] {
//$class_prefix = QcBiMap<AKey>::
//begin o[d]
public:
	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
	QcBiMap()
    	{ }

	virtual ~QcBiMap() { }
//end o[d]


#ifndef NDEBUG
//o[i] TMPL
inline void
assertInvar() const
{
  for(int i = fIndexMap.size(); --i >= 0;)
    {
      fIndexMap[ i].assertInvar();
      if(fIndexMap[ i].isDead())
	continue;
      typename TIdentifierMap::const_iterator f = fIdentifierMap.find(fIndexMap[i]);
      qcAssertInvar( f != fIdentifierMap.end());
      qcAssertInvar( f->second == i);
    }

  for(typename TIdentifierMap::const_iterator ii = fIdentifierMap.begin(); ii != fIdentifierMap.end(); ii++)
    {
      unsigned ix = ii->second;
      qcAssertInvar( ix < fIndexMap.size());
      qcAssertInvar( fIndexMap[ ix] == ii->first);
    }
}

//o[i] TMPL
inline void
assertDeepInvar() const
{
  assertInvar();
}

//o[i] TMPL
virtual inline void
vAssertDeepInvar() const
{
  assertDeepInvar();
}
#endif

//begin o[d]
protected:
	typedef map<AKey, int, less<AKey> > TIdentifierMap;
	typedef vector<AKey> TIndexMap;

public:
	typedef KeyIterator<typename TIdentifierMap::const_iterator, AKey> const_identifier_iterator;
//end o[d]

	//-----------------------------------------------------------------------//
	// BiMap manipulation functions.                                         //
	//-----------------------------------------------------------------------//

/** Remove item with index <tt>index</tt>.
    @precondition <tt>index</tt> is currently in this bimap.
**/
//o[i] TMPL
inline void
EraseByIndex(int index)
{
  qcAssertPre( unsigned( index) < fIndexMap.size());

  AKey &ident = fIndexMap[ index];
  qcAssertPre( ident.notDead());
  fIdentifierMap.erase( ident);
  qcAssertPost( ident.notDead());
  ident.Decrease();
}


/** Erase everything ready to start afresh. */
//o[i] TMPL
inline void
Restart()
{
  fIdentifierMap.clear();
  fIndexMap.clear();
}


/** Swap the mappings around so that {ix1 &harr; id1, ix2 &harr; id2}
    becomes {ix1 &harr; id2, ix2 &harr; id1}.

    @precondition <tt>ix1</tt> and <tt>ix2</tt> are both present in this
    bimap.
**/
//o[i] TMPL
inline void
SwapByIndex(int i1, int i2)
{
  qcAssertPre( unsigned( i1) < fIndexMap.size());
  qcAssertPre( unsigned( i2) < fIndexMap.size());

  AKey ident1 = fIndexMap[ i1];
  AKey ident2 = fIndexMap[ i2];
  qcAssertPre( ident1.notDead());
  qcAssertPre( ident2.notDead());
  fIndexMap[ i1] = ident2;
  fIndexMap[ i2] = ident1;

  typename QcBiMap<AKey>::TIdentifierMap::iterator it1 = fIdentifierMap.find( ident1);
  qcAssertPost( it1 != fIdentifierMap.end());
  it1->second = i2;

  typename QcBiMap<AKey>::TIdentifierMap::iterator it2 = fIdentifierMap.find( ident2);
  qcAssertPost( it2 != fIdentifierMap.end());
  it2->second = i1;
}


/** Insert the mapping <tt>ident</tt> &harr; <tt>index</tt> into this
    bimap.

    @precondition <tt>index</tt> &ge; 0
    @precondition Neither <tt>index</tt> nor <tt>ident</tt> is currently
                  in this bimap.
    @postcondition <tt>identifier(index) == ident</tt>
    @postcondition <tt>index(ident) == index</tt>
**/
//o[i] TMPL
inline void
Update(const AKey &ident, int index)
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
  TIdentifierMap::value_type ins (ident, index);
  typedef pair<TIdentifierMap::iterator, bool> insResultT;
  dbgPre(insResultT insResult =)
    fIdentifierMap.insert (ins);

  qcAssertPre (insResult.second); // assert that ident wasn't present

  qcAssertPost (Identifier (index) == ident);
  qcAssertPost (safeIndex (ident) == index);
}


//-----------------------------------------------------------------------//
// Query functions.                                                      //
//-----------------------------------------------------------------------//

//begin o[d]

  /** Number of entries. */
  unsigned GetSize() const
    { return fIdentifierMap.size(); }

bool IdentifierPresent(AKey const &ident) const
    { return (fIdentifierMap.find(ident) != fIdentifierMap.end()); }

  const_identifier_iterator
  getIdentifiers_begin() const
  {
    typename TIdentifierMap::const_iterator i = fIdentifierMap.begin();
    return const_identifier_iterator (i);
  }

  const_identifier_iterator getIdentifiers_end() const
    { return const_identifier_iterator (fIdentifierMap.end()); }
//end o[d]


//o[i] TMPL
inline AKey &
Identifier(int index)
{
  qcAssertPre( unsigned( index) < fIndexMap.size());
  qcAssertPre( fIndexMap[ index].notDead());
  return fIndexMap[ index];
}


//o[i] TMPL
inline AKey &
Identifier(char const *n)
{
  typename QcBiMap<AKey>::TIdentifierMap::iterator iIt = fIdentifierMap.begin();
  
  while(iIt != fIdentifierMap.end())
    {
      AKey const &id = iIt->first;
      if(strcmp( id.Name(), n) == 0)
	return const_cast<AKey &>( id);
      iIt++;
    }
  
  throw QcWarning("Identifier cannot be found");
}


/** Return the index associated with <tt>ident</tt>.

    @precondition <tt>IdentifierPresent(ident)</tt>
    @postcondition ret &ge; 0.
**/
//o[i] TMPL
inline int
Index(AKey const &ident) const
{
  typename QcBiMap<AKey>::TIdentifierMap::const_iterator iIt = fIdentifierMap.find( ident);
  qcAssertPre( iIt != fIdentifierMap.end());
  unsigned ix = iIt->second;
  qcAssertPost( (ix < fIndexMap.size())
		&& (fIndexMap[ix] == ident));
  return ix;
}


/** Return the index associated with <tt>ident</tt>,
    or -1 if <tt>ident</tt> is not present.

    @postcondition <tt>IdentifierPresent(ident)</tt> &hArr; (ret &ge; 0)
**/
//o[i] TMPL
inline int
safeIndex(AKey const &ident) const
{
  QcBiMap<AKey>::TIdentifierMap::const_iterator iIt = fIdentifierMap.find( ident);
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
//o[i] TMPL
inline void
Print(ostream &os) const
{
  os << "Map start:" << endl;

  for (unsigned int i = 0; i < fIndexMap.size(); i++)
    os << i << " -> " << fIndexMap[i] << endl;

  os << "Reverse Map:" << endl;
  QcBiMap<AKey>::TIdentifierMap::const_iterator itF;

  for (itF = fIdentifierMap.begin(); itF != fIdentifierMap.end(); ++itF)
    os << (*itF).first << " -> " << (*itF).second << endl;

  os << "Map end." << endl;
}
#endif /* !qcNoStream */

//begin o[d]
protected:
	TIdentifierMap fIdentifierMap; 	// AKey -> int
	TIndexMap fIndexMap;		// int -> AKey
//end o[d]
//o[d] };


//begin o[d]
#ifndef qcNoStream
template<class AKey>
ostream &operator<<(ostream &os, const QcBiMap<AKey> &bm)
{
	bm.Print(os);
	return os;
}
#endif /* !qcNoStream */
//end o[d]

//o[ci] #undef TMPL

/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/
