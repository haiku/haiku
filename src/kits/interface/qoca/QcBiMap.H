// Generated automatically from QcBiMap.ch by /home/pmoulder/usr/local/bin/ch2xx.
#ifndef QcBiMapDCL
#define QcBiMapDCL
#line 1 "QcBiMap.ch"

#include <iostream.h>
#include <map.h>
#include <vector.h>
#include "qoca/QcDefines.hh"
#include "qoca/KeyIterator.hh"


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



template<class AKey>
class QcBiMap
{


public:
	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
	QcBiMap()
    	{ }

	virtual ~QcBiMap() { }



#ifndef NDEBUG

inline void
assertInvar() const;
#line 63 "QcBiMap.ch"
inline void
assertDeepInvar() const;
#line 70 "QcBiMap.ch"
virtual inline void
vAssertDeepInvar() const;
#line 75 "QcBiMap.ch"
#endif


protected:
	typedef map<AKey, int, less<AKey> > TIdentifierMap;
	typedef vector<AKey> TIndexMap;

public:
	typedef KeyIterator<TIdentifierMap::const_iterator, AKey> const_identifier_iterator;


	//-----------------------------------------------------------------------//
	// BiMap manipulation functions.                                         //
	//-----------------------------------------------------------------------//

/** Remove item with index <tt>index</tt>.
    @precondition <tt>index</tt> is currently in this bimap.
**/

inline void
EraseByIndex(int index);
#line 107 "QcBiMap.ch"
/** Erase everything ready to start afresh. */

inline void
Restart();
#line 117 "QcBiMap.ch"
/** Swap the mappings around so that {ix1 &harr; id1, ix2 &harr; id2}
    becomes {ix1 &harr; id2, ix2 &harr; id1}.

    @precondition <tt>ix1</tt> and <tt>ix2</tt> are both present in this
    bimap.
**/

inline void
SwapByIndex(int i1, int i2);
#line 147 "QcBiMap.ch"
/** Insert the mapping <tt>ident</tt> &harr; <tt>index</tt> into this
    bimap.

    @precondition <tt>index</tt> &ge; 0
    @precondition Neither <tt>index</tt> nor <tt>ident</tt> is currently
                  in this bimap.
    @postcondition <tt>identifier(index) == ident</tt>
    @postcondition <tt>index(ident) == index</tt>
**/

inline void
Update(const AKey &ident, int index);
#line 184 "QcBiMap.ch"
//-----------------------------------------------------------------------//
// Query functions.                                                      //
//-----------------------------------------------------------------------//



  /** Number of entries. */
  unsigned GetSize() const
    { return fIdentifierMap.size(); }

  bool IdentifierPresent(AKey const &ident) const
    { return (fIdentifierMap.find(ident) != fIdentifierMap.end()); }

  const_identifier_iterator
  getIdentifiers_begin() const
  {
    TIdentifierMap::const_iterator i = fIdentifierMap.begin();
    return const_identifier_iterator (i);
  }

  const_identifier_iterator getIdentifiers_end() const
    { return const_identifier_iterator (fIdentifierMap.end()); }




inline AKey &
Identifier(int index);
#line 220 "QcBiMap.ch"
inline AKey &
Identifier(char const *n);
#line 237 "QcBiMap.ch"
/** Return the index associated with <tt>ident</tt>.

    @precondition <tt>IdentifierPresent(ident)</tt>
    @postcondition ret &ge; 0.
**/

inline int
Index(AKey const &ident) const;
#line 255 "QcBiMap.ch"
/** Return the index associated with <tt>ident</tt>,
    or -1 if <tt>ident</tt> is not present.

    @postcondition <tt>IdentifierPresent(ident)</tt> &hArr; (ret &ge; 0)
**/

inline int
safeIndex(AKey const &ident) const;
#line 273 "QcBiMap.ch"
//-----------------------------------------------------------------------//
// Utility functions.                                                    //
//-----------------------------------------------------------------------//
#ifndef qcNoStream

inline void
Print(ostream &os) const;
#line 294 "QcBiMap.ch"
#endif /* !qcNoStream */


protected:
	TIdentifierMap fIdentifierMap; 	// AKey -> int
	TIndexMap fIndexMap;		// int -> AKey

};



#ifndef qcNoStream
template<class AKey>
ostream &operator<<(ostream &os, const QcBiMap<AKey> &bm)
{
	bm.Print(os);
	return os;
}
#endif /* !qcNoStream */




/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/

#endif /* !QcBiMapDCL */
