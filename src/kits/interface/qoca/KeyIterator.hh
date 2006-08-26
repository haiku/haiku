#ifndef KeyIteratorHH
#define KeyIteratorHH

template <class Iterator, class Key>
class KeyIterator
  : public Iterator
{
public:
  KeyIterator (Iterator i)
    : Iterator (i)
    {
    }

  Key const & operator* () const
    {
      return (*((Iterator const *) this))->first;
    }

  Key const * operator->() const
    {
      return &(*((Iterator const *) this))->first;
    }
};

#endif /* !KeyIteratorHH */
