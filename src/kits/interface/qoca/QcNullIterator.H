// Generated automatically from QcNullIterator.ch by /home/pmoulder/usr/local/bin/ch2xx.
#ifndef QcNullIteratorDCL
#define QcNullIteratorDCL
#line 1 "QcNullIterator.ch"

#include <stdlib.h>

class QcNullIterator
{
  bool AtEnd() const { return true; }
  unsigned getIndex() const { abort(); }
  numT getValue() const { abort(); }
  unsigned Increment() { abort(); }
}



#endif /* !QcNullIteratorDCL */
