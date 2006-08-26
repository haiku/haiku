//begin o[d]
#include <stdlib.h>

class QcNullIterator
{
  bool AtEnd() const { return true; }
  unsigned getIndex() const { abort(); }
  numT getValue() const { abort(); }
  unsigned Increment() { abort(); }
}

//end o[d]
