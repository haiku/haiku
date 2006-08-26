//begin o[d]
#include <qoca/QcBiMap.H>
//end o[d]
//begin o[i]
#include <qoca/QcBiMap.hh>

#define A2E(_nkey) *((ExternalT *)((void *)&(_nkey)))
#define const_E2A(_ekey) reinterpret_cast<NullableT const &>(_ekey)
//end o[i]

#define TMPL template<class NullableT, class ExternalT, class PointedToT>
//o[d] TMPL
//o[d] class QcAliasedBiMap
//o[d]   : public QcBiMap<NullableT>
//o[d] {

//$class_prefix = QcAliasedBiMap<NullableT, ExternalT, PointedToT>::

//begin o[d]
public:
  QcAliasedBiMap() { }
//end o[d]

//o[i] TMPL
inline void
Update(ExternalT const &ident, int index)
{
  qcAssertPre( ident.pointer() != 0);
  QcBiMap<NullableT>::Update(const_E2A(ident), index);
}

//o[i] TMPL
inline void
Update(PointedToT *ident, int index)
{
  qcAssertPre( ident != 0);
  QcBiMap<NullableT>::Update( NullableT( ident), index);
}

//o[i] TMPL
inline bool
IdentifierPresent(ExternalT const &ident) const
{
  return QcBiMap<NullableT>::IdentifierPresent(const_E2A(ident));
}

//o[i] TMPL
inline bool
IdentifierPtrPresent(PointedToT *ident) const
{
  return QcBiMap<NullableT>::IdentifierPresent( NullableT( ident));
}

//o[i] TMPL
inline ExternalT &
Identifier(int index)
{
  NullableT &nid = QcBiMap<NullableT>::Identifier( index);
  NullableT *nidp = &nid;
  void *vp = nidp;
  ExternalT *ep = (ExternalT *) vp;
  ExternalT &eid = *ep;
  return eid;
}

//o[i] TMPL
inline ExternalT &
Identifier(char const *n)
{
  NullableT &nid = QcBiMap<NullableT>::Identifier( n);
  NullableT *nidp = &nid;
  void *vp = nidp;
  ExternalT *ep = (ExternalT *) vp;
  ExternalT &eid = *ep;
  return eid;
}

//o[i] TMPL
inline PointedToT *
getIdentifierPtr(int index)
{
  NullableT &nid = QcBiMap<NullableT>::Identifier( index);
  return nid.pointer();
}

//o[i] TMPL
inline int
Index(ExternalT const &ident) const
{
  return QcBiMap<NullableT>::Index(const_E2A(ident));
}

//o[i] TMPL
inline int
Index(PointedToT *ident) const
{
  return QcBiMap<NullableT>::Index( NullableT( ident));
}

//o[i] TMPL
inline int
safeIndex(ExternalT const &ident) const
{
  return QcBiMap<NullableT>::safeIndex(const_E2A(ident));
}

//o[i] TMPL
inline int
safeIndex(PointedToT *ident) const
{
  return QcBiMap<NullableT>::safeIndex( NullableT( ident));
}

//o[d] };

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
