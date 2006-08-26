// Generated automatically from QcAliasedBiMap.ch by /home/pmoulder/usr/local/bin/ch2xx.
#include "QcAliasedBiMap.H"
#ifndef QcAliasedBiMapIFN
#define QcAliasedBiMapIFN
#line 1 "QcAliasedBiMap.ch"

#line 5 "QcAliasedBiMap.ch"
#include <qoca/QcBiMap.hh>

#define A2E(_nkey) *((ExternalT *)((void *)&(_nkey)))
#define const_E2A(_ekey) reinterpret_cast<NullableT const &>(_ekey)


#define TMPL template<class NullableT, class ExternalT, class PointedToT>








#line 24 "QcAliasedBiMap.ch"
TMPL
inline void
QcAliasedBiMap<NullableT, ExternalT, PointedToT>::Update(ExternalT const &ident, int index)
{
  qcAssertPre( ident.pointer() != 0);
  QcBiMap<NullableT>::Update(const_E2A(ident), index);
}

TMPL
inline void
QcAliasedBiMap<NullableT, ExternalT, PointedToT>::Update(PointedToT *ident, int index)
{
  qcAssertPre( ident != 0);
  QcBiMap<NullableT>::Update( NullableT( ident), index);
}

TMPL
inline bool
QcAliasedBiMap<NullableT, ExternalT, PointedToT>::IdentifierPresent(ExternalT const &ident) const
{
  return QcBiMap<NullableT>::IdentifierPresent(const_E2A(ident));
}

TMPL
inline bool
QcAliasedBiMap<NullableT, ExternalT, PointedToT>::IdentifierPtrPresent(PointedToT *ident) const
{
  return QcBiMap<NullableT>::IdentifierPresent( NullableT( ident));
}

TMPL
inline ExternalT &
QcAliasedBiMap<NullableT, ExternalT, PointedToT>::Identifier(int index)
{
  NullableT &nid = QcBiMap<NullableT>::Identifier( index);
  NullableT *nidp = &nid;
  void *vp = nidp;
  ExternalT *ep = (ExternalT *) vp;
  ExternalT &eid = *ep;
  return eid;
}

TMPL
inline ExternalT &
QcAliasedBiMap<NullableT, ExternalT, PointedToT>::Identifier(char const *n)
{
  NullableT &nid = QcBiMap<NullableT>::Identifier( n);
  NullableT *nidp = &nid;
  void *vp = nidp;
  ExternalT *ep = (ExternalT *) vp;
  ExternalT &eid = *ep;
  return eid;
}

TMPL
inline PointedToT *
QcAliasedBiMap<NullableT, ExternalT, PointedToT>::getIdentifierPtr(int index)
{
  NullableT &nid = QcBiMap<NullableT>::Identifier( index);
  return nid.pointer();
}

TMPL
inline int
QcAliasedBiMap<NullableT, ExternalT, PointedToT>::Index(ExternalT const &ident) const
{
  return QcBiMap<NullableT>::Index(const_E2A(ident));
}

TMPL
inline int
QcAliasedBiMap<NullableT, ExternalT, PointedToT>::Index(PointedToT *ident) const
{
  return QcBiMap<NullableT>::Index( NullableT( ident));
}

TMPL
inline int
QcAliasedBiMap<NullableT, ExternalT, PointedToT>::safeIndex(ExternalT const &ident) const
{
  return QcBiMap<NullableT>::safeIndex(const_E2A(ident));
}

TMPL
inline int
QcAliasedBiMap<NullableT, ExternalT, PointedToT>::safeIndex(PointedToT *ident) const
{
  return QcBiMap<NullableT>::safeIndex( NullableT( ident));
}



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

#endif /* !QcAliasedBiMapIFN */
