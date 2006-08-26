// Generated automatically from QcAliasedBiMap.ch by /home/pmoulder/usr/local/bin/ch2xx.
#ifndef QcAliasedBiMapDCL
#define QcAliasedBiMapDCL
#line 1 "QcAliasedBiMap.ch"

#include <qoca/QcBiMap.H>


#line 11 "QcAliasedBiMap.ch"
#define TMPL template<class NullableT, class ExternalT, class PointedToT>
TMPL
class QcAliasedBiMap
  : public QcBiMap<NullableT>
{




public:
  QcAliasedBiMap() { }



inline void
Update(ExternalT const &ident, int index);
#line 33 "QcAliasedBiMap.ch"
inline void
Update(PointedToT *ident, int index);
#line 41 "QcAliasedBiMap.ch"
inline bool
IdentifierPresent(ExternalT const &ident) const;
#line 48 "QcAliasedBiMap.ch"
inline bool
IdentifierPtrPresent(PointedToT *ident) const;
#line 55 "QcAliasedBiMap.ch"
inline ExternalT &
Identifier(int index);
#line 67 "QcAliasedBiMap.ch"
inline ExternalT &
Identifier(char const *n);
#line 79 "QcAliasedBiMap.ch"
inline PointedToT *
getIdentifierPtr(int index);
#line 87 "QcAliasedBiMap.ch"
inline int
Index(ExternalT const &ident) const;
#line 94 "QcAliasedBiMap.ch"
inline int
Index(PointedToT *ident) const;
#line 101 "QcAliasedBiMap.ch"
inline int
safeIndex(ExternalT const &ident) const;
#line 108 "QcAliasedBiMap.ch"
inline int
safeIndex(PointedToT *ident) const;
#line 114 "QcAliasedBiMap.ch"
};

#undef TMPL
#undef A2E
#undef const_E2A

/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/

#endif /* !QcAliasedBiMapDCL */
